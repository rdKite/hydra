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

	// One quantity, one source: costs come from weight / body weight (x demand) only.
	FHydraWeaponValues Sword;
	Sword.weight = 1.5f;
	Sword.demand = 1.f;
	TestEqual(TEXT("cost ratio of a weapon = weight / body weight"), AttackCostRatio(Sword, 75.f), 0.02f, Tolerance);
	FHydraWeaponValues Slam;
	Slam.cost_ratio = 0.01f;
	TestEqual(TEXT("a body attack names its cost ratio"), AttackCostRatio(Slam, 8.f), 0.01f, Tolerance);
	TestEqual(TEXT("attack cost"), AttackStaminaCost(20000.f, 0.02f, 1.f), 400.f, Tolerance);
	TestEqual(TEXT("demand raises the cost"), AttackStaminaCost(20000.f, 0.02f, 1.06f), 424.f, 0.01f);
	TestEqual(TEXT("dodge cost of the bare body"), DodgeStaminaCost(190.f, 70.f, 70.f), 190.f, Tolerance);
	TestEqual(TEXT("dodge cost with load"), DodgeStaminaCost(190.f, 77.f, 70.f), 209.f, Tolerance);
	FHydraBlockValues Block;
	Block.hold_stamina_per_s_per_weight_ratio = 875.f;
	Block.blocker_share = 1.f;
	Block.reference_shield_ratio = 0.05f;
	Block.share_min = 0.5f;
	Block.share_max = 2.f;
	Block.rebound_share = 0.4f;
	TestEqual(TEXT("holding a block: by shield weight / body weight"), BlockHoldStaminaPerS(Block, 4.f, 80.f), 43.75f, Tolerance);

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

	// Interrupt: a share of the target's maximum HP - weight and weapon play no role.
	FHydraInterruptValues Interrupt;
	Interrupt.threshold_share_of_max_hp = 0.25f;
	TestEqual(TEXT("interrupt threshold"), InterruptThreshold(Interrupt, 1000.f), 250.f, Tolerance);
	TestTrue(TEXT("more HP: harder to interrupt"), InterruptThreshold(Interrupt, 3000.f) > InterruptThreshold(Interrupt, 1000.f));

	// Block cost and rebound: shares of the damage actually blocked.
	TestEqual(TEXT("blocker share at the reference shield"), BlockerShare(Block, 4.f, 80.f), 1.f, Tolerance);
	TestEqual(TEXT("a heavier shield takes hits more cheaply"), BlockerShare(Block, 6.f, 80.f), 2.f / 3.f, Tolerance);
	TestEqual(TEXT("a light shield pays more"), BlockerShare(Block, 2.4f, 80.f), 5.f / 3.f, Tolerance);
	TestEqual(TEXT("the share is limited above"), BlockerShare(Block, 0.5f, 80.f), 2.f, Tolerance);
	TestEqual(TEXT("... and below"), BlockerShare(Block, 40.f, 80.f), 0.5f, Tolerance);
	TestEqual(TEXT("rebound: a share of the blocked damage"), ReboundStamina(Block, 140.f), 56.f, Tolerance);

	// Weak point: fixed multiplier, raised by dexterity, whatever the weapon weighs.
	FHydraWeakPointValues Weak;
	Weak.damage_multiplier = 1.5f;
	Weak.multiplier_per_dexterity_gain = 0.5f;
	Weak.max_damage_multiplier = 2.5f;
	TestEqual(TEXT("no dexterity gain: the base multiplier"), WeakPointMultiplier(Weak, 0.f), 1.5f, Tolerance);
	TestEqual(TEXT("dexterity raises it"), WeakPointMultiplier(Weak, 0.4f), 1.7f, Tolerance);
	TestEqual(TEXT("capped"), WeakPointMultiplier(Weak, 10.f), 2.5f, Tolerance);

	// Damage: the attribute bonus by the fixed strength share of the weapon; base damage from the cost.
	FHydraDamageValues Damage;
	Damage.strength_bonus_per_gain = 0.3f;
	Damage.dexterity_bonus_per_gain = 0.2f;
	Damage.efficiency_base = 0.41f;
	Damage.efficiency_reference_kg = 1.5f;
	Damage.efficiency_exponent = 0.1f;
	TestEqual(TEXT("pure strength weapon"), AttributeDamageBonus(Damage, 1.f, 0.5f, 0.1f), 0.15f, Tolerance);
	TestEqual(TEXT("pure dexterity weapon"), AttributeDamageBonus(Damage, 0.f, 0.5f, 0.1f), 0.02f, Tolerance);
	TestEqual(TEXT("half and half"), AttributeDamageBonus(Damage, 0.5f, 0.5f, 0.1f), 0.085f, Tolerance);
	TestEqual(TEXT("base damage at the reference weight"), BaseDamageFromCost(Damage, 1.5f, 490.f), 200.9f, 0.01f);
	TestTrue(TEXT("a heavier weapon is slightly more efficient"), BaseDamageFromCost(Damage, 3.f, 490.f) > BaseDamageFromCost(Damage, 1.5f, 490.f));
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
	// The numbers the game derives for the forms from the real data files, at level 0.
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
	TestEqual(TEXT("10 HP per vitality"), Humanoid.HpMax, 10.f * Form.vitality, 0.01f);
	TestEqual(TEXT("10 stamina per endurance"), Humanoid.StaminaMax, 10.f * Form.endurance, 0.01f);
	TestEqual(TEXT("regeneration is a share of the bar"), Humanoid.StaminaRegenPerS, Humanoid.StaminaMax * Form.stamina_regen_share_per_s, 0.01f);
	TestEqual(TEXT("a humanoid without a spell form has no mana bar"), Humanoid.ManaMax, 0.f, 0.01f);
	TestEqual(TEXT("weight = body plus load"), Humanoid.TotalWeight, Form.body_weight + Sword.weight + Shield.weight, 0.001f);
	TestEqual(TEXT("handling: total weight / strength"), Humanoid.WeightRatio, Humanoid.TotalWeight / Form.strength, 0.0001f);
	TestEqual(TEXT("light attack cost: weapon weight / body weight"), Humanoid.LightStaminaCost, V.attack.light_stamina_per_weight_ratio * Sword.weight / Form.body_weight * Sword.demand, 0.01f);
	TestEqual(TEXT("dodge cost: total weight / body weight"), Humanoid.DodgeStaminaCost, Form.dodge_stamina_cost * Humanoid.TotalWeight / Form.body_weight, 0.01f);
	// Balancing target: a human at level 0 manages two light longsword strikes in a row, not three.
	TestTrue(FString::Printf(TEXT("two light strikes from a full bar, not three (cost %.0f of %.0f)"), Humanoid.LightStaminaCost, Humanoid.StaminaMax),
		2.f * Humanoid.LightStaminaCost <= Humanoid.StaminaMax && 2.2f * Humanoid.LightStaminaCost > Humanoid.StaminaMax);
	TestTrue(TEXT("a heavy attack fits into the bar"), Humanoid.HeavyStaminaCost > Humanoid.LightStaminaCost && Humanoid.HeavyStaminaCost <= Humanoid.StaminaMax);
	TestTrue(TEXT("the heavy wind-up is a readable telegraph (at least 400 ms)"), Sword.heavy.min_windup_ms >= 400.f);
	// Base damage follows from the cost (stored in the data): longsword light 200, heavy x 2.
	TestEqual(TEXT("longsword: stored light damage matches the efficiency formula"), Sword.light.damage,
		HydraCombatMath::BaseDamageFromCost(V.damage, Sword.weight, Humanoid.LightStaminaCost), Sword.light.damage * 0.01f);
	TestEqual(TEXT("longsword light = 200"), Sword.light.damage, 200.f, 0.01f);
	TestEqual(TEXT("heavy = light x 2"), Sword.heavy.damage, 2.f * Sword.light.damage, 0.01f);
	TestEqual(TEXT("a human at level 0 falls after five light longsword hits"), Humanoid.HpMax / Sword.light.damage, 5.f, 0.01f);
	TestEqual(TEXT("at level 0 the damage is the base damage"), Humanoid.Weapon.light.damage, Sword.light.damage, 0.01f);
	// Guard rail (no hard cap): the strongest light hit stays below the HP of an equal opponent.
	const float StrongestLight = Sword.light.damage * (1.f + V.chain.max_bonus) * V.weak_point.max_damage_multiplier;
	TestTrue(FString::Printf(TEXT("strongest light hit (%.0f) below the HP of an equal (%.0f)"), StrongestLight, Humanoid.HpMax), StrongestLight < Humanoid.HpMax);
	// Interrupt: a light hit does not interrupt an equal, a heavy one does.
	TestTrue(TEXT("light hit below the interrupt threshold, heavy hit above"), Sword.light.damage < Humanoid.InterruptThreshold && Sword.heavy.damage >= Humanoid.InterruptThreshold);
	// Block: a light hit on the shield costs the blocker less stamina than the strike cost the attacker.
	const float BlockedLight = Sword.light.damage * Shield.block_damage_reduction;
	TestTrue(TEXT("blocking a light hit is cheaper than striking it"), BlockedLight * Humanoid.BlockerShare < Humanoid.LightStaminaCost);
	TestTrue(TEXT("holding the block costs stamina every second"), Humanoid.BlockHoldStaminaPerS > 0.f && Humanoid.BlockHoldStaminaPerS < 0.2f * Humanoid.StaminaMax);

	// The slime: little stamina, a lot of mana, resistant to slash and pierce, weak to blunt.
	const FHydraCombatStats Slime = HydraCombatMath::BuildStats(Defs, V, TEXT("slime"));
	const FHydraFormValues& SlimeForm = V.forms.FindChecked(TEXT("slime"));
	const FHydraWeaponValues& Slam = V.weapons.FindChecked(TEXT("slime_slam"));
	TestTrue(TEXT("slime is valid"), Slime.bValid);
	TestTrue(TEXT("slime attacks with its body (slam) and has no shield"), Slime.bHasWeapon && !Slime.bHasShield);
	TestEqual(TEXT("the slam is blunt"), Slime.LightDamageType, FName(TEXT("blunt")));
	TestTrue(TEXT("slime casts the bolt, the humanoid does not (Phase 1)"), Slime.bCanCast && !Humanoid.bCanCast);
	TestTrue(TEXT("slime: little stamina, a lot of mana"), Slime.StaminaMax < Humanoid.StaminaMax && Slime.ManaMax > Slime.StaminaMax);
	TestTrue(TEXT("slime line: resistance points against slash and pierce, a weakness to blunt"), SlimeForm.resistance.slash > 0.f && SlimeForm.resistance.pierce > 0.f && SlimeForm.resistance.blunt < 0.f);
	TestEqual(TEXT("slam cost: its cost ratio in place of weight / body weight"), Slime.LightStaminaCost, V.attack.light_stamina_per_weight_ratio * Slam.cost_ratio * Slam.demand, 0.01f);
	TestTrue(TEXT("about two slams in a row for a slime at level 0"), 2.f * Slime.LightStaminaCost <= Slime.StaminaMax && 3.f * Slime.LightStaminaCost > Slime.StaminaMax);
	TestEqual(TEXT("slam: stored light damage matches its cost (reference efficiency)"), Slam.light.damage, V.damage.efficiency_base * Slime.LightStaminaCost, Slam.light.damage * 0.02f);
	TestTrue(TEXT("slime can pay for a fully charged bolt"), V.bolt.mana_cost * V.bolt.full_charge_cost_factor <= Slime.ManaMax);
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

	// Resistance points per damage type, turned into a reduction by one flattening curve.
	FHydraResistanceValues R;
	R.slash = 100.f;
	R.blunt = -25.f;
	FHydraResistanceRuleValues Curve;
	Curve.curve_constant = 100.f;
	TestEqual(TEXT("points by type"), ResistanceTo(R, TEXT("slash")), 100.f, Tolerance);
	TestEqual(TEXT("no points where none are set"), ResistanceTo(R, TEXT("fire")), 0.f, Tolerance);
	TestEqual(TEXT("unknown type: none"), ResistanceTo(R, TEXT("sonic")), 0.f, Tolerance);
	TestEqual(TEXT("points = the constant: half"), ResistedDamage(Curve, 200.f, 100.f), 100.f, Tolerance);
	TestEqual(TEXT("the curve flattens: three times the points, three quarters"), ResistedDamage(Curve, 200.f, 300.f), 50.f, Tolerance);
	TestEqual(TEXT("no points: full damage"), ResistedDamage(Curve, 200.f, 0.f), 200.f, Tolerance);
	TestEqual(TEXT("weakness: more damage"), ResistedDamage(Curve, 200.f, -25.f), 250.f, Tolerance);
	TestTrue(TEXT("never fully immune"), ResistedDamage(Curve, 200.f, 100000.f) > 0.f);

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
	TestEqual(TEXT("interval"), BurningIntervalS(Burn, Curve, 0.f), 10.f, Tolerance);
	TestEqual(TEXT("100 fire resistance points double the interval"), BurningIntervalS(Burn, Curve, 100.f), 20.f, Tolerance);
	TestEqual(TEXT("a weakness does not shorten it"), BurningIntervalS(Burn, Curve, -50.f), 10.f, Tolerance);

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
