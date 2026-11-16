#include "Core/HydraPlayerController.h"

#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Character/HydraCharacter.h"
#include "Character/HydraMovementComponent.h"
#include "Core/HydraDevSubsystem.h"
#include "Core/HydraGameState.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerState.h"
#include "Hydra.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Misc/ConfigCacheIni.h"

namespace HydraInput
{
	UInputModifier* Negate(UObject* Outer, bool bX, bool bY)
	{
		UInputModifierNegate* M = NewObject<UInputModifierNegate>(Outer);
		M->bX = bX;
		M->bY = bY;
		M->bZ = false;
		return M;
	}

	UInputModifier* SwizzleYX(UObject* Outer)
	{
		UInputModifierSwizzleAxis* M = NewObject<UInputModifierSwizzleAxis>(Outer);
		M->Order = EInputAxisSwizzle::YXZ;
		return M;
	}

	UInputModifier* DeadZone(UObject* Outer)
	{
		UInputModifierDeadZone* M = NewObject<UInputModifierDeadZone>(Outer);
		M->LowerThreshold = 0.2f;
		M->Type = EDeadZoneType::Radial;
		return M;
	}
}

AHydraCharacter* AHydraPlayerController::GetHydraCharacter() const
{
	return Cast<AHydraCharacter>(GetPawn());
}

UInputAction* AHydraPlayerController::MakeAction(FName Name, EInputActionValueType Type)
{
	UInputAction* Action = NewObject<UInputAction>(this, Name);
	Action->ValueType = Type;
	return Action;
}

void AHydraPlayerController::BuildInput()
{
	using namespace HydraInput;

	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookMouseAction = MakeAction(TEXT("IA_LookMouse"), EInputActionValueType::Axis2D);
	LookStickAction = MakeAction(TEXT("IA_LookStick"), EInputActionValueType::Axis2D);
	WalkAction = MakeAction(TEXT("IA_Walk"), EInputActionValueType::Boolean);
	LockOnAction = MakeAction(TEXT("IA_LockOn"), EInputActionValueType::Boolean);
	ReloadValuesAction = MakeAction(TEXT("IA_ReloadValues"), EInputActionValueType::Boolean);
	LatencyStepAction = MakeAction(TEXT("IA_LatencyStep"), EInputActionValueType::Boolean);
	PacketLossAction = MakeAction(TEXT("IA_PacketLoss"), EInputActionValueType::Boolean);

	UInputMappingContext* Ctx = NewObject<UInputMappingContext>(this, TEXT("IMC_Hydra"));
	MappingContext = Ctx;

	// Move: X = right, Y = forward.
	Ctx->MapKey(MoveAction, EKeys::W).Modifiers.Add(SwizzleYX(Ctx));
	{
		FEnhancedActionKeyMapping& S = Ctx->MapKey(MoveAction, EKeys::S);
		S.Modifiers.Add(SwizzleYX(Ctx));
		S.Modifiers.Add(Negate(Ctx, true, true));
	}
	Ctx->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate(Ctx, true, true));
	Ctx->MapKey(MoveAction, EKeys::D);
	Ctx->MapKey(MoveAction, EKeys::Gamepad_Left2D).Modifiers.Add(DeadZone(Ctx));

	// Look: mouse up / stick up = look up.
	Ctx->MapKey(LookMouseAction, EKeys::Mouse2D).Modifiers.Add(Negate(Ctx, false, true));
	{
		FEnhancedActionKeyMapping& Stick = Ctx->MapKey(LookStickAction, EKeys::Gamepad_Right2D);
		Stick.Modifiers.Add(DeadZone(Ctx));
		Stick.Modifiers.Add(Negate(Ctx, false, true));
	}

	Ctx->MapKey(WalkAction, EKeys::LeftAlt);
	Ctx->MapKey(LockOnAction, EKeys::Q);
	Ctx->MapKey(LockOnAction, EKeys::MiddleMouseButton);
	Ctx->MapKey(LockOnAction, EKeys::Gamepad_RightThumbstick);
	Ctx->MapKey(ReloadValuesAction, EKeys::F5);
	Ctx->MapKey(LatencyStepAction, EKeys::F6);
	Ctx->MapKey(PacketLossAction, EKeys::F8);
}

void AHydraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BuildInput();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent);
	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHydraPlayerController::OnMove);
	EIC->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &AHydraPlayerController::OnLookMouse);
	EIC->BindAction(LookStickAction, ETriggerEvent::Triggered, this, &AHydraPlayerController::OnLookStick);
	EIC->BindAction(WalkAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnWalkPressed);
	EIC->BindAction(WalkAction, ETriggerEvent::Completed, this, &AHydraPlayerController::OnWalkReleased);
	EIC->BindAction(LockOnAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnLockOn);
	EIC->BindAction(ReloadValuesAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnReloadValues);
	EIC->BindAction(LatencyStepAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnLatencyStep);
	EIC->BindAction(PacketLossAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPacketLossToggle);
}

