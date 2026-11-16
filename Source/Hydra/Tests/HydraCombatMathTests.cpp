// Automation tests for the pure positional combat rules (HydraCombatMath.h) and the pose
// history used for lag compensation (HydraPoseHistory.h).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/HydraCombatMath.h"
#include "Character/HydraPoseHistory.h"
#include "Data/HydraDataLoader.h"
#include "Tests/HydraTestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraTargetingMathTest, "Hydra.Camera.TargetingMath", HydraTests::Flags)
bool FHydraTargetingMathTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	const FVector Origin(0.f, 0.f, 0.f);
	const FVector Forward(1.f, 0.f, 0.f);
	float Score = 0.f;

	// Cone: range and half angle.
	TestTrue(TEXT("straight ahead in range"), ScoreTargetInCone(Origin, Forward, FVector(1000.f, 0.f, 0.f), 1500.f, 60.f, Score));
	TestFalse(TEXT("beyond range"), ScoreTargetInCone(Origin, Forward, FVector(1600.f, 0.f, 0.f), 1500.f, 60.f, Score));
	TestFalse(TEXT("behind"), ScoreTargetInCone(Origin, Forward, FVector(-500.f, 0.f, 0.f), 1500.f, 60.f, Score));
	TestTrue(TEXT("45 degrees off, inside 60"), ScoreTargetInCone(Origin, Forward, FVector(500.f, 500.f, 0.f), 1500.f, 60.f, Score));
	TestFalse(TEXT("45 degrees off, outside 30"), ScoreTargetInCone(Origin, Forward, FVector(500.f, 500.f, 0.f), 1500.f, 30.f, Score));
	TestTrue(TEXT("height is ignored"), ScoreTargetInCone(Origin, Forward, FVector(1000.f, 0.f, 800.f), 1500.f, 60.f, Score));

	// Score: centre of the cone first, then distance.
	float Centre = 0.f, Side = 0.f, Near = 0.f, Far = 0.f;
	ScoreTargetInCone(Origin, Forward, FVector(1000.f, 0.f, 0.f), 1500.f, 60.f, Centre);
	ScoreTargetInCone(Origin, Forward, FVector(700.f, 700.f, 0.f), 1500.f, 60.f, Side);
	TestTrue(TEXT("target in the centre beats one at the side"), Centre < Side);
	ScoreTargetInCone(Origin, Forward, FVector(300.f, 0.f, 0.f), 1500.f, 60.f, Near);
	ScoreTargetInCone(Origin, Forward, FVector(1200.f, 0.f, 0.f), 1500.f, 60.f, Far);
	TestTrue(TEXT("near target beats far target"), Near < Far);

	// Aim assist: a facing goal part of the way, along the shorter way.
	TestEqual(TEXT("strength 0.5: half way"), AimAssistYaw(0.f, 40.f, 0.5f), 20.f);
	TestEqual(TEXT("strength 0 does nothing"), AimAssistYaw(10.f, 40.f, 0.f), 10.f);
	TestEqual(TEXT("strength 1 faces the target"), AimAssistYaw(10.f, 40.f, 1.f), 40.f);
	TestEqual(TEXT("shorter way across 180"), AimAssistYaw(170.f, -170.f, 0.5f), 180.f);
	TestEqual(TEXT("strength is clamped"), AimAssistYaw(0.f, 40.f, 3.f), 40.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraTurnRateMathTest, "Hydra.Combat.TurnRate", HydraTests::Flags)
