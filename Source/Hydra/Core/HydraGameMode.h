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
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	AHydraArena* FindArena() const;

	int32 NextSpawnIndex = 0;

	UPROPERTY()
	TObjectPtr<AHydraCharacter> AIOpponent;
};
