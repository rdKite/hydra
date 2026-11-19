#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HydraArena.generated.h"

class UStaticMeshComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class UMaterialInstanceDynamic;

/**
 * The Phase 1 arena: a flat square with a low border wall and a few obstacles for
 * positioning (pillars, low cover, a wall segment), plus its own lighting. Built entirely
 * from engine basic shapes in C++, so the layout is readable and editable as text. One
 * instance is placed in /Game/Maps/Arena by Scripts/create_arena_map.py.
 * The layout is level design, not a gameplay value, so it lives here and not in values.json.
 */
UCLASS()
class HYDRA_API AHydraArena : public AActor
{
	GENERATED_BODY()

public:
	AHydraArena();

	virtual void PostInitializeComponents() override;

	/** Player spawn points (world space): two spots facing each other near the centre. */
	TArray<FTransform> GetSpawnTransforms() const;

	/** Where wave enemies appear: spots near the border, clear of the obstacles, facing the centre. */
	TArray<FTransform> GetWaveSpawnTransforms() const;

	/**
	 * Look variants for the M5 decision (looks only, per window; dev command HydraLookVariant):
	 * 1 = plain (the lit scene as it is), 2 = toon (flat bands and ink outlines on characters,
	 * soft bands and a warmer palette in the world), 3 = ink (normal shading with ink outlines,
	 * a paler palette). The post-process material comes from Scripts/create_look_material.py.
	 */
	void ApplyLook(int32 InLook);
	int32 GetLook() const { return Look; }
	static constexpr int32 NumLooks = 3;

	/** Where the AI opponent starts. */
	FTransform GetDummySpawnTransform() const;

private:
	UStaticMeshComponent* AddBox(FName Name, const FVector& Location, const FVector& SizeCm, const FLinearColor& Color, float YawDeg = 0.f);
	UStaticMeshComponent* AddCylinder(FName Name, const FVector& Location, float RadiusCm, float HeightCm, const FLinearColor& Color);
	UStaticMeshComponent* AddShape(FName Name, UStaticMesh* Mesh, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TArray<TObjectPtr<UStaticMeshComponent>> Shapes;

	/** Colour per entry of Shapes (applied as a dynamic material instance at runtime). */
	UPROPERTY()
	TArray<FLinearColor> ShapeColors;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Lighting")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Lighting")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Lighting")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Lighting")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Look")
	TObjectPtr<UPostProcessComponent> LookPostProcess;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LookMaterial;

	int32 Look = 1;
};