bool FHydraTurnRateMathTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	FHydraTurnRateValues V;
	V.min_deg_s = 120.f;
	V.max_deg_s = 720.f;
	V.base_deg_s = 540.f;
	V.weight_strength_exponent = 1.f;
	V.blocking_factor = 0.5f;
	V.attacking_factor = 0.3f;

	// Weight / strength: heavier relative to strength turns slower, lighter turns faster.
	TestEqual(TEXT("ratio 1: base"), TurnRateDegS(V, 1.f), 540.f);
	TestEqual(TEXT("ratio 2: half"), TurnRateDegS(V, 2.f), 270.f);
	TestTrue(TEXT("light form turns faster than a heavy one"), TurnRateDegS(V, 0.8f) > TurnRateDegS(V, 1.5f));
	// Floor and cap.
	TestEqual(TEXT("floor: nobody is helpless"), TurnRateDegS(V, 50.f), 120.f);
	TestEqual(TEXT("cap: nobody turns instantly"), TurnRateDegS(V, 0.1f), 720.f);
	// Slower while blocking and attacking.
	TestEqual(TEXT("blocking"), TurnRateDegS(V, 1.f, true, false), 270.f);
	TestEqual(TEXT("attacking"), TurnRateDegS(V, 1.f, false, true), 162.f);
	TestEqual(TEXT("the floor also holds while attacking"), TurnRateDegS(V, 2.f, false, true), 120.f);
	// The exponent shapes the curve.
	V.weight_strength_exponent = 0.5f;
	TestEqual(TEXT("exponent 0.5: ratio 4 halves the rate"), TurnRateDegS(V, 4.f), 270.f);

	// A turn never exceeds the step the turn rate allows (lock-on, aim assist).
	TestEqual(TEXT("step is limited"), StepYawTowards(0.f, 90.f, 10.f), 10.f);
	TestEqual(TEXT("step stops at the goal"), StepYawTowards(85.f, 90.f, 10.f), 90.f);
	TestEqual(TEXT("step takes the shorter way"), StepYawTowards(170.f, -170.f, 5.f), 175.f);
	TestEqual(TEXT("step the other way"), StepYawTowards(0.f, -90.f, 10.f), -10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraCombatRulesTest, "Hydra.Combat.Rules", HydraTests::Flags)
