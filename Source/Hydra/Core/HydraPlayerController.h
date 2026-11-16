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
 *   Reload values               F5                    -
 *   Latency step                F6                    -
 *   Packet loss                 F8 (toggle)           -
 *
 * Lock-on turns only the camera: while locked, the camera follows the target and the
 * character turns towards the camera direction at its turn rate.
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

	/** Everyone alive with full health and stamina. */
	UFUNCTION(Exec)
	void HydraResetAll();

	UFUNCTION(Server, Reliable)
	void ServerDevCommand(const FString& Command);

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
