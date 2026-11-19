// Automation tests for the progression rules (level and attributes, hXP and automatic skill
// points, imprint, instinct) and what they do to a character's numbers (HydraCombatMath.h).

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
	P.level_growth = 0.02f;
	P.milestone_every_levels = 10;
	TestEqual(TEXT("nXP for level 0 -> 1"), XpForNextLevel(P, 0), 100.f, Tolerance);
	TestEqual(TEXT("nXP for level 29 -> 30"), XpForNextLevel(P, 29), 390.f, Tolerance);
	TestEqual(TEXT("nXP of an enemy grows with its level"), EnemyNxp(P, 40.f, 5), 60.f, Tolerance);
	TestEqual(TEXT("hXP = nXP x the factor of the enemy type"), EnemyHxp(60.f, 1.5f), 90.f, Tolerance);
	TestEqual(TEXT("... at least the nXP"), EnemyHxp(60.f, 0.2f), 60.f, Tolerance);
	TestEqual(TEXT("attributes grow 2 % of the base per level"), AttributeGain(P, 10), 0.2f, Tolerance);
	TestEqual(TEXT("no gain at level 0"), AttributeGain(P, 0), 0.f, Tolerance);
	TestTrue(TEXT("level 10 is a milestone"), IsMilestone(P, 10, 50));
	TestFalse(TEXT("level 11 is not"), IsMilestone(P, 11, 50));
	TestFalse(TEXT("level 0 is not"), IsMilestone(P, 0, 50));
	TestTrue(TEXT("the level cap is a milestone"), IsMilestone(P, 37, 37));

	// Automatic skill points: 1 + 2 x log2(hardship rating), rating = hXP / nXP, at least 1.
	FHydraSkillPointsValues Points;
	Points.base = 1.f;
	Points.factor = 2.f;
	TestEqual(TEXT("hardship rating"), HardshipRating(150.f, 100.f), 1.5f, Tolerance);
	TestEqual(TEXT("hardship rating is at least 1"), HardshipRating(50.f, 100.f), 1.f, Tolerance);
	TestEqual(TEXT("no nXP yet: rating 1"), HardshipRating(0.f, 0.f), 1.f, Tolerance);
	TestEqual(TEXT("rating 1: one point"), SkillPoints(Points, 1.f), 1.f, Tolerance);
	TestEqual(TEXT("rating 2: three points"), SkillPoints(Points, 2.f), 3.f, Tolerance);
	TestEqual(TEXT("rating 4: five points"), SkillPoints(Points, 4.f), 5.f, Tolerance);

	// Splitting the points: in proportion to the hXP of each skill, fractions stay on the bar.
	{
		TArray<float> Weights = { 30.f, 10.f, 0.f };
		TArray<int32> Levels = { 5, 5, 5 };
		TArray<float> Progress = { 0.f, 0.5f, 0.2f };
		DistributeSkillPoints(2.f, Weights, 20, Levels, Progress);
		TestEqual(TEXT("three quarters of two points: one level and a half"), Levels[0], 6);
		TestEqual(TEXT("... the rest stays on the bar"), Progress[0], 0.5f, Tolerance);
		TestEqual(TEXT("a quarter of two points on a half-full bar: one level"), Levels[1], 6);
		TestEqual(TEXT("... bar empty again"), Progress[1], 0.f, Tolerance);
		TestEqual(TEXT("an unused skill gets nothing"), Levels[2], 5);
		TestEqual(TEXT("... and keeps its bar"), Progress[2], 0.2f, Tolerance);
	}
	// The limit: no skill above the character level; its share goes to the other used skills.
	{
		TArray<float> Weights = { 30.f, 10.f };
		TArray<int32> Levels = { 9, 3 };
		TArray<float> Progress = { 0.f, 0.f };
		DistributeSkillPoints(4.f, Weights, 10, Levels, Progress);
		TestEqual(TEXT("the skill at the limit stops at the character level"), Levels[0], 10);
		TestEqual(TEXT("... with an empty bar"), Progress[0], 0.f, Tolerance);
		TestEqual(TEXT("what it could not take goes to the other used skill"), Levels[1], 6);
		TestEqual(TEXT("... nothing is lost"), Progress[1], 0.f, Tolerance);
	}
	// Nobody else can take it: it waits on the bar for the next level-up.
	{
		TArray<float> Weights = { 10.f };
		TArray<int32> Levels = { 10 };
		TArray<float> Progress = { 0.f };
		DistributeSkillPoints(1.5f, Weights, 10, Levels, Progress);
		TestEqual(TEXT("at the limit: no level"), Levels[0], 10);
		TestEqual(TEXT("the points wait"), Progress[0], 1.5f, Tolerance);
		DistributeSkillPoints(1.f, Weights, 11, Levels, Progress);
		TestEqual(TEXT("the next level-up takes what waited (one level, the limit)"), Levels[0], 11);
		TestEqual(TEXT("... and the rest waits on"), Progress[0], 1.5f, Tolerance);
	}

	FHydraImprintValues I;
	I.progress_first_level = 100.f;
	I.progress_per_further_level = 250.f;
	TestEqual(TEXT("the first imprint level is the easiest"), ImprintProgressNeeded(I, 0), 100.f, Tolerance);
	TestEqual(TEXT("later levels need more"), ImprintProgressNeeded(I, 2), 250.f, Tolerance);

	FHydraInstinctValues Instinct;
	Instinct.skill_level_bonus_max = 3.f;
	Instinct.xp_bonus_max = 0.5f;
	TestEqual(TEXT("instinct: skill 20 of cap 30"), InstinctRatio(20, 30), 2.f / 3.f, Tolerance);
	TestEqual(TEXT("instinct is capped at 1"), InstinctRatio(45, 30), 1.f, Tolerance);
	TestEqual(TEXT("instinct adds skill levels"), EffectiveSkillLevel(Instinct, 0, 2.f / 3.f), 2.f, Tolerance);
	TestEqual(TEXT("no instinct: the plain level"), EffectiveSkillLevel(Instinct, 7, 0.f), 7.f, Tolerance);
	TestEqual(TEXT("a skill with an instinct counts more when points are split"), InstinctShareFactor(Instinct, 1.f), 1.5f, Tolerance);
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
	const FHydraFormValues& Human = V.forms[TEXT("humanoid")];

	FHydraProgressView Grown;
	Grown.Level = 10;
	Grown.ImprintLevel = 2;
	Grown.SkillLevels.Add(TEXT("sword_fighting"), 8.f);
	Grown.SkillLevels.Add(TEXT("shield_fighting"), 4.f);
	const FHydraCombatStats S = BuildStats(Defs, V, TEXT("humanoid"), Grown);
	const float Gain = V.progression.level_growth * 10;
	// The level acts through the attributes only.
	TestEqual(TEXT("attribute gain"), S.AttributeGain, Gain, 0.0001f);
	TestEqual(TEXT("vitality grows: more HP"), S.HpMax, Plain.HpMax * (1.f + Gain), Tolerance);
	TestEqual(TEXT("endurance grows: more stamina"), S.StaminaMax, Plain.StaminaMax * (1.f + Gain), Tolerance);
	TestEqual(TEXT("regeneration keeps its share of the bar"), S.StaminaRegenPerS, S.StaminaMax * Human.stamina_regen_share_per_s, Tolerance);
	TestEqual(TEXT("strength grows: better handling"), S.WeightRatio, Plain.TotalWeight / (Human.strength * (1.f + Gain)), 0.0001f);
	TestTrue(TEXT("... so it turns faster"), S.TurnRateDegS > Plain.TurnRateDegS);
	TestEqual(TEXT("dexterity: bigger weak-point factor"), S.WeakPointMultiplier, V.weak_point.damage_multiplier + V.weak_point.multiplier_per_dexterity_gain * Gain, 0.001f);
	TestEqual(TEXT("dexterity: longer dodge step"), S.DodgeDistanceFactor, 1.f + V.dodge.distance_per_dexterity_gain * Gain, 0.001f);
	TestEqual(TEXT("dexterity: faster dodge step"), S.DodgeDurationFactor, 1.f / (1.f + V.dodge.speed_per_dexterity_gain * Gain), 0.001f);
	TestEqual(TEXT("the interrupt threshold follows the HP"), S.InterruptThreshold, V.interrupt.threshold_share_of_max_hp * S.HpMax, Tolerance);
	// One quantity, one source: neither level nor skill changes a cost.
	TestEqual(TEXT("attack cost unchanged by level and skill"), S.LightStaminaCost, Plain.LightStaminaCost, Tolerance);
	TestEqual(TEXT("dodge cost unchanged by level"), S.DodgeStaminaCost, Plain.DodgeStaminaCost, Tolerance);
	TestEqual(TEXT("block cost share unchanged by skill"), S.BlockerShare, Plain.BlockerShare, 0.0001f);
	TestTrue(TEXT("... so a higher level manages more strikes from a full bar"), S.StaminaMax / S.LightStaminaCost > Plain.StaminaMax / Plain.LightStaminaCost);
	// Damage: progress bonuses are added up - skill plus strength / dexterity by the weapon's share.
	const float AttributeBonus = Sword.strength_share * V.damage.strength_bonus_per_gain * Gain + (1.f - Sword.strength_share) * V.damage.dexterity_bonus_per_gain * Gain;
	TestEqual(TEXT("damage = base x (1 + skill bonus + attribute bonus)"), S.Weapon.light.damage, Sword.light.damage * (1.f + V.skills.damage_bonus_per_level * 8 + AttributeBonus), Tolerance);
	TestEqual(TEXT("heavy attack gets the same bonuses"), S.Weapon.heavy.damage / Sword.heavy.damage, S.Weapon.light.damage / Sword.light.damage, 0.0001f);
	TestEqual(TEXT("shield skill: wider block arc"), S.Shield.block_half_arc_deg, Plain.Shield.block_half_arc_deg + V.skills.block_arc_deg_per_level * 4, Tolerance);
	TestEqual(TEXT("imprint: fire resistance points"), S.Resistance.fire, Plain.Resistance.fire + V.imprint.resistance_points_per_level * 2, Tolerance);

	// The slime line: the bolt takes skill and imprint (added), not strength or dexterity.
	FHydraProgressView Caster;
	Caster.Level = 20;
	Caster.ImprintLevel = 1;
	Caster.SkillLevels.Add(TEXT("projectile"), 10.f);
	Caster.SkillLevels.Add(TEXT("fire_magic"), 3.f);
	const FHydraCombatStats Slime = BuildStats(Defs, V, TEXT("slime"), Caster);
	const FHydraCombatStats PlainSlime = BuildStats(Defs, V, TEXT("slime"));
	TestEqual(TEXT("bolt without an element: projectile skill only"), Slime.BoltBonus, V.skills.damage_bonus_per_level * 10, 0.0001f);
	TestEqual(TEXT("fire bolt: projectile + fire magic + imprint"), Slime.FireBoltBonus, V.skills.damage_bonus_per_level * 13 + V.imprint.damage_bonus_per_level, 0.0001f);
	TestEqual(TEXT("fire magic: bigger fire field"), Slime.FieldRadiusFactor, 1.f + V.skills.field_radius_bonus_per_level * 3, 0.0001f);
	TestEqual(TEXT("spirit grows: more mana"), Slime.ManaMax, PlainSlime.ManaMax * (1.f + V.progression.level_growth * 20), Tolerance);
	TestEqual(TEXT("the slime has no element body"), Slime.ElementBodyDamage, 0.f, Tolerance);
	FHydraProgressView Imprinted;
	Imprinted.ImprintLevel = 1;
	const FHydraCombatStats Elemental = BuildStats(Defs, V, TEXT("fire_elemental_slime"), Imprinted);
	const FHydraFormValues& ElementalForm = V.forms[TEXT("fire_elemental_slime")];
	TestTrue(TEXT("the fire elemental slime exists and casts"), Elemental.bValid && Elemental.bCanCast);
	TestEqual(TEXT("element body: its damage x (1 + imprint bonus), no level factor"), Elemental.ElementBodyDamage, ElementalForm.element_body_damage * (1.f + V.imprint.damage_bonus_per_level), Tolerance);
	TestTrue(TEXT("... keeps the resistance of the line"), ElementalForm.resistance.slash >= V.forms[TEXT("slime")].resistance.slash && ElementalForm.resistance.slash > 0.f);
	TestTrue(TEXT("... has higher base values than the slime"), BuildStats(Defs, V, TEXT("fire_elemental_slime")).HpMax > PlainSlime.HpMax);
	TestTrue(TEXT("... but is weaker at first than a slime at its level cap"), BuildStats(Defs, V, TEXT("fire_elemental_slime")).HpMax < Slime.HpMax * 1.2f);

	// The arena: the slime starts so close to its cap that the evolution can be tested at once,
	// and the first waves give enough nXP for a level-up.
	const FHydraFormValues& SlimeForm = V.forms[TEXT("slime")];
	TestTrue(TEXT("slime starts shortly before its level cap"), SlimeForm.start_level < SlimeForm.level_cap && SlimeForm.level_cap - SlimeForm.start_level <= 3);
	float Nxp = 0.f;
	float Hxp = 0.f;
	for (int32 i = 0; i < FMath::Min(3, V.waves.list.Num()); ++i)
	{
		for (const FHydraWaveGroupValues& Group : V.waves.list[i].groups)
		{
			if (const FHydraEnemyValues* Enemy = V.enemies.Find(Group.enemy))
			{
				const float One = EnemyNxp(V.progression, Enemy->nxp_base, Enemy->level);
				Nxp += Group.count * One;
				Hxp += Group.count * EnemyHxp(One, Enemy->hxp_factor);
			}
		}
	}
	TestTrue(TEXT("three waves level the slime up"), Nxp >= XpForNextLevel(V.progression, SlimeForm.start_level));
	TestTrue(TEXT("three waves level the humanoid up"), Nxp >= XpForNextLevel(V.progression, V.forms[TEXT("humanoid")].start_level));
	TestTrue(TEXT("harder enemy types give more hXP than nXP: more than one skill point per level-up"), SkillPoints(V.skill_points, HardshipRating(Hxp, Nxp)) > 1.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
