#include "Character/HydraMovementComponent.h"

#include "Character/HydraCombatMath.h"
#include "Data/HydraValues.h"
#include "GameFramework/Character.h"

UHydraMovementComponent::UHydraMovementComponent()
{
	bWantsWalk = false;
	bWantsStrafe = false;
	bOrientRotationToMovement = true;
}

void UHydraMovementComponent::ApplyValues(const FHydraValues& Values)
{
	WalkSpeedCm = Values.movement.walk_speed_m_s * 100.f;
	RunSpeedCm = Values.movement.run_speed_m_s * 100.f;
	StrafeSpeedCm = Values.movement.strafe_speed_m_s * 100.f;
	MaxWalkSpeed = RunSpeedCm;
	RotationRate = FRotator(0.f, HydraCombatMath::TurnRateDegS(Values.turn_rate), 0.f);
}

float UHydraMovementComponent::GetMaxSpeed() const
{
	if (MovementMode != MOVE_Walking && MovementMode != MOVE_NavWalking)
	{
		return Super::GetMaxSpeed();
	}
	float Speed = bWantsStrafe ? StrafeSpeedCm : RunSpeedCm;
	if (bWantsWalk)
	{
		Speed = FMath::Min(Speed, WalkSpeedCm);
	}
	return Speed;
}

void UHydraMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsWalk = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsStrafe = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

void UHydraMovementComponent::PhysicsRotation(float DeltaTime)
{
	// Same rule on client and server, driven by the predicted strafe flag. Either way the
	// engine turns the character by at most RotationRate x DeltaTime.
	bOrientRotationToMovement = !bWantsStrafe;
	bUseControllerDesiredRotation = bWantsStrafe;
	Super::PhysicsRotation(DeltaTime);
}

FNetworkPredictionData_Client* UHydraMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UHydraMovementComponent* MutableThis = const_cast<UHydraMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Hydra(*this);
	}
	return ClientPredictionData;
}

void FSavedMove_Hydra::Clear()
{
	Super::Clear();
	bSavedWantsWalk = false;
	bSavedWantsStrafe = false;
}

uint8 FSavedMove_Hydra::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsWalk) { Result |= FLAG_Custom_0; }
	if (bSavedWantsStrafe) { Result |= FLAG_Custom_1; }
	return Result;
}

bool FSavedMove_Hydra::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Hydra* Other = static_cast<const FSavedMove_Hydra*>(NewMove.Get());
	if (bSavedWantsWalk != Other->bSavedWantsWalk || bSavedWantsStrafe != Other->bSavedWantsStrafe)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Hydra::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	if (const UHydraMovementComponent* Move = Cast<UHydraMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsWalk = Move->bWantsWalk;
		bSavedWantsStrafe = Move->bWantsStrafe;
	}
}

void FSavedMove_Hydra::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	if (UHydraMovementComponent* Move = Cast<UHydraMovementComponent>(C->GetCharacterMovement()))
	{
		Move->bWantsWalk = bSavedWantsWalk;
		Move->bWantsStrafe = bSavedWantsStrafe;
	}
}
