#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HydraGameMode.generated.h"

class AHydraArena;
class AHydraCharacter;

/**
 * Server-only rules of the arena. M1: spawns players at the arena's spawn points and one
 * passive training dummy (something to lock on to when playing alone; the AI opponent
 * comes with M2).
 */
UCLASS()
class HYDRA_API AHydraGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHydraGameMode();

	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	AHydraArena* FindArena() const;

	int32 NextSpawnIndex = 0;

	UPROPERTY()
	TObjectPtr<AHydraCharacter> TrainingDummy;
};
