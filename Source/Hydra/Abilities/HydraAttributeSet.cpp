#include "Abilities/HydraAttributeSet.h"

#include "Character/HydraCharacter.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UHydraAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UHydraAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHydraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHydraAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHydraAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}

void UHydraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UHydraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// Clamp the base value too (PreAttributeChange only clamps the current value).
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f);
		if (Damage > 0.f && GetHealth() > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth()));
			if (AHydraCharacter* Owner = Cast<AHydraCharacter>(GetOwningActor()))
			{
				Owner->OnDamaged(Damage);
			}
		}
	}
}

void UHydraAttributeSet::OnRep_Health(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UHydraAttributeSet, Health, Old); }
void UHydraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UHydraAttributeSet, MaxHealth, Old); }
void UHydraAttributeSet::OnRep_Stamina(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UHydraAttributeSet, Stamina, Old); }
void UHydraAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UHydraAttributeSet, MaxStamina, Old); }
