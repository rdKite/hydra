#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HydraGameState.generated.h"

/**
 * Replicated session state. M1: the network emulation setting - the latency step (one of
 * net_simulation.latency_steps_ms) and whether packet loss is on.
 *
 * Every process applies the setting to its own net driver with the engine's packet
 * simulation: outgoing packets are delayed by half the step (the steps are round trip, see
 * net_simulation.latency_is_round_trip) and dropped with net_simulation.packet_loss_ratio.
 * Host and client together give the step as ping and loss in both directions.
 *
 * Switch it with the console commands Hydra.LatencyStep [ms] and Hydra.PacketLoss [0|1]
 * (no argument: next step / toggle) or the keys F6 and F8, from the host or a client.
 * Only available in builds with DO_ENABLE_NET_TEST (not in shipping builds).
 */
UCLASS()
class HYDRA_API AHydraGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	AHydraGameState();

	/** Server: set the latency step in ms (-1 = next step from values.json). */
	void SetLatencyStep(int32 StepMs);

	/** Server: packet loss on / off (-1 = toggle). Switched on with the first step above 0, off at 0. */
	void SetPacketLoss(int32 On);

	int32 GetLatencyStepMs() const { return LatencyStepMs; }
	bool IsPacketLossOn() const { return bPacketLoss; }
	float GetPacketLossRatio() const;

	/** Server: what the HUD shows about the waves. NextWaveTime: server time of the next wave, 0 = a wave is running. */
	void SetWaveState(bool bInWaves, int32 InWave, int32 InEnemiesLeft, float InNextWaveTime);
	bool IsWaveMode() const { return bWaves; }
	int32 GetWave() const { return Wave; }
	int32 GetEnemiesLeft() const { return EnemiesLeft; }
	float GetNextWaveTime() const { return NextWaveTime; }

	/**
	 * Is an enemy alive (phase-1.md section 17: no evolution in combat)? An enemy is a living
	 * AI character that is not passive. Replicated, so the HUD of a client can say why the
	 * evolution is not offered; the server asks ComputeEnemyAlive when it decides.
	 */
	bool IsEnemyAlive() const { return bEnemyAlive; }
	static bool ComputeEnemyAlive(const UWorld* World);

	/** Applies the replicated setting to this process's net driver. */
	void ApplyLocal();

	/** From any process: a client asks the server, the server sets and replicates. */
	static void RequestLatencyStep(UWorld* World, int32 StepMs);
	static void RequestPacketLoss(UWorld* World, int32 On);

protected:
	UFUNCTION()
	void OnRep_NetSimulation();

	UPROPERTY(ReplicatedUsing = OnRep_NetSimulation)
	int32 LatencyStepMs = 0;

	UPROPERTY(ReplicatedUsing = OnRep_NetSimulation)
	bool bPacketLoss = false;

	UPROPERTY(Replicated)
	bool bWaves = false;

	UPROPERTY(Replicated)
	int32 Wave = 0;

	UPROPERTY(Replicated)
	int32 EnemiesLeft = 0;

	UPROPERTY(Replicated)
	float NextWaveTime = 0.f;

	UPROPERTY(Replicated)
	bool bEnemyAlive = false;
};
