#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "HydraPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class AHydraCharacter;

/**
 * Third-person controls for mouse/keyboard and gamepad. The Enhanced Input actions and the
 * mapping context are created in C++ at runtime instead of as editor assets, so the whole
 * binding table is readable here. One button per hand: it does what the hand holds.
 *
 *   Action                      Mouse/keyboard        Gamepad
 *   Move                        WASD                  left stick
 *   Look                        mouse                 right stick
 *   Jump                        Space                 A / cross
 *   Dodge                       Left Alt              B / circle
 *   Walk (hold)                 Left Ctrl             -
 *   Weapon hand: press = light  left mouse            right shoulder
 *                hold = heavy
 *   Off hand: shield = block    right mouse           left shoulder
 *     (hold); slime = cast the bolt (press, hold = charge)
 *   Lock-on                     Q / middle mouse      right stick click
 *   Precise movement (toggle)   X                     left stick click
 *   Milestone: evolve           J                     D-pad up
 *   Milestone: raise imprint    K                     D-pad right
 *   Debug panel (dev builds)    F1                    Back / View
 *     in the panel: arrow keys / D-pad choose and change, Enter / A applies, mouse clicks
 *   Reload values               F5                    -
 *   Latency step                F6                    -
 *   Packet loss                 F8 (toggle)           -
 *
 * Lock-on turns only the camera: while locked, the camera follows the target and the
 * character turns towards the camera direction at its turn rate. Precise movement does the
 * same without a target: the character faces where the camera looks and steps backwards and
 * sideways; without it the character turns into its movement.
 *
 * The Hydra* exec functions are for scripted runs (Scripts/run_live.sh) and for trying
 * things from the console (key ~). Those that change the game state go through one dev RPC
 * that is compiled out of shipping builds.
 */
