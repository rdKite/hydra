#include "Combat/HydraProjectile.h"

#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Character/HydraCharacter.h"
#include "Combat/HydraElementField.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HydraDevSubsystem.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Hydra.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace HydraProjectileLook
{
	const FLinearColor PlainColor(0.45f, 0.95f, 0.55f);   // a blob of slime
	const FLinearColor FireColor(1.0f, 0.35f, 0.05f);
}

AHydraProjectile::AHydraProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Ball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
	RootComponent = Ball;
	Ball->SetStaticMesh(Sphere.Object);
	Ball->SetMaterial(0, ShapeMaterial.Object);
	Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ball->SetGenerateOverlapEvents(false);
}

void AHydraProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHydraProjectile, Velocity);
	DOREPLIFETIME(AHydraProjectile, bFire);
	DOREPLIFETIME(AHydraProjectile, RadiusCm);
}

void AHydraProjectile::Launch(AHydraCharacter* InCaster, const FVector& Direction, float InDamage, bool bInFire, float InChargeAlpha)
{
	check(HasAuthority());
	const FHydraBoltValues& B = UHydraValuesSubsystem::Get(this).bolt;
	Caster = InCaster;
	Damage = InDamage;
	bFire = bInFire;
	ChargeAlpha = InChargeAlpha;
	RadiusCm = B.radius_m * 100.f;
	RangeLeftCm = B.range_m * 100.f;
	Velocity = Direction.GetSafeNormal() * B.speed_m_s * 100.f;
	ApplyLook();
}

void AHydraProjectile::BeginPlay()
{
	Super::BeginPlay();
	ApplyLook();
}

void AHydraProjectile::OnRep_Launch()
{
	ApplyLook();
}

void AHydraProjectile::ApplyLook()
{
	// Engine sphere: 100 cm diameter.
	Ball->SetWorldScale3D(FVector(RadiusCm / 50.f));
	if (GetNetMode() != NM_DedicatedServer)
	{
		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Ball->GetMaterial(0));
		if (!Material)
		{
			Material = Ball->CreateAndSetMaterialInstanceDynamic(0);
		}
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), bFire ? HydraProjectileLook::FireColor : HydraProjectileLook::PlainColor);
		}
	}
}

void AHydraProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDone || Velocity.IsNearlyZero())
	{
		return;
	}
	const FVector From = GetActorLocation();
	FVector To = From + FVector(Velocity) * DeltaSeconds;
	if (HasAuthority())
	{
		const float Step = FVector::Dist(From, To);
		if (Step > RangeLeftCm)
		{
			To = From + FVector(Velocity).GetSafeNormal() * RangeLeftCm;
		}
		RangeLeftCm -= Step;
		ServerStep(From, To);
		if (!bDone && RangeLeftCm <= 0.f)
		{
			Burst(TEXT("expired"), To, false);
		}
	}
	if (!bDone)
	{
		SetActorLocation(To);
	}
}

void AHydraProjectile::ServerStep(const FVector& From, const FVector& To)
{
	// Characters first: the nearest one whose body the ball touches on this step.
	AHydraCharacter* Nearest = nullptr;
	float NearestAlong = TNumericLimits<float>::Max();
	const FVector Direction = (To - From).GetSafeNormal();
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		AHydraCharacter* Target = *It;
		if (Target == Caster.Get() || !Target->IsAlive())
		{
			continue;
		}
		const UCapsuleComponent* Capsule = Target->GetCapsuleComponent();
		const FVector Centre = Target->GetActorLocation();
		// The body as a capsule: compare against its axis, flattened to the height of the ball.
		const float AxisHalf = FMath::Max(0.f, Capsule->GetScaledCapsuleHalfHeight() - Capsule->GetScaledCapsuleRadius());
		const FVector OnAxis(Centre.X, Centre.Y, FMath::Clamp((From.Z + To.Z) * 0.5f, Centre.Z - AxisHalf, Centre.Z + AxisHalf));
		if (HydraCombatMath::DistancePointToSegment(OnAxis, From, To) <= Capsule->GetScaledCapsuleRadius() + RadiusCm)
		{
			const float Along = FVector::DotProduct(OnAxis - From, Direction);
			if (Along < NearestAlong)
			{
				NearestAlong = Along;
				Nearest = Target;
			}
		}
	}
	// The world (floor, walls, obstacles).
	FHitResult WorldHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HydraProjectile), false, this);
	const bool bWorld = GetWorld()->SweepSingleByObjectType(WorldHit, From, To, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeSphere(RadiusCm * 0.5f), Params);
	if (Nearest && (!bWorld || NearestAlong <= WorldHit.Distance + RadiusCm))
	{
		HitCharacter(Nearest, From, From + Direction * FMath::Max(0.f, NearestAlong));
	}
	else if (bWorld)
	{
		Burst(TEXT("world"), WorldHit.ImpactPoint, bFire);
	}
}

