// AHydraCharacter: setup, state, input and looks. Combat on the server: HydraCharacterCombat.cpp.

#include "Character/HydraCharacter.h"

#include "Abilities/HydraAttributeSet.h"
#include "Abilities/HydraCombatAbilities.h"
#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Character/HydraAnimInstance.h"
#include "Character/HydraMovementComponent.h"
#include "Character/HydraProgressionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Hydra.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace HydraCharacterLook
{
	// Placeholder look and camera feel - not gameplay values.
	constexpr float CapsuleRadius = 35.f;
	constexpr float CapsuleHalfHeight = 90.f;
	constexpr float CameraArmLength = 430.f;
	const FVector CameraShoulderOffset(0.f, 85.f, 105.f); // high over the right shoulder: the opponent in front stays visible

	const FLinearColor OwnColor(0.10f, 0.30f, 0.85f);
	const FLinearColor OtherColor(0.80f, 0.15f, 0.10f);
	const FLinearColor AIColor(0.65f, 0.60f, 0.35f);
	const FLinearColor VisorColor(0.02f, 0.02f, 0.02f);
	const FLinearColor ShieldColor(0.35f, 0.22f, 0.12f);
	const FLinearColor SteelColor(0.70f, 0.72f, 0.78f);
	const FLinearColor WindupColor(1.00f, 0.85f, 0.10f);     // telegraph of an attack
	const FLinearColor FullChargeColor(1.00f, 0.15f, 0.05f); // heavy attack fully charged
	const FLinearColor StrikeColor(1.00f, 1.00f, 1.00f);
	const FLinearColor HitFlashColor(1.00f, 1.00f, 1.00f);
	const FLinearColor BlockFlashColor(0.60f, 0.80f, 1.00f);
	constexpr float HitFlashS = 0.12f;
	const FLinearColor FireBodyTint(2.2f, 0.75f, 0.25f);     // element body fire: the slime's green turns orange
	const FLinearColor FireBodyGlow(0.9f, 0.18f, 0.02f);

	// Poses of sword and shield relative to the body (cm, degrees).
	const FVector SwordRest(18.f, 30.f, 5.f);
	const FRotator SwordRestRot(55.f, 0.f, 0.f);
	const FRotator SwordLightWindupRot(35.f, 100.f, 0.f);    // cocked back to the right
	const FRotator SwordLightStrikeEndRot(0.f, -75.f, 0.f);  // slash across the front
	const FVector SwordHeavyChargeLoc(-25.f, 30.f, 15.f);    // pulled back for a thrust
	const FVector SwordHeavyStrikeLoc(60.f, 12.f, 15.f);
	const FVector ShieldRest(8.f, -34.f, 0.f);
	const FRotator ShieldRestRot(0.f, -55.f, 0.f);
	const FVector ShieldBlock(34.f, -6.f, 12.f);
	const FRotator ShieldBlockRot(0.f, 0.f, 0.f);

	// The humanoid base model holds the placeholder sword and shield at its socket bones.
	// Bones from Blender have their length along +Y; the blade and the shield's face look along +X.
	const FName WeaponSocket(TEXT("weapon_r"));
	const FName ShieldSocket(TEXT("shield_l"));
	const FVector SwordInHand(0.f, -6.f, 0.f);
	const FRotator SwordInHandRot(0.f, 90.f, 0.f);
	const FVector ShieldOnArm(0.f, 4.f, 0.f);
	const FRotator ShieldOnArmRot(0.f, 90.f, 0.f);
	const FName MorphMale(TEXT("male"));
	const FName MorphFemale(TEXT("female"));
}

AHydraCharacter::AHydraCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UHydraMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	using namespace HydraCharacterLook;

	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	// Facing is driven by the movement component (turn rate), never directly by the controller.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MinAnalogWalkSpeed = 20.f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->bRunPhysicsWithNoController = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	BodyRoot->SetupAttachment(RootComponent);
	SwordPivot = CreateDefaultSubobject<USceneComponent>(TEXT("SwordPivot"));
	SwordPivot->SetupAttachment(BodyRoot);
	ShieldPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ShieldPivot"));
	ShieldPivot->SetupAttachment(BodyRoot);

	auto MakePart = [this](FName Name, USceneComponent* Parent, UStaticMesh* PartMesh, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(PartMesh);
		Part->SetMaterial(0, ShapeMaterial.Object);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		// Marked for the look variants (post-process material PP_Look: characters = stencil 1).
		Part->SetRenderCustomDepth(true);
		Part->SetCustomDepthStencilValue(1);
		return Part;
	};
	// Engine shapes are 100 cm; the capsule centre is the origin (feet at -90).
	BodyTorso = MakePart(TEXT("BodyTorso"), BodyRoot, Cylinder.Object, FVector(0.f, 0.f, -25.f), FVector(0.6f, 0.6f, 1.3f));
	BodyHead = MakePart(TEXT("BodyHead"), BodyRoot, Sphere.Object, FVector(0.f, 0.f, 62.f), FVector(0.38f));
	BodyVisor = MakePart(TEXT("BodyVisor"), BodyRoot, Cube.Object, FVector(17.f, 0.f, 64.f), FVector(0.1f, 0.26f, 0.08f));
	// Sword: the blade points along the pivot's +X. Shield: a disc whose face looks along +X.
	SwordBlade = MakePart(TEXT("SwordBlade"), SwordPivot, Cube.Object, FVector(55.f, 0.f, 0.f), FVector(0.95f, 0.07f, 0.035f));
	SwordGuard = MakePart(TEXT("SwordGuard"), SwordPivot, Cube.Object, FVector(6.f, 0.f, 0.f), FVector(0.03f, 0.2f, 0.03f));
	ShieldDisc = MakePart(TEXT("ShieldDisc"), ShieldPivot, Cylinder.Object, FVector::ZeroVector, FVector(0.6f, 0.6f, 0.05f));
	ShieldDisc->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	SwordPivot->SetRelativeLocation(SwordRest);
	SwordPivot->SetRelativeRotation(SwordRestRot);
	ShieldPivot->SetRelativeLocation(ShieldRest);
	ShieldPivot->SetRelativeRotation(ShieldRestRot);

	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(1);

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

	// GAS: component on the pawn (players and AI alike). Mixed: effects replicate to the
	// owner only, attributes to everyone.
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	Attributes = CreateDefaultSubobject<UHydraAttributeSet>(TEXT("Attributes"));
	Progression = CreateDefaultSubobject<UHydraProgressionComponent>(TEXT("Progression"));
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

