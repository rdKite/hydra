#pragma once

#include "CoreMinimal.h"
#include "Data/HydraDefinitions.h"
#include "Data/HydraValues.h"

class FJsonObject;

/**
 * Reads the JSON files in data/ into the structs of HydraValues.h and HydraDefinitions.h.
 * Shared by the runtime subsystem and the automation tests, so both check the same thing.
 */
namespace HydraDataLoader
{
	/**
	 * Folder the data files are read from, in this order:
	 *   -HydraDataDir=<path>         (tests, later mod data)
	 *   <project>/data/              (dev checkout: edits apply with Hydra.ReloadValues)
	 *   <project>/Config/Hydra/      (copied there by the build, used by packaged builds)
	 */
	HYDRA_API FString FindDataDir();

	/** Parses <Dir>/<FileName> into Struct. Problems (also keys that match nothing) go to OutProblems. */
	HYDRA_API bool LoadFile(const FString& Dir, const TCHAR* FileName, const UStruct* Struct, void* OutStruct, TArray<FString>& OutProblems);

	template <typename T>
	bool LoadFile(const FString& Dir, const TCHAR* FileName, T& Out, TArray<FString>& OutProblems)
	{
		return LoadFile(Dir, FileName, T::StaticStruct(), &Out, OutProblems);
	}

	HYDRA_API bool LoadValues(const FString& Dir, FHydraValues& Out, TArray<FString>& OutProblems);

	/** Loads the four definition files and validates IDs and references. */
	HYDRA_API bool LoadDefinitions(const FString& Dir, FHydraDefinitions& Out, TArray<FString>& OutProblems);

	/** JSON keys without a matching struct member (the game would silently ignore them). Keys starting with "_" are comments. */
	HYDRA_API void FindUnmappedKeys(const TSharedPtr<FJsonObject>& Json, const UStruct* Struct, const FString& Prefix, TArray<FString>& OutKeys);

	/** Struct members without a JSON key (the game would silently run with 0). */
	HYDRA_API void FindMissingKeys(const TSharedPtr<FJsonObject>& Json, const UStruct* Struct, const FString& Prefix, TArray<FString>& OutKeys);

	/** IDs: non-empty, lower snake_case, unique per file; references between files resolve. */
	HYDRA_API void ValidateDefinitions(const FHydraDefinitions& Defs, TArray<FString>& OutProblems);

	HYDRA_API bool IsValidId(FName Id);
}
