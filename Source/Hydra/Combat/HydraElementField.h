#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HydraElementField.generated.h"

class AHydraCharacter;
class UStaticMeshComponent;

/**
 * An element field on the ground (Phase 1: fire only): a circle of fire_field.radius_m that
 * lasts fire_field.duration_ms. Whoever stands in it burns - the server tells every
 * character inside, several times a second, and the character's burning stack does the
 * rest (AHydraCharacter, burning.* in values.json). It hurts everyone, the caster too.
 */
UCLASS()
class HYDRA_API AHydraElementField : public AActor
{
	GENERATED_BODY()

public:
	AHydraElementField();

	/** Server: a fire field with its centre on the ground under Location. */
	static AHydraElementField* SpawnFireField(UWorld* World, const FVector& Location, AHydraCharacter* Source);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float GetRadiusCm() const { return RadiusCm; }

protected:
	UFUNCTION()
	void OnRep_Radius();

	void ApplyLook();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Disc;

	UPROPERTY(ReplicatedUsing = OnRep_Radius)
	float RadiusCm = 100.f;

	TWeakObjectPtr<AHydraCharacter> Source;
};
