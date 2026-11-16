#pragma once

#include "CoreMinimal.h"
#include "HydraDefinitions.generated.h"

// Mirrors data/elements.json, data/skills.json, data/weapons.json and data/forms.json:
// what exists in the game, each entry with a stable ID. IDs are lower snake_case, never
// change once used (saves, mods and other data files refer to them) and are unique per
// file. Numbers stay in data/values.json; these files hold identity and relations only.
// Member names are snake_case to map the JSON keys 1:1 (see HydraValues.h).

USTRUCT()
struct FHydraElementDef
{
	GENERATED_BODY()
	UPROPERTY() FName id;
	UPROPERTY() FString name;
	/** Debuff this element applies (GLOSSARY: fire -> burning). */
	UPROPERTY() FName debuff;
	/** Skill of casting this element (skills.json). */
	UPROPERTY() FName element_skill;
};

USTRUCT()
struct FHydraSkillDef
{
	GENERATED_BODY()
	UPROPERTY() FName id;
	UPROPERTY() FString name;
	/** "weapon_family", "movement" or "element". */
	UPROPERTY() FName kind;
};

USTRUCT()
struct FHydraWeaponDef
{
	GENERATED_BODY()
	UPROPERTY() FName id;
	UPROPERTY() FString name;
	/** Weapon family: "sword", "shield". */
	UPROPERTY() FName family;
	/** Skill that grows by using it (skills.json). */
	UPROPERTY() FName skill;
	/** Damage type of the light / heavy attack: "slash", "pierce", "blunt"; empty = none (shield). */
	UPROPERTY() FName light_damage_type;
	UPROPERTY() FName heavy_damage_type;
	/** True for items whose hold action is the directional block. */
	UPROPERTY() bool blocks = false;
};

USTRUCT()
struct FHydraFormDef
{
	GENERATED_BODY()
	UPROPERTY() FName id;
	UPROPERTY() FString name;
	/** Line the form belongs to: "humanoid", "slime". */
	UPROPERTY() FName line;
	UPROPERTY() bool starting_form = false;
	/** Fixed loadout (weapons.json IDs); empty for monsters, which fight with the body. */
	UPROPERTY() TArray<FName> loadout;
	/** Skills this form can use (skills.json IDs). */
	UPROPERTY() TArray<FName> skills;
};

USTRUCT()
struct FHydraElementsFile
{
	GENERATED_BODY()
	UPROPERTY() TArray<FHydraElementDef> elements;
};

USTRUCT()
struct FHydraSkillsFile
{
	GENERATED_BODY()
	UPROPERTY() TArray<FHydraSkillDef> skills;
};

USTRUCT()
struct FHydraWeaponsFile
{
	GENERATED_BODY()
	UPROPERTY() TArray<FHydraWeaponDef> weapons;
};

USTRUCT()
struct FHydraFormsFile
{
	GENERATED_BODY()
	UPROPERTY() TArray<FHydraFormDef> forms;
};

/** All definition files together. */
USTRUCT()
struct FHydraDefinitions
{
	GENERATED_BODY()
	UPROPERTY() FHydraElementsFile elements;
	UPROPERTY() FHydraSkillsFile skills;
	UPROPERTY() FHydraWeaponsFile weapons;
	UPROPERTY() FHydraFormsFile forms;

	const FHydraElementDef* FindElement(FName Id) const { return elements.elements.FindByPredicate([Id](const FHydraElementDef& D) { return D.id == Id; }); }
	const FHydraSkillDef* FindSkill(FName Id) const { return skills.skills.FindByPredicate([Id](const FHydraSkillDef& D) { return D.id == Id; }); }
	const FHydraWeaponDef* FindWeapon(FName Id) const { return weapons.weapons.FindByPredicate([Id](const FHydraWeaponDef& D) { return D.id == Id; }); }
	const FHydraFormDef* FindForm(FName Id) const { return forms.forms.FindByPredicate([Id](const FHydraFormDef& D) { return D.id == Id; }); }
};