void AHydraProjectile::HitCharacter(AHydraCharacter* Target, const FVector& From, const FVector& ImpactPoint)
{
	const FHydraValues& V = UHydraValuesSubsystem::Get(this);
	AHydraCharacter* Source = Caster.Get();
	const FHydraCombatStats& TargetStats = Target->GetStats();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	const FName DamageType = bFire ? FName(TEXT("fire")) : FName(TEXT("blunt"));

	const TCHAR* Result = TEXT("hit");
	float Raw = Damage;
	FVector FieldLocation = Target->GetActorLocation();
	bool bBlocked = Target->HasStateTag(HydraTags::State_Blocking) && TargetStats.bHasShield
		&& HydraCombatMath::IsInArc(Target->GetActorLocation(), Target->GetActorRotation().Yaw, From, TargetStats.Shield.block_half_arc_deg);
	if (bBlocked)
	{
		// A bolt has no weight of its own: the block costs what a light hit of equal weight costs.
		const float BlockCost = V.block.stamina_cost_per_hit;
		if (HydraGas::CanAfford(TargetASC, BlockCost))
		{
			Result = TEXT("blocked");
			HydraGas::SpendStamina(TargetASC, BlockCost, V.stamina);
			Raw *= FMath::Clamp(V.block.projectile_damage_through, 0.f, 1.f);
			// Bursts in front of the shield: the field lies between the two, not under the blocker.
			const FVector ToCaster = (From - Target->GetActorLocation()).GetSafeNormal2D();
			FieldLocation = Target->GetActorLocation() + ToCaster * (TargetStats.BodyRadiusCm + V.fire_field.radius_m * 100.f);
		}
		else
		{
			Result = TEXT("blockbreak");
			bBlocked = false;
			HydraGas::SpendStamina(TargetASC, Target->GetStamina(), V.stamina);
			const FGameplayTagContainer BlockTag(HydraTags::Ability_Block);
			TargetASC->CancelAbilities(&BlockTag);
		}
	}
	const float Dealt = AHydraCharacter::ServerDealDamage(Source, Target, Raw, DamageType);
	if (bFire && !bBlocked && Target->IsAlive())
	{
		Target->ServerAddBurning(V.burning.stacks_per_hit, Source, TEXT("bolt"));
	}
	UE_LOG(LogHydra, Log, TEXT("HydraBolt t=%.2f attacker=%s target=%s result=%s dmg=%.2f fire=%d charge=%.2f target_hp=%.2f target_stamina=%.2f burning=%d"),
		UHydraDevSubsystem::ClockOf(this), Source ? *Source->GetLogId() : TEXT("none"), *Target->GetLogId(), Result, Dealt, bFire ? 1 : 0, ChargeAlpha,
		Target->GetHealth(), Target->GetStamina(), Target->GetBurningStacks());
	Burst(nullptr, FieldLocation, bFire);
}

void AHydraProjectile::Burst(const TCHAR* Result, const FVector& FieldLocation, bool bLeaveField)
{
	bDone = true;
	if (Result)
	{
		UE_LOG(LogHydra, Log, TEXT("HydraBolt t=%.2f attacker=%s target=none result=%s fire=%d charge=%.2f x=%.0f y=%.0f"),
			UHydraDevSubsystem::ClockOf(this), Caster.IsValid() ? *Caster->GetLogId() : TEXT("none"), Result, bFire ? 1 : 0, ChargeAlpha, FieldLocation.X, FieldLocation.Y);
	}
	if (bLeaveField)
	{
		AHydraElementField::SpawnFireField(GetWorld(), FieldLocation, Caster.Get());
	}
	Destroy();
}
