// Combat side of AHydraCharacter, server only: hit detection with lag compensation, what a
// hit does (block, rebound, weak point, chain, interrupt, damage), death and respawn.

#include "Character/HydraCharacter.h"

#include "AI/HydraAIController.h"
#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Character/HydraMovementComponent.h"
#include "Character/HydraProgressionComponent.h"
#include "Combat/HydraProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Core/HydraDevSubsystem.h"
#include "Core/HydraGameMode.h"
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
	++AttackSerial;
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
		if (!CanHurt(Target) || !Target->IsAlive() || AlreadyHit.Contains(Target))
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
		const float TargetRadius = Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		if (!HydraCombatMath::IsInHitArea(MyLocation, MyYaw, Pose.Location, TargetRadius, ReachCm, HalfArc))
		{
			// A dodge counts only against a real attack: the target is out of the hit area now,
			// stepped away a moment ago, and would have been hit where its step began.
			const double DodgeWindowS = (V.dodge.duration_ms + V.dodge.recovery_ms) / 1000.0;
			if (Now - Target->DodgeStartTime <= DodgeWindowS && !(Target->LastAvoidedAttacker == this && Target->LastAvoidedSerial == AttackSerial)
				&& HydraCombatMath::IsInHitArea(MyLocation, MyYaw, Target->DodgeStartLocation, TargetRadius, ReachCm, HalfArc))
			{
				Target->LastAvoidedAttacker = this;
				Target->LastAvoidedSerial = AttackSerial;
				++Target->Metrics.DodgesAvoided;
				UE_LOG(LogHydra, Log, TEXT("HydraDodged t=%.2f who=%s attacker=%s"), HydraCombat::DevClock(this), *Target->GetLogId(), *GetLogId());
			}
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
	++Metrics.Misses;
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

	const float HitStopS = (bHeavy ? Stats.Weapon.heavy.hit_stop_ms : Stats.Weapon.light.hit_stop_ms) / 1000.f;
	// Damage = base x (1 + progress bonuses, already in Stats.Weapon) x the situational
	// factors charge, chain and weak point, then through the target's resistance.
	const int32 ChainBefore = GetChainHitsNow();
	float Damage = (bHeavy ? Stats.Weapon.heavy.damage * ChargeFactor : Stats.Weapon.light.damage) * HydraCombatMath::ChainMultiplier(V.chain, ChainBefore);
	const float ResistancePoints = HydraCombatMath::ResistanceTo(TargetStats.Resistance, bHeavy ? Stats.HeavyDamageType : Stats.LightDamageType);

	// Directional block: only if the target was blocking and the attacker stood inside its
	// block arc - both as the attacker saw it (rewound pose).
	const TCHAR* Result = TEXT("hit");
	bool bWeakPoint = false;
	bool bInterrupted = false;
	bool bBlocked = TargetPose.bBlocking && TargetStats.bHasShield
		&& HydraCombatMath::IsInArc(TargetPose.Location, TargetPose.Yaw, MyLocation, TargetStats.Shield.block_half_arc_deg);
	if (!bBlocked)
	{
		// Weak point: the back, by position - no random crits. Dexterity raises the factor.
		bWeakPoint = HydraCombatMath::IsInBackArc(TargetPose.Location, TargetPose.Yaw, MyLocation, V.weak_point.back_half_arc_deg);
		if (bWeakPoint)
		{
			Damage *= Stats.WeakPointMultiplier;
		}
	}
	Damage = HydraCombatMath::ResistedDamage(V.resistance, Damage, ResistancePoints);
	if (bBlocked)
	{
		// Block cost and rebound are shares of the damage actually blocked.
		const float BlockedDamage = Damage * FMath::Clamp(TargetStats.Shield.block_damage_reduction, 0.f, 1.f);
		const float BlockCost = BlockedDamage * TargetStats.BlockerShare;
		if (HydraGas::CanAfford(TargetASC, BlockCost))
		{
			// Blocked: strongly reduced damage, the blocker pays stamina for what the shield
			// took, and the attacker loses stamina too (rebound).
			Result = TEXT("blocked");
			++Metrics.HitsBlocked;
			++Target->Metrics.Blocks;
			HydraGas::SpendStamina(TargetASC, BlockCost);
			Target->Progression->ServerAddUse(TargetStats.ShieldSkill, BlockCost);
			Damage -= BlockedDamage;
			HydraGas::SpendStamina(AbilitySystem, HydraCombatMath::ReboundStamina(V.block, BlockedDamage));
		}
		else
		{
			// Stamina empty: the block breaks. The hit lands in full, but nobody is stunned.
			Result = TEXT("blockbreak");
			bBlocked = false;
			++Target->Metrics.BlockBreaks;
			HydraGas::SpendStamina(TargetASC, Target->GetStamina());
			const FGameplayTagContainer BlockTag(HydraTags::Ability_Block);
			TargetASC->CancelAbilities(&BlockTag);
		}
	}
	if (!bBlocked)
	{
		++Metrics.Hits;
		++Target->Metrics.HitsTaken;
		AdvanceChain();
	}
	// Interrupt: a hit that takes a big enough share of the target's maximum HP cancels its
	// attack while it binds. No stun.
	if (Target->HasStateTag(HydraTags::State_AttackCommitted) && Damage >= TargetStats.InterruptThreshold)
	{
		bInterrupted = true;
		Target->ServerInterruptAttack();
	}

	// For the log: what would the server's present have said (shows what lag compensation changes).
	const float TargetRadius = Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float ElementBodyDamage = TargetStats.ElementBodyDamage;
	const float ReachCm = (bHeavy ? Stats.Weapon.heavy.reach_m : Stats.Weapon.light.reach_m) * 100.f;
	const float HalfArc = bHeavy ? Stats.Weapon.heavy.half_arc_deg : Stats.Weapon.light.half_arc_deg;
	const bool bInNow = HydraCombatMath::IsInHitArea(MyLocation, GetActorRotation().Yaw, Target->GetActorLocation(), TargetRadius, ReachCm, HalfArc);

	const bool bBlockNow = Target->HasStateTag(HydraTags::State_Blocking) && TargetStats.bHasShield
		&& HydraCombatMath::IsInArc(Target->GetActorLocation(), Target->GetActorRotation().Yaw, MyLocation, TargetStats.Shield.block_half_arc_deg);

	Target->LastDamager = this;
	HydraGas::DealDamage(TargetASC, Damage, AbilitySystem);
	Target->MulticastHitFeedback(bBlocked ? 1 : 0, HitStopS);
	MulticastHitFeedback(2, HitStopS);

	UE_LOG(LogHydra, Log, TEXT("HydraHit t=%.2f attacker=%s target=%s kind=%s result=%s dmg=%.2f weak=%d chain=%d interrupt=%d rewind_ms=%.0f in_now=%d block_now=%d dist=%.0f dist_now=%.0f target_hp=%.2f target_stamina=%.2f attacker_stamina=%.2f"),
		HydraCombat::DevClock(this), *GetLogId(), *Target->GetLogId(), bHeavy ? TEXT("heavy") : TEXT("light"), Result, Damage, bWeakPoint ? 1 : 0,
		GetChainHitsNow(), bInterrupted ? 1 : 0, RewindS * 1000.f, bInNow ? 1 : 0, bBlockNow ? 1 : 0,
		FVector::Dist2D(MyLocation, TargetPose.Location), FVector::Dist2D(MyLocation, Target->GetActorLocation()),
		Target->GetHealth(), Target->GetStamina(), GetStamina());

	// Element body: a melee hit against it deals some of its element's damage back.
	if (ElementBodyDamage > 0.f && IsAlive())
	{
		const float Back = ServerDealDamage(Target, this, ElementBodyDamage, TEXT("fire"));
		UE_LOG(LogHydra, Log, TEXT("HydraElementBody t=%.2f body=%s attacker=%s dmg=%.2f attacker_hp=%.2f"), HydraCombat::DevClock(this), *Target->GetLogId(), *GetLogId(), Back, GetHealth());
	}
}