void AHydraCharacter::OnRep_AIControlled()
{
	UpdateTint();
}

void AHydraCharacter::OnRep_FormId()
{
	RebuildStats();
}

void AHydraCharacter::OnRep_BodyLook()
{
	ApplyBodyLook();
}

void AHydraCharacter::ServerSetBodyLook(uint8 InBodyPreset, float InLegScale)
{
	BodyPreset = FMath::Min<uint8>(InBodyPreset, 2);
	LegScale = FMath::Clamp(InLegScale, 0.3f, 2.f);
	ApplyBodyLook();
	UE_LOG(LogHydra, Log, TEXT("HydraBodyLook who=%s preset=%d legs=%.2f"), *GetLogId(), BodyPreset, LegScale);
}

void AHydraCharacter::ApplyBodyLook()
{
	// The presets are shape keys of the one base model (morph targets); the leg scale is
	// applied to the bones by the anim instance every frame.
	if (USkeletalMeshComponent* Model = GetMesh(); Model && bUsesModel)
	{
		Model->SetMorphTarget(HydraCharacterLook::MorphMale, BodyPreset == 1 ? 1.f : 0.f);
		Model->SetMorphTarget(HydraCharacterLook::MorphFemale, BodyPreset == 2 ? 1.f : 0.f);
	}
}

void AHydraCharacter::OnRep_Dead()
{
	// The tag blocks abilities and movement on the client as well.
	AbilitySystem->SetLooseGameplayTagCount(HydraTags::State_Dead, bDead ? 1 : 0);
	if (bDead)
	{
		SetLockOnTarget(nullptr);
		PredictedAction = FHydraActionState();
	}
}

void AHydraCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHydraCharacter, FormId);
	DOREPLIFETIME(AHydraCharacter, bAIControlled);
	DOREPLIFETIME(AHydraCharacter, bDead);
	DOREPLIFETIME(AHydraCharacter, ActionState);
	DOREPLIFETIME(AHydraCharacter, ChainCount);
	DOREPLIFETIME(AHydraCharacter, BurningStacks);
	DOREPLIFETIME(AHydraCharacter, BodyPreset);
	DOREPLIFETIME(AHydraCharacter, LegScale);
	DOREPLIFETIME(AHydraCharacter, EnemyId);
	DOREPLIFETIME(AHydraCharacter, EnemySerial);
}

float AHydraCharacter::GetHealth() const { return Attributes->GetHealth(); }
float AHydraCharacter::GetMaxHealth() const { return Attributes->GetMaxHealth(); }
float AHydraCharacter::GetStamina() const { return Attributes->GetStamina(); }
float AHydraCharacter::GetMaxStamina() const { return Attributes->GetMaxStamina(); }
float AHydraCharacter::GetMana() const { return Attributes->GetMana(); }
float AHydraCharacter::GetMaxMana() const { return Attributes->GetMaxMana(); }

bool AHydraCharacter::HasStateTag(const FGameplayTag& Tag) const
{
	return AbilitySystem->HasMatchingGameplayTag(Tag);
}

UHydraMovementComponent* AHydraCharacter::GetHydraMovement() const
{
	return CastChecked<UHydraMovementComponent>(GetCharacterMovement());
}

FString AHydraCharacter::GetLogId() const
{
	if (bAIControlled)
	{
		return EnemyId.IsNone() ? FString(TEXT("ai")) : FString::Printf(TEXT("%s%d"), *EnemyId.ToString(), EnemySerial);
	}
	const APlayerState* PS = GetPlayerState();
	return PS ? FString::FromInt(PS->GetPlayerId()) : FString(TEXT("unknown"));
}

float AHydraCharacter::GetServerTime() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	return GS ? static_cast<float>(GS->GetServerWorldTimeSeconds()) : (World ? World->GetTimeSeconds() : 0.f);
}

void AHydraCharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnTransform = GetActorTransform();
	UpdateTint();
	if (HasAuthority())
	{
		InitProgress();
	}
	RebuildStats();
	if (HasAuthority())
	{
		GrantAbilities();
	}
	if (UHydraValuesSubsystem* ValuesSubsystem = UHydraValuesSubsystem::GetSubsystem(this))
	{
		ValuesReloadedHandle = ValuesSubsystem->OnReloaded.AddUObject(this, &AHydraCharacter::OnValuesReloaded);
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

void AHydraCharacter::OnValuesReloaded(const FHydraValues&)
{
	RebuildStats();
}

void AHydraCharacter::OnProgressChanged()
{
	if (HasActorBegunPlay())
	{
		RebuildStats();
	}
}

bool AHydraCharacter::HasFireImprint() const
{
	return Progression->GetImprintLevel() > 0;
}

void AHydraCharacter::SetEnemyType(FName InEnemyId, int32 InSerial, int32 InLevelBonus)
{
	EnemyId = InEnemyId;
	EnemySerial = InSerial;
	EnemyLevelBonus = InLevelBonus;
	bAIControlled = true;
}

void AHydraCharacter::InitProgress()
{
	// Arena rule: start at fixed levels. A wave enemy has the level and skill level of its type.
	check(HasAuthority());
	const UHydraValuesSubsystem* ValuesSubsystem = UHydraValuesSubsystem::GetSubsystem(this);
	if (!ValuesSubsystem)
	{
		return;
	}
	const FHydraValues& V = ValuesSubsystem->GetValues();
	const FHydraEnemyValues* Enemy = V.enemies.Find(EnemyId);
	const FHydraEnemyDef* EnemyDef = ValuesSubsystem->GetDefinitions().FindEnemy(EnemyId);
	if (Enemy && EnemyDef)
	{
		FormId = EnemyDef->form;
		Progression->ServerInit(Enemy->level + EnemyLevelBonus, Enemy->skill_level + EnemyLevelBonus, EnemyDef->imprint.IsNone() ? 0 : 1);
	}
	else if (const FHydraFormValues* Form = V.forms.Find(FormId))
	{
		Progression->ServerInit(Form->start_level, Form->start_skill_level, 0);
	}
}

void AHydraCharacter::RebuildStats()
{
	const UHydraValuesSubsystem* ValuesSubsystem = UHydraValuesSubsystem::GetSubsystem(this);
	if (!ValuesSubsystem)
	{
		return;
	}
	const FHydraValues& V = ValuesSubsystem->GetValues();
	Stats = HydraCombatMath::BuildStats(ValuesSubsystem->GetDefinitions(), V, FormId, Progression->MakeView());
	if (!Stats.bValid)
	{
		UE_LOG(LogHydra, Error, TEXT("Character %s: form '%s' is unknown or has no numbers in values.json"), *GetName(), *FormId.ToString());
	}
	// Movement values are needed on both sides (prediction); attributes are server-only.
	GetHydraMovement()->ApplyValues(V, Stats.WeightRatio);
	ApplyFormLook();
	if (HasAuthority() && Stats.bValid)
	{
		HydraGas::ApplyHealthValues(AbilitySystem, Stats.HpMax, !bAttributesInitialized);
		HydraGas::ApplyStaminaValues(AbilitySystem, Stats.StaminaMax, Stats.StaminaRegenPerS, StaminaRegenHandle, !bAttributesInitialized);
		HydraGas::ApplyManaValues(AbilitySystem, Stats.ManaMax, Stats.ManaRegenPerS, ManaRegenHandle, !bAttributesInitialized);
		bAttributesInitialized = true;
		UE_LOG(LogHydra, Log, TEXT("HydraCombatStats who=%s level=%d form=%s ai=%d weight=%.2f strength=%.2f ratio=%.4f turn=%.1f hp=%.1f stamina=%.1f regen=%.1f mana=%.1f light_cost=%.2f heavy_cost=%.2f dodge_cost=%.2f light_dmg=%.2f heavy_dmg=%.2f weak=%.3f interrupt=%.1f block_hold=%.2f blocker_share=%.3f block_arc=%.1f bolt_bonus=%.3f fire_bolt_bonus=%.3f radius=%.0f half_height=%.0f"),
			*GetLogId(), Stats.Level, *FormId.ToString(), bAIControlled ? 1 : 0, Stats.TotalWeight, Stats.Strength, Stats.WeightRatio, Stats.TurnRateDegS, Stats.HpMax, Stats.StaminaMax,
			Stats.StaminaRegenPerS, Stats.ManaMax, Stats.LightStaminaCost, Stats.HeavyStaminaCost, Stats.DodgeStaminaCost, Stats.Weapon.light.damage, Stats.Weapon.heavy.damage,
			Stats.WeakPointMultiplier, Stats.InterruptThreshold, Stats.BlockHoldStaminaPerS, Stats.BlockerShare, Stats.Shield.block_half_arc_deg, Stats.BoltBonus, Stats.FireBoltBonus,
			Stats.BodyRadiusCm, Stats.BodyHalfHeightCm);
	}
}

void AHydraCharacter::ApplyFormLook()
{
	if (!Stats.bValid)
	{
		return;
	}
	// Body size of the form. The feet stay where they are.
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float OldHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	if (!FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleRadius(), Stats.BodyRadiusCm) || !FMath::IsNearlyEqual(OldHalfHeight, Stats.BodyHalfHeightCm))
	{
		Capsule->SetCapsuleSize(Stats.BodyRadiusCm, Stats.BodyHalfHeightCm);
		if (HasActorBegunPlay())
		{
			AddActorWorldOffset(FVector(0.f, 0.f, Stats.BodyHalfHeightCm - OldHalfHeight));
		}
	}

	// Looks: a model from the Blender pipeline if the form has one
	// (/Game/Characters/<model>/SK_<model> with animations A_<model>_<Name>; forms.json names
	// the model, by default the form's own id), otherwise the placeholder body from primitives.
	USkeletalMeshComponent* Model = GetMesh();
	if (LookBuiltForForm != FormId)
	{
		LookBuiltForForm = FormId;
		FormAnimations.Reset();
		PlayingAnimation = NAME_None;
		const UHydraValuesSubsystem* ValuesSubsystem = UHydraValuesSubsystem::GetSubsystem(this);
		const FHydraFormDef* FormDef = ValuesSubsystem ? ValuesSubsystem->GetDefinitions().FindForm(FormId) : nullptr;
		const FString Form = (FormDef && !FormDef->model.IsNone() ? FormDef->model : FormId).ToString();
		const FString Folder = FString::Printf(TEXT("/Game/Characters/%s/"), *Form);
		USkeletalMesh* ModelMesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%sSK_%s.SK_%s"), *Folder, *Form, *Form), nullptr, LOAD_NoWarn | LOAD_Quiet);
		bUsesModel = ModelMesh != nullptr;
		Model->SetSkeletalMesh(ModelMesh);
		if (bUsesModel)
		{
			// Animation in C++: UHydraAnimInstance evaluates the clip this class picks.
			Model->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			Model->SetAnimInstanceClass(UHydraAnimInstance::StaticClass());
			Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Model->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			static const TCHAR* Names[] = { TEXT("Idle"), TEXT("Move"), TEXT("Windup"), TEXT("Charge"), TEXT("Strike"), TEXT("HeavyStrike"), TEXT("Recovery"),
				TEXT("Cast"), TEXT("CastCharge"), TEXT("CastRelease"), TEXT("Block"), TEXT("Dodge"), TEXT("Jump"), TEXT("Death") };
			for (const TCHAR* Name : Names)
			{
				const FString Asset = FString::Printf(TEXT("%sA_%s_%s.A_%s_%s"), *Folder, *Form, Name, *Form, Name);
				if (UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *Asset, nullptr, LOAD_NoWarn | LOAD_Quiet))
				{
					FormAnimations.Add(FName(Name), Sequence);
				}
			}
			// A form with an element body shows it: the model of its line, tinted and glowing
			// (material parameters Tint and Glow of the generated material, Scripts/import_art.py).
			if (GetNetMode() != NM_DedicatedServer)
			{
				const bool bFireBody = FormDef && FormDef->element_body == FName(TEXT("fire"));
				if (UMaterialInstanceDynamic* Material = Model->CreateAndSetMaterialInstanceDynamic(0))
				{
					Material->SetVectorParameterValue(TEXT("Tint"), bFireBody ? HydraCharacterLook::FireBodyTint : FLinearColor::White);
					Material->SetVectorParameterValue(TEXT("Glow"), bFireBody ? HydraCharacterLook::FireBodyGlow : FLinearColor::Black);
				}
			}
			UE_LOG(LogHydra, Log, TEXT("Form %s uses the model %s with %d animations"), *FormId.ToString(), *Form, FormAnimations.Num());
		}
		// Sword and shield (placeholders from primitives): in the hands of a model that has the
		// socket bones, otherwise on the placeholder body.
		bModelHoldsItems = bUsesModel && Model->GetBoneIndex(HydraCharacterLook::WeaponSocket) != INDEX_NONE && Model->GetBoneIndex(HydraCharacterLook::ShieldSocket) != INDEX_NONE;
		const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, false);
		if (bModelHoldsItems)
		{
			SwordPivot->AttachToComponent(Model, Rules, HydraCharacterLook::WeaponSocket);
			SwordPivot->SetRelativeLocationAndRotation(HydraCharacterLook::SwordInHand, HydraCharacterLook::SwordInHandRot);
			ShieldPivot->AttachToComponent(Model, Rules, HydraCharacterLook::ShieldSocket);
			ShieldPivot->SetRelativeLocationAndRotation(HydraCharacterLook::ShieldOnArm, HydraCharacterLook::ShieldOnArmRot);
		}
		else
		{
			SwordPivot->AttachToComponent(BodyRoot, Rules);
			SwordPivot->SetRelativeLocationAndRotation(HydraCharacterLook::SwordRest, HydraCharacterLook::SwordRestRot);
			ShieldPivot->AttachToComponent(BodyRoot, Rules);
			ShieldPivot->SetRelativeLocationAndRotation(HydraCharacterLook::ShieldRest, HydraCharacterLook::ShieldRestRot);
		}
		ApplyBodyLook();
	}
	// Models from Blender look along +Y and stand on the floor at their origin.
	Model->SetRelativeLocation(FVector(0.f, 0.f, -Stats.BodyHalfHeightCm));
	Model->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	// Network smoothing puts the mesh of other players' characters back to this cached
	// offset every frame; without it their model floats at the middle of the capsule.
	CacheInitialMeshOffset(Model->GetRelativeLocation(), Model->GetRelativeRotation());
	Model->SetVisibility(bUsesModel);
	BodyTorso->SetVisibility(!bUsesModel);
	BodyHead->SetVisibility(!bUsesModel);
	BodyVisor->SetVisibility(!bUsesModel);
	SwordPivot->SetVisibility((!bUsesModel || bModelHoldsItems) && Stats.bHasWeapon, true);
	ShieldPivot->SetVisibility((!bUsesModel || bModelHoldsItems) && Stats.bHasShield, true);
}

