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
 * Attributes of a character. M1 has stamina only; HP and mana follow with M2 and M3.
 * Changed on the server only and replicated to everyone (both sides see the bars).
 */
UCLASS()
class HYDRA_API UHydraAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(ReplicatedUsing = OnRep_Stamina) FGameplayAttributeData Stamina;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, Stamina)

	UPROPERTY(ReplicatedUsing = OnRep_MaxStamina) FGameplayAttributeData MaxStamina;
	HYDRA_ATTRIBUTE_ACCESSORS(UHydraAttributeSet, MaxStamina)

protected:
	UFUNCTION() void OnRep_Stamina(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxStamina(const FGameplayAttributeData& Old);
};
