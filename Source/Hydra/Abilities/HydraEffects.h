#pragma once

#include "CoreMinimal.h"
#include "Abilities/HydraGameplayEffect.h"
#include "HydraEffects.generated.h"

// The gameplay effects of the game, one small native class each (see HydraGameplayEffect.h).

/** Instant: Stamina += Data.Amount (negative for a cost). */
UCLASS()
class HYDRA_API UHydraGE_StaminaChange : public UHydraGameplayEffect
{
	GENERATED_BODY()
public:
	UHydraGE_StaminaChange();
};

/** Instant: IncomingDamage += Data.Amount (the attribute set takes it from Health). */
UCLASS()
class HYDRA_API UHydraGE_Damage : public UHydraGameplayEffect
{
	GENERATED_BODY()
public:
	UHydraGE_Damage();
};

/**
 * Stamina regeneration: lasts forever, adds Data.Amount every TickS seconds, pauses while
 * the character owns State.StaminaRegenBlocked. The caller passes
 * stamina.regen_per_s x TickS as the amount.
 */
UCLASS()
class HYDRA_API UHydraGE_StaminaRegen : public UHydraGameplayEffect
{
	GENERATED_BODY()
public:
	UHydraGE_StaminaRegen();

	/** Granularity of regeneration in seconds (technical, not a gameplay value: the rate per second is). */
	static constexpr float TickS = 0.1f;
};

/** Instant: Mana += Data.Amount (negative for a cost). */
UCLASS()
class HYDRA_API UHydraGE_ManaChange : public UHydraGameplayEffect
{
	GENERATED_BODY()
public:
	UHydraGE_ManaChange();
};

/** Mana regeneration: lasts forever, adds Data.Amount every TickS seconds (no delay: mana has none). */
UCLASS()
class HYDRA_API UHydraGE_ManaRegen : public UHydraGameplayEffect
{
	GENERATED_BODY()
public:
	UHydraGE_ManaRegen();
};

/**
 * Regeneration delay after an action: grants State.StaminaRegenBlocked for Data.Duration.
 * Applying it again restarts the delay.
 */
UCLASS()
class HYDRA_API UHydraGE_StaminaRegenDelay : public UHydraGameplayEffect
{
	GENERATED_BODY()
public:
	UHydraGE_StaminaRegenDelay();
};
