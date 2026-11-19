#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "HydraGameplayAbility.generated.h"

class AHydraCharacter;
struct FHydraCombatStats;
struct FHydraValues;

/**
 * Base of the Hydra abilities (part of the GAS helper layer). An ability here is
 *  - locally predicted: the owning client starts it at once, the server runs it too and
 *    alone decides hits, damage and stamina;
 *  - one instance per character;
 *  - driven by a few timers that are cleared when it ends.
 * A subclass says what it costs (GetStaminaCost) and what it needs (CanStart) and builds
 * its timeline in ActivateAbility.
 */
UCLASS(Abstract)
class HYDRA_API UHydraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UHydraGameplayAbility();

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** The button that started this ability was released (owning client and server). */
	virtual void OnInputReleased() {}

protected:
	/** Stamina needed to start (and taken on the server when it starts). 0 = free. */
	virtual float GetStaminaCost(const FHydraCombatStats& Stats) const { return 0.f; }

	/** The skill this ability's stamina counts for when hXP are distributed by use; None = no skill (dodge). */
	virtual FName GetUseSkill(const FHydraCombatStats& Stats) const { return NAME_None; }

	/** Anything else the character needs for this ability (a weapon, a shield). */
	virtual bool CanStart(const FHydraCombatStats& Stats) const { return true; }

	AHydraCharacter* GetHydraCharacter() const;
	const FHydraValues& GetValues() const;
	bool IsServer() const;

	/** Server: take stamina and count it as use of this ability's skill. No-op on clients. */
	void SpendStamina(float Cost) const;

	/** One-shot timer, cleared automatically when the ability ends. DelayS <= 0 runs at once. */
	void StartTimer(float DelayS, TFunction<void()> Callback);
	void StartLoopingTimer(float IntervalS, TFunction<void()> Callback);
	void ClearTimers();

	/** End this ability normally (from a timer). */
	void FinishAbility();

private:
	TArray<FTimerHandle> Timers;
};