void AHydraCharacter::GrantAbilities()
{
	check(HasAuthority());
	if (bAbilitiesGranted)
	{
		return;
	}
	bAbilitiesGranted = true;
	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UHydraGA_Attack::StaticClass(), 1));
	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UHydraGA_Block::StaticClass(), 1));
	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UHydraGA_Dodge::StaticClass(), 1));
	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UHydraGA_Cast::StaticClass(), 1));
}

// --- Actions -----------------------------------------------------------------------------

void AHydraCharacter::SetAction(EHydraAction Action, float DurationS)
{
	FHydraActionState NewState;
	NewState.Action = Action;
	NewState.StartServerTime = GetServerTime();
	NewState.DurationS = DurationS;
	if (HasAuthority())
	{
		ActionState = NewState;
	}
	else if (IsLocallyControlled())
	{
		PredictedAction = NewState;
	}
}

void AHydraCharacter::ClearAction(std::initializer_list<EHydraAction> Actions)
{
	FHydraActionState& State = HasAuthority() ? ActionState : PredictedAction;
	for (const EHydraAction Action : Actions)
	{
		if (State.Action == Action)
		{
			State = FHydraActionState();
			State.StartServerTime = GetServerTime();
			return;
		}
	}
}

const FHydraActionState& AHydraCharacter::GetShownAction() const
{
	// The owning client shows its own predicted actions (every action is one of its
	// abilities); everyone else shows what the server replicates.
	return (!HasAuthority() && IsLocallyControlled()) ? PredictedAction : ActionState;
}

