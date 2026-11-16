#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HydraGameMode.generated.h"

class AHydraArena;
class AHydraCharacter;

/**
 * Server-only rules of the arena: spawns players at the arena's spawn points and one AI
 * opponent for practice (AHydraAIController; passive with Hydra.AI 0).
 */
UCLASS()
class HYDRA_API AHydraGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHydraGameMode();

	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void StartPlay() override;
	/** Reads the start options of a player from its URL: ?Form=slime, ?Fire=1. */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	AHydraArena* FindArena() const;

	int32 NextSpawnIndex = 0;

	struct FStartOptions
	{
		FName Form;
		bool bFire = false;
	};
	TMap<TWeakObjectPtr<AController>, FStartOptions> StartOptions;

	UPROPERTY()
	TObjectPtr<AHydraCharacter> AIOpponent;
};
