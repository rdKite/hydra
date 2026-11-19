#pragma once

#include "CoreMinimal.h"
#include "Data/HydraDefinitions.h"
#include "Data/HydraValues.h"

/**
 * What a character has grown into: character level, skill levels (with the instinct bonus
 * already added) and the level of its fire imprint. Level 0 everywhere = the plain numbers
 * of the form.
 */
struct FHydraProgressView
{
	int32 Level = 0;
	int32 ImprintLevel = 0;
	TMap<FName, float> SkillLevels;

	float Skill(FName Id) const
	{
		const float* Found = SkillLevels.Find(Id);
		return Found ? *Found : 0.f;
	}
};

/**
 * What a character is made of for combat: its form with the loadout resolved and the
 * numbers that follow from its attributes (form x level), weight, skills and imprint
 * (Phase 1 design, section 26). Built from forms.json (loadout) and values.json; rebuilt
 * after Hydra.ReloadValues and whenever the progress changes.
 */
struct FHydraCombatStats
{
	bool bValid = false;
	FName FormId;

	int32 Level = 0;
	int32 LevelCap = 0;
	int32 ImprintLevel = 0;
	/** What the level added to every body attribute, as a share of the base value (0.02 x level). */
	float AttributeGain = 0.f;

	float BodyWeight = 0.f;
	/** Body attributes at this level. */
	float Vitality = 0.f;
	float Endurance = 0.f;
	float Spirit = 0.f;
	float Strength = 1.f;
	float Dexterity = 0.f;
	/** Body plus load (every carried weapon and shield). */
	float TotalWeight = 0.f;
	/** TotalWeight / Strength: the handling - it drives the turn rate. */
	float WeightRatio = 1.f;

	/** Body size (capsule) in cm. */
	float BodyRadiusCm = 0.f;
	float BodyHalfHeightCm = 0.f;

	float HpMax = 0.f;
	float StaminaMax = 0.f;
	float StaminaRegenPerS = 0.f;
	float ManaMax = 0.f;
	float ManaRegenPerS = 0.f;
	/** Resistance points per damage type (form plus imprint). */
	FHydraResistanceValues Resistance;

	/** The form has the body skill projectile: its off-hand button casts the bolt. */
	bool bCanCast = false;
	/** Progress bonus on the bolt's damage without an element / as a fire bolt (skill, imprint). */
	float BoltBonus = 0.f;
	float FireBoltBonus = 0.f;
	/** Radius factor of its fire fields (fire magic skill). */
	float FieldRadiusFactor = 1.f;
	/** Element body: fire damage a melee hit against it deals back. */
	float ElementBodyDamage = 0.f;

	bool bHasWeapon = false;
	/** The weapon in the hand; its damage already includes the progress bonuses (skill, strength / dexterity). */
	FHydraWeaponValues Weapon;
	/** Skills that grow by using the weapon / the shield (weapons.json). */
	FName WeaponSkill;
	FName ShieldSkill;
	/** "slash", "pierce" or "blunt" (weapons.json). */
	FName LightDamageType;
	FName HeavyDamageType;
	bool bHasShield = false;
	/** The shield; its block arc already includes the skill. */
	FHydraShieldValues Shield;
	/** Stamina per second while the block is raised. */
	float BlockHoldStaminaPerS = 0.f;
	/** Stamina the blocker pays per point of blocked damage. */
	float BlockerShare = 0.f;

	float LightStaminaCost = 0.f;
	float HeavyStaminaCost = 0.f;
	float DodgeStaminaCost = 0.f;
	/** Dexterity: factor on the length and on the duration of the dodge step. */
	float DodgeDistanceFactor = 1.f;
	float DodgeDurationFactor = 1.f;
	/** Damage factor of this character's hits on a weak point (dexterity). */
	float WeakPointMultiplier = 1.f;
	/** A hit of at least this damage interrupts this character's attack. */
	float InterruptThreshold = 0.f;
	float TurnRateDegS = 0.f;
};

/**
 * Pure functions for the combat and progression rules (no world access), so they can be
 * unit-tested and are identical on server and client. Distances in cm, angles in degrees,
 * times in ms unless the name says otherwise.
 */
