#include "Abilities/HydraGameplayAbility.h"

#include "Abilities/HydraGas.h"
#include "AbilitySystemComponent.h"
#include "Character/HydraCharacter.h"
#include "Character/HydraProgressionComponent.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

UHydraGameplayAbility::UHydraGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

AHydraCharacter* UHydraGameplayAbility::GetHydraCharacter() const
{
	return Cast<AHydraCharacter>(GetAvatarActorFromActorInfo());
}

const FHydraValues& UHydraGameplayAbility::GetValues() const
{
	return UHydraValuesSubsystem::Get(GetAvatarActorFromActorInfo());
}

bool UHydraGameplayAbility::IsServer() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	return Avatar && Avatar->HasAuthority();
}

bool UHydraGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AHydraCharacter* C = ActorInfo ? Cast<AHydraCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!C || !C->GetStats().bValid || !CanStart(C->GetStats()))
	{
		return false;
	}
	return HydraGas::CanAfford(C->GetAbilitySystemComponent(), GetStaminaCost(C->GetStats()));
}

void UHydraGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// Server-authoritative: only the server changes stamina; the client sees the replicated value.
	const AHydraCharacter* C = ActorInfo ? Cast<AHydraCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (C && C->HasAuthority())
	{
		const float Cost = FMath::Min(GetStaminaCost(C->GetStats()), C->GetStamina());
		HydraGas::SpendStamina(C->GetAbilitySystemComponent(), Cost);
		C->GetProgression()->ServerAddUse(GetUseSkill(C->GetStats()), Cost);
	}
}

void UHydraGameplayAbility::SpendStamina(float Cost) const
{
	const AHydraCharacter* C = GetHydraCharacter();
	if (C && C->HasAuthority())
	{
		const float Spent = FMath::Min(Cost, C->GetStamina());
		HydraGas::SpendStamina(C->GetAbilitySystemComponent(), Spent);
		C->GetProgression()->ServerAddUse(GetUseSkill(C->GetStats()), Spent);
	}
}

void UHydraGameplayAbility::StartTimer(float DelayS, TFunction<void()> Callback)
{
	if (DelayS <= 0.f)
	{
		Callback();
		return;
	}
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, MoveTemp(Callback)), DelayS, false);
	Timers.Add(Handle);
}

void UHydraGameplayAbility::StartLoopingTimer(float IntervalS, TFunction<void()> Callback)
{
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, MoveTemp(Callback)), IntervalS, true);
	Timers.Add(Handle);
}

void UHydraGameplayAbility::ClearTimers()
{
	if (const UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : Timers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}
	Timers.Reset();
}

void UHydraGameplayAbility::FinishAbility()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UHydraGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearTimers();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
