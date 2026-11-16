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

	/** Server: set the latency step in ms (-1 = next step from values.json). */
	void SetLatencyStep(int32 StepMs);

	/** Server: packet loss on / off (-1 = toggle). Switched on with the first step above 0, off at 0. */
	void SetPacketLoss(int32 On);

	int32 GetLatencyStepMs() const { return LatencyStepMs; }
	bool IsPacketLossOn() const { return bPacketLoss; }
	float GetPacketLossRatio() const;

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
};