UCLASS()
class HYDRA_API AHydraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** "Leave this part of the setting as it is" for ServerSetNetSimulation. */
	static constexpr int32 NetSimUnchanged = -2;

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	AHydraCharacter* GetHydraCharacter() const;

	/** Client -> server: reload the data files on the server (which then tells all clients). */
	UFUNCTION(Server, Reliable)
	void ServerReloadValues();

	/** Server -> client: reload the data files in this client process. */
	UFUNCTION(Client, Reliable)
	void ClientReloadValues();

	/** Client -> server: change the network emulation (StepMs / On: -1 = next / toggle, NetSimUnchanged = keep). */
	UFUNCTION(Server, Reliable)
	void ServerSetNetSimulation(int32 StepMs, int32 On);

	// --- Scripted runs and console: input -------------------------------------------------

	/** Move input (X right, Y forward, relative to the camera) for Seconds. */
	UFUNCTION(Exec)
	void HydraMove(float X, float Y, float Seconds);

	/** Point the camera (control rotation) to Yaw / Pitch in degrees. */
	UFUNCTION(Exec)
	void HydraLook(float Yaw, float Pitch);

	/** Same as the lock-on button. */
	UFUNCTION(Exec)
	void HydraLockOn();

	/** Precise movement on (1) / off (0); the key toggles it. */
	UFUNCTION(Exec)
	void HydraPrecise(int32 On);

	/** Press the weapon hand's button and hold it for HoldMs (0 = a tap: light attack). */
	UFUNCTION(Exec)
	void HydraAttack(float HoldMs);

	/** Hold (1) or release (0) the shield hand's button. */
	UFUNCTION(Exec)
	void HydraBlock(int32 Hold);

	/** Press the off-hand button and hold it for HoldMs (slime: cast the bolt, charged by holding). */
	UFUNCTION(Exec)
	void HydraOffHand(float HoldMs);

	/** Change the own form: "humanoid" or "slime" (dev; the arena start option is ?Form=). */
	UFUNCTION(Exec)
	void HydraForm(const FString& FormId);

	/** Fire awakened for the own character on (1) / off (0): the bolt carries fire (dev; M4 earns it). */
	UFUNCTION(Exec)
	void HydraFire(int32 On);

	/** Press the dodge button. */
	UFUNCTION(Exec)
	void HydraDodge();

	/** Press the jump button. */
	UFUNCTION(Exec)
	void HydraJump();

	/**
	 * Arm a light attack that fires by itself so that its strike begins when this window sees
	 * the nearest other character StrikeDistanceCm away (for the lag-compensation check: the
	 * attack is timed by what the attacker sees). 0 disarms.
	 */
	UFUNCTION(Exec)
	void HydraAutoAttack(float StrikeDistanceCm);

	/**
	 * Arm a block that is raised by itself DelayMs after this window sees another character
	 * start an attack, and held for 600 ms (for the lag-compensation check of the block arc:
	 * a block raised too late for the attacker to have seen it). Negative disarms.
	 */
	UFUNCTION(Exec)
	void HydraAutoBlock(float DelayMs);

	/** Log position, facing, health, stamina, action, lock-on and ping of all characters as this process sees them. */
	UFUNCTION(Exec)
	void HydraStats();

	// --- Scripted runs and console: game state (through the dev RPC) ----------------------

	/** Spend stamina on the server like an action would (cost check, cost, regeneration delay). */
	UFUNCTION(Exec)
	void HydraSpendStamina(float Amount);

	/** Put the own character at X / Y (cm) facing Yaw. */
	UFUNCTION(Exec)
	void HydraTeleport(float X, float Y, float Yaw);

	/** Put the AI opponent at X / Y (cm) facing Yaw. */
	UFUNCTION(Exec)
	void HydraAITeleport(float X, float Y, float Yaw);

	/** Drive the AI opponent directly: "block 0|1", "attack <hold ms>", "stamina <value>". */
	UFUNCTION(Exec)
	void HydraAI(const FString& Verb, float Value);

	/** Dev, looks only: body preset of every character with the humanoid base model - neutral | male | female. */
	UFUNCTION(Exec)
	void HydraBody(const FString& Preset);

	/** Dev, looks only: length factor of the leg bones of every character (1 = as built, 0.8 = towards a dwarf). */
	UFUNCTION(Exec)
	void HydraLegs(float Scale);

	/** Dev: full stamina and mana for the own character (health and chain stay as they are). */
	UFUNCTION(Exec)
	void HydraRefill();

	/** Everyone alive with full health and stamina. */
	UFUNCTION(Exec)
	void HydraResetAll();

	UFUNCTION(Server, Reliable)
	void ServerDevCommand(const FString& Command);

	/** Client -> server: the choice at a milestone, "imprint" or "evolve" (only what is on offer is accepted). */
	UFUNCTION(Server, Reliable)
	void ServerChooseMilestone(FName Choice);

	/** Same as the milestone keys: HydraChoose imprint | evolve. */
	UFUNCTION(Exec)
	void HydraChoose(const FString& Choice);

	/** Dev: waves on (1) - the practice opponent leaves and the first wave comes - or off (0). */
	UFUNCTION(Exec)
	void HydraWaves(int32 On);

	// --- Test tools: every action of the debug panel is also a console command (phase-1.md section 27) ---

	/** Own form: any id from forms.json (same as HydraForm). Starts at the form's start levels. */
	UFUNCTION(Exec)
	void HydraSetForm(const FString& FormId);

	/** Own character level; attributes, bars and the skill limit follow at once. */
	UFUNCTION(Exec)
	void HydraSetLevel(int32 Level);

	/** One skill of the own form to a level (at most the character level) and its progress bar (0..1). */
	UFUNCTION(Exec)
	void HydraSetSkill(const FString& SkillId, int32 Level, float Progress = 0.f);

	/** Own fire imprint level (0 = none). */
	UFUNCTION(Exec)
	void HydraSetImprint(int32 Level);

	/** Opens a milestone for the own character. */
	UFUNCTION(Exec)
	void HydraMilestone();

	/** Own character: alive, full bars, debuffs removed. */
	UFUNCTION(Exec)
	void HydraReset();

	/** An enemy of a type from enemies.json a few metres in front of the own character. Passive: 1 = it stands still. */
	UFUNCTION(Exec)
	void HydraSpawn(const FString& EnemyId, int32 Level, int32 Passive = 0);

	/** Removes every AI character. */
	UFUNCTION(Exec)
	void HydraClearEnemies();

	/** Opens (1) or closes (0) the debug panel. */
	UFUNCTION(Exec)
	void HydraPanel(int32 Open);

	/** Drives the open panel like the keys do: up | down | left | right | enter. */
	UFUNCTION(Exec)
	void HydraPanelKey(const FString& Key);

	/** One line of the debug panel as the HUD draws it. */
	struct FPanelRow
	{
		FString Label;
		FString Value;
		/** Left / right change the value. */
		bool bAdjustable = false;
	};
	bool IsDebugPanelOpen() const { return bDebugPanelOpen; }
	int32 GetDebugPanelSelection() const { return PanelSelection; }
	TArray<FPanelRow> GetDebugPanelRows() const;
	/** Mouse: a click on a row (Direction -1 / +1 = its arrows, 0 = the row itself). */
	void DebugPanelClick(int32 Row, int32 Direction);

	/** Dev: give the own character nXP / fire imprint progress. */
	UFUNCTION(Exec)
	void HydraXp(float Amount);

	UFUNCTION(Exec)
	void HydraImprintProgress(float Amount);

	/** Dev: put the wave enemy with this log id (e.g. grunt1) at X / Y facing Yaw. */
	UFUNCTION(Exec)
	void HydraEnemyTeleport(const FString& Who, float X, float Y, float Yaw);

