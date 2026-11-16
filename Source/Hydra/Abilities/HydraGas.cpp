#include "Abilities/HydraGas.h"

#include "Abilities/HydraAttributeSet.h"
#include "Abilities/HydraEffects.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Data/HydraValues.h"

FActiveGameplayEffectHandle HydraGas::Apply(UAbilitySystemComponent* Target, TSubclassOf<UGameplayEffect> Effect,
	float Amount, float DurationS, UAbilitySystemComponent* Source)
{
	if (!Target || !Effect)
	{
		return FActiveGameplayEffectHandle();
	}
	UAbilitySystemComponent* Instigator = Source ? Source : Target;
	FGameplayEffectContextHandle Context = Instigator->MakeEffectContext();
	Context.AddInstigator(Instigator->GetOwnerActor(), Instigator->GetAvatarActor());
	const FGameplayEffectSpecHandle Spec = Instigator->MakeOutgoingSpec(Effect, 1.f, Context);
	if (!Spec.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}
	Spec.Data->SetSetByCallerMagnitude(HydraTags::Data_Amount, Amount);
	Spec.Data->SetSetByCallerMagnitude(HydraTags::Data_Duration, DurationS);
	return Instigator->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
}

float HydraGas::GetStamina(const UAbilitySystemComponent* Target)
{
	return Target ? Target->GetNumericAttribute(UHydraAttributeSet::GetStaminaAttribute()) : 0.f;
}

float HydraGas::GetMaxStamina(const UAbilitySystemComponent* Target)
{
	return Target ? Target->GetNumericAttribute(UHydraAttributeSet::GetMaxStaminaAttribute()) : 0.f;
}

bool HydraGas::CanAfford(const UAbilitySystemComponent* Target, float StaminaCost)
{
	// A hair of tolerance: costs and regeneration are floats that add up in ticks.
	return StaminaCost <= 0.f || GetStamina(Target) >= StaminaCost - 0.001f;
}

void HydraGas::SpendStamina(UAbilitySystemComponent* Target, float Cost, const FHydraStaminaValues& Values)
{
	if (!Target || Cost <= 0.f)
	{
		return;
	}
	Apply(Target, UHydraGE_StaminaChange::StaticClass(), -Cost);
	const float DelayS = Values.regen_delay_after_action_ms / 1000.f;
	if (DelayS > 0.f)
	{
		Apply(Target, UHydraGE_StaminaRegenDelay::StaticClass(), 0.f, DelayS);
	}
}

void HydraGas::ApplyStaminaValues(UAbilitySystemComponent* Target, float Max, float RegenPerS, FActiveGameplayEffectHandle& InOutRegen, bool bFill)
{
	if (!Target)
	{
		return;
	}
	const float Current = bFill ? Max : FMath::Min(GetStamina(Target), Max);
	Target->SetNumericAttributeBase(UHydraAttributeSet::GetMaxStaminaAttribute(), Max);
	Target->SetNumericAttributeBase(UHydraAttributeSet::GetStaminaAttribute(), Current);

	// Restart regeneration with the current rate (a running effect keeps its magnitude).
	if (InOutRegen.IsValid())
	{
		Target->RemoveActiveGameplayEffect(InOutRegen);
	}
	InOutRegen = Apply(Target, UHydraGE_StaminaRegen::StaticClass(), RegenPerS * UHydraGE_StaminaRegen::TickS);
}

float HydraGas::GetHealth(const UAbilitySystemComponent* Target)
{
	return Target ? Target->GetNumericAttribute(UHydraAttributeSet::GetHealthAttribute()) : 0.f;
}

float HydraGas::GetMaxHealth(const UAbilitySystemComponent* Target)
{
	return Target ? Target->GetNumericAttribute(UHydraAttributeSet::GetMaxHealthAttribute()) : 0.f;
}

void HydraGas::ApplyHealthValues(UAbilitySystemComponent* Target, float Max, bool bFill)
{
	if (!Target)
	{
		return;
	}
	const float Current = bFill ? Max : FMath::Min(GetHealth(Target), Max);
	Target->SetNumericAttributeBase(UHydraAttributeSet::GetMaxHealthAttribute(), Max);
	Target->SetNumericAttributeBase(UHydraAttributeSet::GetHealthAttribute(), Current);
}

void HydraGas::DealDamage(UAbilitySystemComponent* Target, float Amount, UAbilitySystemComponent* Source)
{
	if (Amount > 0.f)
	{
		Apply(Target, UHydraGE_Damage::StaticClass(), Amount, 0.f, Source);
	}
}
