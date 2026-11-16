#include "Core/HydraGameState.h"

#include "Character/HydraCombatMath.h"
#include "Core/HydraPlayerController.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Hydra.h"
#include "Net/UnrealNetwork.h"

void AHydraGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHydraGameState, LatencyStepMs);
	DOREPLIFETIME(AHydraGameState, bPacketLoss);
	DOREPLIFETIME(AHydraGameState, bWaves);
	DOREPLIFETIME(AHydraGameState, Wave);
	DOREPLIFETIME(AHydraGameState, EnemiesLeft);
	DOREPLIFETIME(AHydraGameState, NextWaveTime);
}

void AHydraGameState::SetWaveState(bool bInWaves, int32 InWave, int32 InEnemiesLeft, float InNextWaveTime)
{
	bWaves = bInWaves;
	Wave = InWave;
	EnemiesLeft = InEnemiesLeft;
	NextWaveTime = InNextWaveTime;
}

void AHydraGameState::BeginPlay()
{
	Super::BeginPlay();
	ApplyLocal();
}

float AHydraGameState::GetPacketLossRatio() const
{
	return bPacketLoss ? UHydraValuesSubsystem::Get(this).net_simulation.packet_loss_ratio : 0.f;
}

void AHydraGameState::SetLatencyStep(int32 StepMs)
{
	check(HasAuthority());
	if (StepMs < 0)
	{
		StepMs = HydraCombatMath::NextLatencyStep(UHydraValuesSubsystem::Get(this).net_simulation.latency_steps_ms, LatencyStepMs);
	}
	const bool bWasZero = LatencyStepMs == 0;
	LatencyStepMs = FMath::Max(0, StepMs);
	if (bWasZero && LatencyStepMs > 0)
	{
		bPacketLoss = true;
	}
	else if (LatencyStepMs == 0)
	{
		bPacketLoss = false;
	}
	ApplyLocal();
}

void AHydraGameState::SetPacketLoss(int32 On)
{
	check(HasAuthority());
	bPacketLoss = On < 0 ? !bPacketLoss : On != 0;
	ApplyLocal();
}

void AHydraGameState::OnRep_NetSimulation()
{
	ApplyLocal();
}

void AHydraGameState::ApplyLocal()
{
#if DO_ENABLE_NET_TEST
	UNetDriver* Driver = GetWorld() ? GetWorld()->GetNetDriver() : nullptr;
	if (!Driver)
	{
		return;
	}
	const FHydraNetSimulationValues& N = UHydraValuesSubsystem::Get(this).net_simulation;
	FPacketSimulationSettings Settings;
	Settings.PktLag = HydraCombatMath::OutgoingLagMs(LatencyStepMs, N.latency_is_round_trip);
	// Each side drops its outgoing packets. With packet_loss_both_directions off, only the
	// server does (loss on the way to the clients).
	const bool bThisSideDrops = bPacketLoss && (N.packet_loss_both_directions || HasAuthority());
	Settings.PktLoss = bThisSideDrops ? HydraCombatMath::PacketLossPercent(N.packet_loss_ratio) : 0;
	Driver->SetPacketSimulationSettings(Settings);
	UE_LOG(LogHydra, Log, TEXT("NetSim: step %d ms (outgoing lag %d ms), packet loss %d%% [%s]"),
		LatencyStepMs, Settings.PktLag, Settings.PktLoss, HasAuthority() ? TEXT("server") : TEXT("client"));
#endif
}

void AHydraGameState::RequestLatencyStep(UWorld* World, int32 StepMs)
{
	if (!World)
	{
		return;
	}
	if (World->GetNetMode() == NM_Client)
	{
		if (AHydraPlayerController* PC = Cast<AHydraPlayerController>(World->GetFirstPlayerController()))
		{
			PC->ServerSetNetSimulation(StepMs, AHydraPlayerController::NetSimUnchanged);
		}
	}
	else if (AHydraGameState* GS = World->GetGameState<AHydraGameState>())
	{
		GS->SetLatencyStep(StepMs);
	}
}

void AHydraGameState::RequestPacketLoss(UWorld* World, int32 On)
{
	if (!World)
	{
		return;
	}
	if (World->GetNetMode() == NM_Client)
	{
		if (AHydraPlayerController* PC = Cast<AHydraPlayerController>(World->GetFirstPlayerController()))
		{
			PC->ServerSetNetSimulation(AHydraPlayerController::NetSimUnchanged, On);
		}
	}
	else if (AHydraGameState* GS = World->GetGameState<AHydraGameState>())
	{
		GS->SetPacketLoss(On);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GHydraLatencyStepCommand(
	TEXT("Hydra.LatencyStep"),
	TEXT("Hydra.LatencyStep [ms]: simulated round-trip latency (no argument: next step from values.json)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		AHydraGameState::RequestLatencyStep(World, Args.Num() > 0 ? FCString::Atoi(*Args[0]) : -1);
	}));

static FAutoConsoleCommandWithWorldAndArgs GHydraPacketLossCommand(
	TEXT("Hydra.PacketLoss"),
	TEXT("Hydra.PacketLoss [0|1]: simulated packet loss off / on (no argument: toggle)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		AHydraGameState::RequestPacketLoss(World, Args.Num() > 0 ? FCString::Atoi(*Args[0]) : -1);
	}));