float AHydraCharacter::ServerDealDamage(AHydraCharacter* Source, AHydraCharacter* Target, float RawDamage, FName DamageType)
{
	if (!Target || !Target->HasAuthority() || !Target->IsAlive())
	{
		return 0.f;
	}
	const float Dealt = HydraCombatMath::ResistedDamage(UHydraValuesSubsystem::Get(Target).resistance, RawDamage, HydraCombatMath::ResistanceTo(Target->GetStats().Resistance, DamageType));
	if (Source && Source != Target)
	{
		Target->LastDamager = Source;
	}
	if (DamageType == FName(TEXT("fire")))
	{
		// Imprint progress comes from contact with the element: taking its damage and dealing it.
		const FHydraImprintValues& I = UHydraValuesSubsystem::Get(Target).imprint;
		Target->Progression->ServerAddImprintProgress(Dealt * I.progress_per_damage_taken);
		if (Source && Source != Target)
		{
			Source->Progression->ServerAddImprintProgress(Dealt * I.progress_per_damage_dealt);
		}
	}
	HydraGas::DealDamage(Target->GetAbilitySystemComponent(), Dealt, Source ? Source->GetAbilitySystemComponent() : nullptr);
	return Dealt;
}

void AHydraCharacter::ServerFireBolt(float WantedChargeAlpha)
{
	check(HasAuthority());
	const FHydraBoltValues& B = UHydraValuesSubsystem::Get(this).bolt;
	const bool bFire = HasFireImprint();
	++Metrics.Casts;
	// The base cost was paid at the press; the charge costs extra, as far as the mana reaches.
	const float Charge = HydraCombatMath::AffordableBoltCharge(B, WantedChargeAlpha, GetMana());
	const float ChargeCost = HydraCombatMath::BoltManaCost(B, Charge) - B.mana_cost;
	HydraGas::SpendMana(AbilitySystem, ChargeCost);
	ServerNoteBoltMana(ChargeCost);

	// Along the facing, level, from the middle of the body.
	const FVector Direction = FRotator(0.f, GetActorRotation().Yaw, 0.f).Vector();
	const FVector Start = GetActorLocation() + Direction * (Stats.BodyRadiusCm + B.radius_m * 100.f + 5.f);
	AHydraProjectile* Bolt = GetWorld()->SpawnActorDeferred<AHydraProjectile>(AHydraProjectile::StaticClass(), FTransform(Start), this, this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Bolt)
	{
		// Spell damage: base x (1 + skill bonus + imprint bonus for the element) x charge.
		Bolt->Launch(this, Direction, HydraCombatMath::BoltDamage(B, Charge) * (1.f + (bFire ? Stats.FireBoltBonus : Stats.BoltBonus)), bFire, Charge);
		Bolt->FinishSpawning(FTransform(Start));
	}
	UE_LOG(LogHydra, Log, TEXT("HydraCast t=%.2f attacker=%s charge=%.2f wanted=%.2f fire=%d yaw=%.1f mana=%.2f"),
		HydraCombat::DevClock(this), *GetLogId(), Charge, WantedChargeAlpha, bFire ? 1 : 0, GetActorRotation().Yaw, GetMana());
}