namespace HydraCombatMath
{
	// --- Attributes and weight ------------------------------------------------------------

	/** What the level adds to a body attribute, as a share of its base value. */
	inline float AttributeGain(const FHydraProgressionValues& P, int32 Level)
	{
		return P.level_growth * FMath::Max(0, Level);
	}

	inline float WeightRatio(float Weight, float Strength)
	{
		return Strength > KINDA_SMALL_NUMBER ? FMath::Max(0.f, Weight) / Strength : 1.f;
	}

	/**
	 * Turn rate in deg/s: base / ratio^exponent (ratio = total weight / strength), slower while
	 * blocking and while attacking, always inside floor and cap (nobody is helpless, nobody
	 * turns instantly).
	 */
	inline float TurnRateDegS(const FHydraTurnRateValues& V, float Ratio = 1.f, bool bBlocking = false, bool bAttacking = false)
	{
		float Rate = V.base_deg_s / FMath::Pow(FMath::Max(Ratio, 0.01f), V.weight_strength_exponent);
		Rate = FMath::Min(Rate, FMath::Max(V.min_deg_s, V.max_deg_s));
		if (bBlocking)
		{
			Rate *= V.blocking_factor;
		}
		if (bAttacking)
		{
			Rate *= V.attacking_factor;
		}
		return FMath::Max(Rate, V.min_deg_s);
	}

	/** One step of turning from Current towards Goal by at most MaxStepDeg, along the shorter way. */
	inline float StepYawTowards(float Current, float Goal, float MaxStepDeg)
	{
		const float Delta = FMath::FindDeltaAngleDegrees(Current, Goal);
		return FRotator::NormalizeAxis(Current + FMath::Clamp(Delta, -FMath::Abs(MaxStepDeg), FMath::Abs(MaxStepDeg)));
	}

	// --- Costs: one source each, weight / body weight -------------------------------------

	/** Weapon weight / body weight; a body attack names its ratio itself. */
	inline float AttackCostRatio(const FHydraWeaponValues& W, float BodyWeight)
	{
		return W.cost_ratio > 0.f ? W.cost_ratio : (BodyWeight > KINDA_SMALL_NUMBER ? FMath::Max(0.f, W.weight) / BodyWeight : 0.f);
	}

	/** Stamina of an attack = factor x cost ratio x demand. Neither strength nor skill lowers it. */
	inline float AttackStaminaCost(float StaminaPerWeightRatio, float CostRatio, float Demand)
	{
		return StaminaPerWeightRatio * FMath::Max(0.f, CostRatio) * FMath::Max(0.f, Demand);
	}

	/** Stamina of a dodge step = the form's cost x total weight / body weight. */
	inline float DodgeStaminaCost(float FormDodgeCost, float TotalWeight, float BodyWeight)
	{
		return BodyWeight > KINDA_SMALL_NUMBER ? FormDodgeCost * FMath::Max(0.f, TotalWeight) / BodyWeight : FormDodgeCost;
	}

	/** Stamina per second while a block is raised = factor x shield weight / body weight. */
	inline float BlockHoldStaminaPerS(const FHydraBlockValues& V, float ShieldWeight, float BodyWeight)
	{
		return BodyWeight > KINDA_SMALL_NUMBER ? V.hold_stamina_per_s_per_weight_ratio * FMath::Max(0.f, ShieldWeight) / BodyWeight : 0.f;
	}

	// --- Attacks ----------------------------------------------------------------------------

	/** Damage factor of a heavy attack released after ChargeMs: 1 at min wind-up, full factor at max charge. */
	inline float HeavyChargeFactor(const FHydraHeavyAttackValues& Heavy, float ChargeMs)
	{
		const float Span = Heavy.max_charge_ms - Heavy.min_windup_ms;
		const float Alpha = Span > KINDA_SMALL_NUMBER ? FMath::Clamp((ChargeMs - Heavy.min_windup_ms) / Span, 0.f, 1.f) : 1.f;
		return FMath::Lerp(1.f, FMath::Max(1.f, Heavy.full_charge_damage_factor), Alpha);
	}

