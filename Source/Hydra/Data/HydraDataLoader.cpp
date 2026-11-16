#include "Data/HydraDataLoader.h"

#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FString HydraDataLoader::FindDataDir()
{
	FString Override;
	if (FParse::Value(FCommandLine::Get(), TEXT("HydraDataDir="), Override) && FPaths::DirectoryExists(Override))
	{
		return FPaths::ConvertRelativePathToFull(Override);
	}
	const FString Candidates[] = {
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("data")),
		FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("Hydra")),
	};
	for (const FString& Dir : Candidates)
	{
		if (FPaths::FileExists(Dir / TEXT("values.json")))
		{
			return Dir;
		}
	}
	return FString();
}

namespace
{
	/** Calls Visit(ChildObject, ChildStruct, ChildPrefix) for a struct member or each entry of a struct array. */
	template <typename TVisit>
	void VisitChildren(const FProperty* Prop, const TSharedPtr<FJsonValue>& Value, const FString& Path, TVisit Visit)
	{
		const TSharedPtr<FJsonObject>* Child = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			if (Value->TryGetObject(Child))
			{
				Visit(*Child, StructProp->Struct, Path + TEXT("."));
			}
		}
		else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		{
			const FStructProperty* Inner = CastField<FStructProperty>(ArrayProp->Inner);
			if (Inner && Value->TryGetArray(Items))
			{
				for (int32 i = 0; i < Items->Num(); ++i)
				{
					if ((*Items)[i]->TryGetObject(Child))
					{
						Visit(*Child, Inner->Struct, FString::Printf(TEXT("%s[%d]."), *Path, i));
					}
				}
			}
		}
	}
}

void HydraDataLoader::FindUnmappedKeys(const TSharedPtr<FJsonObject>& Json, const UStruct* Struct, const FString& Prefix, TArray<FString>& OutKeys)
{
	for (const auto& Pair : Json->Values)
	{
		const FString Key(Pair.Key);
		if (Key.StartsWith(TEXT("_")))
		{
			continue; // comments such as "_note"
		}
		const FProperty* Prop = Struct->FindPropertyByName(FName(*Key));
		if (!Prop)
		{
			OutKeys.Add(Prefix + Key);
			continue;
		}
		VisitChildren(Prop, Pair.Value, Prefix + Key, [&OutKeys](const TSharedPtr<FJsonObject>& Child, const UStruct* ChildStruct, const FString& ChildPrefix)
		{
			FindUnmappedKeys(Child, ChildStruct, ChildPrefix, OutKeys);
		});
	}
}

void HydraDataLoader::FindMissingKeys(const TSharedPtr<FJsonObject>& Json, const UStruct* Struct, const FString& Prefix, TArray<FString>& OutKeys)
{
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FString Key = It->GetName();
		const TSharedPtr<FJsonValue> Value = Json->TryGetField(Key);
		if (!Value.IsValid())
		{
			OutKeys.Add(Prefix + Key);
			continue;
		}
		VisitChildren(*It, Value, Prefix + Key, [&OutKeys](const TSharedPtr<FJsonObject>& Child, const UStruct* ChildStruct, const FString& ChildPrefix)
		{
			FindMissingKeys(Child, ChildStruct, ChildPrefix, OutKeys);
		});
	}
}

bool HydraDataLoader::LoadFile(const FString& Dir, const TCHAR* FileName, const UStruct* Struct, void* OutStruct, TArray<FString>& OutProblems)
{
	const FString Path = Dir / FileName;
	FString Text;
	if (Dir.IsEmpty() || !FFileHelper::LoadFileToString(Text, *Path))
	{
		OutProblems.Add(FString::Printf(TEXT("%s: file not found (%s)"), FileName, *Path));
		return false;
	}
	TSharedPtr<FJsonObject> Json;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		OutProblems.Add(FString::Printf(TEXT("%s: not valid JSON"), FileName));
		return false;
	}
	if (!FJsonObjectConverter::JsonObjectToUStruct(Json.ToSharedRef(), Struct, OutStruct, 0, 0))
	{
		OutProblems.Add(FString::Printf(TEXT("%s: could not be read into %s"), FileName, *Struct->GetName()));
		return false;
	}
	TArray<FString> Keys;
	FindUnmappedKeys(Json, Struct, FString(), Keys);
	for (const FString& Key : Keys)
	{
		OutProblems.Add(FString::Printf(TEXT("%s: key not read by the game: %s"), FileName, *Key));
	}
	Keys.Reset();
	FindMissingKeys(Json, Struct, FString(), Keys);
	for (const FString& Key : Keys)
	{
		OutProblems.Add(FString::Printf(TEXT("%s: key missing (the game would use 0): %s"), FileName, *Key));
	}
	return true;
}

