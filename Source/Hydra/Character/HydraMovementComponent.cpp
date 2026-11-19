#include "Character/HydraMovementComponent.h"

#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/HydraCombatMath.h"
#include "GameFramework/Character.h"

UHydraMovementComponent::UHydraMovementComponent()
{
	bWantsWalk = false;
	bWantsStrafe = false;
	bOrientRotationToMovement = true;
}

void UHydraMovementComponent::ApplyValues(const FHydraValues& Values, float InWeightRatio)
{
	WalkSpeedCm = Values.movement.walk_speed_m_s * 100.f;
	RunSpeedCm = Values.movement.run_speed_m_s * 100.f;
	StrafeSpeedCm = Values.movement.strafe_speed_m_s * 100.f;
	BlockSpeedRatio = Values.block.move_speed_ratio;
	AttackSpeedRatio = Values.attack.move_speed_ratio;
	TurnRate = Values.turn_rate;
	WeightRatio = InWeightRatio;
	MaxWalkSpeed = RunSpeedCm;
	// Take-off speed for the wanted height: v = sqrt(2 g h).
	JumpZVelocity = FMath::Sqrt(2.f * FMath::Abs(GetGravityZ()) * FMath::Max(0.f, Values.movement.jump_height_m * 100.f));
	RotationRate = FRotator(0.f, GetCurrentTurnRateDegS(), 0.f);
}

bool UHydraMovementComponent::OwnerHasTag(const FGameplayTag& Tag) const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	const UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	return ASC && ASC->HasMatchingGameplayTag(Tag);
}

float UHydraMovementComponent::GetCurrentTurnRateDegS() const
{
	return HydraCombatMath::TurnRateDegS(TurnRate, WeightRatio, OwnerHasTag(HydraTags::State_Blocking), OwnerHasTag(HydraTags::State_AttackCommitted));
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
	if (OwnerHasTag(HydraTags::State_Blocking))
	{
		Speed *= BlockSpeedRatio;
	}
	if (OwnerHasTag(HydraTags::State_AttackCommitted))
	{
		Speed *= AttackSpeedRatio;
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
	// Same rule on client and server. The engine turns the character by at most
	// RotationRate x DeltaTime, whatever the goal is.
	const float Rate = GetCurrentTurnRateDegS();
	RotationRate = FRotator(0.f, Rate, 0.f);

	if (bHasFacingGoal && UpdatedComponent && HasValidData() && (CharacterOwner->Controller || bRunPhysicsWithNoController))
	{
		// Aim assist: a fixed facing goal instead of movement or camera direction.
		FRotator Rotation = UpdatedComponent->GetComponentRotation();
		Rotation.Yaw = HydraCombatMath::StepYawTowards(Rotation.Yaw, FacingGoalYaw, Rate * DeltaTime);
		Rotation.Pitch = 0.f;
		Rotation.Roll = 0.f;
		MoveUpdatedComponent(FVector::ZeroVector, Rotation, /*bSweep*/ false);
		return;
	}

	// Lock-on, precise movement, a raised block and a bolt that is wound up or charged face
	// where the camera looks (block and bolt have to be aimed); otherwise the character turns
	// into its movement.
	const bool bFaceCamera = bWantsStrafe || OwnerHasTag(HydraTags::State_Blocking) || OwnerHasTag(HydraTags::State_Casting);
	bOrientRotationToMovement = !bFaceCamera;
	bUseControllerDesiredRotation = bFaceCamera;
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
