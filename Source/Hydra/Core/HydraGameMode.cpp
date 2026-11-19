#include "Core/HydraGameMode.h"

#include "AI/HydraAIController.h"
#include "Arena/HydraArena.h"
#include "Character/HydraCharacter.h"
#include "Character/HydraProgressionComponent.h"
#include "Core/HydraDevSubsystem.h"
#include "Core/HydraGameState.h"
#include "Core/HydraHUD.h"
#include "Core/HydraPlayerController.h"
#include "Data/HydraValuesSubsystem.h"
#include "EngineUtils.h"
#include "Hydra.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

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

void AHydraGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	bWaves = UGameplayStatics::ParseOption(Options, TEXT("Waves")) == TEXT("1");
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
		if (Start->bFire)
		{
			Character->ServerSetFireImprint(true);
		}
	}
	return Pawn;
}

AHydraCharacter* AHydraGameMode::SpawnAI(const FTransform& Spot, FName EnemyId, int32 Serial, int32 LevelBonus)
{
	AHydraCharacter* Character = GetWorld()->SpawnActorDeferred<AHydraCharacter>(AHydraCharacter::StaticClass(), Spot, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Character)
	{
		return nullptr;
	}
	if (EnemyId.IsNone())
	{
		Character->SetAIControlled();
	}
	else
	{
		Character->SetEnemyType(EnemyId, Serial, LevelBonus);
		Character->SetRespawns(false);
	}
	UGameplayStatics::FinishSpawningActor(Character, Spot);
	if (AHydraAIController* AI = GetWorld()->SpawnActor<AHydraAIController>())
	{
		AI->Possess(Character);
	}
	return Character;
}

void AHydraGameMode::SpawnPracticeOpponent()
{
	if (const AHydraArena* Arena = FindArena())
	{
		AIOpponent = SpawnAI(Arena->GetDummySpawnTransform(), NAME_None, 0, 0);
	}
}

void AHydraGameMode::StartPlay()
{
	Super::StartPlay();

	if (!FindArena())
	{
		UE_LOG(LogHydra, Warning, TEXT("No AHydraArena in the map - run Scripts/create_arena_map.sh"));
		return;
	}
	if (bWaves)
	{
		bWaves = false;
		StartWaves();
	}
	else
	{
		SpawnPracticeOpponent();
	}
}

void AHydraGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	WriteMetrics(TEXT("session_end"));
	Super::EndPlay(EndPlayReason);
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

// --- Waves -------------------------------------------------------------------------------

void AHydraGameMode::StartWaves()
{
	if (bWaves)
	{
		return;
	}
	bWaves = true;
	if (AIOpponent)
	{
		if (AController* Controller = AIOpponent->GetController())
		{
			Controller->Destroy();
		}
		AIOpponent->Destroy();
		AIOpponent = nullptr;
	}
	WaveCount = 0;
	EnemiesAlive = 0;
	const float DelayS = FMath::Max(0.1f, UHydraValuesSubsystem::Get(this).waves.first_delay_ms / 1000.f);
	GetWorldTimerManager().SetTimer(NextWaveTimer, this, &AHydraGameMode::SpawnNextWave, DelayS, false);
	UpdateWaveState(DelayS);
	UE_LOG(LogHydra, Log, TEXT("HydraWaves t=%.2f started first_in=%.1f"), UHydraDevSubsystem::ClockOf(this), DelayS);
}

void AHydraGameMode::UpdateWaveState(float NextWaveInS)
{
	if (AHydraGameState* GS = GetGameState<AHydraGameState>())
	{
		GS->SetWaveState(bWaves, WaveCount, EnemiesAlive, NextWaveInS > 0.f ? GS->GetServerWorldTimeSeconds() + NextWaveInS : 0.f);
	}
}

void AHydraGameMode::SpawnNextWave()
{
	const FHydraWavesValues& W = UHydraValuesSubsystem::Get(this).waves;
	const AHydraArena* Arena = FindArena();
	if (W.list.Num() == 0 || !Arena)
	{
		UE_LOG(LogHydra, Error, TEXT("Waves: values.json has no waves (waves.list) or the map has no arena"));
		return;
	}
	// After the last wave of the list they start again, every enemy some levels higher.
	const int32 Index = WaveCount % W.list.Num();
	const int32 Loop = WaveCount / W.list.Num();
	++WaveCount;
	const TArray<FTransform> Spots = Arena->GetWaveSpawnTransforms();
	FString Names;
	int32 Spawned = 0;
	for (const FHydraWaveGroupValues& Group : W.list[Index].groups)
	{
		for (int32 i = 0; i < Group.count && Spots.Num() > 0; ++i)
		{
			// Spread over the spots, starting at another one every wave.
			const FTransform& Spot = Spots[(WaveCount * 3 + Spawned) % Spots.Num()];
			if (const AHydraCharacter* Enemy = SpawnAI(Spot, Group.enemy, NextEnemySerial++, Loop * W.level_bonus_per_loop))
			{
				++Spawned;
				Names += FString::Printf(TEXT("%s%s"), Names.IsEmpty() ? TEXT("") : TEXT(","), *Enemy->GetLogId());
			}
		}
	}
	EnemiesAlive = Spawned;
	UpdateWaveState(0.f);
	UE_LOG(LogHydra, Log, TEXT("HydraWave t=%.2f wave=%d loop=%d enemies=%d who=%s"), UHydraDevSubsystem::ClockOf(this), WaveCount, Loop, Spawned, *Names);
	if (Spawned == 0)
	{
		WaveCleared();
	}
}

