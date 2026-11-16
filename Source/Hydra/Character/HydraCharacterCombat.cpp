// Combat side of AHydraCharacter, server only: hit detection with lag compensation, what a
// hit does (block, rebound, weak point, chain, interrupt, damage), death and respawn.

#include "Character/HydraCharacter.h"

#include "AI/HydraAIController.h"
#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Character/HydraMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/HydraDevSubsystem.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Hydra.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarHydraLagCompensation(
	TEXT("Hydra.LagCompensation"), 1,
	TEXT("1: melee hits and block arcs are checked against what the attacker saw (default). 0: against the server's present (for comparison)."));

namespace HydraCombat
{
	/** How much longer than the maximum rewind the pose history is kept. */
	constexpr double HistorySlackS = 0.25;

	double DevClock(const AActor* Actor)
	{
		const UGameInstance* GI = Actor ? Actor->GetGameInstance() : nullptr;
		const UHydraDevSubsystem* Dev = GI ? GI->GetSubsystem<UHydraDevSubsystem>() : nullptr;
		return Dev ? Dev->GetClock() : 0.0;
	}
}

// --- Pose history and ping ---------------------------------------------------------------

void AHydraCharacter::RecordPose()
{
	FHydraPoseSample Sample;
	Sample.Time = GetWorld()->GetTimeSeconds();
	Sample.Location = GetActorLocation();
	Sample.Yaw = GetActorRotation().Yaw;
	Sample.bBlocking = HasStateTag(HydraTags::State_Blocking);
	const double KeepS = UHydraValuesSubsystem::Get(this).lag_compensation.max_rewind_ms / 1000.0 + HydraCombat::HistorySlackS;
	PoseHistory.Add(Sample, KeepS);
}

float AHydraCharacter::GetPingMs() const
{
	// The host's own character and the AI act on the server directly: nothing to compensate.
	if (bAIControlled || IsLocallyControlled())
	{
		return 0.f;
	}
	const APlayerState* PS = GetPlayerState();
	return PS ? PS->GetPingInMilliseconds() : 0.f;
}

// --- Hit detection -----------------------------------------------------------------------

void AHydraCharacter::OnAttackStarted()
{
	if (!HasAuthority())
	{
		return;
	}
	for (TActorIterator<AHydraAIController> It(GetWorld()); It; ++It)
	{
		It->OnAttackSeen(this);
	}
}

int32 AHydraCharacter::ServerTestAttackHits(bool bHeavy, float ChargeFactor, TArray<TWeakObjectPtr<AHydraCharacter>>& AlreadyHit)
{
	check(HasAuthority());
	if (!Stats.bHasWeapon || !IsAlive())
	{
		return 0;
	}
	const FHydraValues& V = UHydraValuesSubsystem::Get(this);
	const float ReachCm = (bHeavy ? Stats.Weapon.heavy.reach_m : Stats.Weapon.light.reach_m) * 100.f;
	const float HalfArc = bHeavy ? Stats.Weapon.heavy.half_arc_deg : Stats.Weapon.light.half_arc_deg;

	// Lag compensation: check the targets where the attacker saw them. The attacker's own
	// position is the server's present - that is where its inputs have brought it.
	const float RewindS = CVarHydraLagCompensation.GetValueOnGameThread() != 0 ? HydraCombatMath::RewindSeconds(GetPingMs(), V.lag_compensation) : 0.f;
	const double Now = GetWorld()->GetTimeSeconds();
	const FVector MyLocation = GetActorLocation();
	const float MyYaw = GetActorRotation().Yaw;

	int32 NewHits = 0;
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		AHydraCharacter* Target = *It;
		if (Target == this || !Target->IsAlive() || AlreadyHit.Contains(Target))
		{
			continue;
		}
		FHydraPoseSample Pose;
		if (!Target->PoseHistory.Get(Now - RewindS, Pose))
		{
			Pose.Time = Now;
			Pose.Location = Target->GetActorLocation();
			Pose.Yaw = Target->GetActorRotation().Yaw;
			Pose.bBlocking = Target->HasStateTag(HydraTags::State_Blocking);
		}
		if (!HydraCombatMath::IsInHitArea(MyLocation, MyYaw, Pose.Location, Target->GetCapsuleComponent()->GetScaledCapsuleRadius(), ReachCm, HalfArc))
		{
			continue;
		}
		AlreadyHit.Add(Target);
		++NewHits;
		ResolveHit(Target, Pose, bHeavy, ChargeFactor, RewindS);
	}
	return NewHits;
}

void AHydraCharacter::ServerReportMiss(bool bHeavy)
{
	UE_LOG(LogHydra, Log, TEXT("HydraMiss t=%.2f attacker=%s kind=%s"), HydraCombat::DevClock(this), *GetLogId(), bHeavy ? TEXT("heavy") : TEXT("light"));
}

