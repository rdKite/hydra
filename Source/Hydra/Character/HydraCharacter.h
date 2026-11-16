#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Character/HydraCombatMath.h"
#include "Character/HydraPoseHistory.h"
#include "Core/HydraMetrics.h"
#include "GameFramework/Character.h"
#include "HydraCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UHydraAttributeSet;
class UHydraMovementComponent;
class UHydraProgressionComponent;
class UAnimSequence;
class UGameplayAbility;
struct FHydraValues;

/** What a character is doing right now, as far as others must be able to see it (the telegraph). */
UENUM()
enum class EHydraAction : uint8
{
	None,
	LightWindup,   // wind-up of an attack (every attack starts with it)
	HeavyCharge,   // button still held: charging a heavy attack
	LightStrike,   // hit window of a light attack
	HeavyStrike,   // hit window of a heavy attack
	Recovery,      // after the strike
	Block,
	Dodge,
	CastWindup,    // wind-up of the bolt
	CastCharge,    // button still held: charging the bolt
	CastRelease,   // the bolt is out
};

/** Replicated: which action since when. DurationS = planned length of this phase (0 = open). */
USTRUCT()
struct FHydraActionState
{
	GENERATED_BODY()

	UPROPERTY() EHydraAction Action = EHydraAction::None;
	UPROPERTY() float StartServerTime = 0.f;
	UPROPERTY() float DurationS = 0.f;
};

/**
 * A character in the arena: player or AI opponent, same class and same rules.
 *
 * - Form and loadout come from forms.json (FormId), all numbers from values.json; weight,
 *   strength, level, skills and imprint (UHydraProgressionComponent) and what follows from
 *   them are in GetStats().
 * - Abilities (attack, block, dodge) run through GAS; this class takes the input, shows the
 *   action (placeholder body from engine primitives with sword and shield - models come
 *   with the Blender pipeline in M3) and resolves hits on the server.
 * - Hits are checked on the server from positions, facing and hit areas, with lag
 *   compensation: the targets are rewound to what the attacker saw (HydraCharacterCombat.cpp).
 */
