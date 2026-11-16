#include "Combat/HydraElementField.h"

#include "Character/HydraCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HydraDevSubsystem.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Hydra.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace HydraFieldConstants
{
	/** How often the server looks who stands in the field (technical; the stack timing is in values.json). */
	constexpr float CheckIntervalS = 0.1f;
	/** How far above or below the field a character's feet may be to count as standing in it. */
	constexpr float MaxFeetHeightCm = 60.f;
	const FLinearColor FireColor(1.0f, 0.30f, 0.02f);
}

AHydraElementField::AHydraElementField()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = HydraFieldConstants::CheckIntervalS;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	RootComponent = Disc;
	Disc->SetStaticMesh(Cylinder.Object);
	Disc->SetMaterial(0, ShapeMaterial.Object);
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetGenerateOverlapEvents(false);
	Disc->SetCastShadow(false);
}

void AHydraElementField::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHydraElementField, RadiusCm);
}

AHydraElementField* AHydraElementField::SpawnFireField(UWorld* World, const FVector& Location, AHydraCharacter* InSource)
{
	if (!World)
	{
		return nullptr;
	}
	const FHydraFireFieldValues& F = UHydraValuesSubsystem::Get(World).fire_field;
	// Onto the ground below the impact.
	FVector Ground = Location;
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Location + FVector(0.f, 0.f, 50.f), Location - FVector(0.f, 0.f, 500.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		Ground = Hit.ImpactPoint;
	}
	AHydraElementField* Field = World->SpawnActorDeferred<AHydraElementField>(AHydraElementField::StaticClass(), FTransform(Ground));
	if (Field)
	{
		Field->RadiusCm = F.radius_m * 100.f * (InSource ? InSource->GetStats().FieldRadiusFactor : 1.f);   // fire magic skill: a bigger field
		Field->Source = InSource;
		Field->FinishSpawning(FTransform(Ground));
		Field->SetLifeSpan(FMath::Max(0.1f, F.duration_ms / 1000.f));
		UE_LOG(LogHydra, Log, TEXT("HydraField t=%.2f element=fire x=%.0f y=%.0f radius=%.0f duration=%.2f"),
			UHydraDevSubsystem::ClockOf(World), Ground.X, Ground.Y, Field->RadiusCm, F.duration_ms / 1000.f);
	}
	return Field;
}

void AHydraElementField::BeginPlay()
{
	Super::BeginPlay();
	ApplyLook();
}

void AHydraElementField::OnRep_Radius()
{
	ApplyLook();
}

void AHydraElementField::ApplyLook()
{
	// Engine cylinder: 100 cm diameter, 100 cm high. A thin disc lying on the ground.
	Disc->SetWorldScale3D(FVector(RadiusCm / 50.f, RadiusCm / 50.f, 0.03f));
	if (GetNetMode() != NM_DedicatedServer)
	{
		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Disc->GetMaterial(0));
		if (!Material)
		{
			Material = Disc->CreateAndSetMaterialInstanceDynamic(0);
		}
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), HydraFieldConstants::FireColor);
		}
	}
}

void AHydraElementField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}
	const FVector Centre = GetActorLocation();
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		AHydraCharacter* C = *It;
		if (!C->IsAlive())
		{
			continue;
		}
		const float FeetZ = C->GetActorLocation().Z - C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (FVector::Dist2D(C->GetActorLocation(), Centre) <= RadiusCm && FMath::Abs(FeetZ - Centre.Z) <= HydraFieldConstants::MaxFeetHeightCm)
		{
			C->ServerTouchFireField(Source.Get());
		}
	}
}