// --- Burning -----------------------------------------------------------------------------

void AHydraCharacter::SetBurningStacks(int32 NewStacks, const TCHAR* Reason)
{
	const uint8 Clamped = static_cast<uint8>(FMath::Clamp(NewStacks, 0, FMath::Max(0, UHydraValuesSubsystem::Get(this).burning.max_stacks)));
	if (Clamped == BurningStacks)
	{
		return;
	}
	if (BurningStacks == 0)
	{
		NextBurningTick = GetWorld()->GetTimeSeconds() + UHydraValuesSubsystem::Get(this).burning.tick_interval_ms / 1000.0;
	}
	BurningStacks = Clamped;
	AbilitySystem->SetLooseGameplayTagCount(HydraTags::State_Burning, BurningStacks > 0 ? 1 : 0);
	UE_LOG(LogHydra, Log, TEXT("HydraBurn t=%.2f who=%s stacks=%d reason=%s"), HydraCombat::DevClock(this), *GetLogId(), BurningStacks, Reason);
}

void AHydraCharacter::ServerTouchFireField(AHydraCharacter* Source)
{
	LastInFireFieldTime = GetWorld()->GetTimeSeconds();
	if (Source)
	{
		BurningSource = Source;
	}
}

void AHydraCharacter::ServerAddBurning(int32 Stacks, AHydraCharacter* Source, const TCHAR* Reason)
{
	if (!IsAlive() || Stacks <= 0)
	{
		return;
	}
	if (BurningStacks == 0)
	{
		NextBurningStackChange = GetWorld()->GetTimeSeconds() + HydraCombatMath::BurningIntervalS(UHydraValuesSubsystem::Get(this).burning, UHydraValuesSubsystem::Get(this).resistance, Stats.Resistance.fire);
	}
	BurningSource = Source;
	SetBurningStacks(BurningStacks + Stacks, Reason);
}

void AHydraCharacter::UpdateBurning()
{
	// In a fire field the stack rises by one per interval (the first at once); outside it
	// drops by one per interval - burning goes on for a while after leaving the field.
	// Fire resistance lengthens the interval. Each stack deals damage over time.
	if (!IsAlive())
	{
		return;
	}
	const FHydraBurningValues& B = UHydraValuesSubsystem::Get(this).burning;
	const double Now = GetWorld()->GetTimeSeconds();
	const double Interval = HydraCombatMath::BurningIntervalS(B, UHydraValuesSubsystem::Get(this).resistance, Stats.Resistance.fire);
	const bool bInField = Now - LastInFireFieldTime < 0.25;
	if (bInField)
	{
		if (BurningStacks == 0)
		{
			NextBurningStackChange = Now + Interval;
			SetBurningStacks(1, TEXT("field"));
		}
		else if (Now >= NextBurningStackChange)
		{
			NextBurningStackChange = Now + Interval;
			SetBurningStacks(BurningStacks + 1, TEXT("field"));
		}
	}
	else if (BurningStacks > 0 && Now >= NextBurningStackChange)
	{
		NextBurningStackChange = Now + Interval;
		SetBurningStacks(BurningStacks - 1, TEXT("decay"));
	}
	if (BurningStacks > 0 && Now >= NextBurningTick)
	{
		const float TickS = FMath::Max(0.05f, B.tick_interval_ms / 1000.f);
		NextBurningTick = Now + TickS;
		ServerDealDamage(BurningSource.Get(), this, B.damage_per_s_per_stack * BurningStacks * TickS, TEXT("fire"));
	}
}

