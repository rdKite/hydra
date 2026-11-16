#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Templates/SubclassOf.h"

class UAbilitySystemComponent;
class UGameplayEffect;
struct FHydraStaminaValues;

/**
 * Small helper layer over GAS (with HydraGameplayEffect.h, HydraTags.h and the
 * HYDRA_ATTRIBUTE_ACCESSORS macro): game code calls these functions and does not build
 * effect contexts and specs itself. Everything here changes gameplay state and therefore
 * runs on the server only; clients see the replicated attributes.
 */
namespace HydraGas
{
	/**
	 * Applies Effect to Target with the SetByCaller magnitudes Data.Amount and Data.Duration
	 * (seconds). Source = who caused it (default: the target itself).
	 */
	HYDRA_API FActiveGameplayEffectHandle Apply(UAbilitySystemComponent* Target, TSubclassOf<UGameplayEffect> Effect,
		float Amount, float DurationS = 0.f, UAbilitySystemComponent* Source = nullptr);

	// --- Stamina ---------------------------------------------------------------------------

	HYDRA_API float GetStamina(const UAbilitySystemComponent* Target);
	HYDRA_API float GetMaxStamina(const UAbilitySystemComponent* Target);

	/** An action may start only if the character has at least its cost (tech-test rule). */
	HYDRA_API bool CanAfford(const UAbilitySystemComponent* Target, float StaminaCost);

	/**
	 * Takes Cost from the stamina (never below 0) and pauses regeneration for
	 * stamina.regen_delay_after_action_ms. Does not check CanAfford.
	 */
	HYDRA_API void SpendStamina(UAbilitySystemComponent* Target, float Cost, const FHydraStaminaValues& Values);

	/**
	 * Sets maximum and regeneration (numbers of the form in values.json). Call at spawn (bFill: start
	 * with a full bar) and again after Hydra.ReloadValues. InOutRegen is the handle of the
	 * running regeneration effect, kept by the caller.
	 */
	HYDRA_API void ApplyStaminaValues(UAbilitySystemComponent* Target, float Max, float RegenPerS, FActiveGameplayEffectHandle& InOutRegen, bool bFill);

	// --- Health ----------------------------------------------------------------------------

	HYDRA_API float GetHealth(const UAbilitySystemComponent* Target);
	HYDRA_API float GetMaxHealth(const UAbilitySystemComponent* Target);

	/** Sets the maximum (from the form); bFill: full health, otherwise the current value is kept (clamped). */
	HYDRA_API void ApplyHealthValues(UAbilitySystemComponent* Target, float Max, bool bFill);

	/** Takes Amount from the target's health. Source = the attacker. */
	HYDRA_API void DealDamage(UAbilitySystemComponent* Target, float Amount, UAbilitySystemComponent* Source);
}