	/** Damage factor for the next hit of a chain that already has HitsBefore hits. */
	inline float ChainMultiplier(const FHydraChainValues& V, int32 HitsBefore)
	{
		return 1.f + FMath::Clamp(V.damage_bonus_per_hit * FMath::Max(0, HitsBefore), 0.f, FMath::Max(0.f, V.max_bonus));
	}

	/** Damage a hit needs to interrupt the target's running attack: a share of its maximum HP. */
	inline float InterruptThreshold(const FHydraInterruptValues& V, float TargetMaxHp)
	{
		return V.threshold_share_of_max_hp * FMath::Max(0.f, TargetMaxHp);
	}

	/**
	 * Progress bonus from strength and dexterity for a weapon or body attack: its fixed
	 * strength share x the strength bonus plus the rest x the dexterity bonus. Both count only
	 * what the level added on top of the base value (the gain).
	 */
	inline float AttributeDamageBonus(const FHydraDamageValues& V, float StrengthShare, float StrengthGain, float DexterityGain)
	{
		const float Share = FMath::Clamp(StrengthShare, 0.f, 1.f);
		return Share * V.strength_bonus_per_gain * StrengthGain + (1.f - Share) * V.dexterity_bonus_per_gain * DexterityGain;
	}

	/** Light base damage that follows from a light cost (how the stored base damage of a weapon was derived). */
	inline float BaseDamageFromCost(const FHydraDamageValues& V, float WeightKg, float LightCost)
	{
		const float Reference = FMath::Max(V.efficiency_reference_kg, KINDA_SMALL_NUMBER);
		return V.efficiency_base * FMath::Pow(FMath::Max(WeightKg, KINDA_SMALL_NUMBER) / Reference, V.efficiency_exponent) * LightCost;
	}

	// --- Block and rebound: shares of the damage actually blocked -------------------------

	/** Stamina the blocker pays per point of blocked damage: less with a heavier shield, inside the limits. */
	inline float BlockerShare(const FHydraBlockValues& V, float ShieldWeight, float BodyWeight)
	{
		const float Ratio = BodyWeight > KINDA_SMALL_NUMBER ? ShieldWeight / BodyWeight : 0.f;
		const float Raw = Ratio > KINDA_SMALL_NUMBER ? V.blocker_share * V.reference_shield_ratio / Ratio : V.share_max;
		return FMath::Clamp(Raw, V.share_min, FMath::Max(V.share_min, V.share_max));
	}

	/** Rebound: stamina the attacker loses when its hit lands in a block. */
	inline float ReboundStamina(const FHydraBlockValues& V, float BlockedDamage)
	{
		return V.rebound_share * FMath::Max(0.f, BlockedDamage);
	}

	// --- Weak points -----------------------------------------------------------------------

	/** Damage factor for a hit on a weak point: dexterity raises it, whatever the weapon weighs. */
	inline float WeakPointMultiplier(const FHydraWeakPointValues& V, float DexterityGain)
	{
		return FMath::Clamp(V.damage_multiplier + V.multiplier_per_dexterity_gain * FMath::Max(0.f, DexterityGain), 1.f, FMath::Max(1.f, V.max_damage_multiplier));
	}

	// --- Resistances, bolt, burning ------------------------------------------------------

	/** Resistance points of a form against a damage type ("slash", "pierce", "blunt", "fire"); 0 for anything else. */
	inline float ResistanceTo(const FHydraResistanceValues& R, FName DamageType)
	{
		static const FName Slash(TEXT("slash")), Pierce(TEXT("pierce")), Blunt(TEXT("blunt")), Fire(TEXT("fire"));
		if (DamageType == Slash) { return R.slash; }
		if (DamageType == Pierce) { return R.pierce; }
		if (DamageType == Blunt) { return R.blunt; }
		if (DamageType == Fire) { return R.fire; }
		return 0.f;
	}