protected:
	virtual void SetupInputComponent() override;

private:
	UInputAction* MakeAction(FName Name, EInputActionValueType Type);
	void BuildInput();

	void OnMove(const FInputActionValue& Value);
	void OnLookMouse(const FInputActionValue& Value);
	void OnLookStick(const FInputActionValue& Value);
	void OnWalkPressed(const FInputActionValue& Value);
	void OnWalkReleased(const FInputActionValue& Value);
	void OnAttackPressed(const FInputActionValue& Value);
	void OnAttackReleased(const FInputActionValue& Value);
	void OnBlockPressed(const FInputActionValue& Value);
	void OnBlockReleased(const FInputActionValue& Value);
	void OnDodge(const FInputActionValue& Value);
	void OnJumpPressed(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnLockOn(const FInputActionValue& Value);
	void OnPreciseToggle(const FInputActionValue& Value);
	void OnChooseEvolve(const FInputActionValue& Value);
	void OnChooseImprint(const FInputActionValue& Value);
	void OnPanelToggle(const FInputActionValue& Value);
	void OnPanelUp(const FInputActionValue& Value);
	void OnPanelDown(const FInputActionValue& Value);
	void OnPanelLeft(const FInputActionValue& Value);
	void OnPanelRight(const FInputActionValue& Value);
	void OnPanelApply(const FInputActionValue& Value);

	// Debug panel.
	enum class EPanelItem : uint8 { Form, Level, Skill, Imprint, Milestone, Reset, EnemyType, EnemyLevel, EnemyPassive, Spawn, Clear, Waves };
	struct FPanelItem
	{
		EPanelItem Kind;
		FName Name;
	};
	TArray<FPanelItem> BuildPanelItems() const;
	void SetDebugPanelOpen(bool bOpen);
	void PanelMove(int32 Direction);
	void PanelAdjust(int32 Row, int32 Direction);
	void PanelActivate(int32 Row);
	void OnReloadValues(const FInputActionValue& Value);
	void OnLatencyStep(const FInputActionValue& Value);
	void OnPacketLossToggle(const FInputActionValue& Value);

	void UpdateLockOn(float DeltaTime);
	void UpdateAutoAttack();
	void UpdateAutoBlock();

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookMouseAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookStickAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> WalkAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> AttackAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> BlockAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> DodgeAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LockOnAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PreciseAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> EvolveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ImprintAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PanelToggleAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PanelUpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PanelDownAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PanelLeftAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PanelRightAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PanelApplyAction;

	bool bDebugPanelOpen = false;
	int32 PanelSelection = 0;
	/** What the panel's spawn row will spawn. */
	FName PanelEnemyId;
	int32 PanelEnemyLevel = 5;
	bool bPanelEnemyPassive = true;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ReloadValuesAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LatencyStepAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PacketLossAction;

	FVector2D ScriptedMoveInput = FVector2D::ZeroVector;
	float ScriptedMoveRemaining = 0.f;
	/** > 0: scripted attack button is held and released at this world time. */
	double ScriptedAttackReleaseAt = -1.0;
	float AutoAttackDistanceCm = 0.f;
	float AutoBlockDelayS = -1.f;
	double AutoBlockPressAt = -1.0;
	double AutoBlockReleaseAt = -1.0;

	/** Gamepad look speed in deg/s at full deflection (camera feel, not a gameplay value). */
	float StickLookRateDegS = 180.f;
};
