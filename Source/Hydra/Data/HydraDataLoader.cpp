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
		else if (const FMapProperty* MapProp = CastField<FMapProperty>(Prop))
		{
			// A map keyed by ID (values.json: forms, weapons, shields): check every entry.
			const FStructProperty* Inner = CastField<FStructProperty>(MapProp->ValueProp);
			if (Inner && Value->TryGetObject(Child))
			{
				for (const auto& Entry : (*Child)->Values)
				{
					const TSharedPtr<FJsonObject>* EntryObject = nullptr;
					if (Entry.Value->TryGetObject(EntryObject))
					{
						Visit(*EntryObject, Inner->Struct, Path + TEXT(".") + FString(Entry.Key) + TEXT("."));
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
	bOk &= LoadFile(Dir, TEXT("enemies.json"), Out.enemies, OutProblems);
	if (bOk)
	{
		ValidateDefinitions(Out, OutProblems);
	}
	return bOk;
}

void HydraDataLoader::ValidateValues(const FHydraValues& V, const FHydraDefinitions& Defs, TArray<FString>& OutProblems)
{
	// Every form, weapon and shield needs its numbers; numbers without an ID are a typo.
	for (const FHydraFormDef& F : Defs.forms.forms)
	{
		const FHydraFormValues* N = V.forms.Find(F.id);
		if (!N)
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: no numbers for form '%s' (section forms)"), *F.id.ToString()));
		}
		else if (N->body_weight <= 0.f || N->vitality <= 0.f || N->endurance <= 0.f || N->spirit < 0.f || N->strength <= 0.f || N->dexterity <= 0.f
			|| N->stamina_regen_share_per_s < 0.f || N->mana_regen_share_per_s < 0.f || N->dodge_stamina_cost < 0.f
			|| N->body_radius_m <= 0.f || N->body_height_m < 2.f * N->body_radius_m
			|| N->level_cap <= 0 || N->start_level < 0 || N->start_level > N->level_cap || N->start_skill_level < 0 || N->start_skill_level > N->start_level || N->element_body_damage < 0.f)
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: forms.%s has implausible numbers (body weight, vitality, endurance, strength, dexterity, radius > 0; height >= 2 x radius; start_skill_level <= start_level <= level_cap)"), *F.id.ToString()));
		}
		else if (V.resistance.curve_constant > 0.f)
		{
			// A weakness of more than the curve constant would more than double the damage.
			for (const float R : { N->resistance.slash, N->resistance.pierce, N->resistance.blunt, N->resistance.fire })
			{
				if (R < -V.resistance.curve_constant)
				{
					OutProblems.Add(FString::Printf(TEXT("values.json: forms.%s has a weakness below -resistance.curve_constant"), *F.id.ToString()));
				}
			}
		}
	}
	for (const FHydraWeaponDef& W : Defs.weapons.weapons)
	{
		if (W.blocks)
		{
			const FHydraShieldValues* N = V.shields.Find(W.id);
			if (!N)
			{
				OutProblems.Add(FString::Printf(TEXT("values.json: no numbers for shield '%s' (section shields)"), *W.id.ToString()));
			}
			else if (N->weight <= 0.f || N->block_half_arc_deg <= 0.f || N->block_half_arc_deg > 180.f || N->block_damage_reduction < 0.f || N->block_damage_reduction > 1.f)
			{
				OutProblems.Add(FString::Printf(TEXT("values.json: shields.%s has implausible numbers"), *W.id.ToString()));
			}
		}
		else
		{
			const FHydraWeaponValues* N = V.weapons.Find(W.id);
			if (!N)
			{
				OutProblems.Add(FString::Printf(TEXT("values.json: no numbers for weapon '%s' (section weapons)"), *W.id.ToString()));
			}
			else if (N->weight < 0.f || (N->weight <= 0.f && N->cost_ratio <= 0.f) || N->demand <= 0.f || N->strength_share < 0.f || N->strength_share > 1.f
				|| N->light.damage <= 0.f || N->heavy.damage <= 0.f || N->light.reach_m <= 0.f || N->heavy.reach_m <= 0.f
				|| N->heavy.max_charge_ms < N->heavy.min_windup_ms || N->heavy.min_windup_ms < N->light.windup_ms)
			{
				OutProblems.Add(FString::Printf(TEXT("values.json: weapons.%s has implausible numbers (a weight or a cost_ratio; demand, damage, reach > 0; strength_share in 0..1; light windup <= heavy min_windup <= max_charge)"), *W.id.ToString()));
			}
		}
	}
	for (const TPair<FName, FHydraFormValues>& Pair : V.forms)
	{
		if (!Defs.FindForm(Pair.Key))
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: forms.%s is not a form in forms.json"), *Pair.Key.ToString()));
		}
	}
	for (const TPair<FName, FHydraWeaponValues>& Pair : V.weapons)
	{
		const FHydraWeaponDef* W = Defs.FindWeapon(Pair.Key);
		if (!W || W->blocks)
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: weapons.%s is not a weapon in weapons.json"), *Pair.Key.ToString()));
		}
	}
	for (const TPair<FName, FHydraShieldValues>& Pair : V.shields)
	{
		const FHydraWeaponDef* W = Defs.FindWeapon(Pair.Key);
		if (!W || !W->blocks)
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: shields.%s is not a shield in weapons.json"), *Pair.Key.ToString()));
		}
	}
	// Enemy types and waves.
	for (const FHydraEnemyDef& E : Defs.enemies.enemies)
	{
		const FHydraEnemyValues* N = V.enemies.Find(E.id);
		if (!N)
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: no numbers for enemy '%s' (section enemies)"), *E.id.ToString()));
		}
		else if (N->level < 0 || N->skill_level < 0 || N->nxp_base < 0.f || N->hxp_factor < 1.f || N->ai.error_rate < 0.f || N->ai.error_rate > 1.f
			|| N->ai.block_chance < 0.f || N->ai.dodge_chance < 0.f || N->ai.block_chance + N->ai.dodge_chance > 1.f || N->ai.reaction_time_ms < 0.f || N->ai.engage_range_m <= 0.f)
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: enemies.%s has implausible numbers (level, nxp >= 0; hxp_factor >= 1; chances in 0..1, block + dodge <= 1; engage range > 0)"), *E.id.ToString()));
		}
	}
	for (const TPair<FName, FHydraEnemyValues>& Pair : V.enemies)
	{
		if (!Defs.FindEnemy(Pair.Key))
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: enemies.%s is not an enemy in enemies.json"), *Pair.Key.ToString()));
		}
	}
	for (int32 i = 0; i < V.waves.list.Num(); ++i)
	{
		if (V.waves.list[i].groups.Num() == 0)
		{
			OutProblems.Add(FString::Printf(TEXT("values.json: waves.list[%d] has no enemies"), i));
		}
		for (const FHydraWaveGroupValues& G : V.waves.list[i].groups)
		{
			if (!Defs.FindEnemy(G.enemy) || G.count <= 0)
			{
				OutProblems.Add(FString::Printf(TEXT("values.json: waves.list[%d] refers to unknown enemy '%s' or has a count <= 0"), i, *G.enemy.ToString()));
			}
		}
	}
	if (V.progression.xp_base <= 0.f || V.progression.level_growth < 0.f || V.progression.milestone_every_levels <= 0 || V.skill_points.base < 0.f || V.skill_points.factor < 0.f
		|| V.imprint.max_level <= 0 || V.imprint.progress_first_level <= 0.f || V.imprint.progress_per_further_level < V.imprint.progress_first_level
		|| V.resistance.curve_constant <= 0.f || V.interrupt.threshold_share_of_max_hp <= 0.f || V.block.share_min > V.block.share_max || V.block.reference_shield_ratio <= 0.f
		|| V.damage.efficiency_base <= 0.f || V.damage.efficiency_reference_kg <= 0.f)
	{
		OutProblems.Add(TEXT("values.json: progression, skill_points, imprint, resistance, interrupt, block or damage has implausible numbers (xp_base, milestone_every_levels, max_level, curve_constant, interrupt share, efficiency > 0; share_min <= share_max; the first imprint level must be the easiest)"));
	}
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
	CheckIds(Defs.enemies.enemies, TEXT("enemies.json"), OutProblems);

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
		for (const FName Evolution : F.evolutions)
		{
			if (!Defs.FindForm(Evolution))
			{
				OutProblems.Add(FString::Printf(TEXT("forms.json: '%s' evolves into unknown form '%s'"), *F.id.ToString(), *Evolution.ToString()));
			}
		}
		for (const FName Element : { F.requires_imprint, F.element_body })
		{
			if (!Element.IsNone() && !Defs.FindElement(Element))
			{
				OutProblems.Add(FString::Printf(TEXT("forms.json: '%s' refers to unknown element '%s'"), *F.id.ToString(), *Element.ToString()));
			}
		}
		if (!F.model.IsNone() && !Defs.FindForm(F.model))
		{
			OutProblems.Add(FString::Printf(TEXT("forms.json: '%s' uses the model of unknown form '%s'"), *F.id.ToString(), *F.model.ToString()));
		}
	}
	for (const FHydraEnemyDef& E : Defs.enemies.enemies)
	{
		if (!Defs.FindForm(E.form))
		{
			OutProblems.Add(FString::Printf(TEXT("enemies.json: '%s' refers to unknown form '%s'"), *E.id.ToString(), *E.form.ToString()));
		}
		if (!E.imprint.IsNone() && !Defs.FindElement(E.imprint))
		{
			OutProblems.Add(FString::Printf(TEXT("enemies.json: '%s' refers to unknown element '%s'"), *E.id.ToString(), *E.imprint.ToString()));
		}
	}
}
