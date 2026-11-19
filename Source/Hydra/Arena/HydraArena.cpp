#include "Arena/HydraArena.h"

#include "Components/PostProcessComponent.h"
#include "Hydra.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace HydraArenaLayout
{
	// All in cm. Square arena, 40 m wide, top of the floor at Z = 0.
	constexpr float HalfSize = 2000.f;
	constexpr float FloorThickness = 50.f;
	constexpr float WallHeight = 150.f;
	constexpr float WallThickness = 60.f;

	const FLinearColor FloorColor(0.30f, 0.32f, 0.30f);
	const FLinearColor WallColor(0.22f, 0.22f, 0.25f);
	const FLinearColor PillarColor(0.55f, 0.50f, 0.42f);
	const FLinearColor CoverColor(0.45f, 0.30f, 0.20f);

	// Look variants: colours of floor, wall, pillar and cover, and the sun (looks, not gameplay).
	struct FLookPreset
	{
		FLinearColor Floor, Wall, Pillar, Cover;
		float SunIntensity;
		float CharBands, WorldBands, WorldInk, InkAmount, OutlinePx;
	};
	const FLookPreset Looks[] = {
		{ FloorColor, WallColor, PillarColor, CoverColor, 8.f, 0.f, 0.f, 0.f, 0.f, 1.5f },                                              // 1 plain
		{ FLinearColor(0.52f, 0.42f, 0.26f), FLinearColor(0.20f, 0.26f, 0.40f), FLinearColor(0.74f, 0.56f, 0.30f), FLinearColor(0.62f, 0.26f, 0.14f),
		  9.f, 3.f, 5.f, 0.35f, 1.f, 1.6f },                                                                                         // 2 toon
		{ FLinearColor(0.58f, 0.56f, 0.50f), FLinearColor(0.36f, 0.36f, 0.38f), FLinearColor(0.66f, 0.62f, 0.54f), FLinearColor(0.50f, 0.40f, 0.32f),
		  7.f, 0.f, 0.f, 1.f, 1.f, 1.3f },                                                                                           // 3 ink
	};
}

AHydraArena::AHydraArena()
{
	PrimaryActorTick.bCanEverTick = false;
	// Placed in the map: exists on server and clients without being replicated.
	bReplicates = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	using namespace HydraArenaLayout;

	AddBox(TEXT("Floor"), FVector(0.f, 0.f, -FloorThickness * 0.5f), FVector(HalfSize * 2.f, HalfSize * 2.f, FloorThickness), FloorColor);

	// Border walls.
	const float WallZ = WallHeight * 0.5f;
	const float WallLen = HalfSize * 2.f + WallThickness * 2.f;
	AddBox(TEXT("WallNorth"), FVector(HalfSize + WallThickness * 0.5f, 0.f, WallZ), FVector(WallThickness, WallLen, WallHeight), WallColor);
	AddBox(TEXT("WallSouth"), FVector(-HalfSize - WallThickness * 0.5f, 0.f, WallZ), FVector(WallThickness, WallLen, WallHeight), WallColor);
	AddBox(TEXT("WallEast"), FVector(0.f, HalfSize + WallThickness * 0.5f, WallZ), FVector(WallLen, WallThickness, WallHeight), WallColor);
	AddBox(TEXT("WallWest"), FVector(0.f, -HalfSize - WallThickness * 0.5f, WallZ), FVector(WallLen, WallThickness, WallHeight), WallColor);

	// Obstacles for positioning: pillars to circle around, low cover, a block, a wall segment.
	AddCylinder(TEXT("PillarA"), FVector(700.f, 700.f, 0.f), 80.f, 400.f, PillarColor);
	AddCylinder(TEXT("PillarB"), FVector(-700.f, -700.f, 0.f), 80.f, 400.f, PillarColor);
	AddCylinder(TEXT("PillarC"), FVector(-1000.f, 1000.f, 0.f), 120.f, 400.f, PillarColor);
	AddBox(TEXT("CoverA"), FVector(1100.f, -500.f, 50.f), FVector(400.f, 100.f, 100.f), CoverColor, 20.f);
	AddBox(TEXT("CoverB"), FVector(-1200.f, 200.f, 50.f), FVector(100.f, 500.f, 100.f), CoverColor);
	AddBox(TEXT("Block"), FVector(300.f, -1200.f, 150.f), FVector(300.f, 300.f, 300.f), PillarColor, 45.f);
	AddBox(TEXT("WallSegment"), FVector(1300.f, 1200.f, 125.f), FVector(800.f, 80.f, 250.f), WallColor, -30.f);

	// Lighting: all movable, Lumen from the project settings, no baked data.
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(FRotator(-50.f, -35.f, 0.f));
	Sun->SetIntensity(8.f);
	Sun->SetAtmosphereSunLight(true);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(RootComponent);
	HeightFog->SetFogDensity(0.005f);

	// The look variants are one post-process material over the whole view (see ApplyLook).
	LookPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("LookPostProcess"));
	LookPostProcess->SetupAttachment(RootComponent);
	LookPostProcess->bUnbound = true;
}

void AHydraArena::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Tint the shapes (the engine's basic shape material has a "Color" parameter).
	for (int32 i = 0; i < Shapes.Num() && i < ShapeColors.Num(); ++i)
	{
		if (UMaterialInstanceDynamic* Material = Shapes[i] ? Shapes[i]->CreateAndSetMaterialInstanceDynamic(0) : nullptr)
		{
			Material->SetVectorParameterValue(TEXT("Color"), ShapeColors[i]);
		}
	}
}

