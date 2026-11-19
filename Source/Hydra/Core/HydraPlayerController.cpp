#include "Core/HydraPlayerController.h"

#include "AI/HydraAIController.h"
#include "Arena/HydraArena.h"
#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Character/HydraCharacter.h"
#include "Character/HydraMovementComponent.h"
#include "Character/HydraProgressionComponent.h"
#include "Core/HydraDevSubsystem.h"
#include "Core/HydraGameMode.h"
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
	AttackAction = MakeAction(TEXT("IA_WeaponHand"), EInputActionValueType::Boolean);
	BlockAction = MakeAction(TEXT("IA_ShieldHand"), EInputActionValueType::Boolean);
	DodgeAction = MakeAction(TEXT("IA_Dodge"), EInputActionValueType::Boolean);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	LockOnAction = MakeAction(TEXT("IA_LockOn"), EInputActionValueType::Boolean);
	PreciseAction = MakeAction(TEXT("IA_Precise"), EInputActionValueType::Boolean);
	EvolveAction = MakeAction(TEXT("IA_Evolve"), EInputActionValueType::Boolean);
	ImprintAction = MakeAction(TEXT("IA_Imprint"), EInputActionValueType::Boolean);
	PanelToggleAction = MakeAction(TEXT("IA_PanelToggle"), EInputActionValueType::Boolean);
	PanelUpAction = MakeAction(TEXT("IA_PanelUp"), EInputActionValueType::Boolean);
	PanelDownAction = MakeAction(TEXT("IA_PanelDown"), EInputActionValueType::Boolean);
	PanelLeftAction = MakeAction(TEXT("IA_PanelLeft"), EInputActionValueType::Boolean);
	PanelRightAction = MakeAction(TEXT("IA_PanelRight"), EInputActionValueType::Boolean);
	PanelApplyAction = MakeAction(TEXT("IA_PanelApply"), EInputActionValueType::Boolean);
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

	Ctx->MapKey(WalkAction, EKeys::LeftControl);
	Ctx->MapKey(AttackAction, EKeys::LeftMouseButton);
	Ctx->MapKey(AttackAction, EKeys::Gamepad_RightShoulder);
	Ctx->MapKey(BlockAction, EKeys::RightMouseButton);
	Ctx->MapKey(BlockAction, EKeys::Gamepad_LeftShoulder);
	Ctx->MapKey(DodgeAction, EKeys::LeftAlt);
	Ctx->MapKey(JumpAction, EKeys::SpaceBar);
	Ctx->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	Ctx->MapKey(DodgeAction, EKeys::Gamepad_FaceButton_Right);
	Ctx->MapKey(LockOnAction, EKeys::Q);
	Ctx->MapKey(LockOnAction, EKeys::MiddleMouseButton);
	Ctx->MapKey(LockOnAction, EKeys::Gamepad_RightThumbstick);
	Ctx->MapKey(PreciseAction, EKeys::X);
	Ctx->MapKey(PreciseAction, EKeys::Gamepad_LeftThumbstick);
	Ctx->MapKey(EvolveAction, EKeys::J);
	Ctx->MapKey(EvolveAction, EKeys::Gamepad_DPad_Up);
	Ctx->MapKey(ImprintAction, EKeys::K);
	Ctx->MapKey(ImprintAction, EKeys::Gamepad_DPad_Right);
	// Debug panel: its keys only act while it is open (and the game keys only while it is closed).
	Ctx->MapKey(PanelToggleAction, EKeys::F1);
	Ctx->MapKey(PanelToggleAction, EKeys::Gamepad_Special_Left);
	Ctx->MapKey(PanelUpAction, EKeys::Up);
	Ctx->MapKey(PanelUpAction, EKeys::Gamepad_DPad_Up);
	Ctx->MapKey(PanelDownAction, EKeys::Down);
	Ctx->MapKey(PanelDownAction, EKeys::Gamepad_DPad_Down);
	Ctx->MapKey(PanelLeftAction, EKeys::Left);
	Ctx->MapKey(PanelLeftAction, EKeys::Gamepad_DPad_Left);
	Ctx->MapKey(PanelRightAction, EKeys::Right);
	Ctx->MapKey(PanelRightAction, EKeys::Gamepad_DPad_Right);
	Ctx->MapKey(PanelApplyAction, EKeys::Enter);
	Ctx->MapKey(PanelApplyAction, EKeys::Gamepad_FaceButton_Bottom);
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
	EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnAttackPressed);
	EIC->BindAction(AttackAction, ETriggerEvent::Completed, this, &AHydraPlayerController::OnAttackReleased);
	EIC->BindAction(BlockAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnBlockPressed);
	EIC->BindAction(BlockAction, ETriggerEvent::Completed, this, &AHydraPlayerController::OnBlockReleased);
	EIC->BindAction(DodgeAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnDodge);
	EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnJumpPressed);
	EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AHydraPlayerController::OnJumpReleased);
	EIC->BindAction(LockOnAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnLockOn);
	EIC->BindAction(PreciseAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPreciseToggle);
	EIC->BindAction(EvolveAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnChooseEvolve);
	EIC->BindAction(ImprintAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnChooseImprint);
	EIC->BindAction(PanelToggleAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPanelToggle);
	EIC->BindAction(PanelUpAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPanelUp);
	EIC->BindAction(PanelDownAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPanelDown);
	EIC->BindAction(PanelLeftAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPanelLeft);
	EIC->BindAction(PanelRightAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPanelRight);
	EIC->BindAction(PanelApplyAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPanelApply);
	EIC->BindAction(ReloadValuesAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnReloadValues);
	EIC->BindAction(LatencyStepAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnLatencyStep);
	EIC->BindAction(PacketLossAction, ETriggerEvent::Started, this, &AHydraPlayerController::OnPacketLossToggle);
}