void AHydraCharacter::ServerSetForm(FName NewFormId)
{
	check(HasAuthority());
	if (NewFormId.IsNone() || NewFormId == FormId)
	{
		return;
	}
	FormId = NewFormId;
	InitProgress();   // a form chosen at the start begins at its start levels
	ChangeForm(NewFormId);
}

void AHydraCharacter::ServerEvolveTo(FName NewFormId)
{
	check(HasAuthority());
	ChangeForm(NewFormId);
}

void AHydraCharacter::ServerSetFireImprint(bool bOn)
{
	check(HasAuthority());
	Progression->ServerSetImprintLevel(bOn ? FMath::Max(1, Progression->GetImprintLevel()) : 0);
}

void AHydraCharacter::ServerNoteBoltMana(float Mana) const
{
	// The bolt is the body skill projectile; a fire bolt is also a cast of fire magic.
	Progression->ServerAddUse(TEXT("projectile"), Mana);
	if (HasFireImprint())
	{
		Progression->ServerAddUse(TEXT("fire_magic"), Mana);
	}
}

void AHydraCharacter::ServerNoteDodge()
{
	++Metrics.Dodges;
	DodgeStartTime = GetWorld()->GetTimeSeconds();
	DodgeStartLocation = GetActorLocation();
}

void AHydraCharacter::ChangeForm(FName NewFormId)
{
	AbilitySystem->CancelAllAbilities();
	ActionState = FHydraActionState();
	FormId = NewFormId;
	bAttributesInitialized = false;   // full bars of the new form
	RebuildStats();
	ClearChain();
	SetBurningStacks(0, TEXT("form"));
	PoseHistory.Reset();
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
	Metrics.DamageTaken += Damage;
	if (AHydraCharacter* Damager = LastDamager.Get())
	{
		Damager->Metrics.DamageDealt += Damage;
	}
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
	SetBurningStacks(0, TEXT("death"));
	SetLockOnTarget(nullptr);
	const float DelayS = FMath::Max(0.1f, UHydraValuesSubsystem::Get(this).character.respawn_delay_ms / 1000.f);
	AHydraCharacter* Killer = LastDamager.Get();
	++Metrics.Deaths;
	if (Killer && Killer != this)
	{
		++Killer->Metrics.Kills;
	}
	UE_LOG(LogHydra, Log, TEXT("HydraDeath t=%.2f who=%s killer=%s"), HydraCombat::DevClock(this), *GetLogId(), Killer ? *Killer->GetLogId() : TEXT("none"));
	if (bRespawns)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &AHydraCharacter::Respawn, DelayS, false);
	}
	else
	{
		SetLifeSpan(DelayS); // a wave enemy lies there for a moment and is removed
	}
	if (AHydraGameMode* GameMode = GetWorld()->GetAuthGameMode<AHydraGameMode>())
	{
		GameMode->OnCharacterDied(this, Killer);
	}
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
	HydraGas::ApplyManaValues(AbilitySystem, Stats.ManaMax, Stats.ManaRegenPerS, ManaRegenHandle, true);
	ClearChain();
	SetBurningStacks(0, TEXT("reset"));
	LastInFireFieldTime = -1000.0;
	LastDamager = nullptr;
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

void AHydraCharacter::ServerRefill()
{
	check(HasAuthority());
	HydraGas::ApplyStaminaValues(AbilitySystem, Stats.StaminaMax, Stats.StaminaRegenPerS, StaminaRegenHandle, true);
	HydraGas::ApplyManaValues(AbilitySystem, Stats.ManaMax, Stats.ManaRegenPerS, ManaRegenHandle, true);
}

void AHydraCharacter::ServerSetStamina(float Value)
{
	check(HasAuthority());
	const float Current = GetStamina();
	if (Value < Current)
	{
		HydraGas::SpendStamina(AbilitySystem, Current - Value);
	}
}
