#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HydraGameplayEffect.generated.h"

class UTargetTagsGameplayEffectComponent;
class UTargetTagRequirementsGameplayEffectComponent;

/**
 * Base class of all Hydra gameplay effects. Part of the small helper layer over GAS:
 * effects are native C++ classes configured in their constructor (no Blueprint or data
 * assets - diffable, readable by agents), and every number is SetByCaller, so it comes
 * from values.json when the effect is applied and follows Hydra.ReloadValues.
 *
 * A new effect is a subclass whose constructor calls a few of the helpers below, e.g.
 *
 *     UHydraGE_StaminaChange::UHydraGE_StaminaChange()
 *     {
 *         MakeInstant();
 *         AddAmountTo(UHydraAttributeSet::GetStaminaAttribute());
 *     }
 *
 * and is applied with HydraGas::Apply (HydraGas.h). The helpers must be called from the
 * constructor only (they create default subobjects).
 */
UCLASS(Abstract)
class HYDRA_API UHydraGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

protected:
	/** Applies once and is gone. */
	void MakeInstant();

	/** Lasts for Data.Duration seconds (SetByCaller). */
	void MakeTimed();

	/** Lasts until removed. PeriodS > 0: the modifiers execute every PeriodS seconds. */
	void MakeInfinite(float PeriodS = 0.f);

	/** Adds Data.Amount (SetByCaller) to Attribute. */
	void AddAmountTo(const FGameplayAttribute& Attribute);

	/** The target owns Tag while the effect is active. */
	void GrantTag(const FGameplayTag& Tag);

	/** The effect stays on the target but does nothing while the target owns Tag. */
	void PauseWhileTargetHas(const FGameplayTag& Tag);

	/** At most Limit stacks per target; applying it again refreshes the duration. */
	void StackPerTarget(int32 Limit);

private:
	UPROPERTY()
	TObjectPtr<UTargetTagsGameplayEffectComponent> GrantedTagsComponent;

	UPROPERTY()
	TObjectPtr<UTargetTagRequirementsGameplayEffectComponent> RequirementsComponent;

	FGameplayTagContainer GrantedTags;
};