void AHydraPlayerController::OnMove(const FInputActionValue& Value)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
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
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
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
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
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

void AHydraPlayerController::OnAttackPressed(const FInputActionValue&)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->InputAttackPressed();
	}
}

void AHydraPlayerController::OnAttackReleased(const FInputActionValue&)
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->InputAttackReleased();
	}
}

void AHydraPlayerController::OnBlockPressed(const FInputActionValue&)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
	// The off-hand button: block with a shield, cast for the slime. A cast is aimed where the camera looks.
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->InputOffHand(true, GetControlRotation().Yaw);
	}
}

void AHydraPlayerController::OnBlockReleased(const FInputActionValue&)
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->InputOffHand(false, GetControlRotation().Yaw);
	}
}

void AHydraPlayerController::OnDodge(const FInputActionValue&)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->InputDodge();
	}
}

void AHydraPlayerController::OnJumpPressed(const FInputActionValue&)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->InputJump(true);
	}
}

void AHydraPlayerController::OnJumpReleased(const FInputActionValue&)
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->InputJump(false);
	}
}

void AHydraPlayerController::OnLockOn(const FInputActionValue&)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
	AHydraCharacter* C = GetHydraCharacter();
	if (!C || !C->IsAlive())
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

void AHydraPlayerController::OnPreciseToggle(const FInputActionValue&)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->SetPreciseMovement(!C->IsPreciseMovement());
	}
}

void AHydraPlayerController::OnChooseEvolve(const FInputActionValue&)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
	ServerChooseMilestone(TEXT("evolve"));
}

void AHydraPlayerController::OnChooseImprint(const FInputActionValue&)
{
	if (bDebugPanelOpen)
	{
		return; // the debug panel has the input
	}
	ServerChooseMilestone(TEXT("imprint"));
}

void AHydraPlayerController::HydraChoose(const FString& Choice)
{
	ServerChooseMilestone(FName(*Choice));
}

void AHydraPlayerController::ServerChooseMilestone_Implementation(FName Choice)
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		const bool bTaken = C->GetProgression()->ServerChoose(Choice);
		UE_LOG(LogHydra, Log, TEXT("HydraChoose who=%s choice=%s %s"), *C->GetLogId(), *Choice.ToString(), bTaken ? TEXT("taken") : TEXT("refused"));
	}
}

// --- Test tools: console commands and the debug panel --------------------------------------

void AHydraPlayerController::HydraSetForm(const FString& FormId)
{
	ServerDevCommand(FString::Printf(TEXT("form %s"), *FormId));
}

void AHydraPlayerController::HydraSetLevel(int32 Level)
{
	ServerDevCommand(FString::Printf(TEXT("setlevel %d"), Level));
}

void AHydraPlayerController::HydraSetSkill(const FString& SkillId, int32 Level, float Progress)
{
	ServerDevCommand(FString::Printf(TEXT("setskill %s %d %f"), *SkillId, Level, Progress));
}

void AHydraPlayerController::HydraSetImprint(int32 Level)
{
	ServerDevCommand(FString::Printf(TEXT("setimprint %d"), Level));
}

void AHydraPlayerController::HydraMilestone()
{
	ServerDevCommand(TEXT("milestone"));
}

void AHydraPlayerController::HydraReset()
{
	ServerDevCommand(TEXT("resetself"));
}

void AHydraPlayerController::HydraSpawn(const FString& EnemyId, int32 Level, int32 Passive)
{
	ServerDevCommand(FString::Printf(TEXT("spawn %s %d %d"), *EnemyId, Level, Passive));
}

void AHydraPlayerController::HydraClearEnemies()
{
	ServerDevCommand(TEXT("clearenemies"));
}

void AHydraPlayerController::HydraPanel(int32 Open)
{
	SetDebugPanelOpen(Open != 0);
}

