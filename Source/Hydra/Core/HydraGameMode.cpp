#include "Core/HydraGameMode.h"

#include "AI/HydraAIController.h"
#include "Arena/HydraArena.h"
#include "Character/HydraCharacter.h"
#include "Core/HydraGameState.h"
#include "Core/HydraHUD.h"
#include "Core/HydraPlayerController.h"
#include "EngineUtils.h"
#include "Hydra.h"
#include "Kismet/GameplayStatics.h"

AHydraGameMode::AHydraGameMode()
{
	DefaultPawnClass = AHydraCharacter::StaticClass();
	PlayerControllerClass = AHydraPlayerController::StaticClass();
	HUDClass = AHydraHUD::StaticClass();
	GameStateClass = AHydraGameState::StaticClass();
}

AHydraArena* AHydraGameMode::FindArena() const
{
	for (TActorIterator<AHydraArena> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

APawn* AHydraGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	if (const AHydraArena* Arena = FindArena())
	{
		const TArray<FTransform> Spawns = Arena->GetSpawnTransforms();
		if (Spawns.Num() > 0)
		{
			return SpawnDefaultPawnAtTransform(NewPlayer, Spawns[NextSpawnIndex++ % Spawns.Num()]);
		}
	}
	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

void AHydraGameMode::StartPlay()
{
	Super::StartPlay();

	if (const AHydraArena* Arena = FindArena())
	{
		const FTransform Spot = Arena->GetDummySpawnTransform();
		AIOpponent = GetWorld()->SpawnActorDeferred<AHydraCharacter>(AHydraCharacter::StaticClass(), Spot, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (AIOpponent)
		{
			AIOpponent->SetAIControlled();
			UGameplayStatics::FinishSpawningActor(AIOpponent, Spot);
			if (AHydraAIController* AI = GetWorld()->SpawnActor<AHydraAIController>())
			{
				AI->Possess(AIOpponent);
			}
		}
	}
	else
	{
		UE_LOG(LogHydra, Warning, TEXT("No AHydraArena in the map - run Scripts/create_arena_map.sh"));
	}
}

void AHydraGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	// A new connection must get the current network emulation settings too.
	if (AHydraGameState* GS = GetGameState<AHydraGameState>())
	{
		GS->ApplyLocal();
	}
}