// --- What a hit does ---------------------------------------------------------------------

void AHydraCharacter::ResolveHit(AHydraCharacter* Target, const FHydraPoseSample& TargetPose, bool bHeavy, float ChargeFactor, float RewindS)
{
	check(HasAuthority());
	const FHydraValues& V = UHydraValuesSubsystem::Get(this);
	const FHydraCombatStats& TargetStats = Target->GetStats();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	const FVector MyLocation = GetActorLocation();

	const float AttackCost = bHeavy ? Stats.HeavyStaminaCost : Stats.LightStaminaCost;
	const float HitStopS = (bHeavy ? Stats.Weapon.heavy.hit_stop_ms : Stats.Weapon.light.hit_stop_ms) / 1000.f;
	// Chain: every further hit of a chain deals more damage.
	const int32 ChainBefore = GetChainHitsNow();
	float Damage = (bHeavy ? Stats.Weapon.heavy.damage * ChargeFactor : Stats.Weapon.light.damage) * HydraCombatMath::ChainMultiplier(V.chain, ChainBefore);

	// Directional block: only if the target was blocking and the attacker stood inside its
	// block arc - both as the attacker saw it (rewound pose).
	const TCHAR* Result = TEXT("hit");
	bool bWeakPoint = false;
	bool bInterrupted = false;
	bool bBlocked = TargetPose.bBlocking && TargetStats.bHasShield
		&& HydraCombatMath::IsInArc(TargetPose.Location, TargetPose.Yaw, MyLocation, TargetStats.Shield.block_half_arc_deg);
	if (bBlocked)
	{
		const float BlockCost = HydraCombatMath::BlockStaminaCost(V.block, Stats.Weapon.weight, TargetStats.Shield.weight, bHeavy);
		if (HydraGas::CanAfford(TargetASC, BlockCost))
		{
			// Blocked: strongly reduced damage, the blocker pays stamina by the weight of the
			// attack, and the attacker loses extra stamina (rebound).
			Result = TEXT("blocked");
			HydraGas::SpendStamina(TargetASC, BlockCost, V.stamina);
			Damage *= FMath::Clamp(1.f - TargetStats.Shield.block_damage_reduction, 0.f, 1.f);
			HydraGas::SpendStamina(AbilitySystem, HydraCombatMath::ReboundStamina(V.block, AttackCost, Stats.Weapon.weight, TargetStats.Shield.weight), V.stamina);
		}
		else
		{
			// Stamina empty: the block breaks. The hit lands in full, but nobody is stunned.
			Result = TEXT("blockbreak");
			bBlocked = false;
			HydraGas::SpendStamina(TargetASC, Target->GetStamina(), V.stamina);
			const FGameplayTagContainer BlockTag(HydraTags::Ability_Block);
			TargetASC->CancelAbilities(&BlockTag);
		}
	}
	if (!bBlocked)
	{
		// Weak point: the back, by position - no random crits. Lighter weapons gain more.
		bWeakPoint = HydraCombatMath::IsInBackArc(TargetPose.Location, TargetPose.Yaw, MyLocation, V.weak_point.back_half_arc_deg);
		if (bWeakPoint)
		{
			Damage *= HydraCombatMath::WeakPointMultiplier(V.weak_point, Stats.WeaponWeightRatio);
		}
		// Interrupt: enough damage cancels the target's attack while it binds. No stun.
		if (Target->HasStateTag(HydraTags::State_AttackCommitted)
			&& Damage >= HydraCombatMath::InterruptThreshold(V.interrupt, TargetStats.TotalWeight, TargetStats.Weapon.weight))
		{
			bInterrupted = true;
			Target->ServerInterruptAttack();
		}
		AdvanceChain();
	}

	// For the log: what would the server's present have said (shows what lag compensation changes).
	const float TargetRadius = Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float ReachCm = (bHeavy ? Stats.Weapon.heavy.reach_m : Stats.Weapon.light.reach_m) * 100.f;
	const float HalfArc = bHeavy ? Stats.Weapon.heavy.half_arc_deg : Stats.Weapon.light.half_arc_deg;
	const bool bInNow = HydraCombatMath::IsInHitArea(MyLocation, GetActorRotation().Yaw, Target->GetActorLocation(), TargetRadius, ReachCm, HalfArc);

	const bool bBlockNow = Target->HasStateTag(HydraTags::State_Blocking) && TargetStats.bHasShield
		&& HydraCombatMath::IsInArc(Target->GetActorLocation(), Target->GetActorRotation().Yaw, MyLocation, TargetStats.Shield.block_half_arc_deg);

	HydraGas::DealDamage(TargetASC, Damage, AbilitySystem);
	Target->MulticastHitFeedback(bBlocked ? 1 : 0, HitStopS);
	MulticastHitFeedback(2, HitStopS);

	UE_LOG(LogHydra, Log, TEXT("HydraHit t=%.2f attacker=%s target=%s kind=%s result=%s dmg=%.2f weak=%d chain=%d interrupt=%d rewind_ms=%.0f in_now=%d block_now=%d dist=%.0f dist_now=%.0f target_hp=%.2f target_stamina=%.2f attacker_stamina=%.2f"),
		HydraCombat::DevClock(this), *GetLogId(), *Target->GetLogId(), bHeavy ? TEXT("heavy") : TEXT("light"), Result, Damage, bWeakPoint ? 1 : 0,
		GetChainHitsNow(), bInterrupted ? 1 : 0, RewindS * 1000.f, bInNow ? 1 : 0, bBlockNow ? 1 : 0,
		FVector::Dist2D(MyLocation, TargetPose.Location), FVector::Dist2D(MyLocation, Target->GetActorLocation()),
		Target->GetHealth(), Target->GetStamina(), GetStamina());
}

