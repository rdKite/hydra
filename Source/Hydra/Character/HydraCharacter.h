#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameFramework/Character.h"
#include "HydraCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UHydraAttributeSet;
class UHydraMovementComponent;
struct FHydraValues;

/**
 * The character of M1: a placeholder body from engine primitives (models come with the
 * Blender pipeline in M3) with a third-person camera, the ability system (stamina) and the
 * client-side lock-on state. Used for both players and for the passive training dummy.
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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	float GetStamina() const;
	float GetMaxStamina() const;

	UHydraMovementComponent* GetHydraMovement() const;

	/** Server, before BeginPlay: this character is the passive training dummy (no controller). */
	void SetTrainingDummy() { bTrainingDummy = true; }
	bool IsTrainingDummy() const { return bTrainingDummy; }

	/**
	 * Lock-on (owning client). Only switches the camera to follow the target and the
	 * character to face the camera direction (strafe flag, predicted and sent with the move);
	 * the character still turns no faster than its turn rate.
	 */
	void SetLockOnTarget(AHydraCharacter* NewTarget);
	AHydraCharacter* GetLockOnTarget() const { return LockOnTarget.Get(); }

	/** Best other character inside a cone (lock-on and aim assist). */
	AHydraCharacter* FindBestTarget(const FVector& Origin, const FVector& Direction, float RangeCm, float MaxAngleDeg) const;

	/**
	 * Soft aim assist: turns the character towards the best target inside the assist cone by
	 * aim_assist.strength. Meant for the start of an attack (M2); must run on the server
	 * (facing for hit checks) and on the owning client (same result locally). Does nothing
	 * while locked on. Returns the target it turned towards, if any.
	 */
	AHydraCharacter* ApplyAimAssist();

protected:
	void ApplyValues(const FHydraValues& Values);
	void UpdateTint();

	UFUNCTION()
	void OnRep_TrainingDummy();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Placeholder body: torso, head and a visor that shows the facing. */
	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> BodyTorso;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> BodyHead;

	UPROPERTY(VisibleAnywhere, Category = "Body")
	TObjectPtr<UStaticMeshComponent> BodyVisor;

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UHydraAttributeSet> Attributes;

	UPROPERTY(ReplicatedUsing = OnRep_TrainingDummy)
	bool bTrainingDummy = false;

	FActiveGameplayEffectHandle StaminaRegenHandle;
	bool bStaminaInitialized = false;

	TWeakObjectPtr<AHydraCharacter> LockOnTarget;
	FDelegateHandle ValuesReloadedHandle;
};
