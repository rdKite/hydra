#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HydraMovementComponent.generated.h"

struct FHydraValues;

/**
 * CharacterMovement with the Hydra movement states carried in the predicted move
 * (compressed flags), so client and server agree on speed and facing without corrections
 * (tech-test finding: switching these by RPC causes corrections):
 *   Walk   (FLAG_Custom_0): walk speed instead of run speed
 *   Strafe (FLAG_Custom_1): lock-on; strafe speed, and the character turns towards the
 *                           camera direction instead of the movement direction
 * Turning always goes through RotationRate (the turn rate), also while locked on: lock-on
 * turns the camera, the character follows no faster than its turn rate.
 * Speeds and turn rate come from values.json (movement.*, turn_rate.*).
 */
UCLASS()
class HYDRA_API UHydraMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UHydraMovementComponent();

	void ApplyValues(const FHydraValues& Values);

	/** Local intent; goes into the saved move on the owning client, read from the flags on the server. */
	uint8 bWantsWalk : 1;
	uint8 bWantsStrafe : 1;

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void PhysicsRotation(float DeltaTime) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

private:
	float WalkSpeedCm = 0.f;
	float RunSpeedCm = 0.f;
	float StrafeSpeedCm = 0.f;
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
