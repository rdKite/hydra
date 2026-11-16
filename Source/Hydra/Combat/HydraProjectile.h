#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HydraProjectile.generated.h"

class AHydraCharacter;
class UStaticMeshComponent;

/**
 * A bolt in flight (Phase 1: the slime's projectile). It flies straight - no gravity, no
 * homing - at bolt.speed_m_s until it hits a character, the world, or its range runs out.
 *
 * The server alone moves it for real and decides what it hits; clients get its start and
 * velocity once and fly their copy along the same line.
 *
 * A hit: damage of its type (blunt without an element, fire with one) after the target's
 * resistance. A fire bolt also adds burning and leaves a fire field where it lands.
 * Against a block: it bursts in front of the shield - only block.projectile_damage_through
 * of the damage gets through, the blocker pays stamina, and the field appears in front of
 * the blocker, not under its feet. Dodging works as for melee: out of the way = not hit.
 */
UCLASS()
class HYDRA_API AHydraProjectile : public AActor
{
	GENERATED_BODY()

public:
	AHydraProjectile();

	/** Server, right after spawning. */
	void Launch(AHydraCharacter* InCaster, const FVector& Direction, float InDamage, bool bInFire, float InChargeAlpha);

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	void ServerStep(const FVector& From, const FVector& To);
	void HitCharacter(AHydraCharacter* Target, const FVector& From, const FVector& ImpactPoint);
	void Burst(const TCHAR* Result, const FVector& FieldLocation, bool bLeaveField);
	void ApplyLook();

	UFUNCTION()
	void OnRep_Launch();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Ball;

	UPROPERTY(ReplicatedUsing = OnRep_Launch)
	FVector_NetQuantize Velocity = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_Launch)
	bool bFire = false;

	UPROPERTY(Replicated)
	float RadiusCm = 20.f;

	// Server.
	TWeakObjectPtr<AHydraCharacter> Caster;
	float Damage = 0.f;
	float ChargeAlpha = 0.f;
	float RangeLeftCm = 0.f;
	bool bDone = false;
};
