#include "AI/HydraAIController.h"

#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "Character/HydraCharacter.h"
#include "Character/HydraMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Data/HydraValuesSubsystem.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Hydra.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace HydraAI
{
	// Behaviour shape, not balance: how the simple opponent uses its reach.
	constexpr float TargetUpdateIntervalS = 0.25f;
	constexpr float StandAtReachRatio = 0.8f;    // walks up until the target is at 80 % of its reach
	constexpr float StandAtCastRangeRatio = 0.6f; // a caster stands at 60 % of its cast range
	constexpr float AttackAtReachRatio = 0.95f;  // attacks when the target is inside 95 % of its reach
	constexpr float FaceBeyondReachFactor = 3.f; // beyond 3 x reach it runs instead of circling
	constexpr float ThreatReachFactor = 1.3f;    // reacts to attacks that start within 1.3 x the attacker's reach
	constexpr float TapHoldS = 0.03f;            // how long it holds the button for a light attack
	constexpr float ProbeAheadCm = 160.f;        // how far ahead it looks for an obstacle
	constexpr float DetourS = 0.35f;             // how long it keeps a sideways step before it looks again
}

AHydraAIController::AHydraAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bAttachToPawn = true;
	bPassive = FParse::Param(FCommandLine::Get(), TEXT("HydraAIPassive"));
}

AHydraCharacter* AHydraAIController::GetHydraCharacter() const
{
	return Cast<AHydraCharacter>(GetPawn());
}

const FHydraAiValues& AHydraAIController::GetAiValues() const
{
	const FHydraValues& V = UHydraValuesSubsystem::Get(this);
	const AHydraCharacter* Self = GetHydraCharacter();
	const FHydraEnemyValues* Enemy = Self ? V.enemies.Find(Self->GetEnemyId()) : nullptr;
	return Enemy ? Enemy->ai : V.ai_practice;
}

void AHydraAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	bHadPawn = true;
	UE_LOG(LogHydra, Log, TEXT("HydraAI: opponent ready (%s)"), bPassive ? TEXT("passive") : TEXT("active"));
}

void AHydraAIController::SetPassive(bool bInPassive)
{
	bPassive = bInPassive;
	UE_LOG(LogHydra, Log, TEXT("HydraAI: %s"), bPassive ? TEXT("passive") : TEXT("active"));
	if (AHydraCharacter* Self = GetHydraCharacter())
	{
		StopActing(Self);
	}
}

void AHydraAIController::StopActing(AHydraCharacter* Self)
{
	if (ReleaseAttackAt > 0.0)
	{
		Self->InputAttackReleased();
	}
	Self->InputBlock(false);
	if (ReleaseCastAt > 0.0)
	{
		Self->InputOffHand(false, Self->GetActorRotation().Yaw);
	}
	ReleaseAttackAt = -1.0;
	ReleaseBlockAt = -1.0;
	ReleaseCastAt = -1.0;
	PendingReaction = EReaction::None;
	Target = nullptr;
	ClearFocus(EAIFocusPriority::Gameplay);
	Self->GetHydraMovement()->bWantsStrafe = false;
}

void AHydraAIController::CommandBlock(bool bHold)
{
	if (AHydraCharacter* Self = GetHydraCharacter())
	{
		ReleaseBlockAt = -1.0;
		Self->InputBlock(bHold);
	}
}

void AHydraAIController::CommandAttack(float HoldMs)
{
	if (AHydraCharacter* Self = GetHydraCharacter())
	{
		Self->InputAttackPressed();
		ReleaseAttackAt = GetWorld()->GetTimeSeconds() + FMath::Max(HydraAI::TapHoldS, HoldMs / 1000.f);
	}
}

void AHydraAIController::UpdateTarget(const AHydraCharacter* Self)
{
	// Nearest living player inside the engage range.
	const float RangeCm = GetAiValues().engage_range_m * 100.f;
	AHydraCharacter* Best = nullptr;
	float BestDistSq = FMath::Square(RangeCm);
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		AHydraCharacter* Candidate = *It;
		if (Candidate == Self || Candidate->IsAIControlledCharacter() || !Candidate->IsAlive())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared2D(Candidate->GetActorLocation(), Self->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}
	Target = Best;
}

bool AHydraAIController::IsWayBlocked(const AHydraCharacter* Self, const FVector& From, const FVector& Direction, float DistanceCm) const
{
	FHitResult Hit;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(HydraAIWay), false, Self);
	const float Radius = Self->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.85f;
	return DistanceCm > 1.f && GetWorld()->SweepSingleByObjectType(Hit, From, From + Direction * DistanceCm, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeSphere(Radius), Params);
}

