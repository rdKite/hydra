#include "Abilities/HydraCombatAbilities.h"

#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "AbilitySystemComponent.h"
#include "Character/HydraCharacter.h"
#include "Character/HydraCombatMath.h"
#include "Character/HydraMovementComponent.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/World.h"

namespace HydraAbilityConstants
{
	/** How often the server checks the hit area during a strike (technical, not a gameplay value). */
	constexpr float HitTestIntervalS = 1.f / 60.f;
}

// --- Attack ------------------------------------------------------------------------------

UHydraGA_Attack::UHydraGA_Attack()
{
	SetAssetTags(FGameplayTagContainer(HydraTags::Ability_Attack));
	ActivationOwnedTags.AddTag(HydraTags::State_Attacking);
	ActivationBlockedTags.AddTag(HydraTags::State_Attacking);
	ActivationBlockedTags.AddTag(HydraTags::State_Blocking);
	ActivationBlockedTags.AddTag(HydraTags::State_Dodging);
	ActivationBlockedTags.AddTag(HydraTags::State_Dead);
}

bool UHydraGA_Attack::CanStart(const FHydraCombatStats& Stats) const
{
	return Stats.bHasWeapon;
}

float UHydraGA_Attack::GetStaminaCost(const FHydraCombatStats& Stats) const
{
	// Every attack starts as a light one; a heavy attack pays the difference when it is clear
	// that the button is still held.
	return Stats.LightStaminaCost;
}

float UHydraGA_Attack::ElapsedMs() const
{
	return static_cast<float>((GetWorld()->GetTimeSeconds() - StartWorldTime) * 1000.0);
}

void UHydraGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	AHydraCharacter* C = GetHydraCharacter();
	const FHydraWeaponValues& Weapon = C->GetStats().Weapon;

	Phase = EPhase::Windup;
	bHeld = true;
	bHeavy = false;
	ChargeFactor = 1.f;
	HitCount = 0;
	AlreadyHit.Reset();
	StartWorldTime = GetWorld()->GetTimeSeconds();

	SetCommitted(true);
	C->BeginAttackFacing();
	C->SetAction(EHydraAction::LightWindup, Weapon.light.windup_ms / 1000.f);
	C->OnAttackStarted();

	StartTimer(Weapon.light.windup_ms / 1000.f, [this]() { OnLightWindupOver(); });
}

void UHydraGA_Attack::OnInputReleased()
{
	bHeld = false;
	if (Phase != EPhase::Charging)
	{
		return; // during the light wind-up the release is evaluated when the wind-up is over
	}
	const AHydraCharacter* C = GetHydraCharacter();
	const FHydraHeavyAttackValues& Heavy = C->GetStats().Weapon.heavy;
	const float Elapsed = ElapsedMs();
	ClearTimers();
	if (Elapsed >= Heavy.min_windup_ms)
	{
		StartStrike(true, HydraCombatMath::HeavyChargeFactor(Heavy, Elapsed));
	}
	else
	{
		// Released early: the heavy attack still needs its minimum wind-up (the telegraph).
		StartTimer((Heavy.min_windup_ms - Elapsed) / 1000.f, [this]() { StartStrike(true, 1.f); });
	}
}

void UHydraGA_Attack::OnLightWindupOver()
{
	AHydraCharacter* C = GetHydraCharacter();
	const FHydraCombatStats& Stats = C->GetStats();
	const float ExtraCost = FMath::Max(0.f, Stats.HeavyStaminaCost - Stats.LightStaminaCost);
	if (!bHeld || !HydraGas::CanAfford(C->GetAbilitySystemComponent(), ExtraCost))
	{
		StartStrike(false, 1.f);
		return;
	}
	// Still held: this is a heavy attack. Keep charging until release or full charge.
	SpendStamina(ExtraCost);
	bHeavy = true;
	Phase = EPhase::Charging;
	const FHydraHeavyAttackValues& Heavy = Stats.Weapon.heavy;
	const float RemainingS = FMath::Max(0.f, (Heavy.max_charge_ms - ElapsedMs()) / 1000.f);
	C->SetAction(EHydraAction::HeavyCharge, RemainingS);
	StartTimer(RemainingS, [this]() { StartStrike(true, HydraCombatMath::HeavyChargeFactor(GetHydraCharacter()->GetStats().Weapon.heavy, ElapsedMs())); });
}

void UHydraGA_Attack::StartStrike(bool bHeavyStrike, float InChargeFactor)
{
	ClearTimers();
	AHydraCharacter* C = GetHydraCharacter();
	const FHydraWeaponValues& Weapon = C->GetStats().Weapon;
	bHeavy = bHeavyStrike;
	ChargeFactor = InChargeFactor;
	Phase = EPhase::Strike;
	const float HitS = (bHeavy ? Weapon.heavy.hit_ms : Weapon.light.hit_ms) / 1000.f;
	C->SetAction(bHeavy ? EHydraAction::HeavyStrike : EHydraAction::LightStrike, HitS);
	if (IsServer())
	{
		TestHits();
		StartLoopingTimer(HydraAbilityConstants::HitTestIntervalS, [this]() { TestHits(); });
	}
	StartTimer(HitS, [this]() { EndStrike(); });
}