	/** Share of the damage that resistance points take away: points / (points + constant); 0 for a weakness. */
	inline float ResistanceReduction(const FHydraResistanceRuleValues& V, float Points)
	{
		return Points > 0.f ? Points / (Points + FMath::Max(V.curve_constant, KINDA_SMALL_NUMBER)) : 0.f;
	}

	/** Damage after resistance: the flattening curve for points, x (1 + |points| / constant) for a weakness. */
	inline float ResistedDamage(const FHydraResistanceRuleValues& V, float Damage, float Points)
	{
		const float Constant = FMath::Max(V.curve_constant, KINDA_SMALL_NUMBER);
		const float Factor = Points >= 0.f ? 1.f - ResistanceReduction(V, Points) : 1.f + (-Points) / Constant;
		return FMath::Max(0.f, Damage * Factor);
	}

	/** How far a bolt was charged when released after HeldMs: 0 at the wind-up, 1 at max charge. */
	inline float BoltChargeAlpha(const FHydraBoltValues& B, float HeldMs)
	{
		const float Span = B.max_charge_ms - B.windup_ms;
		return Span > KINDA_SMALL_NUMBER ? FMath::Clamp((HeldMs - B.windup_ms) / Span, 0.f, 1.f) : 0.f;
	}

	/** Charging makes the bolt stronger and costlier. */
	inline float BoltDamage(const FHydraBoltValues& B, float ChargeAlpha)
	{
		return B.damage * FMath::Lerp(1.f, FMath::Max(1.f, B.full_charge_damage_factor), FMath::Clamp(ChargeAlpha, 0.f, 1.f));
	}

	inline float BoltManaCost(const FHydraBoltValues& B, float ChargeAlpha)
	{
		return B.mana_cost * FMath::Lerp(1.f, FMath::Max(1.f, B.full_charge_cost_factor), FMath::Clamp(ChargeAlpha, 0.f, 1.f));
	}

	/**
	 * The charge a caster can pay for: with ManaLeft after the base cost, the wanted charge is
	 * cut down to what the extra mana buys.
	 */
	inline float AffordableBoltCharge(const FHydraBoltValues& B, float WantedAlpha, float ManaLeft)
	{
		const float ExtraAtFull = B.mana_cost * (FMath::Max(1.f, B.full_charge_cost_factor) - 1.f);
		const float Affordable = ExtraAtFull > KINDA_SMALL_NUMBER ? FMath::Clamp(ManaLeft / ExtraAtFull, 0.f, 1.f) : 1.f;
		return FMath::Min(FMath::Clamp(WantedAlpha, 0.f, 1.f), Affordable);
	}

	/** Seconds between two changes of the burning stack; fire resistance lengthens it. */
	inline float BurningIntervalS(const FHydraBurningValues& B, const FHydraResistanceRuleValues& V, float FireResistancePoints)
	{
		return B.stack_interval_ms / 1000.f / (1.f - FMath::Min(ResistanceReduction(V, FireResistancePoints), 0.9f));
	}

	/** Shortest distance between a point and the segment A-B. */
	inline float DistancePointToSegment(const FVector& Point, const FVector& A, const FVector& B)
	{
		const FVector Axis = B - A;
		const float T = Axis.SizeSquared() > KINDA_SMALL_NUMBER ? FMath::Clamp(FVector::DotProduct(Point - A, Axis) / Axis.SizeSquared(), 0.f, 1.f) : 0.f;
		return FVector::Dist(Point, A + Axis * T);
	}

	// --- Positions and arcs ----------------------------------------------------------------

	/** Is Other inside the arc of HalfArcDeg around the facing OwnerYaw (2D)? */
	inline bool IsInArc(const FVector& OwnerLocation, float OwnerYaw, const FVector& OtherLocation, float HalfArcDeg)
	{
		const FVector Delta = OtherLocation - OwnerLocation;
		if (Delta.SizeSquared2D() < KINDA_SMALL_NUMBER)
		{
			return true;
		}
		const float ToOther = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		return FMath::Abs(FMath::FindDeltaAngleDegrees(OwnerYaw, ToOther)) <= HalfArcDeg;
	}

