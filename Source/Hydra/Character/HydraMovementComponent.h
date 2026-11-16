#pragma once

#include "CoreMinimal.h"
#include "Data/HydraValues.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HydraMovementComponent.generated.h"

class UAbilitySystemComponent;

/**
 * CharacterMovement with the Hydra rules for speed and turning.
 *
 * Movement states travel in the predicted move (compressed flags), so client and server
 * agree on speed and facing without corrections (tech-test finding):
 *   Walk   (FLAG_Custom_0): walk speed instead of run speed
 *   Strafe (FLAG_Custom_1): lock-on; strafe speed, and the character turns towards the
 *                           camera direction instead of the movement direction
 *
 * Turn rate: every turn of the character goes through the turn rate from weight / strength
 * (turn_rate.* in values.json), slower while blocking and while an attack binds, inside
 * floor and cap. That also holds for lock-on (the camera turns, the character follows) and
 * for aim assist (a facing goal the character turns towards during an attack).
 * Blocking and attacking also slow movement (block.move_speed_ratio, attack.move_speed_ratio).
 * Both states are read from the character's gameplay tags, which exist on the server and on
 * the owning client.
 */
UCLASS()
class HYDRA_API UHydraMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UHydraMovementComponent();

	/** Values and the character's weight / strength (body plus load). */
	void ApplyValues(const FHydraValues& Values, float InWeightRatio);

	/** Local intent; goes into the saved move on the owning client, read from the flags on the server. */
	uint8 bWantsWalk : 1;
	uint8 bWantsStrafe : 1;

	/** Aim assist: turn towards this yaw (at the turn rate) until cleared. */
	void SetFacingGoal(float Yaw) { bHasFacingGoal = true; FacingGoalYaw = Yaw; }
	void ClearFacingGoal() { bHasFacingGoal = false; }
	bool HasFacingGoal() const { return bHasFacingGoal; }

	/** Turn rate right now in deg/s (weight, block and attack state included). */
	float GetCurrentTurnRateDegS() const;

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void PhysicsRotation(float DeltaTime) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

private:
	bool OwnerHasTag(const struct FGameplayTag& Tag) const;

	float WalkSpeedCm = 0.f;
	float RunSpeedCm = 0.f;
	float StrafeSpeedCm = 0.f;
	float BlockSpeedRatio = 1.f;
	float AttackSpeedRatio = 1.f;
	float WeightRatio = 1.f;
	FHydraTurnRateValues TurnRate;

	bool bHasFacingGoal = false;
	float FacingGoalYaw = 0.f;
};

class FSavedMove_Hydra : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	uint8 bSavedWantsWalk : 1;
	uint8 bSavedWantsStrafe : 1;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;
};

class FNetworkPredictionData_Client_Hydra : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	explicit FNetworkPredictionData_Client_Hydra(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement) {}

	virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FSavedMove_Hydra()); }
};
