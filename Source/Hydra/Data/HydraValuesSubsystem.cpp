#include "Data/HydraValuesSubsystem.h"

#include "Core/HydraPlayerController.h"
#include "Data/HydraDataLoader.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Hydra.h"

void UHydraValuesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (!Reload())
	{
		UE_LOG(LogHydra, Error, TEXT("Gameplay data could not be loaded - gameplay values are all zero"));
	}
}

bool UHydraValuesSubsystem::Reload()
{
	const FString Dir = DataDirOverride.IsEmpty() ? HydraDataLoader::FindDataDir() : DataDirOverride;

	FHydraValues NewValues;
	FHydraDefinitions NewDefinitions;
	TArray<FString> Problems;
	const bool bValuesOk = HydraDataLoader::LoadValues(Dir, NewValues, Problems);
	const bool bDefinitionsOk = HydraDataLoader::LoadDefinitions(Dir, NewDefinitions, Problems);
	const bool bOk = bValuesOk && bDefinitionsOk;
	for (const FString& Problem : Problems)
	{
		// Unreadable files are errors; stray or missing keys are warnings (the game still runs).
		if (bOk)
		{
			UE_LOG(LogHydra, Warning, TEXT("Data: %s"), *Problem);
		}
		else
		{
			UE_LOG(LogHydra, Error, TEXT("Data: %s"), *Problem);
		}
	}
	if (!bOk)
	{
		return false;
	}

	Values = MoveTemp(NewValues);
	Definitions = MoveTemp(NewDefinitions);
	LoadedDir = Dir;
	UE_LOG(LogHydra, Log, TEXT("Loaded values.json v%d and %d forms, %d skills, %d elements, %d weapons from %s"), Values.version,
		Definitions.forms.forms.Num(), Definitions.skills.skills.Num(), Definitions.elements.elements.Num(), Definitions.weapons.weapons.Num(), *Dir);
	OnReloaded.Broadcast(Values);
	return true;
}

UHydraValuesSubsystem* UHydraValuesSubsystem::GetSubsystem(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UHydraValuesSubsystem>() : nullptr;
}

const FHydraValues& UHydraValuesSubsystem::Get(const UObject* WorldContext)
{
	static const FHydraValues Empty;
	const UHydraValuesSubsystem* Subsystem = GetSubsystem(WorldContext);
	return Subsystem ? Subsystem->GetValues() : Empty;
}

void UHydraValuesSubsystem::ReloadEverywhere(UWorld* World)
{
	if (!World)
	{
		return;
	}
	// The server is authoritative: a client asks the server, the server reloads its own copy
	// and tells every remote client to reload theirs.
	if (World->GetNetMode() == NM_Client)
	{
		if (AHydraPlayerController* PC = Cast<AHydraPlayerController>(World->GetFirstPlayerController()))
		{
			PC->ServerReloadValues();
		}
		return;
	}
	if (UHydraValuesSubsystem* Subsystem = GetSubsystem(World))
	{
		Subsystem->Reload();
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		AHydraPlayerController* PC = Cast<AHydraPlayerController>(It->Get());
		if (PC && !PC->IsLocalController())
		{
			PC->ClientReloadValues();
		}
	}
}

static FAutoConsoleCommandWithWorld GHydraReloadValuesCommand(
	TEXT("Hydra.ReloadValues"),
	TEXT("Reloads data/values.json and the definition files on the server and on all clients."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UHydraValuesSubsystem::ReloadEverywhere(World);
	}));