void AHydraPlayerController::OnMove(const FInputActionValue& Value)
{
	APawn* P = GetPawn();
	if (!P)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	P->AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Axis.Y);
	P->AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Axis.X);
}

void AHydraPlayerController::OnLookMouse(const FInputActionValue& Value)
{
	const AHydraCharacter* C = GetHydraCharacter();
	if (C && C->GetLockOnTarget())
	{
		return; // lock-on drives the camera
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	AddYawInput(Axis.X);
	AddPitchInput(Axis.Y);
}

void AHydraPlayerController::OnLookStick(const FInputActionValue& Value)
{
	const AHydraCharacter* C = GetHydraCharacter();
	if (C && C->GetLockOnTarget())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Dt = GetWorld()->GetDeltaSeconds();
	AddYawInput(Axis.X * StickLookRateDegS * Dt);
	AddPitchInput(Axis.Y * StickLookRateDegS * Dt);
}

void AHydraPlayerController::OnWalkPressed(const FInputActionValue&)
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->GetHydraMovement()->bWantsWalk = true;
	}
}

void AHydraPlayerController::OnWalkReleased(const FInputActionValue&)
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->GetHydraMovement()->bWantsWalk = false;
	}
}

void AHydraPlayerController::OnLockOn(const FInputActionValue&)
{
	AHydraCharacter* C = GetHydraCharacter();
	if (!C)
	{
		return;
	}
	if (C->GetLockOnTarget())
	{
		C->SetLockOnTarget(nullptr);
		return;
	}
	// Target: the best character inside the lock-on cone around the camera direction.
	const FHydraLockOnValues& L = UHydraValuesSubsystem::Get(this).lock_on;
	FVector CamLoc;
	FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	C->SetLockOnTarget(C->FindBestTarget(C->GetActorLocation(), CamRot.Vector(), L.range_m * 100.f, L.max_angle_deg));
}

void AHydraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// Frame cap from Config/DefaultGame.ini ([Hydra] MaxFPS), see the comment there. Set here and
	// not as a cvar in an ini, because the engine's game user settings overwrite t.MaxFPS at start.
	float MaxFPS = 0.f;
	if (IsLocalController() && GEngine && GConfig->GetFloat(TEXT("Hydra"), TEXT("MaxFPS"), MaxFPS, GGameIni) && MaxFPS > 0.f)
	{
		GEngine->SetMaxFPS(MaxFPS);
	}
}

void AHydraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (ScriptedMoveRemaining > 0.f)
	{
		ScriptedMoveRemaining -= DeltaTime;
		OnMove(FInputActionValue(ScriptedMoveInput));
	}
	UpdateLockOn(DeltaTime);
}

void AHydraPlayerController::UpdateLockOn(float DeltaTime)
{
	AHydraCharacter* C = GetHydraCharacter();
	AHydraCharacter* Target = C ? C->GetLockOnTarget() : nullptr;
	if (!C || !C->GetLockOnTarget())
	{
		if (C && C->GetHydraMovement()->bWantsStrafe)
		{
			C->SetLockOnTarget(nullptr); // target is gone (destroyed): release the lock
		}
		return;
	}
	// Break the lock when the target leaves lock_on.break_range_m.
	const FHydraLockOnValues& L = UHydraValuesSubsystem::Get(this).lock_on;
	if (FVector::Dist2D(C->GetActorLocation(), Target->GetActorLocation()) > L.break_range_m * 100.f)
	{
		C->SetLockOnTarget(nullptr);
		return;
	}
	// Only the camera is turned here: it looks at the target from slightly above. The
	// character follows the camera direction through the movement component (turn rate).
	const FVector From = C->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const FVector To = Target->GetActorLocation() + FVector(0.f, 0.f, 20.f);
	FRotator Desired = (To - From).Rotation();
	Desired.Pitch = FMath::Clamp(Desired.Pitch - 10.f, -40.f, 20.f);
	SetControlRotation(FMath::RInterpTo(GetControlRotation(), Desired, DeltaTime, 10.f));
}

void AHydraPlayerController::OnReloadValues(const FInputActionValue&)
{
	UHydraValuesSubsystem::ReloadEverywhere(GetWorld());
}

void AHydraPlayerController::ServerReloadValues_Implementation()
{
	UHydraValuesSubsystem::ReloadEverywhere(GetWorld());
}

void AHydraPlayerController::ClientReloadValues_Implementation()
{
	if (UHydraValuesSubsystem* Subsystem = UHydraValuesSubsystem::GetSubsystem(this))
	{
		Subsystem->Reload();
	}
}

void AHydraPlayerController::OnLatencyStep(const FInputActionValue&)
{
	AHydraGameState::RequestLatencyStep(GetWorld(), -1);
}

void AHydraPlayerController::OnPacketLossToggle(const FInputActionValue&)
{
	AHydraGameState::RequestPacketLoss(GetWorld(), -1);
}