bool FHydraCombatRulesTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	const float Tolerance = 0.001f;

	// Weight costs stamina: cost = factor x weapon weight / strength.
	TestEqual(TEXT("attack cost"), AttackStaminaCost(800.f, 2.5f, 80.f), 25.f, Tolerance);
	TestTrue(TEXT("a heavier weapon costs more"), AttackStaminaCost(800.f, 5.f, 80.f) > AttackStaminaCost(800.f, 2.5f, 80.f));
	TestTrue(TEXT("more strength costs less"), AttackStaminaCost(800.f, 2.5f, 160.f) < AttackStaminaCost(800.f, 2.5f, 80.f));
	FHydraDodgeValues Dodge;
	Dodge.stamina_cost = 20.f;
	TestEqual(TEXT("dodge cost at ratio 1"), DodgeStaminaCost(Dodge, 1.f), 20.f, Tolerance);
	TestEqual(TEXT("dodge cost with a heavy load"), DodgeStaminaCost(Dodge, 1.5f), 30.f, Tolerance);

	// Heavy attack: charge.
	FHydraHeavyAttackValues Heavy;
	Heavy.min_windup_ms = 400.f;
	Heavy.max_charge_ms = 1200.f;
	Heavy.full_charge_damage_factor = 1.5f;
	TestEqual(TEXT("released at the minimum: factor 1"), HeavyChargeFactor(Heavy, 400.f), 1.f, Tolerance);
	TestEqual(TEXT("released early: still factor 1"), HeavyChargeFactor(Heavy, 100.f), 1.f, Tolerance);
	TestEqual(TEXT("half charged"), HeavyChargeFactor(Heavy, 800.f), 1.25f, Tolerance);
	TestEqual(TEXT("fully charged"), HeavyChargeFactor(Heavy, 1200.f), 1.5f, Tolerance);
	TestEqual(TEXT("no more than fully charged"), HeavyChargeFactor(Heavy, 5000.f), 1.5f, Tolerance);

	// Chain: each further hit deals more, up to the cap.
	FHydraChainValues Chain;
	Chain.damage_bonus_per_hit = 0.1f;
	Chain.max_bonus = 0.5f;
	TestEqual(TEXT("first hit of a chain"), ChainMultiplier(Chain, 0), 1.f, Tolerance);
	TestEqual(TEXT("third hit"), ChainMultiplier(Chain, 2), 1.2f, Tolerance);
	TestEqual(TEXT("cap"), ChainMultiplier(Chain, 40), 1.5f, Tolerance);

	// Interrupt by weight: heavy characters are hard to interrupt.
	FHydraInterruptValues Interrupt;
	Interrupt.damage_threshold_per_weight = 0.3f;
	Interrupt.attacking_weapon_weight_factor = 2.f;
	TestEqual(TEXT("threshold"), InterruptThreshold(Interrupt, 76.5f, 2.5f), 0.3f * 81.5f, Tolerance);
	TestTrue(TEXT("a heavier target needs more damage"), InterruptThreshold(Interrupt, 200.f, 2.5f) > InterruptThreshold(Interrupt, 76.5f, 2.5f));
	TestTrue(TEXT("a heavier weapon is harder to interrupt"), InterruptThreshold(Interrupt, 76.5f, 12.f) > InterruptThreshold(Interrupt, 76.5f, 2.5f));

	// Block cost and rebound by weight.
	FHydraBlockValues Block;
	Block.stamina_cost_per_hit = 24.f;
	Block.heavy_attack_factor = 1.6f;
	Block.weight_ratio_min = 0.25f;
	Block.weight_ratio_max = 4.f;
	Block.rebound_attacker_stamina_ratio = 0.15f;
	TestEqual(TEXT("block cost: longsword on round shield"), BlockStaminaCost(Block, 2.5f, 4.f, false), 15.f, Tolerance);
	TestEqual(TEXT("block cost: heavy attack"), BlockStaminaCost(Block, 2.5f, 4.f, true), 24.f, Tolerance);
	TestEqual(TEXT("rebound: longsword on round shield"), ReboundStamina(Block, 25.f, 2.5f, 4.f), 6.f, Tolerance);
	// Dagger blocks a giant club: the blocker pays a lot, the attacker hardly anything.
	TestTrue(TEXT("light item blocking a heavy weapon costs the blocker much more"), BlockStaminaCost(Block, 12.f, 1.f, false) > 4.f * BlockStaminaCost(Block, 2.5f, 4.f, false));
	TestTrue(TEXT("... and rebounds less on the attacker"), ReboundStamina(Block, 25.f, 12.f, 1.f) < ReboundStamina(Block, 25.f, 2.5f, 4.f));
	TestEqual(TEXT("the ratio is limited"), BlockWeightRatio(Block, 1000.f, 1.f), 4.f, Tolerance);

	// Weak point: fixed multiplier, bigger for weapons that are light for their wielder.
	FHydraWeakPointValues Weak;
	Weak.damage_multiplier = 1.5f;
	Weak.reference_weapon_weight_ratio = 0.03125f;
	Weak.max_damage_multiplier = 2.5f;
	TestEqual(TEXT("reference weapon"), WeakPointMultiplier(Weak, 0.03125f), 1.5f, Tolerance);
	TestEqual(TEXT("half as heavy: double bonus"), WeakPointMultiplier(Weak, 0.015625f), 2.f, Tolerance);
	TestEqual(TEXT("twice as heavy: half bonus"), WeakPointMultiplier(Weak, 0.0625f), 1.25f, Tolerance);
	TestEqual(TEXT("cap for very light weapons"), WeakPointMultiplier(Weak, 0.001f), 2.5f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraHitAreaTest, "Hydra.Combat.HitAreaAndArcs", HydraTests::Flags)