UCLASS()
class HYDRA_API AHydraCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	explicit AHydraCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** No jump while an attack binds, while blocking, during a dodge or when defeated. */
	virtual bool CanJumpInternal_Implementation() const override;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// --- State -----------------------------------------------------------------------------

	const FHydraCombatStats& GetStats() const { return Stats; }
	float GetHealth() const;
	float GetMaxHealth() const;
	float GetStamina() const;
	float GetMaxStamina() const;
	float GetMana() const;
	float GetMaxMana() const;
	bool IsAlive() const { return !bDead; }
	FName GetFormId() const { return FormId; }
	/** Stacks of the burning debuff (replicated). */
	int32 GetBurningStacks() const { return BurningStacks; }
	/** The element fire is awakened (fire imprint of level 1 or more): the bolt carries fire. */
	bool HasFireImprint() const;
	/** Level, skills, imprint and evolution. */
	UHydraProgressionComponent* GetProgression() const { return Progression; }
	/** The progress changed (server, and clients when it arrives): work out the numbers again. */
	void OnProgressChanged();
	bool HasStateTag(const struct FGameplayTag& Tag) const;
	int32 GetChainCount() const { return ChainCount; }

	UHydraMovementComponent* GetHydraMovement() const;

	/** Server, before BeginPlay: this character belongs to the AI opponent. */
	void SetAIControlled() { bAIControlled = true; }
	bool IsAIControlledCharacter() const { return bAIControlled; }

	/**
	 * Server, before BeginPlay: this character is an enemy of a wave (enemies.json ID). Serial
	 * tells enemies of one type apart; LevelBonus is added to the level of the type.
	 */
	void SetEnemyType(FName InEnemyId, int32 InSerial, int32 InLevelBonus);
	FName GetEnemyId() const { return EnemyId; }

	/** Server: false = it does not come back after a defeat (wave enemies); it is removed instead. */
	void SetRespawns(bool bInRespawns) { bRespawns = bInRespawns; }

	/** May this character's attacks hurt Target? (AI characters are one side and do not hurt each other.) */
	bool CanHurt(const AHydraCharacter* Target) const { return Target && Target != this && !(bAIControlled && Target->bAIControlled); }

	/** Server: counters for the metrics CSV. */
	FHydraMetricCounters Metrics;

	/** Name for logs: the player id, "ai" for the practice opponent, type and number for a wave enemy. The same in every process. */
	FString GetLogId() const;

	/** Server world time as this process knows it (synced on clients). */
	float GetServerTime() const;

	// --- Actions (what is shown) -----------------------------------------------------------

	/**
	 * Start showing an action. On the server this sets the replicated state; on the owning
	 * client (predicted ability) a local state, so its own actions show without waiting for
	 * the server.
	 */
	void SetAction(EHydraAction Action, float DurationS);

	/** Back to None if the current action is one of these. */
	void ClearAction(std::initializer_list<EHydraAction> Actions);

	/** The action to show in this process. */
	const FHydraActionState& GetShownAction() const;

	// --- Input (owning client, or the server for the AI) -----------------------------------

	void InputAttackPressed();
	void InputAttackReleased();
	void InputBlock(bool bPressed);
	/** The off-hand button: block with a shield, cast the bolt for a form with the projectile skill. */
	void InputOffHand(bool bPressed, float AimYaw);
	void InputDodge();
	void InputJump(bool bPressed);

	/** AI: direction for the next dodge (players dodge in their input direction). */
	void SetDodgeDirection(const FVector& Direction) { PendingDodgeDirection = Direction; }
	FVector ConsumeDodgeDirection();

	// --- Lock-on and aim assist -----------------------------------------------------------

	/**
	 * Lock-on (owning client). Only switches the camera to follow the target and the
	 * character to face the camera direction (strafe flag, predicted and sent with the move);
	 * the character still turns no faster than its turn rate.
	 */
	void SetLockOnTarget(AHydraCharacter* NewTarget);
	AHydraCharacter* GetLockOnTarget() const { return LockOnTarget.Get(); }

	/**
	 * Precise movement (owning client, a toggle): the character keeps facing where the camera
	 * looks and steps backwards and sideways instead of turning into its movement. The same
	 * strafe flag as lock-on, without a target; the turn rate still limits the turn.
	 */
	void SetPreciseMovement(bool bOn);
	bool IsPreciseMovement() const { return bPreciseMovement; }

	/** Strafe flag = locked on or precise movement (also drops a lock whose target is gone). */
	void RefreshStrafe();

	/** Best other living character inside a cone (lock-on and aim assist). */
	AHydraCharacter* FindBestTarget(const FVector& Origin, const FVector& Direction, float RangeCm, float MaxAngleDeg) const;

	/**
	 * Soft aim assist at the start of an attack (server and owning client): if a target is
	 * inside the assist cone, the character turns part of the way towards it
	 * (aim_assist.strength) - at its turn rate, like every other turn. Nothing while locked on.
	 */
	void BeginAttackFacing();
	void EndAttackFacing();

	/** Casting: turn towards the aim direction of the cast (at the turn rate). */
	void BeginCastFacing();

	// --- Combat (server) -------------------------------------------------------------------

	/** Server: a dodge step starts here (for the check whether it avoided a hit). */
	void ServerNoteDodge();

	/** An attack of this character just started: tell the AI opponents (they may react). */
	void OnAttackStarted();

	/**
	 * Checks the hit area of the running strike against every other living character not in
	 * AlreadyHit, with lag compensation, and resolves the hits (block, rebound, weak point,
	 * chain, interrupt, damage). Returns the number of new hits.
	 */
	int32 ServerTestAttackHits(bool bHeavy, float ChargeFactor, TArray<TWeakObjectPtr<AHydraCharacter>>& AlreadyHit);

	/** The strike ended without touching anyone. */
	void ServerReportMiss(bool bHeavy);

	/** Spawns the bolt along the facing; the charge is cut to what the mana pays for. */
	void ServerFireBolt(float WantedChargeAlpha);

	/** Damage of a type (slash, pierce, blunt, fire) after the target's resistance. Returns what was dealt. */
	static float ServerDealDamage(AHydraCharacter* Source, AHydraCharacter* Target, float RawDamage, FName DamageType);

	/** A fire field reports that this character stands in it (several times a second). */
	void ServerTouchFireField(AHydraCharacter* Source);

	/** Adds stacks of burning (a fire bolt hit). */
	void ServerAddBurning(int32 Stacks, AHydraCharacter* Source, const TCHAR* Reason);

	/** Change the form (arena start option and dev command): rebuilds numbers and looks, full bars. */
	void ServerSetForm(FName NewFormId);

	/** Evolution: the new form with full bars; level, skills and imprint were already reset by the progression. */
	void ServerEvolveTo(FName NewFormId);

	/** Start option and dev command: fire awakened (imprint level 1) or not. */
	void ServerSetFireImprint(bool bOn);

	/** Called by the attribute set after damage was taken from Health. */
	void OnDamaged(float Damage);

	/** Full health and stamina, alive, chain cleared (respawn; also a dev command). */
	void ServerResetState();

	/** Move the character and forget its pose history (respawn; also a dev command). */
	void ServerTeleport(const FVector& Location, float Yaw);

	/** Stamina to a fixed value (dev command for scripted runs). */
	void ServerSetStamina(float Value);