FVector AHydraAIController::SteerTowards(const AHydraCharacter* Self, const FVector& ToTarget, double Now)
{
	const FVector Direction = ToTarget.GetSafeNormal2D();
	if (Now < DetourUntil)
	{
		return DetourDirection;
	}
	const FVector Location = Self->GetActorLocation();
	const float Probe = FMath::Min(ToTarget.Size2D(), HydraAI::ProbeAheadCm);
	if (!IsWayBlocked(Self, Location, Direction, Probe))
	{
		return Direction;
	}
	// Something is in the way: step sideways, to the side from which the way ahead is free
	// (the side taken last first, so it does not dither in front of a wide obstacle).
	const FVector Left = FVector::CrossProduct(Direction, FVector::UpVector);
	const float Offset = Self->GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.f + 60.f;
	for (const float Side : { DetourSide, -DetourSide })
	{
		if (!IsWayBlocked(Self, Location + Left * Side * Offset, Direction, Probe))
		{
			DetourSide = Side;
			break;
		}
	}
	DetourDirection = (Left * DetourSide + Direction * 0.15f).GetSafeNormal2D();
	DetourUntil = Now + HydraAI::DetourS;
	return DetourDirection;
}

void AHydraAIController::OnAttackSeen(AHydraCharacter* Attacker)
{
	const AHydraCharacter* Self = GetHydraCharacter();
	if (bPassive || !Self || !Attacker || Attacker == Self || !Self->IsAlive() || PendingReaction != EReaction::None)
	{
		return;
	}
	// Only attacks that could reach it.
	const float ThreatCm = Attacker->GetStats().Weapon.heavy.reach_m * 100.f * HydraAI::ThreatReachFactor + Self->GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (FVector::Dist2D(Self->GetActorLocation(), Attacker->GetActorLocation()) > ThreatCm)
	{
		return;
	}
	// Human mistakes: sometimes it misses the moment, and it always needs its reaction time.
	const FHydraAiValues& A = GetAiValues();
	if (FMath::FRand() < A.error_rate)
	{
		return;
	}
	const float Roll = FMath::FRand();
	PendingReaction = Roll < A.block_chance ? EReaction::Block : (Roll < A.block_chance + A.dodge_chance ? EReaction::Dodge : EReaction::None);
	ReactAt = GetWorld()->GetTimeSeconds() + A.reaction_time_ms / 1000.f;
	ReactTo = Attacker;
}