bool FHydraHitAreaTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	const FVector Attacker(0.f, 0.f, 0.f);

	// Hit area: reach to the surface of the target and the attack arc. Attacker faces +X.
	TestTrue(TEXT("in front, in reach"), IsInHitArea(Attacker, 0.f, FVector(200.f, 0.f, 0.f), 35.f, 220.f, 70.f));
	TestTrue(TEXT("just in reach thanks to the body radius"), IsInHitArea(Attacker, 0.f, FVector(250.f, 0.f, 0.f), 35.f, 220.f, 70.f));
	TestFalse(TEXT("stepped out of reach (dodge)"), IsInHitArea(Attacker, 0.f, FVector(260.f, 0.f, 0.f), 35.f, 220.f, 70.f));
	TestTrue(TEXT("60 degrees to the side, inside a 70 degree arc"), IsInHitArea(Attacker, 0.f, FVector(100.f, 173.f, 0.f), 35.f, 220.f, 70.f));
	TestFalse(TEXT("60 degrees to the side, outside a 35 degree arc (thrust)"), IsInHitArea(Attacker, 0.f, FVector(100.f, 173.f, 0.f), 35.f, 220.f, 35.f));
	TestFalse(TEXT("behind the attacker"), IsInHitArea(Attacker, 0.f, FVector(-100.f, 0.f, 0.f), 35.f, 220.f, 70.f));
	TestTrue(TEXT("the facing of the attacker counts"), IsInHitArea(Attacker, 180.f, FVector(-100.f, 0.f, 0.f), 35.f, 220.f, 70.f));
	TestFalse(TEXT("far above"), IsInHitArea(Attacker, 0.f, FVector(100.f, 0.f, 400.f), 35.f, 220.f, 70.f));

	// Block arc: a defender at the origin facing +X with a 70 degree shield.
	const FVector Defender(0.f, 0.f, 0.f);
	TestTrue(TEXT("attack from the front is inside the block arc"), IsInArc(Defender, 0.f, FVector(150.f, 0.f, 0.f), 70.f));
	TestTrue(TEXT("45 degrees: still inside"), IsInArc(Defender, 0.f, FVector(100.f, 100.f, 0.f), 70.f));
	TestFalse(TEXT("flank: outside the block arc"), IsInArc(Defender, 0.f, FVector(0.f, 150.f, 0.f), 70.f));
	TestFalse(TEXT("back: outside the block arc"), IsInArc(Defender, 0.f, FVector(-150.f, 0.f, 0.f), 70.f));
	TestTrue(TEXT("turning towards the attacker brings it into the arc"), IsInArc(Defender, 90.f, FVector(0.f, 150.f, 0.f), 70.f));

	// Weak point: the back.
	TestTrue(TEXT("directly behind"), IsInBackArc(Defender, 0.f, FVector(-150.f, 0.f, 0.f), 60.f));
	TestTrue(TEXT("45 degrees off the back"), IsInBackArc(Defender, 0.f, FVector(-100.f, 100.f, 0.f), 60.f));
	TestFalse(TEXT("flank is not the back"), IsInBackArc(Defender, 0.f, FVector(0.f, 150.f, 0.f), 60.f));
	TestFalse(TEXT("front is not the back"), IsInBackArc(Defender, 0.f, FVector(150.f, 0.f, 0.f), 60.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraLagCompensationTest, "Hydra.Combat.LagCompensation", HydraTests::Flags)
bool FHydraLagCompensationTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	FHydraLagCompensationValues Lag;
	Lag.max_rewind_ms = 250.f;
	TestEqual(TEXT("rewind = round trip"), RewindSeconds(120.f, Lag), 0.12f, 0.0001f);
	TestEqual(TEXT("rewind is capped"), RewindSeconds(900.f, Lag), 0.25f, 0.0001f);
	TestEqual(TEXT("no negative rewind"), RewindSeconds(-5.f, Lag), 0.f, 0.0001f);

	// A target runs away along +X at 5 m/s, sampled at 60 Hz; it starts to block at t = 1.0
	// and turns its back (yaw 0 -> 180) between t = 1.0 and t = 1.5.
	FHydraPoseHistory History;
	FHydraPoseSample Out;
	TestFalse(TEXT("empty history has no pose"), History.Get(1.0, Out));
	for (int32 i = 0; i <= 120; ++i)
	{
		FHydraPoseSample S;
		S.Time = i / 60.0;
		S.Location = FVector(500.f * S.Time, 0.f, 0.f);
		S.bBlocking = S.Time >= 1.0;
		S.Yaw = FRotator::NormalizeAxis(static_cast<float>(FMath::Clamp((S.Time - 1.0) / 0.5, 0.0, 1.0)) * 180.f);
		History.Add(S, 0.5);
	}
	// Now = 2.0, the target is at x = 1000.
	TestTrue(TEXT("pose at the present"), History.Get(2.0, Out));
	TestEqual(TEXT("present position"), Out.Location.X, 1000.0, 0.5);
	// 150 ms ago it was 75 cm closer: that is what an attacker with 150 ms ping saw.
	TestTrue(TEXT("pose 150 ms ago"), History.Get(1.85, Out));
	TestEqual(TEXT("rewound position"), Out.Location.X, 925.0, 0.5);
	TestEqual(TEXT("interpolated between samples"), (History.Get(1.8417, Out), Out.Location.X), 920.85, 0.5);

	// The hit area check with and without rewind: attacker at x = 700, reach 220 + radius 35.
	const FVector Attacker(700.f, 0.f, 0.f);
	History.Get(2.0, Out);
	TestFalse(TEXT("at the server's present the target is out of reach"), IsInHitArea(Attacker, 0.f, Out.Location, 35.f, 220.f, 70.f));
	History.Get(1.85, Out);
	TestTrue(TEXT("where the attacker saw it, it is in reach"), IsInHitArea(Attacker, 0.f, Out.Location, 35.f, 220.f, 70.f));

	// Older than the memory: the oldest pose that is still there, never an invented one.
	History.Get(0.2, Out);
	TestTrue(TEXT("old poses are dropped"), Out.Time >= 1.45 && History.Num() < 40);
	// Newer than the newest sample: the newest.
	History.Get(9.0, Out);
	TestEqual(TEXT("future = newest"), Out.Location.X, 1000.0, 0.5);

	// Block state and facing are rewound too (block arc).
	FHydraPoseHistory Turning;
	for (int32 i = 0; i <= 120; ++i)
	{
		FHydraPoseSample S;
		S.Time = i / 60.0;
		S.bBlocking = S.Time >= 1.0;
		S.Yaw = FRotator::NormalizeAxis(static_cast<float>(FMath::Clamp((S.Time - 1.0) / 0.5, 0.0, 1.0)) * 180.f);
		Turning.Add(S, 2.0);
	}
	const FVector InFront(150.f, 0.f, 0.f);
	Turning.Get(0.9, Out);
	TestFalse(TEXT("before the block started: not blocking"), Out.bBlocking);
	Turning.Get(1.05, Out);
	TestTrue(TEXT("just after it started: blocking"), Out.bBlocking);
	TestTrue(TEXT("... and still facing the attacker: inside the block arc"), IsInArc(Out.Location, Out.Yaw, InFront, 70.f));
	Turning.Get(1.25, Out);
	TestEqual(TEXT("half turned"), Out.Yaw, 90.f, 1.f);
	TestFalse(TEXT("half turned: the attacker is outside the block arc"), IsInArc(Out.Location, Out.Yaw, InFront, 70.f));
	Turning.Get(2.0, Out);
	TestTrue(TEXT("turned away: the attacker is at the back (weak point)"), IsInBackArc(Out.Location, Out.Yaw, InFront, 60.f));

	// A teleport or respawn clears the memory.
	Turning.Reset();
	TestFalse(TEXT("reset history has no pose"), Turning.Get(1.0, Out));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraCombatStatsTest, "Hydra.Combat.StatsFromData", HydraTests::Flags)