void AHydraPlayerController::HydraPanelKey(const FString& Key)
{
	if (!bDebugPanelOpen)
	{
		return;
	}
	if (Key == TEXT("up")) { PanelMove(-1); }
	else if (Key == TEXT("down")) { PanelMove(1); }
	else if (Key == TEXT("left")) { PanelAdjust(PanelSelection, -1); }
	else if (Key == TEXT("right")) { PanelAdjust(PanelSelection, 1); }
	else if (Key == TEXT("enter")) { PanelActivate(PanelSelection); }
}

void AHydraPlayerController::OnPanelToggle(const FInputActionValue&) { SetDebugPanelOpen(!bDebugPanelOpen); }
void AHydraPlayerController::OnPanelUp(const FInputActionValue&) { if (bDebugPanelOpen) { PanelMove(-1); } }
void AHydraPlayerController::OnPanelDown(const FInputActionValue&) { if (bDebugPanelOpen) { PanelMove(1); } }
void AHydraPlayerController::OnPanelLeft(const FInputActionValue&) { if (bDebugPanelOpen) { PanelAdjust(PanelSelection, -1); } }
void AHydraPlayerController::OnPanelRight(const FInputActionValue&) { if (bDebugPanelOpen) { PanelAdjust(PanelSelection, 1); } }
void AHydraPlayerController::OnPanelApply(const FInputActionValue&) { if (bDebugPanelOpen) { PanelActivate(PanelSelection); } }

void AHydraPlayerController::SetDebugPanelOpen(bool bOpen)
{
#if UE_BUILD_SHIPPING
	bOpen = false; // a test tool: not in shipping builds
#endif
	if (bOpen == bDebugPanelOpen)
	{
		return;
	}
	bDebugPanelOpen = bOpen;
	// While it is open the game input is paused for the own character only; the game runs on.
	if (bOpen)
	{
		OnWalkReleased(FInputActionValue());
		if (AHydraCharacter* C = GetHydraCharacter())
		{
			C->InputAttackReleased();
			C->InputOffHand(false, GetControlRotation().Yaw);
		}
	}
	bShowMouseCursor = bOpen;
	bEnableClickEvents = bOpen;
	if (IsLocalController())
	{
		if (bOpen)
		{
			SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
		}
		else
		{
			SetInputMode(FInputModeGameOnly());
		}
	}
	UE_LOG(LogHydra, Log, TEXT("HydraPanel %s"), bOpen ? TEXT("open") : TEXT("closed"));
}

TArray<AHydraPlayerController::FPanelItem> AHydraPlayerController::BuildPanelItems() const
{
	TArray<FPanelItem> Items;
	Items.Add({ EPanelItem::Form, NAME_None });
	Items.Add({ EPanelItem::Level, NAME_None });
	if (const AHydraCharacter* C = GetHydraCharacter())
	{
		// The skills of the form (and the element skill once it is awake); new forms and skills appear by themselves.
		const UHydraValuesSubsystem* Values = UHydraValuesSubsystem::GetSubsystem(this);
		const FHydraFormDef* Form = Values ? Values->GetDefinitions().FindForm(C->GetFormId()) : nullptr;
		for (const FHydraSkillState& S : C->GetProgression()->GetSkills())
		{
			if ((Form && Form->skills.Contains(S.Id)) || S.Level > 0)
			{
				Items.Add({ EPanelItem::Skill, S.Id });
			}
		}
	}
	for (const EPanelItem Kind : { EPanelItem::Imprint, EPanelItem::Milestone, EPanelItem::Reset, EPanelItem::EnemyType, EPanelItem::EnemyLevel,
		EPanelItem::EnemyPassive, EPanelItem::Spawn, EPanelItem::Clear, EPanelItem::Waves })
	{
		Items.Add({ Kind, NAME_None });
	}
	return Items;
}