// --- Input -------------------------------------------------------------------------------

void AHydraCharacter::InputAttackPressed()
{
	AbilitySystem->TryActivateAbilityByClass(UHydraGA_Attack::StaticClass());
}

void AHydraCharacter::InputAttackReleased()
{
	NotifyReleased(UHydraGA_Attack::StaticClass());
	if (!HasAuthority())
	{
		ServerAttackReleased();
	}
}

void AHydraCharacter::ServerAttackReleased_Implementation()
{
	NotifyReleased(UHydraGA_Attack::StaticClass());
}

void AHydraCharacter::ServerCastReleased_Implementation()
{
	NotifyReleased(UHydraGA_Cast::StaticClass());
}

void AHydraCharacter::ServerSetCastAim_Implementation(float Yaw)
{
	CastAimYaw = Yaw;
}

void AHydraCharacter::NotifyReleased(TSubclassOf<UGameplayAbility> AbilityClass)
{
	const FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(AbilityClass);
	if (Spec && Spec->IsActive())
	{
		if (UHydraGameplayAbility* Ability = Cast<UHydraGameplayAbility>(Spec->GetPrimaryInstance()))
		{
			Ability->OnInputReleased();
		}
	}
}

void AHydraCharacter::InputOffHand(bool bPressed, float AimYaw)
{
	// The off hand does what it holds: a shield blocks; a form with the projectile skill casts.
	if (Stats.bHasShield)
	{
		InputBlock(bPressed);
	}
	else if (Stats.bCanCast)
	{
		if (bPressed)
		{
			CastAimYaw = AimYaw;
			if (!HasAuthority())
			{
				ServerSetCastAim(AimYaw);
			}
			AbilitySystem->TryActivateAbilityByClass(UHydraGA_Cast::StaticClass());
		}
		else
		{
			NotifyReleased(UHydraGA_Cast::StaticClass());
			if (!HasAuthority())
			{
				ServerCastReleased();
			}
		}
	}
}

void AHydraCharacter::BeginCastFacing()
{
	// While the bolt is wound up and charged the caster follows the camera direction (the
	// movement component does that at the turn rate); an AI has no camera and turns to where
	// it aimed the cast.
	UHydraMovementComponent* Move = GetHydraMovement();
	Move->ClearFacingGoal();
	if (bAIControlled && !Move->bWantsStrafe)
	{
		Move->SetFacingGoal(CastAimYaw);
	}
}

