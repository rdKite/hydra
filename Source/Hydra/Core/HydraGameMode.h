#pragma once

#include "CoreMinimal.h"
#include "Core/HydraMetrics.h"
#include "GameFramework/GameModeBase.h"
#include "HydraGameMode.generated.h"

class AHydraArena;
class AHydraCharacter;

/**
 * Server-only rules of the arena. Two modes:
 *
 *  - Practice (default): players at the arena's spawn points and one AI opponent
 *    (AHydraAIController; passive with Hydra.AI 0).
 *  - Waves (start option ?Waves=1 on the host, or the dev command HydraWaves 1): AI enemies
 *    come in waves (values.json: waves, enemies). Defeated enemies give nXP and hXP to every
 *    player (hXP go to the skills by use); a cleared wave writes a metrics row per player
 *    and enemy type to Saved/Metrics/metrics_<time>.csv (-HydraMetricsDir=<dir> overrides).
 *    After the last wave of the list they start again with stronger enemies.
 */
UCLASS()
class HYDRA_API AHydraGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHydraGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** Reads the start options of a player from its URL: ?Form=slime, ?Fire=1. */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** A character was defeated (called by the character on the server). */
	void OnCharacterDied(AHydraCharacter* Victim, AHydraCharacter* Killer);

	/** Switch to wave mode: the practice opponent leaves, the first wave comes after waves.first_delay_ms. */
	void StartWaves();
	bool IsWaveMode() const { return bWaves; }

protected:
	AHydraArena* FindArena() const;
	void SpawnPracticeOpponent();
	AHydraCharacter* SpawnAI(const FTransform& Spot, FName EnemyId, int32 Serial, int32 LevelBonus);
	void SpawnNextWave();
	void WaveCleared();
	void UpdateWaveState(float NextWaveInS);
	/** Appends one row per player and per enemy type: what happened since the last rows. */
	void WriteMetrics(const TCHAR* Event);

	int32 NextSpawnIndex = 0;

	struct FStartOptions
	{
		FName Form;
		bool bFire = false;
	};
	TMap<TWeakObjectPtr<AController>, FStartOptions> StartOptions;

	UPROPERTY()
	TObjectPtr<AHydraCharacter> AIOpponent;

	// Waves.
	bool bWaves = false;
	/** Waves started so far (1 = the first wave is running or cleared). */
	int32 WaveCount = 0;
	int32 EnemiesAlive = 0;
	int32 NextEnemySerial = 1;
	FTimerHandle NextWaveTimer;

	// Metrics.
	struct FEnemyMetrics
	{
		int32 Count = 0;
		int32 Level = 0;
		FName Form;
		FHydraMetricCounters Counters;
	};
	/** Enemies of the running wave that were defeated, per type (the living ones are added when the row is written). */
	TMap<FName, FEnemyMetrics> DefeatedEnemies;
	/** Counters of each player at the last row. */
	TMap<TWeakObjectPtr<AHydraCharacter>, FHydraMetricCounters> PlayerMetricsWritten;
	FString MetricsFile;
};