TArray<AHydraPlayerController::FPanelRow> AHydraPlayerController::GetDebugPanelRows() const
{
	TArray<FPanelRow> Rows;
	const AHydraCharacter* C = GetHydraCharacter();
	const UHydraValuesSubsystem* Values = UHydraValuesSubsystem::GetSubsystem(this);
	if (!C || !Values)
	{
		return Rows;
	}
	const UHydraProgressionComponent* P = C->GetProgression();
	const AHydraGameState* GS = GetWorld()->GetGameState<AHydraGameState>();
	const TArray<FHydraEnemyDef>& Enemies = Values->GetDefinitions().enemies.enemies;
	const FName EnemyId = PanelEnemyId.IsNone() && Enemies.Num() > 0 ? Enemies[0].id : PanelEnemyId;
	for (const FPanelItem& Item : BuildPanelItems())
	{
		FPanelRow Row;
		switch (Item.Kind)
		{
		case EPanelItem::Form: Row = { TEXT("Form"), C->GetFormId().ToString(), true }; break;
		case EPanelItem::Level: Row = { TEXT("Level"), FString::Printf(TEXT("%d / %d"), P->GetLevel(), C->GetStats().LevelCap), true }; break;
		case EPanelItem::Skill:
		{
			const FHydraSkillDef* Def = Values->GetDefinitions().FindSkill(Item.Name);
			const FHydraSkillState* S = P->FindSkill(Item.Name);
			Row = { FString::Printf(TEXT("Skill: %s"), Def ? *Def->name : *Item.Name.ToString()), S ? FString::Printf(TEXT("%d  (bar %.0f %%)"), S->Level, S->Progress * 100.f) : FString(TEXT("-")), true };
			break;
		}
		case EPanelItem::Imprint: Row = { TEXT("Fire imprint"), FString::FromInt(P->GetImprintLevel()), true }; break;
		case EPanelItem::Milestone: Row = { TEXT("Open a milestone"), FString::Printf(TEXT("open: %d"), P->GetOpenMilestones()), false }; break;
		case EPanelItem::Reset: Row = { TEXT("Reset: full bars, debuffs removed"), FString(), false }; break;
		case EPanelItem::EnemyType: Row = { TEXT("Enemy type"), EnemyId.ToString(), true }; break;
		case EPanelItem::EnemyLevel: Row = { TEXT("Enemy level"), FString::FromInt(PanelEnemyLevel), true }; break;
		case EPanelItem::EnemyPassive: Row = { TEXT("Enemy is"), bPanelEnemyPassive ? TEXT("passive") : TEXT("active"), true }; break;
		case EPanelItem::Spawn: Row = { TEXT("Spawn this enemy in front of me"), FString(), false }; break;
		case EPanelItem::Clear: Row = { TEXT("Remove all enemies"), FString(), false }; break;
		case EPanelItem::Waves: Row = { TEXT("Waves"), GS && GS->IsWaveMode() ? TEXT("on") : TEXT("off"), true }; break;
		}
		Rows.Add(Row);
	}
	return Rows;
}

void AHydraPlayerController::PanelMove(int32 Direction)
{
	const int32 Num = BuildPanelItems().Num();
	PanelSelection = Num > 0 ? (PanelSelection + Direction + Num) % Num : 0;
}

void AHydraPlayerController::DebugPanelClick(int32 Row, int32 Direction)
{
	if (!bDebugPanelOpen || !BuildPanelItems().IsValidIndex(Row))
	{
		return;
	}
	PanelSelection = Row;
	Direction == 0 ? PanelActivate(Row) : PanelAdjust(Row, Direction);
}

void AHydraPlayerController::PanelAdjust(int32 Row, int32 Direction)
{
	// The panel only sends requests; the server applies them (the same commands as the console).
	const TArray<FPanelItem> Items = BuildPanelItems();
	const AHydraCharacter* C = GetHydraCharacter();
	const UHydraValuesSubsystem* Values = UHydraValuesSubsystem::GetSubsystem(this);
	if (!Items.IsValidIndex(Row) || !C || !Values || Direction == 0)
	{
		return;
	}
	const UHydraProgressionComponent* P = C->GetProgression();
	const FPanelItem& Item = Items[Row];
	switch (Item.Kind)
	{
	case EPanelItem::Form:
	{
		const TArray<FHydraFormDef>& Forms = Values->GetDefinitions().forms.forms;
		const int32 Index = Forms.IndexOfByPredicate([C](const FHydraFormDef& F) { return F.id == C->GetFormId(); });
		if (Forms.Num() > 0)
		{
			HydraSetForm(Forms[(FMath::Max(0, Index) + Direction + Forms.Num()) % Forms.Num()].id.ToString());
		}
		break;
	}
	case EPanelItem::Level: HydraSetLevel(P->GetLevel() + Direction); break;
	case EPanelItem::Skill:
		if (const FHydraSkillState* S = P->FindSkill(Item.Name))
		{
			HydraSetSkill(Item.Name.ToString(), S->Level + Direction, S->Progress);
		}
		break;
	case EPanelItem::Imprint: HydraSetImprint(P->GetImprintLevel() + Direction); break;
	case EPanelItem::EnemyType:
	{
		const TArray<FHydraEnemyDef>& Enemies = Values->GetDefinitions().enemies.enemies;
		const int32 Index = Enemies.IndexOfByPredicate([this](const FHydraEnemyDef& E) { return E.id == PanelEnemyId; });
		if (Enemies.Num() > 0)
		{
			PanelEnemyId = Enemies[(FMath::Max(0, Index) + Direction + Enemies.Num()) % Enemies.Num()].id;
		}
		break;
	}
	case EPanelItem::EnemyLevel: PanelEnemyLevel = FMath::Max(0, PanelEnemyLevel + Direction); break;
	case EPanelItem::EnemyPassive: bPanelEnemyPassive = !bPanelEnemyPassive; break;
	case EPanelItem::Waves:
	{
		const AHydraGameState* GS = GetWorld()->GetGameState<AHydraGameState>();
		HydraWaves(GS && GS->IsWaveMode() ? 0 : 1);
		break;
	}
	default: break;
	}
}