void AHydraCharacter::InputBlock(bool bPressed)
{
	bBlockHeld = bPressed;
	if (bPressed)
	{
		AbilitySystem->TryActivateAbilityByClass(UHydraGA_Block::StaticClass());
	}
	else
	{
		const FGameplayTagContainer CancelTags(HydraTags::Ability_Block);
		AbilitySystem->CancelAbilities(&CancelTags);
	}
}

void AHydraCharacter::InputDodge()
{
	AbilitySystem->TryActivateAbilityByClass(UHydraGA_Dodge::StaticClass());
}

void AHydraCharacter::InputJump(bool bPressed)
{
	// Jumping is ordinary character movement (predicted by the engine).
	bPressed ? Jump() : StopJumping();
}

bool AHydraCharacter::CanJumpInternal_Implementation() const
{
	return Super::CanJumpInternal_Implementation() && IsAlive()
		&& !HasStateTag(HydraTags::State_AttackCommitted) && !HasStateTag(HydraTags::State_Blocking) && !HasStateTag(HydraTags::State_Dodging);
}

FVector AHydraCharacter::ConsumeDodgeDirection()
{
	const FVector Direction = PendingDodgeDirection;
	PendingDodgeDirection = FVector::ZeroVector;
	return Direction;
}

// --- Lock-on and aim assist --------------------------------------------------------------

void AHydraCharacter::SetLockOnTarget(AHydraCharacter* NewTarget)
{
	LockOnTarget = NewTarget;
	RefreshStrafe();
}

void AHydraCharacter::SetPreciseMovement(bool bOn)
{
	bPreciseMovement = bOn;
	RefreshStrafe();
}

void AHydraCharacter::RefreshStrafe()
{
	GetHydraMovement()->bWantsStrafe = LockOnTarget.IsValid() || bPreciseMovement;
}

AHydraCharacter* AHydraCharacter::FindBestTarget(const FVector& Origin, const FVector& Direction, float RangeCm, float MaxAngleDeg) const
{
	AHydraCharacter* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		AHydraCharacter* Other = *It;
		float Score = 0.f;
		if (Other != this && Other->IsAlive() && HydraCombatMath::ScoreTargetInCone(Origin, Direction, Other->GetActorLocation(), RangeCm, MaxAngleDeg, Score) && Score < BestScore)
		{
			BestScore = Score;
			Best = Other;
		}
	}
	return Best;
}

void AHydraCharacter::BeginAttackFacing()
{
	UHydraMovementComponent* Move = GetHydraMovement();
	Move->ClearFacingGoal();
	if (Move->bWantsStrafe)
	{
		return; // locked on, precise movement or AI: the character already faces where it aims
	}
	const FHydraAimAssistValues& A = UHydraValuesSubsystem::Get(this).aim_assist;
	if (const AHydraCharacter* Target = FindBestTarget(GetActorLocation(), GetActorForwardVector(), A.range_m * 100.f, A.angle_deg))
	{
		const float CurrentYaw = GetActorRotation().Yaw;
		const float TargetYaw = (Target->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
		Move->SetFacingGoal(HydraCombatMath::AimAssistYaw(CurrentYaw, TargetYaw, A.strength));
		if (HasAuthority())
		{
			UE_LOG(LogHydra, Log, TEXT("HydraAimAssist attacker=%s yaw_before=%.1f yaw_to_target=%.1f yaw_goal=%.1f strength=%.2f"),
				*GetLogId(), CurrentYaw, TargetYaw, HydraCombatMath::AimAssistYaw(CurrentYaw, TargetYaw, A.strength), A.strength);
		}
	}
}

void AHydraCharacter::EndAttackFacing()
{
	GetHydraMovement()->ClearFacingGoal();
}

// --- Tick and looks ----------------------------------------------------------------------

void AHydraCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		RecordPose();
		// The chain ends after a pause without a hit.
		if (ChainCount > 0 && GetChainHitsNow() == 0)
		{
			ClearChain();
		}
		UpdateBurning();
	}
	UpdateRegenTags();
	// Holding the block button blocks as soon as it is possible (after an attack, a dodge).
	if (bBlockHeld && IsLocallyControlled() && !HasStateTag(HydraTags::State_Blocking))
	{
		AbilitySystem->TryActivateAbilityByClass(UHydraGA_Block::StaticClass());
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		UpdatePose();
	}
}

void AHydraCharacter::UpdateRegenTags()
{
	// No regeneration during an action: stamina not while an attack (press to end of
	// recovery), a dodge (step and recovery) or a raised block runs; mana not while a bolt is
	// wound up or charged. The server decides; the owning client knows its own actions too
	// (predicted abilities) and shows the same.
	if (!HasAuthority() && !IsLocallyControlled())
	{
		return;
	}
	const bool bWeaponAttack = HasStateTag(HydraTags::State_Attacking) && !HasStateTag(HydraTags::State_Spell);
	const bool bStaminaBlocked = bWeaponAttack || HasStateTag(HydraTags::State_Dodging) || HasStateTag(HydraTags::State_Blocking);
	const bool bManaBlocked = HasStateTag(HydraTags::State_Casting);
	if (bStaminaBlocked != HasStateTag(HydraTags::State_StaminaRegenBlocked))
	{
		AbilitySystem->SetLooseGameplayTagCount(HydraTags::State_StaminaRegenBlocked, bStaminaBlocked ? 1 : 0);
	}
	if (bManaBlocked != HasStateTag(HydraTags::State_ManaRegenBlocked))
	{
		AbilitySystem->SetLooseGameplayTagCount(HydraTags::State_ManaRegenBlocked, bManaBlocked ? 1 : 0);
	}
}