void AHydraCharacter::ServerInterruptAttack()
{
	const FGameplayTagContainer AttackTag(HydraTags::Ability_Attack);
	AbilitySystem->CancelAbilities(&AttackTag);
	ClearChain(); // being interrupted ends your own chain
}

// --- Chain -------------------------------------------------------------------------------

int32 AHydraCharacter::GetChainHitsNow() const
{
	const double ResetS = UHydraValuesSubsystem::Get(this).chain.reset_after_ms / 1000.0;
	return (GetWorld()->GetTimeSeconds() - LastChainHitTime <= ResetS) ? ChainCount : 0;
}

void AHydraCharacter::AdvanceChain()
{
	ChainCount = static_cast<uint8>(FMath::Min(GetChainHitsNow() + 1, 255));
	LastChainHitTime = GetWorld()->GetTimeSeconds();
}

void AHydraCharacter::ClearChain()
{
	ChainCount = 0;
	LastChainHitTime = -1000.0;
}

// --- Damage, death, respawn --------------------------------------------------------------

void AHydraCharacter::OnDamaged(float Damage)
{
	if (HasAuthority() && IsAlive() && GetHealth() <= 0.f)
	{
		Die();
	}
}

void AHydraCharacter::Die()
{
	check(HasAuthority());
	bDead = true;
	AbilitySystem->CancelAllAbilities();
	AbilitySystem->SetLooseGameplayTagCount(HydraTags::State_Dead, 1);
	ActionState = FHydraActionState();
	ClearChain();
	SetLockOnTarget(nullptr);
	const float DelayS = FMath::Max(0.1f, UHydraValuesSubsystem::Get(this).character.respawn_delay_ms / 1000.f);
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AHydraCharacter::Respawn, DelayS, false);
	UE_LOG(LogHydra, Log, TEXT("HydraDeath t=%.2f who=%s"), HydraCombat::DevClock(this), *GetLogId());
}

void AHydraCharacter::Respawn()
{
	// Arena rule: no respawn penalty. Back to the spawn point with full bars.
	ServerTeleport(SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator().Yaw);
	ServerResetState();
	UE_LOG(LogHydra, Log, TEXT("HydraRespawn t=%.2f who=%s"), HydraCombat::DevClock(this), *GetLogId());
}

void AHydraCharacter::ServerResetState()
{
	check(HasAuthority());
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	bDead = false;
	AbilitySystem->SetLooseGameplayTagCount(HydraTags::State_Dead, 0);
	HydraGas::ApplyHealthValues(AbilitySystem, Stats.HpMax, true);
	HydraGas::ApplyStaminaValues(AbilitySystem, Stats.StaminaMax, Stats.StaminaRegenPerS, StaminaRegenHandle, true);
	ClearChain();
}

void AHydraCharacter::ServerTeleport(const FVector& Location, float Yaw)
{
	check(HasAuthority());
	const FRotator Rotation(0.f, Yaw, 0.f);
	TeleportTo(Location, Rotation, false, true);
	GetCharacterMovement()->StopMovementImmediately();
	PoseHistory.Reset(); // nobody may hit the place the character was before
	if (GetController())
	{
		GetController()->SetControlRotation(FRotator(-10.f, Yaw, 0.f));
	}
	if (!bAIControlled && !IsLocallyControlled())
	{
		ClientSetFacing(Yaw);
	}
}

void AHydraCharacter::ServerSetStamina(float Value)
{
	check(HasAuthority());
	const float Current = GetStamina();
	if (Value < Current)
	{
		HydraGas::SpendStamina(AbilitySystem, Current - Value, UHydraValuesSubsystem::Get(this).stamina);
	}
}