void UHydraGA_Attack::TestHits()
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		HitCount += C->ServerTestAttackHits(bHeavy, ChargeFactor, AlreadyHit);
	}
}

void UHydraGA_Attack::EndStrike()
{
	ClearTimers();
	AHydraCharacter* C = GetHydraCharacter();
	if (IsServer())
	{
		TestHits(); // last check at the end of the hit window
		if (HitCount == 0)
		{
			C->ServerReportMiss(bHeavy);
		}
	}
	// A miss binds too: the attack plays out. From here on a dodge may cancel the recovery.
	SetCommitted(false);
	C->EndAttackFacing();
	Phase = EPhase::Recovery;
	const FHydraWeaponValues& Weapon = C->GetStats().Weapon;
	const float RecoveryS = (bHeavy ? Weapon.heavy.recovery_ms : Weapon.light.recovery_ms) / 1000.f;
	C->SetAction(EHydraAction::Recovery, RecoveryS);
	StartTimer(RecoveryS, [this]() { FinishAbility(); });
}

void UHydraGA_Attack::SetCommitted(bool bCommitted)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || bCommitted == bCommittedTag)
	{
		return;
	}
	bCommittedTag = bCommitted;
	if (bCommitted)
	{
		ASC->AddLooseGameplayTag(HydraTags::State_AttackCommitted);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(HydraTags::State_AttackCommitted);
	}
}

void UHydraGA_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	SetCommitted(false);
	Phase = EPhase::None;
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->EndAttackFacing();
		C->ClearAction({ EHydraAction::LightWindup, EHydraAction::HeavyCharge, EHydraAction::LightStrike, EHydraAction::HeavyStrike, EHydraAction::Recovery });
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// --- Block -------------------------------------------------------------------------------

UHydraGA_Block::UHydraGA_Block()
{
	SetAssetTags(FGameplayTagContainer(HydraTags::Ability_Block));
	ActivationOwnedTags.AddTag(HydraTags::State_Blocking);
	ActivationBlockedTags.AddTag(HydraTags::State_Blocking);
	ActivationBlockedTags.AddTag(HydraTags::State_Attacking);
	ActivationBlockedTags.AddTag(HydraTags::State_Dodging);
	ActivationBlockedTags.AddTag(HydraTags::State_Dead);
}

bool UHydraGA_Block::CanStart(const FHydraCombatStats& Stats) const
{
	return Stats.bHasShield;
}

void UHydraGA_Block::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	GetHydraCharacter()->SetAction(EHydraAction::Block, 0.f);
	if (GetValues().block.no_regen_while_holding)
	{
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(HydraTags::State_StaminaRegenBlocked);
		bRegenBlockedTag = true;
	}
	// Ends when the button is released (AHydraCharacter::InputBlock cancels it) or the block breaks.
}

void UHydraGA_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bRegenBlockedTag)
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(HydraTags::State_StaminaRegenBlocked);
		bRegenBlockedTag = false;
	}
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->ClearAction({ EHydraAction::Block });
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// --- Dodge -------------------------------------------------------------------------------

UHydraGA_Dodge::UHydraGA_Dodge()
{
	SetAssetTags(FGameplayTagContainer(HydraTags::Ability_Dodge));
	ActivationOwnedTags.AddTag(HydraTags::State_Dodging);
	ActivationBlockedTags.AddTag(HydraTags::State_AttackCommitted);
	ActivationBlockedTags.AddTag(HydraTags::State_Dodging);
	ActivationBlockedTags.AddTag(HydraTags::State_Dead);
	CancelAbilitiesWithTag.AddTag(HydraTags::Ability_Attack);
	CancelAbilitiesWithTag.AddTag(HydraTags::Ability_Block);
}

float UHydraGA_Dodge::GetStaminaCost(const FHydraCombatStats& Stats) const
{
	return Stats.DodgeStaminaCost;
}

void UHydraGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	AHydraCharacter* C = GetHydraCharacter();
	const FHydraDodgeValues& D = GetValues().dodge;
	const float DurationS = FMath::Max(0.05f, D.duration_ms / 1000.f);
	const float DistanceCm = D.distance_m * 100.f;

	// Direction: the current movement input (the same on client and server through the
	// predicted move); without input straight back.
	// The AI names its direction itself (it has no input from a client).
	FVector Dir = C->ConsumeDodgeDirection().GetSafeNormal2D();
	if (Dir.IsNearlyZero())
	{
		Dir = C->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
	}
	if (Dir.IsNearlyZero())
	{
		Dir = -C->GetActorForwardVector().GetSafeNormal2D();
	}

	// A predicted root-motion source moves the capsule; nothing else happens - no invulnerability.
	UAbilityTask_ApplyRootMotionConstantForce* Task = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, TEXT("Dodge"), Dir, DistanceCm / DurationS, DurationS, false, nullptr,
		ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, true);
	Task->ReadyForActivation();

	C->SetAction(EHydraAction::Dodge, DurationS);
	StartTimer(DurationS + D.recovery_ms / 1000.f, [this]() { FinishAbility(); });
}

void UHydraGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->ClearAction({ EHydraAction::Dodge });
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