UStaticMeshComponent* AHydraArena::AddShape(FName Name, UStaticMesh* Mesh, const FLinearColor& Color)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Comp->SetupAttachment(RootComponent);
	Comp->SetStaticMesh(Mesh);
	Comp->SetMaterial(0, ShapeMaterial.Object);
	Comp->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Comp->SetMobility(EComponentMobility::Static);
	Shapes.Add(Comp);
	ShapeColors.Add(Color);
	return Comp;
}

UStaticMeshComponent* AHydraArena::AddBox(FName Name, const FVector& Location, const FVector& SizeCm, const FLinearColor& Color, float YawDeg)
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Engine cube: 100 cm, pivot at its centre.
	UStaticMeshComponent* Comp = AddShape(Name, CubeMesh.Object, Color);
	Comp->SetRelativeLocation(Location);
	Comp->SetRelativeRotation(FRotator(0.f, YawDeg, 0.f));
	Comp->SetRelativeScale3D(SizeCm / 100.f);
	return Comp;
}

UStaticMeshComponent* AHydraArena::AddCylinder(FName Name, const FVector& Location, float RadiusCm, float HeightCm, const FLinearColor& Color)
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Engine cylinder: 100 cm diameter, 100 cm high, pivot at its centre. Location = foot.
	UStaticMeshComponent* Comp = AddShape(Name, CylinderMesh.Object, Color);
	Comp->SetRelativeLocation(Location + FVector(0.f, 0.f, HeightCm * 0.5f));
	Comp->SetRelativeScale3D(FVector(RadiusCm / 50.f, RadiusCm / 50.f, HeightCm / 100.f));
	return Comp;
}

void AHydraArena::ApplyLook(int32 InLook)
{
	using namespace HydraArenaLayout;
	Look = FMath::Clamp(InLook, 1, NumLooks);
	const FLookPreset& Preset = Looks[Look - 1];
	// The palette of the arena: every shape knows which of the four colours it was built with.
	for (int32 i = 0; i < Shapes.Num() && i < ShapeColors.Num(); ++i)
	{
		const FLinearColor& Built = ShapeColors[i];
		const FLinearColor& Wanted = Built.Equals(FloorColor) ? Preset.Floor : (Built.Equals(WallColor) ? Preset.Wall : (Built.Equals(PillarColor) ? Preset.Pillar : Preset.Cover));
		if (UMaterialInstanceDynamic* Material = Shapes[i] ? Cast<UMaterialInstanceDynamic>(Shapes[i]->GetMaterial(0)) : nullptr)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Wanted);
		}
	}
	Sun->SetIntensity(Preset.SunIntensity);
	if (!LookMaterial)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Looks/PP_Look.PP_Look"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			LookMaterial = UMaterialInstanceDynamic::Create(Base, this);
			LookPostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, LookMaterial));
		}
	}
	if (LookMaterial)
	{
		LookMaterial->SetScalarParameterValue(TEXT("CharBands"), Preset.CharBands);
		LookMaterial->SetScalarParameterValue(TEXT("WorldBands"), Preset.WorldBands);
		LookMaterial->SetScalarParameterValue(TEXT("WorldInk"), Preset.WorldInk);
		LookMaterial->SetScalarParameterValue(TEXT("InkAmount"), Preset.InkAmount);
		LookMaterial->SetScalarParameterValue(TEXT("OutlinePx"), Preset.OutlinePx);
	}
	UE_LOG(LogHydra, Log, TEXT("HydraLook look=%d material=%d"), Look, LookMaterial ? 1 : 0);
}

TArray<FTransform> AHydraArena::GetSpawnTransforms() const
{
	const FTransform& T = GetActorTransform();
	TArray<FTransform> Result;
	Result.Add(FTransform(FRotator(0.f, 0.f, 0.f), T.TransformPosition(FVector(-400.f, 0.f, 100.f))));
	Result.Add(FTransform(FRotator(0.f, 180.f, 0.f), T.TransformPosition(FVector(400.f, 0.f, 100.f))));
	return Result;
}

TArray<FTransform> AHydraArena::GetWaveSpawnTransforms() const
{
	const FTransform& T = GetActorTransform();
	TArray<FTransform> Result;
	for (const FVector2D& Spot : { FVector2D(0.f, 1500.f), FVector2D(1500.f, 0.f), FVector2D(0.f, -1500.f), FVector2D(-1500.f, 0.f),
		FVector2D(1000.f, -1300.f), FVector2D(-500.f, 1600.f), FVector2D(-1300.f, -1300.f), FVector2D(1600.f, 600.f) })
	{
		const float YawToCentre = FMath::RadiansToDegrees(FMath::Atan2(-Spot.Y, -Spot.X));
		Result.Add(FTransform(FRotator(0.f, YawToCentre, 0.f), T.TransformPosition(FVector(Spot.X, Spot.Y, 100.f))));
	}
	return Result;
}

FTransform AHydraArena::GetDummySpawnTransform() const
{
	return FTransform(FRotator(0.f, -90.f, 0.f), GetActorTransform().TransformPosition(FVector(0.f, 600.f, 100.f)));
}