void AHydraPlayerController::PanelActivate(int32 Row)
{
	const TArray<FPanelItem> Items = BuildPanelItems();
	const UHydraValuesSubsystem* Values = UHydraValuesSubsystem::GetSubsystem(this);
	if (!Items.IsValidIndex(Row) || !Values)
	{
		return;
	}
	switch (Items[Row].Kind)
	{
	case EPanelItem::Milestone: HydraMilestone(); break;
	case EPanelItem::Reset: HydraReset(); break;
	case EPanelItem::Spawn:
	{
		const TArray<FHydraEnemyDef>& Enemies = Values->GetDefinitions().enemies.enemies;
		const FName EnemyId = PanelEnemyId.IsNone() && Enemies.Num() > 0 ? Enemies[0].id : PanelEnemyId;
		HydraSpawn(EnemyId.ToString(), PanelEnemyLevel, bPanelEnemyPassive ? 1 : 0);
		break;
	}
	case EPanelItem::Clear: HydraClearEnemies(); break;
	default: PanelAdjust(Row, 1); break;   // Enter on a value steps it forward
	}
}

void AHydraPlayerController::HydraWaves(int32 On)
{
	ServerDevCommand(FString::Printf(TEXT("waves %d"), On));
}

void AHydraPlayerController::HydraXp(float Amount)
{
	ServerDevCommand(FString::Printf(TEXT("xp %f"), Amount));
}

void AHydraPlayerController::HydraImprintProgress(float Amount)
{
	ServerDevCommand(FString::Printf(TEXT("imprintprogress %f"), Amount));
}

void AHydraPlayerController::HydraEnemyTeleport(const FString& Who, float X, float Y, float Yaw)
{
	ServerDevCommand(FString::Printf(TEXT("enemyteleport %s %f %f %f"), *Who, X, Y, Yaw));
}

void AHydraPlayerController::HydraPrecise(int32 On)
{
	if (AHydraCharacter* C = GetHydraCharacter())
	{
		C->SetPreciseMovement(On != 0);
	}
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
	if (ScriptedAttackReleaseAt > 0.0 && GetWorld()->GetTimeSeconds() >= ScriptedAttackReleaseAt)
	{
		ScriptedAttackReleaseAt = -1.0;
		OnAttackReleased(FInputActionValue());
	}
	UpdateAutoAttack();
	UpdateAutoBlock();
	UpdateLockOn(DeltaTime);
}

void AHydraPlayerController::UpdateLockOn(float DeltaTime)
{
	AHydraCharacter* C = GetHydraCharacter();
	AHydraCharacter* Target = C ? C->GetLockOnTarget() : nullptr;
	if (!Target)
	{
		if (C)
		{
			C->RefreshStrafe(); // a target that is gone (destroyed) releases the lock
		}
		return;
	}
	// Break the lock when the target dies or leaves lock_on.break_range_m.
	const FHydraLockOnValues& L = UHydraValuesSubsystem::Get(this).lock_on;
	if (!Target->IsAlive() || FVector::Dist2D(C->GetActorLocation(), Target->GetActorLocation()) > L.break_range_m * 100.f)
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

// --- Scripted runs and console: input ----------------------------------------------------

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

void AHydraPlayerController::HydraAttack(float HoldMs)
{
	OnAttackPressed(FInputActionValue());
	if (HoldMs <= 0.f)
	{
		OnAttackReleased(FInputActionValue());
	}
	else
	{
		ScriptedAttackReleaseAt = GetWorld()->GetTimeSeconds() + HoldMs / 1000.f;
	}
}

void AHydraPlayerController::HydraBlock(int32 Hold)
{
	Hold ? OnBlockPressed(FInputActionValue()) : OnBlockReleased(FInputActionValue());
}

void AHydraPlayerController::HydraOffHand(float HoldMs)
{
	OnBlockPressed(FInputActionValue());
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { OnBlockReleased(FInputActionValue()); }), FMath::Max(0.02f, HoldMs / 1000.f), false);
}

void AHydraPlayerController::HydraForm(const FString& FormId)
{
	ServerDevCommand(FString::Printf(TEXT("form %s"), *FormId));
}

void AHydraPlayerController::HydraFire(int32 On)
{
	ServerDevCommand(FString::Printf(TEXT("fire %d"), On));
}

void AHydraPlayerController::HydraDodge()
{
	OnDodge(FInputActionValue());
}

void AHydraPlayerController::HydraJump()
{
	OnJumpPressed(FInputActionValue());
	// Released on the next press; a scripted jump is a tap.
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { OnJumpReleased(FInputActionValue()); }), 0.1f, false);
}