void AHydraPlayerController::ServerSetNetSimulation_Implementation(int32 StepMs, int32 On)
{
	if (AHydraGameState* GS = GetWorld()->GetGameState<AHydraGameState>())
	{
		if (StepMs != NetSimUnchanged)
		{
			GS->SetLatencyStep(StepMs);
		}
		if (On != NetSimUnchanged)
		{
			GS->SetPacketLoss(On);
		}
	}
}

// --- Scripted runs and console -----------------------------------------------------------

void AHydraPlayerController::HydraMove(float X, float Y, float Seconds)
{
	ScriptedMoveInput = FVector2D(X, Y);
	ScriptedMoveRemaining = Seconds;
}

void AHydraPlayerController::HydraLook(float Yaw, float Pitch)
{
	SetControlRotation(FRotator(Pitch, Yaw, 0.f));
}

void AHydraPlayerController::HydraLockOn()
{
	OnLockOn(FInputActionValue());
}

void AHydraPlayerController::HydraAimAssist()
{
	AHydraCharacter* C = GetHydraCharacter();
	if (!C)
	{
		return;
	}
	const float Before = C->GetActorRotation().Yaw;
	const AHydraCharacter* Target = C->ApplyAimAssist();
	const float ToTarget = Target ? (Target->GetActorLocation() - C->GetActorLocation()).Rotation().Yaw : 0.f;
	UE_LOG(LogHydra, Log, TEXT("HydraAimAssist target=%s yaw_before=%.1f yaw_to_target=%.1f yaw_after=%.1f strength=%.2f"),
		Target ? TEXT("yes") : TEXT("none"), Before, ToTarget, C->GetActorRotation().Yaw, UHydraValuesSubsystem::Get(this).aim_assist.strength);
	if (!HasAuthority())
	{
		ServerDevAimAssist();
	}
}

void AHydraPlayerController::ServerDevAimAssist_Implementation()
{
#if !UE_BUILD_SHIPPING
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->ApplyAimAssist();
	}
#endif
}

void AHydraPlayerController::HydraSpendStamina(float Amount)
{
	ServerDevSpendStamina(Amount);
}

void AHydraPlayerController::ServerDevSpendStamina_Implementation(float Amount)
{
#if !UE_BUILD_SHIPPING
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		UAbilitySystemComponent* ASC = C->GetAbilitySystemComponent();
		const bool bCanAfford = HydraGas::CanAfford(ASC, Amount);
		if (bCanAfford)
		{
			HydraGas::SpendStamina(ASC, Amount, UHydraValuesSubsystem::Get(this).stamina);
		}
		UE_LOG(LogHydra, Log, TEXT("HydraSpendStamina amount=%.1f %s stamina=%.1f"), Amount, bCanAfford ? TEXT("spent") : TEXT("refused"), HydraGas::GetStamina(ASC));
	}
#endif
}

void AHydraPlayerController::HydraStats()
{
	const UHydraDevSubsystem* Dev = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHydraDevSubsystem>() : nullptr;
	const double Clock = Dev ? Dev->GetClock() : 0.0;
	const TCHAR* Who = GetNetMode() == NM_Client ? TEXT("client") : TEXT("host");
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		const AHydraCharacter* C = *It;
		const bool bMe = C == GetPawn();
		const APlayerState* PS = C->GetPlayerState();
		const FString Id = C->IsTrainingDummy() ? FString(TEXT("dummy")) : (PS ? FString::FromInt(PS->GetPlayerId()) : FString(TEXT("unknown")));

		FString Lock = TEXT("none");
		float LockOff = 0.f;
		if (const AHydraCharacter* Target = C->GetLockOnTarget())
		{
			const APlayerState* TargetPS = Target->GetPlayerState();
			Lock = Target->IsTrainingDummy() ? FString(TEXT("dummy")) : (TargetPS ? FString::FromInt(TargetPS->GetPlayerId()) : FString(TEXT("unknown")));
			const float ToTarget = (Target->GetActorLocation() - C->GetActorLocation()).Rotation().Yaw;
			LockOff = FMath::Abs(FMath::FindDeltaAngleDegrees(GetControlRotation().Yaw, ToTarget));
		}
		UE_LOG(LogHydra, Log, TEXT("HydraStats t=%.2f who=%s id=%s me=%d stamina=%.1f max=%.1f x=%.0f y=%.0f yaw=%.1f ctrl=%.1f lock=%s lockoff=%.1f ping=%.0f regenblocked=%d speed=%.0f"),
			Clock, Who, *Id, bMe ? 1 : 0, C->GetStamina(), C->GetMaxStamina(), C->GetActorLocation().X, C->GetActorLocation().Y,
			C->GetActorRotation().Yaw, bMe ? GetControlRotation().Yaw : 0.f, *Lock, LockOff, PS ? PS->GetPingInMilliseconds() : 0.f,
			C->GetAbilitySystemComponent()->HasMatchingGameplayTag(HydraTags::State_StaminaRegenBlocked) ? 1 : 0, C->GetVelocity().Size2D());
	}
}
