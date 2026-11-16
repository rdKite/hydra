#include "Character/HydraCharacter.h"

#include "Abilities/HydraAttributeSet.h"
#include "Abilities/HydraGas.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/HydraCombatMath.h"
#include "Character/HydraMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace HydraCharacterLook
{
	// Placeholder look and camera feel - not gameplay values.
	constexpr float CapsuleRadius = 35.f;
	constexpr float CapsuleHalfHeight = 90.f;
	constexpr float CameraArmLength = 380.f;
	const FVector CameraShoulderOffset(0.f, 40.f, 60.f);

	const FLinearColor OwnColor(0.10f, 0.30f, 0.85f);
	const FLinearColor OtherColor(0.80f, 0.15f, 0.10f);
	const FLinearColor DummyColor(0.65f, 0.60f, 0.35f);
	const FLinearColor VisorColor(0.02f, 0.02f, 0.02f);
}

AHydraCharacter::AHydraCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UHydraMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	using namespace HydraCharacterLook;

	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	// Facing is driven by the movement component (turn rate), never directly by the controller.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MinAnalogWalkSpeed = 20.f;
	Move->BrakingDecelerationWalking = 2000.f;
	// The training dummy has no controller but must still stand on the floor.
	Move->bRunPhysicsWithNoController = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	auto MakePart = [this](FName Name, UStaticMesh* PartMesh, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(RootComponent);
		Part->SetStaticMesh(PartMesh);
		Part->SetMaterial(0, ShapeMaterial.Object);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		return Part;
	};
	// Engine shapes are 100 cm; the capsule centre is the origin (feet at -90).
	BodyTorso = MakePart(TEXT("BodyTorso"), Cylinder.Object, FVector(0.f, 0.f, -25.f), FVector(0.6f, 0.6f, 1.3f));
	BodyHead = MakePart(TEXT("BodyHead"), Sphere.Object, FVector(0.f, 0.f, 62.f), FVector(0.38f));
	BodyVisor = MakePart(TEXT("BodyVisor"), Cube.Object, FVector(17.f, 0.f, 64.f), FVector(0.1f, 0.26f, 0.08f));

	// Third-person camera over the right shoulder, following the control rotation.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = CameraArmLength;
	CameraBoom->SocketOffset = CameraShoulderOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// GAS: component on the pawn (players and dummy alike). Mixed: effects replicate to the
	// owner only, attributes and tags to everyone.
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	Attributes = CreateDefaultSubobject<UHydraAttributeSet>(TEXT("Attributes"));
}

UAbilitySystemComponent* AHydraCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AHydraCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	AbilitySystem->InitAbilityActorInfo(this, this);
}

void AHydraCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystem->InitAbilityActorInfo(this, this); // refresh the controller in the actor info
	UpdateTint();
}

void AHydraCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	AbilitySystem->InitAbilityActorInfo(this, this);
	UpdateTint();
}

void AHydraCharacter::OnRep_TrainingDummy()
{
	UpdateTint();
}

void AHydraCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHydraCharacter, bTrainingDummy);
}

float AHydraCharacter::GetStamina() const { return Attributes->GetStamina(); }
float AHydraCharacter::GetMaxStamina() const { return Attributes->GetMaxStamina(); }

UHydraMovementComponent* AHydraCharacter::GetHydraMovement() const
{
	return CastChecked<UHydraMovementComponent>(GetCharacterMovement());
}

void AHydraCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateTint();
	if (UHydraValuesSubsystem* ValuesSubsystem = UHydraValuesSubsystem::GetSubsystem(this))
	{
		ApplyValues(ValuesSubsystem->GetValues());
		ValuesReloadedHandle = ValuesSubsystem->OnReloaded.AddUObject(this, &AHydraCharacter::ApplyValues);
	}
}

void AHydraCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHydraValuesSubsystem* ValuesSubsystem = UHydraValuesSubsystem::GetSubsystem(this))
	{
		ValuesSubsystem->OnReloaded.Remove(ValuesReloadedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AHydraCharacter::ApplyValues(const FHydraValues& Values)
{
	// Movement values are needed on both sides (prediction); attributes are server-only.
	GetHydraMovement()->ApplyValues(Values);
	if (HasAuthority())
	{
		HydraGas::ApplyStaminaValues(AbilitySystem, Values.stamina, StaminaRegenHandle, !bStaminaInitialized);
		bStaminaInitialized = true;
	}
}

void AHydraCharacter::UpdateTint()
{
	using namespace HydraCharacterLook;
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const FLinearColor BodyColor = bTrainingDummy ? DummyColor : (IsLocallyControlled() ? OwnColor : OtherColor);
	auto Tint = [](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0));
		if (!Material)
		{
			Material = Part->CreateAndSetMaterialInstanceDynamic(0);
		}
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
		}
	};
	Tint(BodyTorso, BodyColor);
	Tint(BodyHead, BodyColor);
	Tint(BodyVisor, VisorColor);
}

void AHydraCharacter::SetLockOnTarget(AHydraCharacter* NewTarget)
{
	LockOnTarget = NewTarget;
	GetHydraMovement()->bWantsStrafe = NewTarget != nullptr;
}

AHydraCharacter* AHydraCharacter::FindBestTarget(const FVector& Origin, const FVector& Direction, float RangeCm, float MaxAngleDeg) const
{
	AHydraCharacter* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		AHydraCharacter* Other = *It;
		float Score = 0.f;
		if (Other != this && HydraCombatMath::ScoreTargetInCone(Origin, Direction, Other->GetActorLocation(), RangeCm, MaxAngleDeg, Score) && Score < BestScore)
		{
			BestScore = Score;
			Best = Other;
		}
	}
	return Best;
}

AHydraCharacter* AHydraCharacter::ApplyAimAssist()
{
	if (LockOnTarget.IsValid() || GetHydraMovement()->bWantsStrafe)
	{
		return nullptr; // locked on: the character already turns towards the target
	}
	const FHydraAimAssistValues& A = UHydraValuesSubsystem::Get(this).aim_assist;
	AHydraCharacter* Target = FindBestTarget(GetActorLocation(), GetActorForwardVector(), A.range_m * 100.f, A.angle_deg);
	if (Target)
	{
		const float CurrentYaw = GetActorRotation().Yaw;
		const float TargetYaw = (Target->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
		SetActorRotation(FRotator(0.f, HydraCombatMath::AimAssistYaw(CurrentYaw, TargetYaw, A.strength), 0.f));
	}
	return Target;
}