	/** Is Other inside the arc around the back of the owner (weak point)? */
	inline bool IsInBackArc(const FVector& OwnerLocation, float OwnerYaw, const FVector& OtherLocation, float HalfArcDeg)
	{
		return IsInArc(OwnerLocation, OwnerYaw + 180.f, OtherLocation, HalfArcDeg);
	}

	/**
	 * Hit area of a melee attack: the target is hit if its body (a circle of TargetRadiusCm)
	 * reaches into ReachCm and its centre is inside the attack arc.
	 */
	inline bool IsInHitArea(const FVector& AttackerLocation, float AttackerYaw, const FVector& TargetLocation, float TargetRadiusCm, float ReachCm, float HalfArcDeg, float MaxHeightDiffCm = 150.f)
	{
		const FVector Delta = TargetLocation - AttackerLocation;
		if (FMath::Abs(Delta.Z) > MaxHeightDiffCm || Delta.Size2D() - TargetRadiusCm > ReachCm)
		{
			return false;
		}
		return IsInArc(AttackerLocation, AttackerYaw, TargetLocation, HalfArcDeg);
	}

	/**
	 * Is Target inside the cone from Origin along Direction (2D, half angle MaxAngleDeg, length
	 * RangeCm)? OutScore: lower is better - near the centre of the cone first, then near.
	 */
	inline bool ScoreTargetInCone(const FVector& Origin, const FVector& Direction, const FVector& Target, float RangeCm, float MaxAngleDeg, float& OutScore)
	{
		const FVector Delta = Target - Origin;
		const float Dist = Delta.Size2D();
		if (Dist > RangeCm || RangeCm <= 0.f)
		{
			return false;
		}
		const float Cos = Dist > KINDA_SMALL_NUMBER ? FVector::DotProduct(Delta.GetSafeNormal2D(), Direction.GetSafeNormal2D()) : 1.f;
		if (Cos < FMath::Cos(FMath::DegreesToRadians(MaxAngleDeg)))
		{
			return false;
		}
		OutScore = (1.f - Cos) * 4.f + Dist / RangeCm;
		return true;
	}

