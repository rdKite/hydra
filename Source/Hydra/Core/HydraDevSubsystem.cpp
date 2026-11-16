#include "Core/HydraDevSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Hydra.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void UHydraDevSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const TCHAR* CmdLine = FCommandLine::Get();
	FParse::Value(CmdLine, TEXT("HydraWaitPlayers="), WaitPlayers);
	FParse::Value(CmdLine, TEXT("HydraShotName="), ShotName);
	FParse::Value(CmdLine, TEXT("HydraShotDir="), ShotDir);

	FString Value;
	if (FParse::Value(CmdLine, TEXT("HydraShot="), Value, false))
	{
		TArray<FString> Times;
		Value.ParseIntoArray(Times, TEXT(","));
		for (const FString& T : Times)
		{
			Pending.Add({ FCString::Atod(*T), EKind::Shot, T });
		}
	}
	if (FParse::Value(CmdLine, TEXT("HydraExec="), Value, false))
	{
		TArray<FString> Entries;
		Value.ParseIntoArray(Entries, TEXT(";"));
		for (const FString& Entry : Entries)
		{
			FString Time, Command;
			if (Entry.Split(TEXT(":"), &Time, &Command))
			{
				Command.ReplaceCharInline(TEXT('|'), TEXT(' ')); // '|' = space (shell-safe)
				Pending.Add({ FCString::Atod(*Time), EKind::Exec, Command });
			}
		}
	}
	if (FParse::Value(CmdLine, TEXT("HydraQuit="), Value, false))
	{
		Pending.Add({ FCString::Atod(*Value), EKind::Quit, FString() });
	}
	// Earliest first; entries with the same time keep their order from the command line.
	Pending.StableSort([](const FPending& A, const FPending& B) { return A.At < B.At; });

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UHydraDevSubsystem::Tick));
}

void UHydraDevSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

double UHydraDevSubsystem::GetClock() const
{
	return StartTime < 0.0 ? 0.0 : FPlatformTime::Seconds() - StartTime;
}

double UHydraDevSubsystem::ClockOf(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UHydraDevSubsystem* Dev = GI ? GI->GetSubsystem<UHydraDevSubsystem>() : nullptr;
	return Dev ? Dev->GetClock() : 0.0;
}

bool UHydraDevSubsystem::IsReadyToStart() const
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const APlayerController* PC = World ? GetGameInstance()->GetFirstLocalPlayerController(World) : nullptr;
	const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	return PC && PC->GetPawn() && GS && GS->PlayerArray.Num() >= WaitPlayers;
}

bool UHydraDevSubsystem::Tick(float DeltaTime)
{
	if (StartTime < 0.0)
	{
		if (!IsReadyToStart())
		{
			return true;
		}
		StartTime = FPlatformTime::Seconds();
		UE_LOG(LogHydra, Log, TEXT("HydraDev: clock started (%d players)"), WaitPlayers);
	}

	const double Clock = GetClock();
	UWorld* World = GetGameInstance()->GetWorld();
	APlayerController* PC = World ? GetGameInstance()->GetFirstLocalPlayerController(World) : nullptr;
	while (Pending.Num() > 0 && Pending[0].At <= Clock && World)
	{
		const FPending Next = Pending[0];
		Pending.RemoveAt(0);
		switch (Next.Kind)
		{
		case EKind::Shot:
		{
			const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / ShotDir / FString::Printf(TEXT("%s_%s.png"), *ShotName, *Next.Command));
			UE_LOG(LogHydra, Log, TEXT("HydraDev: t=%.2f screenshot %s"), Clock, *File);
			FScreenshotRequest::RequestScreenshot(File, /*bShowUI*/ true, /*bAddFilenameSuffix*/ false);
			break;
		}
		case EKind::Quit:
			UE_LOG(LogHydra, Log, TEXT("HydraDev: t=%.2f quit"), Clock);
			FPlatformMisc::RequestExit(false);
			return false;
		case EKind::Exec:
		default:
			UE_LOG(LogHydra, Log, TEXT("HydraDev: t=%.2f exec '%s'"), Clock, *Next.Command);
			// Through the player controller, so its exec functions and viewport commands work.
			if (PC)
			{
				PC->ConsoleCommand(Next.Command, true);
			}
			else if (GEngine)
			{
				GEngine->Exec(World, *Next.Command);
			}
			break;
		}
	}
	return true;
}