void AHydraCharacter::UpdateTint()
{
	using namespace HydraCharacterLook;
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
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
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	FLinearColor BodyColor = bAIControlled ? AIColor : (IsLocallyControlled() ? OwnColor : OtherColor);
	if (Now < HitFlashUntil)
	{
		BodyColor = HitFlashKind == 1 ? BlockFlashColor : HitFlashColor;
	}
	// The humanoid base model is plain: it takes the colour of its side like the placeholder body.
	if (bModelHoldsItems)
	{
		if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(GetMesh()->GetMaterial(0)))
		{
			Material->SetVectorParameterValue(TEXT("Tint"), FMath::Lerp(FLinearColor::White, BodyColor, 0.65f) * 1.25f);
		}
	}
	Tint(BodyTorso, BodyColor);
	Tint(BodyHead, BodyColor);
	Tint(BodyVisor, VisorColor);
	Tint(ShieldDisc, ShieldColor);
	Tint(SwordGuard, SteelColor);

	// The blade carries the telegraph: yellow in the wind-up, towards red while a heavy attack charges.
	const FHydraActionState& Shown = GetShownAction();
	FLinearColor BladeColor = SteelColor;
	if (IsAlive())
	{
		const float Elapsed = GetServerTime() - Shown.StartServerTime;
		switch (Shown.Action)
		{
		case EHydraAction::LightWindup: BladeColor = WindupColor; break;
		case EHydraAction::HeavyCharge: BladeColor = FMath::Lerp(WindupColor, FullChargeColor, Shown.DurationS > 0.f ? FMath::Clamp(Elapsed / Shown.DurationS, 0.f, 1.f) : 1.f); break;
		case EHydraAction::LightStrike:
		case EHydraAction::HeavyStrike: BladeColor = StrikeColor; break;
		default: break;
		}
	}
	Tint(SwordBlade, BladeColor);
}

void AHydraCharacter::UpdatePose()
{
	using namespace HydraCharacterLook;
	UpdateTint();

	// Dead: the body lies on its back.
	BodyRoot->SetRelativeLocation(IsAlive() ? FVector::ZeroVector : FVector(-40.f, 0.f, -65.f));
	BodyRoot->SetRelativeRotation(IsAlive() ? FRotator::ZeroRotator : FRotator(85.f, 0.f, 0.f));

	if (bUsesModel)
	{
		UpdateMeshAnimation();
		return;
	}

	// Hit stop: the pose freezes for a moment (looks only; the timeline of the ability runs on).
	if (GetWorld()->GetTimeSeconds() < HitStopUntil)
	{
		return;
	}

	const FHydraActionState& Shown = GetShownAction();
	const float Elapsed = FMath::Max(0.f, GetServerTime() - Shown.StartServerTime);
	const float Alpha = Shown.DurationS > 0.f ? FMath::Clamp(Elapsed / Shown.DurationS, 0.f, 1.f) : FMath::Clamp(Elapsed / 0.1f, 0.f, 1.f);

	FVector SwordLoc = SwordRest;
	FRotator SwordRot = SwordRestRot;
	FVector ShieldLoc = ShieldRest;
	FRotator ShieldRot = ShieldRestRot;
	switch (IsAlive() ? Shown.Action : EHydraAction::None)
	{
	case EHydraAction::LightWindup:
		SwordRot = FMath::Lerp(SwordRestRot, SwordLightWindupRot, Alpha);
		break;
	case EHydraAction::HeavyCharge:
		SwordLoc = FMath::Lerp(SwordRest, SwordHeavyChargeLoc, FMath::Min(1.f, Elapsed / 0.15f));
		SwordRot = FRotator::ZeroRotator;
		break;
	case EHydraAction::LightStrike:
		SwordRot = FMath::Lerp(SwordLightWindupRot, SwordLightStrikeEndRot, Alpha);
		break;
	case EHydraAction::HeavyStrike:
		SwordLoc = FMath::Lerp(SwordHeavyChargeLoc, SwordHeavyStrikeLoc, Alpha);
		SwordRot = FRotator::ZeroRotator;
		break;
	case EHydraAction::Recovery:
		// From roughly where the strike ended back to rest.
		SwordLoc = FMath::Lerp(SwordPivot->GetRelativeLocation(), SwordRest, 0.15f);
		SwordRot = FMath::Lerp(SwordPivot->GetRelativeRotation(), SwordRestRot, 0.15f);
		break;
	case EHydraAction::Block:
		ShieldLoc = FMath::Lerp(ShieldRest, ShieldBlock, Alpha);
		ShieldRot = FMath::Lerp(ShieldRestRot, ShieldBlockRot, Alpha);
		break;
	default:
		break;
	}
	SwordPivot->SetRelativeLocation(SwordLoc);
	SwordPivot->SetRelativeRotation(SwordRot);
	ShieldPivot->SetRelativeLocation(ShieldLoc);
	ShieldPivot->SetRelativeRotation(ShieldRot);
}