void AHydraGameMode::OnCharacterDied(AHydraCharacter* Victim, AHydraCharacter* Killer)
{
	if (!Victim)
	{
		return;
	}
	const FHydraValues& V = UHydraValuesSubsystem::Get(this);
	if (Victim->GetEnemyId().IsNone())
	{
		// A defeated player loses the use counted since the last defeated enemy (hXP are
		// credited only on victory, per enemy); there is no other penalty in the arena.
		if (!Victim->IsAIControlledCharacter())
		{
			Victim->GetProgression()->ServerDropUse();
		}
		return;
	}
	EnemiesAlive = FMath::Max(0, EnemiesAlive - 1);
	const bool bCleared = bWaves && EnemiesAlive == 0;
	// nXP depends only on the enemy's type and level; hXP = nXP x the stand-in factor of its
	// type. Everyone in the arena gets both in full. Each player's hXP go to its skills by the
	// stamina and mana they spent since the last defeated enemy.
	if (const FHydraEnemyValues* Enemy = V.enemies.Find(Victim->GetEnemyId()))
	{
		const float Nxp = HydraCombatMath::EnemyNxp(V.progression, Enemy->nxp_base, Victim->GetStats().Level);
		const float Hxp = HydraCombatMath::EnemyHxp(Nxp, Enemy->hxp_factor);
		for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsAIControlledCharacter())
			{
				It->GetProgression()->ServerEnemyDefeated(Nxp, Hxp);
			}
		}
	}
	FEnemyMetrics& Defeated = DefeatedEnemies.FindOrAdd(Victim->GetEnemyId());
	++Defeated.Count;
	Defeated.Level = Victim->GetStats().Level;
	Defeated.Form = Victim->GetFormId();
	Defeated.Counters += Victim->Metrics;
	Victim->Metrics = FHydraMetricCounters();
	UpdateWaveState(0.f);
	if (bCleared)
	{
		WaveCleared();
	}
}

void AHydraGameMode::WaveCleared()
{
	const float BreakS = FMath::Max(0.1f, UHydraValuesSubsystem::Get(this).waves.break_ms / 1000.f);
	UE_LOG(LogHydra, Log, TEXT("HydraWaveCleared t=%.2f wave=%d next_in=%.1f"), UHydraDevSubsystem::ClockOf(this), WaveCount, BreakS);
	WriteMetrics(TEXT("wave_cleared"));
	GetWorldTimerManager().SetTimer(NextWaveTimer, this, &AHydraGameMode::SpawnNextWave, BreakS, false);
	UpdateWaveState(BreakS);
}

// --- Metrics -----------------------------------------------------------------------------

void AHydraGameMode::WriteMetrics(const TCHAR* Event)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FString Rows;
	const double Time = World->GetTimeSeconds();
	const int32 ListLength = FMath::Max(1, UHydraValuesSubsystem::Get(this).waves.list.Num());
	const int32 Loop = WaveCount > 0 ? (WaveCount - 1) / ListLength : 0;
	auto AddRow = [&](const TCHAR* Kind, const FString& Who, FName Form, int32 Level, int32 Count, const FHydraMetricCounters& Counters)
	{
		Rows += FString::Printf(TEXT("%.1f,%s,%d,%d,%s,%s,%s,%d,%d,%s\n"), Time, Event, WaveCount, Loop, Kind, *Who, *Form.ToString(), Level, Count, *Counters.ToCsv());
	};

	// Enemies of this wave per type: the defeated ones plus those still standing.
	TMap<FName, FEnemyMetrics> Enemies = DefeatedEnemies;
	DefeatedEnemies.Reset();
	for (TActorIterator<AHydraCharacter> It(World); It; ++It)
	{
		AHydraCharacter* C = *It;
		if (!C->IsAIControlledCharacter())
		{
			FHydraMetricCounters& Written = PlayerMetricsWritten.FindOrAdd(C);
			AddRow(TEXT("player"), C->GetLogId(), C->GetFormId(), C->GetStats().Level, 1, C->Metrics - Written);
			Written = C->Metrics;
		}
		else if (C->IsAlive())
		{
			FEnemyMetrics& E = Enemies.FindOrAdd(C->GetEnemyId().IsNone() ? FName(TEXT("practice_opponent")) : C->GetEnemyId());
			++E.Count;
			E.Level = C->GetStats().Level;
			E.Form = C->GetFormId();
			E.Counters += C->Metrics;
			C->Metrics = FHydraMetricCounters();
		}
	}
	for (const TPair<FName, FEnemyMetrics>& Pair : Enemies)
	{
		AddRow(TEXT("enemy"), Pair.Key.ToString(), Pair.Value.Form, Pair.Value.Level, Pair.Value.Count, Pair.Value.Counters);
	}
	if (Rows.IsEmpty())
	{
		return;
	}
	if (MetricsFile.IsEmpty())
	{
		FString Dir;
		if (!FParse::Value(FCommandLine::Get(), TEXT("HydraMetricsDir="), Dir))
		{
			Dir = FPaths::ProjectSavedDir() / TEXT("Metrics");
		}
		MetricsFile = FPaths::ConvertRelativePathToFull(Dir / FString::Printf(TEXT("metrics_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))));
		Rows = FString::Printf(TEXT("time_s,event,wave,loop,kind,who,form,level,count,%s\n"), FHydraMetricCounters::CsvHeader()) + Rows;
		UE_LOG(LogHydra, Log, TEXT("HydraMetrics file=%s"), *MetricsFile);
	}
	FFileHelper::SaveStringToFile(Rows, *MetricsFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}
