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

FString AHydraGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FStartOptions Start;
	Start.Form = FName(*UGameplayStatics::ParseOption(Options, TEXT("Form")));
	Start.bFire = UGameplayStatics::ParseOption(Options, TEXT("Fire")) == TEXT("1");
	StartOptions.Add(NewPlayerController, Start);
	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

APawn* AHydraGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	APawn* Pawn = nullptr;
	const AHydraArena* Arena = FindArena();
	const TArray<FTransform> Spawns = Arena ? Arena->GetSpawnTransforms() : TArray<FTransform>();
	if (Spawns.Num() > 0)
	{
		Pawn = SpawnDefaultPawnAtTransform(NewPlayer, Spawns[NextSpawnIndex++ % Spawns.Num()]);
	}
	else
	{
		Pawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}
	// Arena start options: which form the player is and whether fire is awakened.
	AHydraCharacter* Character = Cast<AHydraCharacter>(Pawn);
	if (const FStartOptions* Start = Character ? StartOptions.Find(NewPlayer) : nullptr)
	{
		Character->ServerSetForm(Start->Form);
		Character->ServerSetFireImprint(Start->bFire);
	}
	return Pawn;
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