bool FHydraCombatStatsTest::RunTest(const FString& Parameters)
{
	// The numbers the game derives for the humanoid from the real data files.
	FHydraValues V;
	FHydraDefinitions Defs;
	TArray<FString> Problems;
	if (!TestTrue(TEXT("data loads"), HydraDataLoader::LoadValues(HydraTests::DataDir(), V, Problems) && HydraDataLoader::LoadDefinitions(HydraTests::DataDir(), Defs, Problems)))
	{
		return false;
	}
	const FHydraCombatStats Humanoid = HydraCombatMath::BuildStats(Defs, V, TEXT("humanoid"));
	const FHydraFormValues& Form = V.forms.FindChecked(TEXT("humanoid"));
	const FHydraWeaponValues& Sword = V.weapons.FindChecked(TEXT("longsword"));
	const FHydraShieldValues& Shield = V.shields.FindChecked(TEXT("round_shield"));
	TestTrue(TEXT("humanoid is valid"), Humanoid.bValid);
	TestTrue(TEXT("humanoid holds its sword and shield"), Humanoid.bHasWeapon && Humanoid.bHasShield);
	TestEqual(TEXT("weight = body plus load"), Humanoid.TotalWeight, Form.body_weight + Sword.weight + Shield.weight, 0.001f);
	TestEqual(TEXT("weight ratio"), Humanoid.WeightRatio, Humanoid.TotalWeight / Form.strength, 0.0001f);
	TestEqual(TEXT("light attack cost"), Humanoid.LightStaminaCost, V.attack.light_stamina_per_weight_ratio * Sword.weight / Form.strength, 0.001f);
	TestTrue(TEXT("a light attack costs 10 to 90 % of the bar (design)"), Humanoid.LightStaminaCost >= 0.1f * Form.stamina_max && Humanoid.LightStaminaCost <= 0.9f * Form.stamina_max);
	TestTrue(TEXT("a heavy attack fits into the bar"), Humanoid.HeavyStaminaCost > Humanoid.LightStaminaCost && Humanoid.HeavyStaminaCost <= Form.stamina_max);
	TestTrue(TEXT("the heavy wind-up is a readable telegraph (at least 400 ms)"), Sword.heavy.min_windup_ms >= 400.f);
	// Guard rail: no death in one hit from full health between equals, even at full charge on the back.
	const float WorstHit = Sword.heavy.damage * Sword.heavy.full_charge_damage_factor * HydraCombatMath::WeakPointMultiplier(V.weak_point, Humanoid.WeaponWeightRatio);
	TestTrue(FString::Printf(TEXT("no one-hit kill between equal humanoids (worst hit %.0f of %.0f hp)"), WorstHit, Form.hp_max), WorstHit < Form.hp_max);

	// Phase 1 check from the design: humanoid with shield and slime turn clearly differently.
	const FHydraCombatStats Slime = HydraCombatMath::BuildStats(Defs, V, TEXT("slime"));
	const FHydraFormValues& SlimeForm = V.forms.FindChecked(TEXT("slime"));
	TestTrue(TEXT("slime is valid"), Slime.bValid);
	TestTrue(TEXT("slime attacks with its body (slam) and has no shield"), Slime.bHasWeapon && !Slime.bHasShield);
	TestEqual(TEXT("the slam is blunt"), Slime.LightDamageType, FName(TEXT("blunt")));
	TestTrue(TEXT("slime casts the bolt, the humanoid does not (Phase 1)"), Slime.bCanCast && !Humanoid.bCanCast);
	TestTrue(TEXT("slime: little stamina, a lot of mana"), SlimeForm.stamina_max < Form.stamina_max && SlimeForm.mana_max > SlimeForm.stamina_max && Form.mana_max == 0.f);
	TestTrue(TEXT("slime line: raised resistance to slash and pierce"), SlimeForm.resistance.slash > 0.f && SlimeForm.resistance.pierce > 0.f);
	TestTrue(TEXT("slime can pay for its slam and its bolt"), Slime.HeavyStaminaCost <= SlimeForm.stamina_max && V.bolt.mana_cost * V.bolt.full_charge_cost_factor <= SlimeForm.mana_max);
	TestTrue(TEXT("slime is small"), Slime.BodyHalfHeightCm < Humanoid.BodyHalfHeightCm && Slime.BodyHalfHeightCm >= Slime.BodyRadiusCm);
	TestEqual(TEXT("humanoid is 1.8 m tall"), Humanoid.BodyHalfHeightCm * 2.f, Form.body_height_m * 100.f, 0.01f);
	TestTrue(FString::Printf(TEXT("slime turns clearly faster (%.0f vs %.0f deg/s)"), Slime.TurnRateDegS, Humanoid.TurnRateDegS), Slime.TurnRateDegS > Humanoid.TurnRateDegS * 1.15f);

	TestFalse(TEXT("unknown form is not valid"), HydraCombatMath::BuildStats(Defs, V, TEXT("no_such_form")).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraMagicMathTest, "Hydra.Combat.ResistanceBoltBurning", HydraTests::Flags)
bool FHydraMagicMathTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	const float Tolerance = 0.001f;

	// Resistance per damage type.
	FHydraResistanceValues R;
	R.slash = 0.5f;
	R.blunt = -0.25f;
	TestEqual(TEXT("resistance by type"), ResistanceTo(R, TEXT("slash")), 0.5f, Tolerance);
	TestEqual(TEXT("no resistance where none is set"), ResistanceTo(R, TEXT("fire")), 0.f, Tolerance);
	TestEqual(TEXT("unknown type: none"), ResistanceTo(R, TEXT("sonic")), 0.f, Tolerance);
	TestEqual(TEXT("half resisted"), ResistedDamage(20.f, 0.5f), 10.f, Tolerance);
	TestEqual(TEXT("vulnerable: more damage"), ResistedDamage(20.f, -0.25f), 25.f, Tolerance);
	TestTrue(TEXT("never fully immune"), ResistedDamage(20.f, 5.f) > 0.f);

	// Bolt: charging makes it stronger and costlier.
	FHydraBoltValues B;
	B.mana_cost = 20.f;
	B.damage = 15.f;
	B.windup_ms = 300.f;
	B.max_charge_ms = 1200.f;
	B.full_charge_damage_factor = 1.6f;
	B.full_charge_cost_factor = 1.8f;
	TestEqual(TEXT("released during the wind-up: no charge"), BoltChargeAlpha(B, 200.f), 0.f, Tolerance);
	TestEqual(TEXT("half charged"), BoltChargeAlpha(B, 750.f), 0.5f, Tolerance);
	TestEqual(TEXT("fully charged"), BoltChargeAlpha(B, 3000.f), 1.f, Tolerance);
	TestEqual(TEXT("damage uncharged"), BoltDamage(B, 0.f), 15.f, Tolerance);
	TestEqual(TEXT("damage fully charged"), BoltDamage(B, 1.f), 24.f, Tolerance);
	TestEqual(TEXT("cost uncharged"), BoltManaCost(B, 0.f), 20.f, Tolerance);
	TestEqual(TEXT("cost fully charged"), BoltManaCost(B, 1.f), 36.f, Tolerance);
	TestEqual(TEXT("enough mana: the wanted charge"), AffordableBoltCharge(B, 1.f, 50.f), 1.f, Tolerance);
	TestEqual(TEXT("half the extra mana: half the charge"), AffordableBoltCharge(B, 1.f, 8.f), 0.5f, Tolerance);
	TestEqual(TEXT("no mana left: no charge"), AffordableBoltCharge(B, 1.f, 0.f), 0.f, Tolerance);

	// Burning: resistance lengthens the interval between stacks.
	FHydraBurningValues Burn;
	Burn.stack_interval_ms = 10000.f;
	TestEqual(TEXT("interval"), BurningIntervalS(Burn, 0.f), 10.f, Tolerance);
	TestEqual(TEXT("fire resistance 0.5 doubles the interval"), BurningIntervalS(Burn, 0.5f), 20.f, Tolerance);
	TestEqual(TEXT("a vulnerability does not shorten it"), BurningIntervalS(Burn, -0.5f), 10.f, Tolerance);

	// A bolt flying past: distance of a body to the path of one step.
	TestEqual(TEXT("point beside the path"), DistancePointToSegment(FVector(50.f, 30.f, 0.f), FVector::ZeroVector, FVector(100.f, 0.f, 0.f)), 30.f, Tolerance);
	TestEqual(TEXT("point beyond the end of the step"), DistancePointToSegment(FVector(130.f, 40.f, 0.f), FVector::ZeroVector, FVector(100.f, 0.f, 0.f)), 50.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraNetSimMathTest, "Hydra.Net.EmulationMath", HydraTests::Flags)
bool FHydraNetSimMathTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	TestEqual(TEXT("round-trip step: half per side"), OutgoingLagMs(100, true), 50);
	TestEqual(TEXT("one-way step: all per side"), OutgoingLagMs(100, false), 100);
	TestEqual(TEXT("no negative lag"), OutgoingLagMs(-5, true), 0);
	TestEqual(TEXT("2 % loss"), PacketLossPercent(0.02f), 2);
	TestEqual(TEXT("loss is clamped"), PacketLossPercent(3.f), 100);

	const TArray<int32> Steps = { 0, 50, 100, 150 };
	TestEqual(TEXT("next step"), NextLatencyStep(Steps, 50), 100);
	TestEqual(TEXT("wraps to the first"), NextLatencyStep(Steps, 150), 0);
	TestEqual(TEXT("unknown value starts at the first"), NextLatencyStep(Steps, 75), 0);
	TestEqual(TEXT("no steps"), NextLatencyStep(TArray<int32>(), 50), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