	/**
	 * Soft aim assist: the facing to turn towards, between CurrentYaw and TargetYaw by Strength
	 * (0 = no help, 1 = face the target exactly). The character turns there at its turn rate.
	 */
	inline float AimAssistYaw(float CurrentYaw, float TargetYaw, float Strength)
	{
		return CurrentYaw + FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw) * FMath::Clamp(Strength, 0.f, 1.f);
	}

	// --- Network ---------------------------------------------------------------------------

	/**
	 * Lag compensation: how far the server looks back for an attacker with this ping. The
	 * attacker saw the others one way late and its input took one way to arrive, so the
	 * rewind is the measured round trip, capped by lag_compensation.max_rewind_ms.
	 */
	inline float RewindSeconds(float AttackerPingMs, const FHydraLagCompensationValues& V)
	{
		return FMath::Clamp(AttackerPingMs, 0.f, FMath::Max(0.f, V.max_rewind_ms)) / 1000.f;
	}

	/** Network emulation: delay each side adds to its outgoing packets for a latency step. */
	inline int32 OutgoingLagMs(int32 LatencyStepMs, bool bStepIsRoundTrip)
	{
		return FMath::Max(0, bStepIsRoundTrip ? LatencyStepMs / 2 : LatencyStepMs);
	}

	/** Network emulation: the engine takes packet loss as a whole percent. */
	inline int32 PacketLossPercent(float Ratio)
	{
		return FMath::Clamp(FMath::RoundToInt(Ratio * 100.f), 0, 100);
	}

	/** Next entry of the latency steps after Current (wraps; unknown Current starts at the first). */
	inline int32 NextLatencyStep(const TArray<int32>& Steps, int32 Current)
	{
		if (Steps.Num() == 0)
		{
			return 0;
		}
		return Steps[(Steps.IndexOfByKey(Current) + 1) % Steps.Num()];
	}

	// --- Progression ------------------------------------------------------------------------

	/** nXP needed for the step from Level to Level + 1. */
	inline float XpForNextLevel(const FHydraProgressionValues& P, int32 Level)
	{
		return FMath::Max(1.f, P.xp_base + P.xp_per_level * FMath::Max(0, Level));
	}

	/** nXP for defeating an enemy: by its type and level only, never by the level of the winner. */
	inline float EnemyNxp(const FHydraProgressionValues& P, float NxpBase, int32 EnemyLevel)
	{
		return NxpBase * (1.f + P.nxp_bonus_per_enemy_level * FMath::Max(0, EnemyLevel));
	}

	/** hXP of a defeated enemy: its nXP x the stand-in factor of its type (at least 1). */
	inline float EnemyHxp(float Nxp, float HxpFactor)
	{
		return Nxp * FMath::Max(1.f, HxpFactor);
	}

	/** A milestone: every milestone_every_levels levels, and the form's level cap. */
	inline bool IsMilestone(const FHydraProgressionValues& P, int32 Level, int32 LevelCap)
	{
		return Level > 0 && ((P.milestone_every_levels > 0 && Level % P.milestone_every_levels == 0) || Level == LevelCap);
	}

	/** Hardship rating of a level: hXP / nXP earned in it, at least 1. */
	inline float HardshipRating(float Hxp, float Nxp)
	{
		return Nxp > KINDA_SMALL_NUMBER ? FMath::Max(1.f, Hxp / Nxp) : 1.f;
	}

	/** Automatic skill points at a level-up: base + factor x log2(hardship rating). */
	inline float SkillPoints(const FHydraSkillPointsValues& V, float Hardship)
	{
		return V.base + V.factor * FMath::Log2(FMath::Max(1.f, Hardship));
	}

	/** Imprint progress needed for the next imprint level; the first one is the easiest. */
	inline float ImprintProgressNeeded(const FHydraImprintValues& I, int32 CurrentImprintLevel)
	{
		return CurrentImprintLevel <= 0 ? I.progress_first_level : I.progress_per_further_level;
	}

	/** Instinct: how high a skill was relative to the old form's level cap (0..1). */
	inline float InstinctRatio(int32 OldSkillLevel, int32 OldLevelCap)
	{
		return OldLevelCap > 0 ? FMath::Clamp(static_cast<float>(OldSkillLevel) / OldLevelCap, 0.f, 1.f) : 0.f;
	}

	/** Skill level as the rules use it: the level plus the permanent instinct bonus. */
	inline float EffectiveSkillLevel(const FHydraInstinctValues& I, int32 SkillLevel, float Ratio)
	{
		return SkillLevel + I.skill_level_bonus_max * FMath::Clamp(Ratio, 0.f, 1.f);
	}

	/** A skill with an instinct is relearned faster: its share of the hXP counts this much more when points are split. */
	inline float InstinctShareFactor(const FHydraInstinctValues& I, float Ratio)
	{
		return 1.f + I.xp_bonus_max * FMath::Clamp(Ratio, 0.f, 1.f);
	}

	/**
	 * Splits skill points over skills in proportion to Weights (hXP of the level x instinct
	 * factor). Each skill has a level and a progress bar (0..1): whole points raise the level,
	 * the rest stays on the bar. No skill rises above MaxLevel (the character level); what
	 * such a skill cannot take goes to the other skills with a weight, and if there is none,
	 * it waits on its bar. Arrays are parallel.
	 */
	inline void DistributeSkillPoints(float Points, const TArray<float>& Weights, int32 MaxLevel, TArray<int32>& InOutLevels, TArray<float>& InOutProgress)
	{
		const int32 Num = Weights.Num();
		check(InOutLevels.Num() == Num && InOutProgress.Num() == Num);
		float Remaining = FMath::Max(0.f, Points);
		// A few rounds are enough: every round at least one more skill reaches the limit.
		for (int32 Round = 0; Round <= Num && Remaining > KINDA_SMALL_NUMBER; ++Round)
		{
			float WeightSum = 0.f;
			for (int32 i = 0; i < Num; ++i)
			{
				if (Weights[i] > 0.f && InOutLevels[i] < MaxLevel)
				{
					WeightSum += Weights[i];
				}
			}
			if (WeightSum <= KINDA_SMALL_NUMBER)
			{
				break;
			}
			float Excess = 0.f;
			for (int32 i = 0; i < Num; ++i)
			{
				if (Weights[i] <= 0.f || InOutLevels[i] >= MaxLevel)
				{
					continue;
				}
				InOutProgress[i] += Remaining * Weights[i] / WeightSum;
				while (InOutProgress[i] >= 1.f - KINDA_SMALL_NUMBER && InOutLevels[i] < MaxLevel)
				{
					InOutProgress[i] = FMath::Max(0.f, InOutProgress[i] - 1.f);
					++InOutLevels[i];
				}
				if (InOutLevels[i] >= MaxLevel)
				{
					Excess += InOutProgress[i];
					InOutProgress[i] = 0.f;
				}
			}
			Remaining = Excess;
		}
		if (Remaining > KINDA_SMALL_NUMBER)
		{
			// Nobody else can take it: it waits on the bars of the used skills.
			float WeightSum = 0.f;
			for (int32 i = 0; i < Num; ++i)
			{
				WeightSum += FMath::Max(0.f, Weights[i]);
			}
			for (int32 i = 0; i < Num && WeightSum > KINDA_SMALL_NUMBER; ++i)
			{
				InOutProgress[i] += Remaining * FMath::Max(0.f, Weights[i]) / WeightSum;
			}
		}
	}

	// --- Stats ------------------------------------------------------------------------------

	/**
	 * Resolves a form and its loadout and derives every number of the character from its
	 * attributes (form x level), weight, skills and imprint. bValid is false for an unknown
	 * form or one without numbers.
	 */
	inline FHydraCombatStats BuildStats(const FHydraDefinitions& Defs, const FHydraValues& V, FName FormId, const FHydraProgressView& Progress = FHydraProgressView())
	{
		static const FName SkillProjectile(TEXT("projectile")), ElementFire(TEXT("fire"));
		FHydraCombatStats S;
		S.FormId = FormId;
		const FHydraFormDef* Form = Defs.FindForm(FormId);
		if (!Form)
		{
			return S;
		}
		const FHydraFormValues* Numbers = V.forms.Find(FormId);
		if (!Numbers)
		{
			return S;
		}
		S.bValid = true;
		S.Level = Progress.Level;
		S.LevelCap = Numbers->level_cap;
		S.ImprintLevel = Progress.ImprintLevel;

		// Body attributes: base value of the form x (1 + growth x level). No other level bonus.
		S.AttributeGain = AttributeGain(V.progression, Progress.Level);
		const float Grown = 1.f + S.AttributeGain;
		S.BodyWeight = Numbers->body_weight;
		S.Vitality = Numbers->vitality * Grown;
		S.Endurance = Numbers->endurance * Grown;
		S.Spirit = Numbers->spirit * Grown;
		S.Strength = Numbers->strength * Grown;
		S.Dexterity = Numbers->dexterity * Grown;
		S.BodyRadiusCm = Numbers->body_radius_m * 100.f;
		S.BodyHalfHeightCm = FMath::Max(Numbers->body_height_m * 50.f, S.BodyRadiusCm);
		S.bCanCast = Form->skills.Contains(SkillProjectile);

		// Bars: 10 per point; regeneration as a share of the maximum. A form without a spell form has no mana bar.
		S.HpMax = 10.f * S.Vitality;
		S.StaminaMax = 10.f * S.Endurance;
		S.StaminaRegenPerS = S.StaminaMax * Numbers->stamina_regen_share_per_s;
		S.ManaMax = S.bCanCast ? 10.f * S.Spirit : 0.f;
		S.ManaRegenPerS = S.ManaMax * Numbers->mana_regen_share_per_s;
		S.InterruptThreshold = InterruptThreshold(V.interrupt, S.HpMax);

		// Resistance points; the imprint adds points against its element.
		S.Resistance = Numbers->resistance;
		S.Resistance.fire += V.imprint.resistance_points_per_level * Progress.ImprintLevel;
		const float ImprintBonus = V.imprint.damage_bonus_per_level * Progress.ImprintLevel;
		S.ElementBodyDamage = Form->element_body == ElementFire ? Numbers->element_body_damage * (1.f + ImprintBonus) : 0.f;

		// The bolt: spell damage comes from skill and imprint, not from strength or dexterity.
		S.BoltBonus = V.skills.damage_bonus_per_level * Progress.Skill(SkillProjectile);
		S.FireBoltBonus = S.BoltBonus + ImprintBonus;
		if (const FHydraElementDef* Fire = Defs.FindElement(ElementFire))
		{
			S.FireBoltBonus += V.skills.damage_bonus_per_level * Progress.Skill(Fire->element_skill);
			S.FieldRadiusFactor = 1.f + V.skills.field_radius_bonus_per_level * Progress.Skill(Fire->element_skill);
		}

		// Everything carried counts towards the weight; the first weapon and the first shield are in the hands.
		S.TotalWeight = S.BodyWeight;
		for (const FName Item : Form->loadout)
		{
			if (const FHydraWeaponValues* W = V.weapons.Find(Item))
			{
				S.TotalWeight += W->weight;
				if (!S.bHasWeapon)
				{
					S.bHasWeapon = true;
					S.Weapon = *W;
					if (const FHydraWeaponDef* Def = Defs.FindWeapon(Item))
					{
						S.LightDamageType = Def->light_damage_type;
						S.HeavyDamageType = Def->heavy_damage_type;
						S.WeaponSkill = Def->skill;
					}
				}
			}
			else if (const FHydraShieldValues* Sh = V.shields.Find(Item))
			{
				S.TotalWeight += Sh->weight;
				if (!S.bHasShield)
				{
					S.bHasShield = true;
					S.Shield = *Sh;
					if (const FHydraWeaponDef* Def = Defs.FindWeapon(Item))
					{
						S.ShieldSkill = Def->skill;
					}
				}
			}
		}

		// Handling comes from strength: total weight / strength.
		S.WeightRatio = WeightRatio(S.TotalWeight, S.Strength);
		S.TurnRateDegS = TurnRateDegS(V.turn_rate, S.WeightRatio);
		// Dodge: a capability. Cost from the weight, distance and speed from dexterity.
		S.DodgeStaminaCost = DodgeStaminaCost(Numbers->dodge_stamina_cost, S.TotalWeight, S.BodyWeight);
		S.DodgeDistanceFactor = 1.f + V.dodge.distance_per_dexterity_gain * S.AttributeGain;
		S.DodgeDurationFactor = 1.f / (1.f + V.dodge.speed_per_dexterity_gain * S.AttributeGain);
		S.WeakPointMultiplier = WeakPointMultiplier(V.weak_point, S.AttributeGain);
		if (S.bHasWeapon)
		{
			// Cost: weight / body weight x demand only. Damage: base x (1 + skill bonus + attribute bonus).
			const float CostRatio = AttackCostRatio(S.Weapon, S.BodyWeight);
			S.LightStaminaCost = AttackStaminaCost(V.attack.light_stamina_per_weight_ratio, CostRatio, S.Weapon.demand);
			S.HeavyStaminaCost = AttackStaminaCost(V.attack.heavy_stamina_per_weight_ratio, CostRatio, S.Weapon.demand);
			const float Bonus = V.skills.damage_bonus_per_level * Progress.Skill(S.WeaponSkill)
				+ AttributeDamageBonus(V.damage, S.Weapon.strength_share, S.AttributeGain, S.AttributeGain);
			S.Weapon.light.damage *= 1.f + Bonus;
			S.Weapon.heavy.damage *= 1.f + Bonus;
		}
		if (S.bHasShield)
		{
			S.BlockHoldStaminaPerS = BlockHoldStaminaPerS(V.block, S.Shield.weight, S.BodyWeight);
			S.BlockerShare = BlockerShare(V.block, S.Shield.weight, S.BodyWeight);
			S.Shield.block_half_arc_deg = FMath::Min(180.f, S.Shield.block_half_arc_deg + V.skills.block_arc_deg_per_level * Progress.Skill(S.ShieldSkill));
		}
		return S;
	}
}