void AHydraPlayerController::HydraAutoAttack(float StrikeDistanceCm)
{
	AutoAttackDistanceCm = StrikeDistanceCm;
}

void AHydraPlayerController::UpdateAutoAttack()
{
	const AHydraCharacter* Self = GetHydraCharacter();
	if (AutoAttackDistanceCm <= 0.f || !Self || !Self->GetStats().bHasWeapon)
	{
		return;
	}
	// Nearest other character as this window sees it right now.
	const AHydraCharacter* Nearest = nullptr;
	float NearestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		const float Dist = FVector::Dist2D(It->GetActorLocation(), Self->GetActorLocation());
		if (*It != Self && It->IsAlive() && Dist < NearestDist)
		{
			NearestDist = Dist;
			Nearest = *It;
		}
	}
	if (!Nearest)
	{
		return;
	}
	// Fire when the distance seen here will be the wanted one at the moment the strike begins.
	const FVector Direction = (Nearest->GetActorLocation() - Self->GetActorLocation()).GetSafeNormal2D();
	const float RadialSpeed = FVector::DotProduct(Nearest->GetVelocity() - Self->GetVelocity(), Direction);
	const float WindupS = Self->GetStats().Weapon.light.windup_ms / 1000.f;
	if (RadialSpeed > 50.f && NearestDist + RadialSpeed * WindupS >= AutoAttackDistanceCm)
	{
		UE_LOG(LogHydra, Log, TEXT("HydraAutoAttack fired seen_dist=%.0f radial_speed=%.0f strike_dist=%.0f"), NearestDist, RadialSpeed, AutoAttackDistanceCm);
		AutoAttackDistanceCm = 0.f;
		HydraAttack(0.f);
	}
}

void AHydraPlayerController::HydraAutoBlock(float DelayMs)
{
	AutoBlockDelayS = DelayMs / 1000.f;
}

void AHydraPlayerController::UpdateAutoBlock()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (AutoBlockPressAt > 0.0 && Now >= AutoBlockPressAt)
	{
		AutoBlockPressAt = -1.0;
		AutoBlockReleaseAt = Now + 0.6;
		UE_LOG(LogHydra, Log, TEXT("HydraAutoBlock pressed"));
		OnBlockPressed(FInputActionValue());
	}
	if (AutoBlockReleaseAt > 0.0 && Now >= AutoBlockReleaseAt)
	{
		AutoBlockReleaseAt = -1.0;
		OnBlockReleased(FInputActionValue());
	}
	if (AutoBlockDelayS < 0.f)
	{
		return;
	}
	// Armed: the first wind-up of another character seen in this window triggers it once.
	const AHydraCharacter* Self = GetHydraCharacter();
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		if (*It != Self && It->GetShownAction().Action == EHydraAction::LightWindup)
		{
			AutoBlockPressAt = Now + AutoBlockDelayS;
			AutoBlockDelayS = -1.f;
			break;
		}
	}
}

