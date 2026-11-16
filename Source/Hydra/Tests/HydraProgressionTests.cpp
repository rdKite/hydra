// Automation tests for the progression rules (level, skills, imprint, instinct) and what
// they do to a character's numbers (HydraCombatMath.h).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/HydraCombatMath.h"
#include "Data/HydraDataLoader.h"
#include "Tests/HydraTestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraProgressionMathTest, "Hydra.Progression.Rules", HydraTests::Flags)
bool FHydraProgressionMathTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	const float Tolerance = 0.001f;

	FHydraProgressionValues P;
	P.xp_base = 100.f;
	P.xp_per_level = 10.f;
	P.nxp_bonus_per_enemy_level = 0.1f;
	P.milestone_every_levels = 10;
	TestEqual(TEXT("nXP for level 0 -> 1"), XpForNextLevel(P, 0), 100.f, Tolerance);
	TestEqual(TEXT("nXP for level 29 -> 30"), XpForNextLevel(P, 29), 390.f, Tolerance);
	TestEqual(TEXT("nXP of an enemy grows with its level"), EnemyNxp(P, 40.f, 5), 60.f, Tolerance);
	TestEqual(TEXT("level factor"), LevelFactor(0.01f, 29), 1.29f, Tolerance);
	TestTrue(TEXT("level 10 is a milestone"), IsMilestone(P, 10, 50));
	TestFalse(TEXT("level 11 is not"), IsMilestone(P, 11, 50));
	TestFalse(TEXT("level 0 is not"), IsMilestone(P, 0, 50));
	TestTrue(TEXT("the level cap is a milestone"), IsMilestone(P, 37, 37));

	FHydraSkillsValues S;
	S.xp_base = 20.f;
	S.xp_per_level = 4.f;
	S.damage_bonus_per_level = 0.01f;
	S.cost_reduction_per_level = 0.01f;
	S.min_cost_factor = 0.5f;
	TestEqual(TEXT("skill XP for skill level 6 -> 7"), SkillXpForNextLevel(S, 6), 44.f, Tolerance);
	TestEqual(TEXT("skill 20: a fifth more damage"), SkillDamageFactor(S, 20.f), 1.2f, Tolerance);
	TestEqual(TEXT("skill 20: a fifth less cost"), SkillCostFactor(S, 20.f), 0.8f, Tolerance);
	TestEqual(TEXT("the cost never drops below the floor"), SkillCostFactor(S, 90.f), 0.5f, Tolerance);

	FHydraImprintValues I;
	I.progress_first_level = 30.f;
	I.progress_per_further_level = 60.f;
	TestEqual(TEXT("the first imprint level is the easiest"), ImprintProgressNeeded(I, 0), 30.f, Tolerance);
	TestEqual(TEXT("later levels need more"), ImprintProgressNeeded(I, 2), 60.f, Tolerance);

	FHydraInstinctValues Instinct;
	Instinct.skill_level_bonus_max = 3.f;
	Instinct.xp_bonus_max = 0.5f;
	TestEqual(TEXT("instinct: skill 20 of cap 30"), InstinctRatio(20, 30), 2.f / 3.f, Tolerance);
	TestEqual(TEXT("instinct is capped at 1"), InstinctRatio(45, 30), 1.f, Tolerance);
	TestEqual(TEXT("instinct adds skill levels"), EffectiveSkillLevel(Instinct, 0, 2.f / 3.f), 2.f, Tolerance);
	TestEqual(TEXT("no instinct: the plain level"), EffectiveSkillLevel(Instinct, 7, 0.f), 7.f, Tolerance);
	TestEqual(TEXT("a skill with an instinct is relearned faster"), SkillXpGain(Instinct, 10.f, 1.f), 15.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraProgressionStatsTest, "Hydra.Progression.StatsFromProgress", HydraTests::Flags)
