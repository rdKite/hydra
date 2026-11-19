#pragma once

#include "CoreMinimal.h"
#include "Abilities/HydraGameplayAbility.h"
#include "HydraCombatAbilities.generated.h"

/**
 * Weapon attack of one hand: press = light attack, hold = heavy attack (charged).
 * Numbers per weapon from values.json (weapons.<id>), cost from weight / strength (values.json attack.*).
 *
 *   press ......... wind-up starts (the telegraph), light stamina cost is paid
 *   light wind-up over:
 *     button released -> light strike
 *     still held ...... becomes a heavy attack if the extra stamina is there: keeps charging;
 *                       strikes on release (not before heavy min_windup) or at max_charge
 *   strike ......... hit window: the server checks the hit area every tick (each target once)
 *   recovery ....... the attack no longer binds; a dodge may cancel it
 *
 * Attacks bind: from the press to the end of the strike the character cannot dodge or
 * block and turns slowly (State.AttackCommitted). Hits are resolved on the server in
 * AHydraCharacter::ServerTestAttackHits with lag compensation.
 * Light or heavy is decided on the server by when the release arrives there.
 */
UCLASS()
class HYDRA_API UHydraGA_Attack : public UHydraGameplayAbility
{
	GENERATED_BODY()

public:
	UHydraGA_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void OnInputReleased() override;

protected:
	virtual float GetStaminaCost(const FHydraCombatStats& Stats) const override;
	virtual FName GetUseSkill(const FHydraCombatStats& Stats) const override;
	virtual bool CanStart(const FHydraCombatStats& Stats) const override;

private:
	enum class EPhase : uint8 { None, Windup, Charging, Strike, Recovery };

	void OnLightWindupOver();
	void StartStrike(bool bHeavyStrike, float InChargeFactor);
	void TestHits();
	void EndStrike();
	void SetCommitted(bool bCommitted);
	float ElapsedMs() const;

	EPhase Phase = EPhase::None;
	bool bHeld = false;
	bool bHeavy = false;
	bool bCommittedTag = false;
	float ChargeFactor = 1.f;
	double StartWorldTime = 0.0;
	int32 HitCount = 0;
	TArray<TWeakObjectPtr<AHydraCharacter>> AlreadyHit;
};

/**
 * Cast the bolt (Phase 1: the slime's projectile, a body skill): press = cast after the
 * wind-up, hold = charge - stronger and costlier - until release or bolt.max_charge_ms.
 * Costs mana, not stamina. The caster turns towards the aim direction at its turn rate
 * during the wind-up; the bolt leaves along its facing when it is released. Binds like an
 * attack until the bolt is out. The server spawns the bolt (AHydraProjectile).
 */
UCLASS()
class HYDRA_API UHydraGA_Cast : public UHydraGameplayAbility
{
	GENERATED_BODY()

public:
	UHydraGA_Cast();

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void OnInputReleased() override;

private:
	enum class EPhase : uint8 { None, Windup, Charging, Released };

	void OnWindupOver();
	void Release(float ChargeAlpha);
	void SetCommitted(bool bCommitted);
	float HeldMs() const;

	EPhase Phase = EPhase::None;
	bool bHeld = false;
	bool bCommittedTag = false;
	double StartWorldTime = 0.0;
};

/**
 * Directional block: active while the shield hand's button is held. Protects only the block
 * arc of the shield (values.json shields.<id>); costs stamina every second while raised, no
 * regeneration, slower movement and turning. What a hit does to a block is resolved in
 * AHydraCharacter (block cost and rebound as shares of the blocked damage, block break at
 * empty stamina).
 */
UCLASS()
class HYDRA_API UHydraGA_Block : public UHydraGameplayAbility
{
	GENERATED_BODY()

public:
	UHydraGA_Block();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	virtual bool CanStart(const FHydraCombatStats& Stats) const override;
	virtual FName GetUseSkill(const FHydraCombatStats& Stats) const override;
};

/**
 * Dodge: pure movement, no invulnerability. A quick step of dodge.distance_m over
 * dodge.duration_ms in the input direction (backwards without input), then a short
 * recovery. A capability of the form: cost by total weight / body weight, distance and speed
 * by dexterity. Whether an attack still hits is decided by the hit
 * area alone. Cancels a block and an attack in its recovery.
 */
UCLASS()
class HYDRA_API UHydraGA_Dodge : public UHydraGameplayAbility
{
	GENERATED_BODY()

public:
	UHydraGA_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	virtual float GetStaminaCost(const FHydraCombatStats& Stats) const override;
};
