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
 * binding table is readable here:
 *
 *   Action          Mouse/keyboard        Gamepad
 *   Move            WASD                  left stick
 *   Look            mouse                 right stick
 *   Walk (hold)     Left Alt              -
 *   Lock-on         Q / middle mouse      right stick click
 *   Reload values   F5                    -
 *   Latency step    F6                    -
 *   Packet loss     F8 (toggle)           -
 *
 * Lock-on turns only the camera: while locked, the camera follows the target and the
 * character turns towards the camera direction at its turn rate.
 *
 * The Hydra* exec functions are for scripted runs (Scripts/run_live.sh) and for trying
 * things from the console (key ~).
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

	// --- Scripted runs and console -------------------------------------------------------

	/** Move input (X right, Y forward, relative to the camera) for Seconds. */
	UFUNCTION(Exec)
	void HydraMove(float X, float Y, float Seconds);

	/** Point the camera (control rotation) to Yaw / Pitch in degrees. */
	UFUNCTION(Exec)
	void HydraLook(float Yaw, float Pitch);

	/** Same as the lock-on button. */
	UFUNCTION(Exec)
	void HydraLockOn();

	/** Run the soft aim assist once, as the start of an attack will do from M2 on. */
	UFUNCTION(Exec)
	void HydraAimAssist();

	/** Spend stamina on the server like an action would (cost check, cost, regeneration delay). */
	UFUNCTION(Exec)
	void HydraSpendStamina(float Amount);

	/** Log position, facing, stamina, lock-on and ping of all characters as this process sees them. */
	UFUNCTION(Exec)
	void HydraStats();

	UFUNCTION(Server, Reliable)
	void ServerDevSpendStamina(float Amount);

	UFUNCTION(Server, Reliable)
	void ServerDevAimAssist();

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
	void OnLockOn(const FInputActionValue& Value);
	void OnReloadValues(const FInputActionValue& Value);
	void OnLatencyStep(const FInputActionValue& Value);
	void OnPacketLossToggle(const FInputActionValue& Value);

	void UpdateLockOn(float DeltaTime);

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookMouseAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookStickAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> WalkAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LockOnAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ReloadValuesAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LatencyStepAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PacketLossAction;

	FVector2D ScriptedMoveInput = FVector2D::ZeroVector;
	float ScriptedMoveRemaining = 0.f;

	/** Gamepad look speed in deg/s at full deflection (camera feel, not a gameplay value). */
	float StickLookRateDegS = 180.f;
};