bool HydraDataLoader::LoadValues(const FString& Dir, FHydraValues& Out, TArray<FString>& OutProblems)
{
	return LoadFile(Dir, TEXT("values.json"), Out, OutProblems);
}

bool HydraDataLoader::LoadDefinitions(const FString& Dir, FHydraDefinitions& Out, TArray<FString>& OutProblems)
{
	bool bOk = LoadFile(Dir, TEXT("elements.json"), Out.elements, OutProblems);
	bOk &= LoadFile(Dir, TEXT("skills.json"), Out.skills, OutProblems);
	bOk &= LoadFile(Dir, TEXT("weapons.json"), Out.weapons, OutProblems);
	bOk &= LoadFile(Dir, TEXT("forms.json"), Out.forms, OutProblems);
	if (bOk)
	{
		ValidateDefinitions(Out, OutProblems);
	}
	return bOk;
}

bool HydraDataLoader::IsValidId(FName Id)
{
	// FName compares case-insensitively but keeps the spelling it was first created with, so
	// the characters are checked one by one.
	const FString S = Id.ToString();
	if (Id.IsNone() || S.IsEmpty() || !(S[0] >= 'a' && S[0] <= 'z') || S.EndsWith(TEXT("_")))
	{
		return false;
	}
	for (const TCHAR C : S)
	{
		if (!(C >= 'a' && C <= 'z') && !(C >= '0' && C <= '9') && C != '_')
		{
			return false;
		}
	}
	return true;
}

namespace
{
	template <typename T>
	void CheckIds(const TArray<T>& Entries, const TCHAR* File, TArray<FString>& OutProblems)
	{
		TSet<FName> Seen;
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			const FName Id = Entries[i].id;
			if (!HydraDataLoader::IsValidId(Id))
			{
				OutProblems.Add(FString::Printf(TEXT("%s: entry %d has an invalid id '%s' (lower snake_case needed)"), File, i, *Id.ToString()));
			}
			if (Seen.Contains(Id))
			{
				OutProblems.Add(FString::Printf(TEXT("%s: id '%s' is used twice"), File, *Id.ToString()));
			}
			Seen.Add(Id);
			if (Entries[i].name.IsEmpty())
			{
				OutProblems.Add(FString::Printf(TEXT("%s: '%s' has no name"), File, *Id.ToString()));
			}
		}
	}
}

void HydraDataLoader::ValidateDefinitions(const FHydraDefinitions& Defs, TArray<FString>& OutProblems)
{
	CheckIds(Defs.elements.elements, TEXT("elements.json"), OutProblems);
	CheckIds(Defs.skills.skills, TEXT("skills.json"), OutProblems);
	CheckIds(Defs.weapons.weapons, TEXT("weapons.json"), OutProblems);
	CheckIds(Defs.forms.forms, TEXT("forms.json"), OutProblems);

	auto NeedSkill = [&](FName Skill, const TCHAR* File, FName Owner)
	{
		if (!Defs.FindSkill(Skill))
		{
			OutProblems.Add(FString::Printf(TEXT("%s: '%s' refers to unknown skill '%s'"), File, *Owner.ToString(), *Skill.ToString()));
		}
	};
	for (const FHydraElementDef& E : Defs.elements.elements)
	{
		NeedSkill(E.element_skill, TEXT("elements.json"), E.id);
	}
	for (const FHydraWeaponDef& W : Defs.weapons.weapons)
	{
		NeedSkill(W.skill, TEXT("weapons.json"), W.id);
	}
	for (const FHydraFormDef& F : Defs.forms.forms)
	{
		for (const FName Weapon : F.loadout)
		{
			if (!Defs.FindWeapon(Weapon))
			{
				OutProblems.Add(FString::Printf(TEXT("forms.json: '%s' refers to unknown weapon '%s'"), *F.id.ToString(), *Weapon.ToString()));
			}
		}
		for (const FName Skill : F.skills)
		{
			NeedSkill(Skill, TEXT("forms.json"), F.id);
		}
	}
}