bool FHydraProgressionStatsTest::RunTest(const FString& Parameters)
{
	// With the real data files: level, skills and imprint change the numbers the way the rules say.
	using namespace HydraCombatMath;
	FHydraValues V;
	FHydraDefinitions Defs;
	TArray<FString> Problems;
	if (!TestTrue(TEXT("data loads"), HydraDataLoader::LoadValues(HydraTests::DataDir(), V, Problems) && HydraDataLoader::LoadDefinitions(HydraTests::DataDir(), Defs, Problems)))
	{
		return false;
	}
	const float Tolerance = 0.01f;
	const FHydraCombatStats Plain = BuildStats(Defs, V, TEXT("humanoid"));
	const FHydraWeaponValues& Sword = V.weapons[TEXT("longsword")];
	TestEqual(TEXT("no progress: the weapon's plain damage"), Plain.Weapon.light.damage, Sword.light.damage, Tolerance);
	TestEqual(TEXT("no progress: the form's plain HP"), Plain.HpMax, V.forms[TEXT("humanoid")].hp_max, Tolerance);

	FHydraProgressView Grown;
	Grown.Level = 10;
	Grown.ImprintLevel = 2;
	Grown.SkillLevels.Add(TEXT("sword_fighting"), 8.f);
	Grown.SkillLevels.Add(TEXT("shield_fighting"), 4.f);
	Grown.SkillLevels.Add(TEXT("dodge"), 5.f);
	const FHydraCombatStats S = BuildStats(Defs, V, TEXT("humanoid"), Grown);
	const float LevelDamage = 1.f + V.progression.damage_bonus_per_level * 10;
	TestEqual(TEXT("level: more HP"), S.HpMax, Plain.HpMax * (1.f + V.progression.hp_bonus_per_level * 10), Tolerance);
	TestEqual(TEXT("level and sword skill: more damage"), S.Weapon.light.damage, Sword.light.damage * LevelDamage * (1.f + V.skills.damage_bonus_per_level * 8), Tolerance);
	TestEqual(TEXT("sword skill: cheaper attacks"), S.LightStaminaCost, Plain.LightStaminaCost * (1.f - V.skills.cost_reduction_per_level * 8), Tolerance);
	TestEqual(TEXT("shield skill: wider block arc"), S.Shield.block_half_arc_deg, Plain.Shield.block_half_arc_deg + V.skills.block_arc_deg_per_level * 4, Tolerance);
	TestEqual(TEXT("shield skill: cheaper blocks"), S.BlockCostFactor, 1.f - V.skills.cost_reduction_per_level * 4, Tolerance);
	TestEqual(TEXT("dodge skill: longer step"), S.DodgeDistanceFactor, 1.f + V.skills.dodge_distance_bonus_per_level * 5, Tolerance);
	TestEqual(TEXT("dodge skill: cheaper step"), S.DodgeStaminaCost, Plain.DodgeStaminaCost * (1.f - V.skills.cost_reduction_per_level * 5), Tolerance);
	TestEqual(TEXT("imprint: fire resistance"), S.Resistance.fire, Plain.Resistance.fire + V.imprint.resistance_per_level * 2, Tolerance);
	TestEqual(TEXT("imprint: more fire damage"), S.FireDamageFactor, 1.f + V.imprint.damage_bonus_per_level * 2, Tolerance);
	TestEqual(TEXT("the weight rules stay as they are"), S.TurnRateDegS, Plain.TurnRateDegS, Tolerance);

	// The slime line: the bolt grows with the projectile skill; the evolution has an element body.
	FHydraProgressView Slime;
	Slime.SkillLevels.Add(TEXT("projectile"), 10.f);
	const FHydraCombatStats Caster = BuildStats(Defs, V, TEXT("slime"), Slime);
	TestEqual(TEXT("projectile skill: stronger bolt"), Caster.Bolt.damage, V.bolt.damage * (1.f + V.skills.damage_bonus_per_level * 10), Tolerance);
	TestEqual(TEXT("projectile skill: cheaper bolt"), Caster.Bolt.mana_cost, V.bolt.mana_cost * (1.f - V.skills.cost_reduction_per_level * 10), Tolerance);
	TestEqual(TEXT("the slime has no element body"), Caster.ElementBodyDamage, 0.f, Tolerance);
	const FHydraCombatStats Elemental = BuildStats(Defs, V, TEXT("fire_elemental_slime"));
	TestTrue(TEXT("the fire elemental slime exists and casts"), Elemental.bValid && Elemental.bCanCast);
	TestTrue(TEXT("... has an element body"), Elemental.ElementBodyDamage > 0.f);
	TestTrue(TEXT("... keeps the raised physical resistance"), Elemental.Resistance.slash >= Caster.Resistance.slash && Elemental.Resistance.slash > 0.f);
	TestTrue(TEXT("... has higher base values than the slime"), Elemental.HpMax > BuildStats(Defs, V, TEXT("slime")).HpMax);

	// The arena: the slime starts so close to its cap that the evolution can be tested at once,
	// and the first waves give enough nXP for a level-up.
	const FHydraFormValues& SlimeForm = V.forms[TEXT("slime")];
	TestTrue(TEXT("slime starts shortly before its level cap"), SlimeForm.start_level < SlimeForm.level_cap && SlimeForm.level_cap - SlimeForm.start_level <= 3);
	float Nxp = 0.f;
	for (int32 i = 0; i < FMath::Min(3, V.waves.list.Num()); ++i)
	{
		for (const FHydraWaveGroupValues& Group : V.waves.list[i].groups)
		{
			if (const FHydraEnemyValues* Enemy = V.enemies.Find(Group.enemy))
			{
				Nxp += Group.count * EnemyNxp(V.progression, Enemy->nxp_base, Enemy->level);
			}
		}
	}
	TestTrue(TEXT("three waves level the slime up"), Nxp >= XpForNextLevel(V.progression, SlimeForm.start_level));
	TestTrue(TEXT("three waves level the humanoid up"), Nxp >= XpForNextLevel(V.progression, V.forms[TEXT("humanoid")].start_level));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