void AHydraCharacter::UpdateMeshAnimation()
{
	// One clip per thing the game shows; clips of actions are time-scaled to the action's
	// duration from values.json, so wind-up, strike and recovery follow the data. The anim
	// instance (C++) fades between the clips and scales bone lengths.
	USkeletalMeshComponent* Model = GetMesh();
	UHydraAnimInstance* Anim = Cast<UHydraAnimInstance>(Model->GetAnimInstance());
	if (!Anim)
	{
		return;
	}
	const FHydraActionState& Shown = GetShownAction();
	FName Wanted = TEXT("Idle");
	bool bLoop = true;
	if (!IsAlive())
	{
		Wanted = TEXT("Death");
		bLoop = false;
	}
	else
	{
		switch (Shown.Action)
		{
		case EHydraAction::LightWindup: Wanted = TEXT("Windup"); bLoop = false; break;
		case EHydraAction::HeavyCharge: Wanted = TEXT("Charge"); break;
		case EHydraAction::LightStrike: Wanted = TEXT("Strike"); bLoop = false; break;
		case EHydraAction::HeavyStrike: Wanted = FormAnimations.Contains(TEXT("HeavyStrike")) ? TEXT("HeavyStrike") : TEXT("Strike"); bLoop = false; break;
		case EHydraAction::Recovery: Wanted = TEXT("Recovery"); bLoop = false; break;
		case EHydraAction::CastWindup: Wanted = TEXT("Cast"); bLoop = false; break;
		case EHydraAction::CastCharge: Wanted = TEXT("CastCharge"); break;
		case EHydraAction::CastRelease: Wanted = TEXT("CastRelease"); bLoop = false; break;
		case EHydraAction::Block: Wanted = TEXT("Block"); bLoop = false; break;
		case EHydraAction::Dodge: Wanted = TEXT("Dodge"); bLoop = false; break;
		default:
			Wanted = (GetCharacterMovement()->IsFalling() && FormAnimations.Contains(TEXT("Jump"))) ? TEXT("Jump")
				: (GetVelocity().Size2D() > 20.f ? TEXT("Move") : TEXT("Idle"));
			break;
		}
	}
	const TObjectPtr<UAnimSequence>* Found = FormAnimations.Find(Wanted);
	if (!Found)
	{
		// A form without this clip (the slime has no block, no jump clip): its idle instead.
		Wanted = TEXT("Idle");
		bLoop = true;
		Found = FormAnimations.Find(Wanted);
	}
	const bool bActionRestarted = !bLoop && IsAlive() && !FMath::IsNearlyEqual(Shown.StartServerTime, PlayingStartTime);
	if (Wanted != PlayingAnimation || bActionRestarted)
	{
		PlayingAnimation = Wanted;
		PlayingStartTime = Shown.StartServerTime;
		AnimClipTime = 0.f;
		++AnimSerial;
	}
	const UAnimSequence* Clip = Found ? Found->Get() : nullptr;
	const float Length = Clip ? Clip->GetPlayLength() : 0.f;
	// Hit stop: the pose freezes for a moment (looks only; the timeline of the ability runs on).
	if (GetWorld()->GetTimeSeconds() >= HitStopUntil && Length > 0.f)
	{
		const bool bTimed = !bLoop && IsAlive() && Shown.DurationS > 0.f;
		if (bTimed)
		{
			AnimClipTime = FMath::Clamp((GetServerTime() - Shown.StartServerTime) / Shown.DurationS, 0.f, 1.f) * Length;
		}
		else if (bLoop)
		{
			AnimClipTime = FMath::Fmod(AnimClipTime + GetWorld()->GetDeltaSeconds(), Length);
		}
		else
		{
			AnimClipTime = FMath::Min(AnimClipTime + GetWorld()->GetDeltaSeconds(), Length);   // plays once and holds (block, death)
		}
	}
	FHydraAnimInput Input;
	Input.Clip = Clip;
	Input.Time = AnimClipTime;
	Input.bLoop = bLoop;
	Input.Serial = AnimSerial;
	Input.LegScale = LegScale;
	Anim->SetInput(Input);
}

void AHydraCharacter::MulticastHitFeedback_Implementation(uint8 Kind, float HitStopS)
{
	if (const UWorld* World = GetWorld())
	{
		const double Now = World->GetTimeSeconds();
		HitStopUntil = Now + HitStopS;
		if (Kind != 2)
		{
			HitFlashKind = Kind;
			HitFlashUntil = Now + FMath::Max(HydraCharacterLook::HitFlashS, HitStopS);
		}
	}
}

void AHydraCharacter::ClientSetFacing_Implementation(float Yaw)
{
	SetActorRotation(FRotator(0.f, Yaw, 0.f));
	if (AController* C = GetController())
	{
		C->SetControlRotation(FRotator(-10.f, Yaw, 0.f));
	}
}
