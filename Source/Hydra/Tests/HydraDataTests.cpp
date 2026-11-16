// Automation tests for the data files. Run from the command line with Scripts/run_tests.sh.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Data/HydraDataLoader.h"
#include "Data/HydraValuesSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/HydraTestHelpers.h"

namespace HydraTests
{
	TSharedPtr<FJsonObject> ReadJson(const TCHAR* FileName)
	{
		FString Text;
		TSharedPtr<FJsonObject> Json;
		if (FFileHelper::LoadFileToString(Text, *(DataDir() / FileName)))
		{
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json);
		}
		return Json;
	}

	struct FDataFile
	{
		const TCHAR* Name;
		const UStruct* Struct;
	};

	TArray<FDataFile> AllFiles()
	{
		return {
			{ TEXT("values.json"), FHydraValues::StaticStruct() },
			{ TEXT("elements.json"), FHydraElementsFile::StaticStruct() },
			{ TEXT("skills.json"), FHydraSkillsFile::StaticStruct() },
			{ TEXT("weapons.json"), FHydraWeaponsFile::StaticStruct() },
			{ TEXT("forms.json"), FHydraFormsFile::StaticStruct() },
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraValuesLoadTest, "Hydra.Values.Load", HydraTests::Flags)
bool FHydraValuesLoadTest::RunTest(const FString& Parameters)
{
	FHydraValues V;
	TArray<FString> Problems;
	if (!TestTrue(TEXT("data/values.json loads"), HydraDataLoader::LoadValues(HydraTests::DataDir(), V, Problems)))
	{
		return false;
	}
	TestTrue(TEXT("version > 0"), V.version > 0);
	TestTrue(TEXT("stamina.max > 0"), V.stamina.max > 0.f);
	TestTrue(TEXT("stamina.regen_per_s > 0"), V.stamina.regen_per_s > 0.f);
	TestTrue(TEXT("stamina.regen_delay_after_action_ms >= 0"), V.stamina.regen_delay_after_action_ms >= 0.f);
	TestTrue(TEXT("run speed above walk speed"), V.movement.run_speed_m_s > V.movement.walk_speed_m_s);
	TestTrue(TEXT("turn rate min <= base <= max"), V.turn_rate.min_deg_s <= V.turn_rate.base_deg_s && V.turn_rate.base_deg_s <= V.turn_rate.max_deg_s);
	TestTrue(TEXT("lock-on breaks beyond its range"), V.lock_on.break_range_m >= V.lock_on.range_m);
	TestTrue(TEXT("aim assist strength in 0..1"), V.aim_assist.strength >= 0.f && V.aim_assist.strength <= 1.f);
	TestTrue(TEXT("latency steps present"), V.net_simulation.latency_steps_ms.Num() > 0);
	TestTrue(TEXT("packet loss ratio in 0..1"), V.net_simulation.packet_loss_ratio >= 0.f && V.net_simulation.packet_loss_ratio < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraAllKeysMappedTest, "Hydra.Values.AllKeysMapped", HydraTests::Flags)
bool FHydraAllKeysMappedTest::RunTest(const FString& Parameters)
{
	// Every key in a data file must have a member in its struct - otherwise a value added to
	// the file would silently be ignored by the game.
	for (const HydraTests::FDataFile& File : HydraTests::AllFiles())
	{
		const TSharedPtr<FJsonObject> Json = HydraTests::ReadJson(File.Name);
		if (!Json.IsValid())
		{
			AddError(FString::Printf(TEXT("data/%s missing or invalid"), File.Name));
			continue;
		}
		TArray<FString> Keys;
		HydraDataLoader::FindUnmappedKeys(Json, File.Struct, FString(), Keys);
		for (const FString& Key : Keys)
		{
			AddError(FString::Printf(TEXT("data/%s: key not read by the game: %s"), File.Name, *Key));
		}
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraAllFieldsPresentTest, "Hydra.Values.AllFieldsPresent", HydraTests::Flags)
bool FHydraAllFieldsPresentTest::RunTest(const FString& Parameters)
{
	// The other direction: every struct member must have a key - otherwise the game would
	// silently run with 0 for it.
	for (const HydraTests::FDataFile& File : HydraTests::AllFiles())
	{
		const TSharedPtr<FJsonObject> Json = HydraTests::ReadJson(File.Name);
		if (!Json.IsValid())
		{
			AddError(FString::Printf(TEXT("data/%s missing or invalid"), File.Name));
			continue;
		}
		TArray<FString> Keys;
		HydraDataLoader::FindMissingKeys(Json, File.Struct, FString(), Keys);
		for (const FString& Key : Keys)
		{
			AddError(FString::Printf(TEXT("data/%s: key missing: %s"), File.Name, *Key));
		}
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraKeyCheckTest, "Hydra.Values.KeyCheckFindsProblems", HydraTests::Flags)
bool FHydraKeyCheckTest::RunTest(const FString& Parameters)
{
	// The two checks above are only worth something if they really find stray and missing keys.
	TSharedPtr<FJsonObject> Json;
	const FString Text = TEXT("{ \"_note\": \"x\", \"max\": 1, \"regen_per_s\": 2, \"typo_key\": 3 }");
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json);
	TArray<FString> Unmapped, Missing;
	HydraDataLoader::FindUnmappedKeys(Json, FHydraStaminaValues::StaticStruct(), TEXT("stamina."), Unmapped);
	HydraDataLoader::FindMissingKeys(Json, FHydraStaminaValues::StaticStruct(), TEXT("stamina."), Missing);
	TestEqual(TEXT("one stray key"), Unmapped.Num(), 1);
	TestTrue(TEXT("stray key named"), Unmapped.Contains(TEXT("stamina.typo_key")));
	TestEqual(TEXT("one missing key"), Missing.Num(), 1);
	TestTrue(TEXT("missing key named"), Missing.Contains(TEXT("stamina.regen_delay_after_action_ms")));

	// Entries of arrays are checked too (definition files).
	const FString ArrayText = TEXT("{ \"skills\": [ { \"id\": \"dodge\", \"name\": \"Dodge\", \"kind\": \"movement\", \"oops\": 1 } ] }");
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ArrayText), Json);
	Unmapped.Reset();
	HydraDataLoader::FindUnmappedKeys(Json, FHydraSkillsFile::StaticStruct(), FString(), Unmapped);
	TestTrue(TEXT("stray key inside an array entry named"), Unmapped.Contains(TEXT("skills[0].oops")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraDefinitionsTest, "Hydra.Data.Definitions", HydraTests::Flags)
bool FHydraDefinitionsTest::RunTest(const FString& Parameters)
{
	FHydraDefinitions Defs;
	TArray<FString> Problems;
	TestTrue(TEXT("definition files load"), HydraDataLoader::LoadDefinitions(HydraTests::DataDir(), Defs, Problems));
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}
	// First entries of M1.
	TestNotNull(TEXT("form humanoid"), Defs.FindForm(TEXT("humanoid")));
	TestNotNull(TEXT("form slime"), Defs.FindForm(TEXT("slime")));
	TestNotNull(TEXT("weapon longsword"), Defs.FindWeapon(TEXT("longsword")));
	TestNotNull(TEXT("weapon round_shield"), Defs.FindWeapon(TEXT("round_shield")));
	TestNotNull(TEXT("element fire"), Defs.FindElement(TEXT("fire")));
	if (const FHydraFormDef* Humanoid = Defs.FindForm(TEXT("humanoid")))
	{
		TestEqual(TEXT("humanoid carries two items"), Humanoid->loadout.Num(), 2);
	}
	if (const FHydraWeaponDef* Sword = Defs.FindWeapon(TEXT("longsword")))
	{
		TestEqual(TEXT("longsword light = slash"), Sword->light_damage_type, FName(TEXT("slash")));
		TestEqual(TEXT("longsword heavy = pierce"), Sword->heavy_damage_type, FName(TEXT("pierce")));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraDefinitionsValidationTest, "Hydra.Data.ValidationFindsProblems", HydraTests::Flags)
bool FHydraDefinitionsValidationTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("snake_case id accepted"), HydraDataLoader::IsValidId(TEXT("fire_elemental_slime")));
	TestFalse(TEXT("empty id rejected"), HydraDataLoader::IsValidId(NAME_None));
	TestFalse(TEXT("space rejected"), HydraDataLoader::IsValidId(TEXT("round shield")));
	TestFalse(TEXT("leading digit rejected"), HydraDataLoader::IsValidId(TEXT("1sword")));

	FHydraDefinitions Defs;
	FHydraSkillDef Skill;
	Skill.id = TEXT("sword_fighting");
	Skill.name = TEXT("Sword fighting");
	Defs.skills.skills.Add(Skill);
	Defs.skills.skills.Add(Skill);                       // duplicate id
	FHydraWeaponDef Weapon;
	Weapon.id = TEXT("longsword");
	Weapon.name = TEXT("Longsword");
	Weapon.skill = TEXT("no_such_skill");                // broken reference
	Defs.weapons.weapons.Add(Weapon);
	FHydraFormDef Form;
	Form.id = TEXT("humanoid");
	Form.name = TEXT("Humanoid");
	Form.loadout.Add(TEXT("no_such_weapon"));            // broken reference
	Defs.forms.forms.Add(Form);

	TArray<FString> Problems;
	HydraDataLoader::ValidateDefinitions(Defs, Problems);
	TestEqual(TEXT("three problems found"), Problems.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraValuesReloadTest, "Hydra.Values.Reload", HydraTests::Flags)
bool FHydraValuesReloadTest::RunTest(const FString& Parameters)
{
	// Copy data/ to a temp folder, load it through the subsystem, change a value on disk and
	// reload: the new value must be there without a restart. A broken file keeps the old data.
	IFileManager& FM = IFileManager::Get();
	const FString TempDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Tests/ReloadData"));
	FM.DeleteDirectory(*TempDir, false, true);
	FM.MakeDirectory(*TempDir, true);
	for (const HydraTests::FDataFile& File : HydraTests::AllFiles())
	{
		FM.Copy(*(TempDir / File.Name), *(HydraTests::DataDir() / File.Name));
	}

	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	UHydraValuesSubsystem* Subsystem = NewObject<UHydraValuesSubsystem>(GI);
	Subsystem->SetDataDirOverride(TempDir);
	int32 Broadcasts = 0;
	Subsystem->OnReloaded.AddLambda([&Broadcasts](const FHydraValues&) { ++Broadcasts; });

	if (!TestTrue(TEXT("first load"), Subsystem->Reload()))
	{
		return false;
	}
	const float OldMax = Subsystem->GetValues().stamina.max;

	FString Text;
	FFileHelper::LoadFileToString(Text, *(TempDir / TEXT("values.json")));
	TSharedPtr<FJsonObject> Json;
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json);
	Json->GetObjectField(TEXT("stamina"))->SetNumberField(TEXT("max"), OldMax + 23.0);
	FString NewText;
	FJsonSerializer::Serialize(Json.ToSharedRef(), TJsonWriterFactory<>::Create(&NewText));
	FFileHelper::SaveStringToFile(NewText, *(TempDir / TEXT("values.json")));

	TestTrue(TEXT("reload after edit"), Subsystem->Reload());
	TestEqual(TEXT("edited value is live"), Subsystem->GetValues().stamina.max, OldMax + 23.f);
	TestEqual(TEXT("listeners were told twice"), Broadcasts, 2);

	FFileHelper::SaveStringToFile(TEXT("{ this is not json"), *(TempDir / TEXT("values.json")));
	AddExpectedError(TEXT("not valid JSON"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("broken file is rejected"), Subsystem->Reload());
	TestEqual(TEXT("old values kept"), Subsystem->GetValues().stamina.max, OldMax + 23.f);

	Subsystem->OnReloaded.Clear();
	FM.DeleteDirectory(*TempDir, false, true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
