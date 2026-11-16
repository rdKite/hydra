#pragma once

#include "CoreMinimal.h"
#include "Data/HydraDefinitions.h"
#include "Data/HydraValues.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HydraValuesSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHydraValuesReloaded, const FHydraValues&);

/**
 * Holds the gameplay data of this process: data/values.json (all numbers) and the definition
 * files (forms, skills, elements, weapons). Loaded at startup and again by the console
 * command Hydra.ReloadValues, without recompiling.
 *
 * A reload applies to new actions; effects that are already running keep their magnitudes.
 * If a file cannot be read, the old data stays in place and the error is logged.
 */
UCLASS()
class HYDRA_API UHydraValuesSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Re-reads all data files of this process. Returns false (and keeps the old data) on error. */
	bool Reload();

	/**
	 * Hydra.ReloadValues: the server reloads and tells every client to reload its own copy
	 * (clients use values for prediction, movement and UI). A client asks the server first.
	 */
	static void ReloadEverywhere(UWorld* World);

	const FHydraValues& GetValues() const { return Values; }
	const FHydraDefinitions& GetDefinitions() const { return Definitions; }
	const FString& GetLoadedDir() const { return LoadedDir; }

	/** Values from any world context object. Never fails; all zero if there is no game instance. */
	static const FHydraValues& Get(const UObject* WorldContext);
	static UHydraValuesSubsystem* GetSubsystem(const UObject* WorldContext);

	/** Read from this folder instead of the default lookup (tests). */
	void SetDataDirOverride(const FString& Dir) { DataDirOverride = Dir; }

	FOnHydraValuesReloaded OnReloaded;

private:
	FHydraValues Values;
	FHydraDefinitions Definitions;
	FString LoadedDir;
	FString DataDirOverride;
};