protected:
	UFUNCTION(Server, Reliable)
	void ServerAttackReleased();

	UFUNCTION(Server, Reliable)
	void ServerCastReleased();

	UFUNCTION(Server, Reliable)
	void ServerSetCastAim(float Yaw);

	/** Owner: face this way (after a teleport the client keeps its own facing otherwise). */
	UFUNCTION(Client, Reliable)
	void ClientSetFacing(float Yaw);

	/** Everyone: flash and short freeze of the pose (hit stop). Kind: 0 = was hit, 1 = blocked, 2 = landed a hit. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitFeedback(uint8 Kind, float HitStopS);

	UFUNCTION()
	void OnRep_Dead();

	UFUNCTION()
	void OnRep_FormId();

	UFUNCTION()
	void OnRep_AIControlled();

	void OnValuesReloaded(const FHydraValues& Values);
	void InitProgress();
	void ChangeForm(FName NewFormId);
	void RebuildStats();
	void GrantAbilities();
	void NotifyReleased(TSubclassOf<UGameplayAbility> AbilityClass);
	void ApplyFormLook();
	void UpdateMeshAnimation();
	void UpdateBurning();
	void SetBurningStacks(int32 NewStacks, const TCHAR* Reason);

	// Combat, server (HydraCharacterCombat.cpp).
	void ResolveHit(AHydraCharacter* Target, const FHydraPoseSample& TargetPose, bool bHeavy, float ChargeFactor, float RewindS);
	void ServerInterruptAttack();
	int32 GetChainHitsNow() const;
	void AdvanceChain();
	void ClearChain();
	float GetPingMs() const;
	void RecordPose();
	void Die();
	void Respawn();

	// Looks (HydraCharacter.cpp).
	void UpdateTint();
	void UpdatePose();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Placeholder body: everything visible hangs on BodyRoot. */
	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<USceneComponent> BodyRoot;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> BodyTorso;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> BodyHead;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> BodyVisor;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<USceneComponent> SwordPivot;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> SwordBlade;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> SwordGuard;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<USceneComponent> ShieldPivot;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> ShieldDisc;

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UHydraAttributeSet> Attributes;

	UPROPERTY(VisibleAnywhere, Category = "Progression")
	TObjectPtr<UHydraProgressionComponent> Progression;

	/** Form from forms.json. Phase 1 before M3: everyone is the humanoid. */
	UPROPERTY(ReplicatedUsing = OnRep_FormId)
	FName FormId = TEXT("humanoid");

	UPROPERTY(ReplicatedUsing = OnRep_AIControlled)
	bool bAIControlled = false;

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	bool bDead = false;

	UPROPERTY(Replicated)
	FHydraActionState ActionState;

	/** Hits in the running chain (for the HUD; the server keeps the timing). */
	UPROPERTY(Replicated)
	uint8 ChainCount = 0;

	UPROPERTY(Replicated)
	uint8 BurningStacks = 0;

	/** Wave enemies: type (enemies.json) and a number to tell them apart. */
	UPROPERTY(Replicated)
	FName EnemyId;

	UPROPERTY(Replicated)
	int32 EnemySerial = 0;

	/** Animations of a form with a model, by name (Idle, Move, Windup, ...). */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UAnimSequence>> FormAnimations;

	/** The form is shown by a skeletal mesh from the Blender pipeline (otherwise: placeholder primitives). */
	bool bUsesModel = false;
	FName PlayingAnimation;
	float PlayingStartTime = -1.f;
	FName LookBuiltForForm;

	/** Owning client only: action of a predicted ability. */
	FHydraActionState PredictedAction;

	FHydraCombatStats Stats;
	FActiveGameplayEffectHandle StaminaRegenHandle;
	FActiveGameplayEffectHandle ManaRegenHandle;
	bool bAttributesInitialized = false;
	bool bAbilitiesGranted = false;

	// Server.
	FHydraPoseHistory PoseHistory;
	double LastChainHitTime = -1000.0;
	FTransform SpawnTransform;
	FTimerHandle RespawnTimer;
	FVector PendingDodgeDirection = FVector::ZeroVector;
	double LastInFireFieldTime = -1000.0;
	double NextBurningStackChange = 0.0;
	double NextBurningTick = 0.0;
	TWeakObjectPtr<AHydraCharacter> BurningSource;
	bool bRespawns = true;
	int32 EnemyLevelBonus = 0;
	/** Who dealt the last damage (gets the kill). */
	TWeakObjectPtr<AHydraCharacter> LastDamager;
	/** Counts this character's attacks, so a dodge is credited once per attack. */
	int32 AttackSerial = 0;
	double DodgeStartTime = -1000.0;
	FVector DodgeStartLocation = FVector::ZeroVector;
	TWeakObjectPtr<AHydraCharacter> LastAvoidedAttacker;
	int32 LastAvoidedSerial = 0;

	// Owning client and server: where the cast is aimed.
	float CastAimYaw = 0.f;

	// Local.
	bool bBlockHeld = false;
	double HitFlashUntil = 0.0;
	uint8 HitFlashKind = 0;
	double HitStopUntil = 0.0;
	TWeakObjectPtr<AHydraCharacter> LockOnTarget;
	bool bPreciseMovement = false;
	FDelegateHandle ValuesReloadedHandle;
};