void AHydraAIController::React(AHydraCharacter* Self)
{
	const EReaction Reaction = PendingReaction;
	PendingReaction = EReaction::None;
	const AHydraCharacter* Attacker = ReactTo.Get();
	// Its own attack binds it like anyone else: no block, no dodge while committed.
	if (!Attacker || Self->HasStateTag(HydraTags::State_AttackCommitted))
	{
		return;
	}
	if (Reaction == EReaction::Block)
	{
		Self->InputBlock(true);
		ReleaseBlockAt = GetWorld()->GetTimeSeconds() + GetAiValues().block_hold_ms / 1000.f;
	}
	else if (Reaction == EReaction::Dodge)
	{
		// Step back or to a side, away from the attacker.
		const FVector Away = (Self->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D();
		const FVector Side = FVector::CrossProduct(Away, FVector::UpVector) * (FMath::RandBool() ? 1.f : -1.f);
		Self->SetDodgeDirection(FMath::RandBool() ? Away : (Away * 0.3f + Side).GetSafeNormal2D());
		Self->InputBlock(false);
		Self->InputDodge();
	}
}

void AHydraAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AHydraCharacter* Self = GetHydraCharacter();
	if (!Self)
	{
		if (bHadPawn)
		{
			Destroy(); // its character is gone (a defeated wave enemy)
		}
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();

	// Held buttons are released on time in every mode (dev commands use them too).
	if (ReleaseAttackAt > 0.0 && Now >= ReleaseAttackAt)
	{
		ReleaseAttackAt = -1.0;
		Self->InputAttackReleased();
	}
	if (ReleaseBlockAt > 0.0 && Now >= ReleaseBlockAt)
	{
		ReleaseBlockAt = -1.0;
		Self->InputBlock(false);
	}
	if (ReleaseCastAt > 0.0 && Now >= ReleaseCastAt)
	{
		ReleaseCastAt = -1.0;
		Self->InputOffHand(false, Self->GetActorRotation().Yaw);
	}
	if (bPassive)
	{
		return;
	}
	if (!Self->IsAlive())
	{
		StopActing(Self);
		return;
	}

	if (Now >= NextTargetUpdate)
	{
		NextTargetUpdate = Now + HydraAI::TargetUpdateIntervalS;
		UpdateTarget(Self);
	}
	const AHydraCharacter* T = Target.Get();
	UHydraMovementComponent* Move = Self->GetHydraMovement();
	if (!T || !T->IsAlive())
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		Move->bWantsStrafe = false;
		return;
	}

	if (PendingReaction != EReaction::None && Now >= ReactAt)
	{
		React(Self);
	}

	const FHydraCombatStats& Stats = Self->GetStats();
	const FHydraAiValues& A = GetAiValues();
	const float ReachCm = Stats.Weapon.light.reach_m * 100.f;
	const FVector ToTarget = T->GetActorLocation() - Self->GetActorLocation();
	const float Gap = ToTarget.Size2D() - T->GetCapsuleComponent()->GetScaledCapsuleRadius();

	// Face the target: the controller looks at it, the character follows at its turn rate.
	// Near the target it keeps facing it while moving (like a locked-on player); far away it runs.
	SetFocus(const_cast<AHydraCharacter*>(T), EAIFocusPriority::Gameplay);
	Move->bWantsStrafe = Gap < ReachCm * HydraAI::FaceBeyondReachFactor;

	// A caster keeps its distance while it has the mana for a bolt and a clear line to its
	// target; otherwise it comes close.
	const bool bCaster = Stats.bCanCast && !Stats.bHasShield && A.cast_range_m > 0.f;
	const bool bRanged = bCaster && Self->GetMana() >= UHydraValuesSubsystem::Get(this).bolt.mana_cost
		&& !IsWayBlocked(Self, Self->GetActorLocation(), ToTarget.GetSafeNormal2D(), FMath::Max(0.f, ToTarget.Size2D() - T->GetCapsuleComponent()->GetScaledCapsuleRadius() - 10.f));
	const float CastRangeCm = A.cast_range_m * 100.f;
	const float StandGap = bRanged ? CastRangeCm * HydraAI::StandAtCastRangeRatio : ReachCm * HydraAI::StandAtReachRatio;
	if (Gap > StandGap)
	{
		Self->AddMovementInput(SteerTowards(Self, ToTarget, Now), 1.f);
	}

	// Attack: target inside reach and arc, pause over, hands free, stamina there.
	const bool bBusy = Self->HasStateTag(HydraTags::State_Attacking) || Self->HasStateTag(HydraTags::State_Blocking) || Self->HasStateTag(HydraTags::State_Dodging);
	if (!bBusy && ReleaseAttackAt < 0.0 && Now >= NextAttackTime && Stats.bHasWeapon
		&& Gap <= ReachCm * HydraAI::AttackAtReachRatio
		&& HydraCombatMath::IsInArc(Self->GetActorLocation(), Self->GetActorRotation().Yaw, T->GetActorLocation(), Stats.Weapon.heavy.half_arc_deg)
		&& HydraGas::CanAfford(Self->GetAbilitySystemComponent(), Stats.LightStaminaCost))
	{
		const bool bHeavy = FMath::FRand() < A.heavy_attack_chance;
		const float HoldS = bHeavy ? FMath::FRandRange(Stats.Weapon.heavy.min_windup_ms, Stats.Weapon.heavy.max_charge_ms) / 1000.f : HydraAI::TapHoldS;
		Self->InputAttackPressed();
		ReleaseAttackAt = Now + HoldS;
		NextAttackTime = Now + HoldS + A.attack_pause_ms / 1000.f * FMath::FRandRange(0.75f, 1.25f);
	}
	// Cast: target out of melee reach and inside the cast range. Aimed where the target is now.
	else if (bRanged && !bBusy && ReleaseAttackAt < 0.0 && ReleaseCastAt < 0.0 && Now >= NextCastTime && Gap > ReachCm && Gap <= CastRangeCm)
	{
		Self->InputOffHand(true, ToTarget.Rotation().Yaw);
		ReleaseCastAt = Now + HydraAI::TapHoldS;
		NextCastTime = Now + A.cast_pause_ms / 1000.f * FMath::FRandRange(0.75f, 1.25f);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GHydraAICommand(
	TEXT("Hydra.AI"),
	TEXT("Hydra.AI [0|1]: AI opponent passive / active (no argument: toggle). On the host."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogHydra, Warning, TEXT("Hydra.AI works on the host only"));
			return;
		}
		for (TActorIterator<AHydraAIController> It(World); It; ++It)
		{
			It->SetPassive(Args.Num() > 0 ? FCString::Atoi(*Args[0]) == 0 : !It->IsPassive());
		}
	}));