void AHydraPlayerController::HydraStats()
{
	const UHydraDevSubsystem* Dev = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHydraDevSubsystem>() : nullptr;
	const double Clock = Dev ? Dev->GetClock() : 0.0;
	const TCHAR* Who = GetNetMode() == NM_Client ? TEXT("client") : TEXT("host");
	const UEnum* ActionEnum = StaticEnum<EHydraAction>();
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		const AHydraCharacter* C = *It;
		const bool bMe = C == GetPawn();
		const APlayerState* PS = C->GetPlayerState();

		// Progress: skills as id:level:progress bar, what the milestone offers as imprint / evolve:<form>.
		const UHydraProgressionComponent* Progress = C->GetProgression();
		FString Skills;
		for (const FHydraSkillState& S : Progress->GetSkills())
		{
			Skills += FString::Printf(TEXT("%s%s:%d:%.3f"), Skills.IsEmpty() ? TEXT("") : TEXT(","), *S.Id.ToString(), S.Level, S.Progress);
		}
		FString Offer = Progress->CanRaiseImprint() ? TEXT("imprint") : TEXT("");
		if (!Progress->GetEvolutionOffer().IsNone())
		{
			Offer += FString::Printf(TEXT("%sevolve:%s"), Offer.IsEmpty() ? TEXT("") : TEXT("+"), *Progress->GetEvolutionOffer().ToString());
		}
		if (Offer.IsEmpty())
		{
			Offer = TEXT("none");
		}
		if (Skills.IsEmpty())
		{
			Skills = TEXT("none");
		}

		FString Lock = TEXT("none");
		float LockOff = 0.f;
		if (const AHydraCharacter* Target = C->GetLockOnTarget())
		{
			Lock = Target->GetLogId();
			const float ToTarget = (Target->GetActorLocation() - C->GetActorLocation()).Rotation().Yaw;
			LockOff = FMath::Abs(FMath::FindDeltaAngleDegrees(GetControlRotation().Yaw, ToTarget));
		}
		UE_LOG(LogHydra, Log, TEXT("HydraStats t=%.2f who=%s id=%s me=%d form=%s hp=%.2f maxhp=%.1f stamina=%.2f max=%.1f mana=%.2f maxmana=%.1f burning=%d fire=%d x=%.0f y=%.0f z=%.0f yaw=%.1f ctrl=%.1f lock=%s lockoff=%.1f precise=%d body=%d legs=%.2f level=%d xp=%.1f hardship=%.3f imprint=%d imprintprogress=%.1f milestones=%d offer=%s skills=%s ping=%.0f regenblocked=%d speed=%.0f action=%s chain=%d dead=%d blocking=%d"),
			Clock, Who, *C->GetLogId(), bMe ? 1 : 0, *C->GetFormId().ToString(), C->GetHealth(), C->GetMaxHealth(), C->GetStamina(), C->GetMaxStamina(),
			C->GetMana(), C->GetMaxMana(), C->GetBurningStacks(), C->HasFireImprint() ? 1 : 0,
			C->GetActorLocation().X, C->GetActorLocation().Y, C->GetActorLocation().Z, C->GetActorRotation().Yaw, bMe ? GetControlRotation().Yaw : 0.f, *Lock, LockOff, C->IsPreciseMovement() ? 1 : 0, C->GetBodyPreset(), C->GetLegScale(),
			Progress->GetLevel(), Progress->GetXp(), Progress->GetHardship(), Progress->GetImprintLevel(), Progress->GetImprintProgress(), Progress->GetOpenMilestones(), *Offer, *Skills,
			PS ? PS->GetPingInMilliseconds() : 0.f, C->HasStateTag(HydraTags::State_StaminaRegenBlocked) ? 1 : 0, C->GetVelocity().Size2D(),
			*ActionEnum->GetNameStringByValue(static_cast<int64>(C->GetShownAction().Action)), C->GetChainCount(), C->IsAlive() ? 0 : 1,
			C->GetShownAction().Action == EHydraAction::Block ? 1 : 0);
	}
}

// --- Scripted runs and console: game state -----------------------------------------------

void AHydraPlayerController::HydraSpendStamina(float Amount)
{
	ServerDevCommand(FString::Printf(TEXT("spend %f"), Amount));
}

void AHydraPlayerController::HydraTeleport(float X, float Y, float Yaw)
{
	ServerDevCommand(FString::Printf(TEXT("teleport %f %f %f"), X, Y, Yaw));
}

void AHydraPlayerController::HydraAITeleport(float X, float Y, float Yaw)
{
	ServerDevCommand(FString::Printf(TEXT("aiteleport %f %f %f"), X, Y, Yaw));
}

void AHydraPlayerController::HydraAI(const FString& Verb, float Value)
{
	ServerDevCommand(FString::Printf(TEXT("ai %s %f"), *Verb, Value));
}

void AHydraPlayerController::HydraLookVariant(int32 Look)
{
	// Looks only and per window: nothing is sent to the server.
	for (TActorIterator<AHydraArena> It(GetWorld()); It; ++It)
	{
		It->ApplyLook(Look > 0 ? Look : It->GetLook() % AHydraArena::NumLooks + 1);
	}
}

void AHydraPlayerController::HydraBody(const FString& Preset)
{
	ServerDevCommand(FString::Printf(TEXT("body %s"), *Preset));
}

void AHydraPlayerController::HydraLegs(float Scale)
{
	ServerDevCommand(FString::Printf(TEXT("legs %f"), Scale));
}

void AHydraPlayerController::HydraRefill()
{
	ServerDevCommand(TEXT("refill"));
}

void AHydraPlayerController::HydraResetAll()
{
	ServerDevCommand(TEXT("reset"));
}

