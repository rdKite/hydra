#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "HydraAttributeSet.generated.h"

/** Declares the usual accessors for an attribute: GetXAttribute(), GetX(), SetX(), InitX(). */
#define HYDRA_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Attributes of a character: health, stamina and mana. Changed on the
 * server only and replicated to everyone (both sides see the bars). Damage arrives through
 * the meta attribute IncomingDamage.
 */
UCLASS()
class HYDRA_API UHydraAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(ReplicatedUsing = OnRep_Health) FGameplayAttributeData Health;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, Health)

	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth) FGameplayAttributeData MaxHealth;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, MaxHealth)

	UPROPERTY(ReplicatedUsing = OnRep_Stamina) FGameplayAttributeData Stamina;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, Stamina)

	UPROPERTY(ReplicatedUsing = OnRep_MaxStamina) FGameplayAttributeData MaxStamina;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, MaxStamina)

	UPROPERTY(ReplicatedUsing = OnRep_Mana) FGameplayAttributeData Mana;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, Mana)

	UPROPERTY(ReplicatedUsing = OnRep_MaxMana) FGameplayAttributeData MaxMana;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, MaxMana)

	/** Meta attribute (server only, not replicated): damage to take from Health. */
	UPROPERTY() FGameplayAttributeData IncomingDamage;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, IncomingDamage)

protected:
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_Stamina(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxStamina(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_Mana(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxMana(const FGameplayAttributeData& Old);
};