void AHydraPlayerController::ServerDevCommand_Implementation(const FString& Command)
{
#if !UE_BUILD_SHIPPING
	TArray<FString> Args;
	Command.ParseIntoArrayWS(Args);
	if (Args.Num() == 0)
	{
		return;
	}
	auto Number = [&Args](int32 Index) { return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : 0.f; };
	AHydraCharacter* Self = GetHydraCharacter();
	AHydraAIController* AI = nullptr;
	for (TActorIterator<AHydraAIController> It(GetWorld()); It; ++It)
	{
		AI = *It;
		break;
	}
	AHydraCharacter* AICharacter = AI ? Cast<AHydraCharacter>(AI->GetPawn()) : nullptr;
	UE_LOG(LogHydra, Log, TEXT("HydraDevCommand from=%s '%s'"), Self ? *Self->GetLogId() : TEXT("?"), *Command);

	if (Args[0] == TEXT("spend") && Self)
	{
		UAbilitySystemComponent* ASC = Self->GetAbilitySystemComponent();
		const bool bCanAfford = HydraGas::CanAfford(ASC, Number(1));
		if (bCanAfford)
		{
			HydraGas::SpendStamina(ASC, Number(1));
		}
		UE_LOG(LogHydra, Log, TEXT("HydraSpendStamina amount=%.1f %s stamina=%.1f"), Number(1), bCanAfford ? TEXT("spent") : TEXT("refused"), HydraGas::GetStamina(ASC));
	}
	else if (Args[0] == TEXT("teleport") && Self)
	{
		Self->ServerTeleport(FVector(Number(1), Number(2), Self->GetActorLocation().Z), Number(3));
	}
	else if (Args[0] == TEXT("aiteleport") && AICharacter)
	{
		AICharacter->ServerTeleport(FVector(Number(1), Number(2), AICharacter->GetActorLocation().Z), Number(3));
	}
	else if (Args[0] == TEXT("form") && Self && Args.Num() >= 2)
	{
		Self->ServerSetForm(FName(*Args[1]));
	}
	else if (Args[0] == TEXT("fire") && Self)
	{
		Self->ServerSetFireImprint(Number(1) != 0.f);
	}
	else if (Args[0] == TEXT("waves"))
	{
		if (AHydraGameMode* GameMode = GetWorld()->GetAuthGameMode<AHydraGameMode>())
		{
			Number(1) != 0.f ? GameMode->StartWaves() : GameMode->StopWaves();
		}
	}
	else if (Args[0] == TEXT("setlevel") && Self)
	{
		Self->GetProgression()->ServerSetLevel(FMath::RoundToInt(Number(1)));
	}
	else if (Args[0] == TEXT("setskill") && Self && Args.Num() >= 3)
	{
		Self->GetProgression()->ServerSetSkill(FName(*Args[1]), FMath::RoundToInt(Number(2)), Number(3));
	}
	else if (Args[0] == TEXT("setimprint") && Self)
	{
		Self->GetProgression()->ServerSetImprintLevel(FMath::RoundToInt(Number(1)));
	}
	else if (Args[0] == TEXT("milestone") && Self)
	{
		Self->GetProgression()->ServerOpenMilestone();
	}
	else if (Args[0] == TEXT("resetself") && Self)
	{
		Self->ServerResetState();
	}
	else if (Args[0] == TEXT("spawn") && Self && Args.Num() >= 2)
	{
		if (AHydraGameMode* GameMode = GetWorld()->GetAuthGameMode<AHydraGameMode>())
		{
			GameMode->SpawnDebugEnemy(FName(*Args[1]), Args.Num() >= 3 ? FMath::RoundToInt(Number(2)) : -1, Number(3) != 0.f, Self);
		}
	}
	else if (Args[0] == TEXT("clearenemies"))
	{
		if (AHydraGameMode* GameMode = GetWorld()->GetAuthGameMode<AHydraGameMode>())
		{
			GameMode->ClearEnemies();
		}
	}
	else if (Args[0] == TEXT("xp") && Self)
	{
		Self->GetProgression()->ServerEnemyDefeated(Number(1), Number(1));
	}
	else if (Args[0] == TEXT("imprintprogress") && Self)
	{
		Self->GetProgression()->ServerAddImprintProgress(Number(1));
	}
	else if (Args[0] == TEXT("enemyteleport") && Args.Num() >= 2)
	{
		for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
		{
			if (It->GetLogId() == Args[1])
			{
				It->ServerTeleport(FVector(Number(2), Number(3), It->GetActorLocation().Z), Number(4));
			}
		}
	}
	else if ((Args[0] == TEXT("body") || Args[0] == TEXT("legs")) && Args.Num() >= 2)
	{
		// Looks of the humanoid base model, for every character (a pipeline test, not a game feature).
		for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
		{
			uint8 Preset = It->GetBodyPreset();
			float Legs = It->GetLegScale();
			if (Args[0] == TEXT("body"))
			{
				Preset = Args[1] == TEXT("male") ? 1 : (Args[1] == TEXT("female") ? 2 : 0);
			}
			else
			{
				Legs = Number(1);
			}
			It->ServerSetBodyLook(Preset, Legs);
		}
	}
	else if (Args[0] == TEXT("refill") && Self)
	{
		Self->ServerRefill();
	}
	else if (Args[0] == TEXT("reset"))
	{
		for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
		{
			It->ServerResetState();
		}
	}
	else if (Args[0] == TEXT("ai") && AI && AICharacter && Args.Num() >= 2)
	{
		if (Args[1] == TEXT("block"))
		{
			AI->CommandBlock(Number(2) != 0.f);
		}
		else if (Args[1] == TEXT("attack"))
		{
			AI->CommandAttack(Number(2));
		}
		else if (Args[1] == TEXT("stamina"))
		{
			AICharacter->ServerSetStamina(Number(2));
		}
	}
#endif
}
