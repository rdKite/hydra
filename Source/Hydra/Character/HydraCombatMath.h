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
 * numbers that follow from weight and strength, level, skills and imprint. Built from forms.json (loadout) and
 * values.json (numbers per form, weapon and shield); rebuilt after
 * Hydra.ReloadValues.
 */
struct FHydraCombatStats
{
	bool bValid = false;
	FName FormId;

	float BodyWeight = 0.f;
	float Strength = 1.f;
	/** Body plus load (every carried weapon and shield). */
	float TotalWeight = 0.f;
	/** TotalWeight / Strength: drives turn rate and dodge cost. */
	float WeightRatio = 1.f;

	/** Body size (capsule) in cm. */
	float BodyRadiusCm = 0.f;
	float BodyHalfHeightCm = 0.f;

	float HpMax = 0.f;
	float StaminaMax = 0.f;
	float StaminaRegenPerS = 0.f;
	float ManaMax = 0.f;
	float ManaRegenPerS = 0.f;
	FHydraResistanceValues Resistance;

	int32 Level = 0;
	int32 LevelCap = 0;
	int32 ImprintLevel = 0;

	/** The form has the body skill projectile: its off-hand button casts the bolt. */
	bool bCanCast = false;
	/** The bolt with this character's level, skill and cost factor applied (damage, mana_cost). */
	FHydraBoltValues Bolt;
	/** Damage factor for fire damage it deals (imprint). */
	float FireDamageFactor = 1.f;
	/** Radius factor of its fire fields (fire magic skill). */
	float FieldRadiusFactor = 1.f;
	/** Element body: fire damage a melee hit against it deals back. */
	float ElementBodyDamage = 0.f;

	bool bHasWeapon = false;
	/** The weapon in the hand; its damage already includes level and skill. */
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
	/** Factor on the stamina a blocked hit costs (shield skill). */
	float BlockCostFactor = 1.f;
	/** Factor on the length of the dodge step (dodge skill). */
	float DodgeDistanceFactor = 1.f;

	/** Weapon weight / strength: drives attack cost and the weak-point bonus. */
	float WeaponWeightRatio = 0.f;
	float LightStaminaCost = 0.f;
	float HeavyStaminaCost = 0.f;
	float DodgeStaminaCost = 0.f;
	float TurnRateDegS = 0.f;
};

/**
 * Pure functions for the positional combat rules (no world access), so they can be
 * unit-tested and are identical on server and client. Distances in cm, angles in degrees,
 * times in ms unless the name says otherwise.
 */
namespace HydraCombatMath
{
	// --- Weight and strength --------------------------------------------------------------

	inline float WeightRatio(float Weight, float Strength)
	{
		return Strength > KINDA_SMALL_NUMBER ? FMath::Max(0.f, Weight) / Strength : 1.f;
	}

	/**
	 * Turn rate in deg/s: base / ratio^exponent, slower while blocking and while attacking,
	 * always inside floor and cap (nobody is helpless, nobody turns instantly).
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

	// --- Costs ------------------------------------------------------------------------------

	/** stamina cost = factor x weapon weight / strength (x skill factor from M4 on). */
	inline float AttackStaminaCost(float StaminaPerWeightRatio, float WeaponWeight, float Strength)
	{
		return StaminaPerWeightRatio * WeightRatio(WeaponWeight, Strength);
	}

	inline float DodgeStaminaCost(const FHydraDodgeValues& V, float CharacterWeightRatio)
	{
		return V.stamina_cost * FMath::Max(0.f, CharacterWeightRatio);
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

	/**
	 * Damage a hit needs to interrupt the target's running attack. Depends on the target's
	 * weight (body plus load); the weapon it is attacking with counts extra.
	 */
	inline float InterruptThreshold(const FHydraInterruptValues& V, float TargetTotalWeight, float TargetAttackingWeaponWeight)
	{
		return V.damage_threshold_per_weight * (TargetTotalWeight + V.attacking_weapon_weight_factor * TargetAttackingWeaponWeight);
	}

	// --- Block and rebound ----------------------------------------------------------------

	/** Attacking weapon weight / blocking item weight, inside the limits from values.json. */
	inline float BlockWeightRatio(const FHydraBlockValues& V, float AttackWeaponWeight, float BlockItemWeight)
	{
		const float Raw = BlockItemWeight > KINDA_SMALL_NUMBER ? AttackWeaponWeight / BlockItemWeight : V.weight_ratio_max;
		return FMath::Clamp(Raw, V.weight_ratio_min, FMath::Max(V.weight_ratio_min, V.weight_ratio_max));
	}

	/** Stamina the blocker pays for a hit inside the block arc: more against heavier weapons and heavy attacks. */
	inline float BlockStaminaCost(const FHydraBlockValues& V, float AttackWeaponWeight, float BlockItemWeight, bool bHeavyAttack)
	{
		return V.stamina_cost_per_hit * BlockWeightRatio(V, AttackWeaponWeight, BlockItemWeight) * (bHeavyAttack ? V.heavy_attack_factor : 1.f);
	}

	/** Rebound: extra stamina the attacker loses when the hit lands in a block. Less for heavier weapons. */
	inline float ReboundStamina(const FHydraBlockValues& V, float AttackStaminaCost, float AttackWeaponWeight, float BlockItemWeight)
	{
		return V.rebound_attacker_stamina_ratio * AttackStaminaCost / BlockWeightRatio(V, AttackWeaponWeight, BlockItemWeight);
	}

	// --- Weak points -----------------------------------------------------------------------

	/** Damage factor for a hit on a weak point; the lighter the weapon relative to strength, the bigger. */
	inline float WeakPointMultiplier(const FHydraWeakPointValues& V, float WeaponWeightRatio)
	{
		const float Scale = WeaponWeightRatio > KINDA_SMALL_NUMBER ? V.reference_weapon_weight_ratio / WeaponWeightRatio : 1.f;
		return FMath::Clamp(1.f + (V.damage_multiplier - 1.f) * Scale, 1.f, FMath::Max(1.f, V.max_damage_multiplier));
	}

	// --- Resistances, bolt, burning ------------------------------------------------------

	/** Resistance of a form against a damage type ("slash", "pierce", "blunt", "fire"); 0 for anything else. */
	inline float ResistanceTo(const FHydraResistanceValues& R, FName DamageType)
	{
		static const FName Slash(TEXT("slash")), Pierce(TEXT("pierce")), Blunt(TEXT("blunt")), Fire(TEXT("fire"));
		if (DamageType == Slash) { return R.slash; }
		if (DamageType == Pierce) { return R.pierce; }
		if (DamageType == Blunt) { return R.blunt; }
		if (DamageType == Fire) { return R.fire; }
		return 0.f;
	}

	/** Damage after resistance: a resistance of 0.5 halves it, -0.25 adds a quarter. */
	inline float ResistedDamage(float Damage, float Resistance)
	{
		return FMath::Max(0.f, Damage * (1.f - FMath::Clamp(Resistance, -1.f, 0.95f)));
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
	inline float BurningIntervalS(const FHydraBurningValues& B, float FireResistance)
	{
		return B.stack_interval_ms / 1000.f / (1.f - FMath::Clamp(FireResistance, 0.f, 0.9f));
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

	/** A milestone: every milestone_every_levels levels, and the form's level cap. */
	inline bool IsMilestone(const FHydraProgressionValues& P, int32 Level, int32 LevelCap)
	{
		return Level > 0 && ((P.milestone_every_levels > 0 && Level % P.milestone_every_levels == 0) || Level == LevelCap);
	}

	/** Factor from the character level (HP, base damage): 1 + bonus per level x level. */
	inline float LevelFactor(float BonusPerLevel, int32 Level)
	{
		return 1.f + BonusPerLevel * FMath::Max(0, Level);
	}

	/** Skill XP needed for the step from SkillLevel to SkillLevel + 1. */
	inline float SkillXpForNextLevel(const FHydraSkillsValues& S, int32 SkillLevel)
	{
		return FMath::Max(1.f, S.xp_base + S.xp_per_level * FMath::Max(0, SkillLevel));
	}

	inline float SkillDamageFactor(const FHydraSkillsValues& S, float SkillLevel)
	{
		return 1.f + S.damage_bonus_per_level * FMath::Max(0.f, SkillLevel);
	}

	/** A higher skill lowers the cost of its actions, down to min_cost_factor. */
	inline float SkillCostFactor(const FHydraSkillsValues& S, float SkillLevel)
	{
		return FMath::Max(S.min_cost_factor, 1.f - S.cost_reduction_per_level * FMath::Max(0.f, SkillLevel));
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

	/** Skill XP an action gives; a skill with an instinct is relearned faster. */
	inline float SkillXpGain(const FHydraInstinctValues& I, float Amount, float Ratio)
	{
		return Amount * (1.f + I.xp_bonus_max * FMath::Clamp(Ratio, 0.f, 1.f));
	}

	// --- Stats ------------------------------------------------------------------------------

	/**
	 * Resolves a form and its loadout and derives the numbers that follow from weight and
	 * strength and from the progress (level, skills, imprint). bValid is false for an unknown
	 * form or one without numbers.
	 */
	inline FHydraCombatStats BuildStats(const FHydraDefinitions& Defs, const FHydraValues& V, FName FormId, const FHydraProgressView& Progress = FHydraProgressView())
	{
		static const FName SkillDodge(TEXT("dodge")), SkillProjectile(TEXT("projectile")), ElementFire(TEXT("fire"));
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
		S.BodyWeight = Numbers->body_weight;
		S.Strength = Numbers->strength;
		S.Level = Progress.Level;
		S.LevelCap = Numbers->level_cap;
		S.ImprintLevel = Progress.ImprintLevel;
		// Each level gives a little more HP and base damage; most strength comes from skills.
		const float LevelDamage = LevelFactor(V.progression.damage_bonus_per_level, Progress.Level);
		S.HpMax = Numbers->hp_max * LevelFactor(V.progression.hp_bonus_per_level, Progress.Level);
		S.StaminaMax = Numbers->stamina_max;
		S.StaminaRegenPerS = Numbers->stamina_regen_per_s;
		S.ManaMax = Numbers->mana_max;
		S.ManaRegenPerS = Numbers->mana_regen_per_s;
		S.Resistance = Numbers->resistance;
		// Imprint: resistance against the element and more damage with it.
		S.Resistance.fire = FMath::Min(0.95f, S.Resistance.fire + V.imprint.resistance_per_level * Progress.ImprintLevel);
		S.FireDamageFactor = 1.f + V.imprint.damage_bonus_per_level * Progress.ImprintLevel;
		S.ElementBodyDamage = Form->element_body == ElementFire ? Numbers->element_body_damage * LevelDamage : 0.f;
		S.BodyRadiusCm = Numbers->body_radius_m * 100.f;
		S.BodyHalfHeightCm = FMath::Max(Numbers->body_height_m * 50.f, S.BodyRadiusCm);
		S.bCanCast = Form->skills.Contains(SkillProjectile);
		S.Bolt = V.bolt;
		S.Bolt.damage *= LevelDamage * SkillDamageFactor(V.skills, Progress.Skill(SkillProjectile));
		S.Bolt.mana_cost *= SkillCostFactor(V.skills, Progress.Skill(SkillProjectile));
		if (const FHydraElementDef* Fire = Defs.FindElement(ElementFire))
		{
			S.FieldRadiusFactor = 1.f + V.skills.field_radius_bonus_per_level * Progress.Skill(Fire->element_skill);
		}
		S.TotalWeight = S.BodyWeight;
		// Everything carried counts towards the weight; the first weapon and the first shield are in the hands.
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
		S.WeightRatio = WeightRatio(S.TotalWeight, S.Strength);
		S.TurnRateDegS = TurnRateDegS(V.turn_rate, S.WeightRatio);
		// Skills: lower cost, more damage, and per kind a longer dodge step and a wider block arc.
		S.DodgeStaminaCost = DodgeStaminaCost(V.dodge, S.WeightRatio) * SkillCostFactor(V.skills, Progress.Skill(SkillDodge));
		S.DodgeDistanceFactor = 1.f + V.skills.dodge_distance_bonus_per_level * Progress.Skill(SkillDodge);
		if (S.bHasWeapon)
		{
			const float WeaponSkill = Progress.Skill(S.WeaponSkill);
			const float Cost = SkillCostFactor(V.skills, WeaponSkill);
			const float Damage = LevelDamage * SkillDamageFactor(V.skills, WeaponSkill);
			S.WeaponWeightRatio = WeightRatio(S.Weapon.weight, S.Strength);
			S.LightStaminaCost = AttackStaminaCost(V.attack.light_stamina_per_weight_ratio, S.Weapon.weight, S.Strength) * Cost;
			S.HeavyStaminaCost = AttackStaminaCost(V.attack.heavy_stamina_per_weight_ratio, S.Weapon.weight, S.Strength) * Cost;
			S.Weapon.light.damage *= Damage;
			S.Weapon.heavy.damage *= Damage;
		}
		if (S.bHasShield)
		{
			const float ShieldSkill = Progress.Skill(S.ShieldSkill);
			S.BlockCostFactor = SkillCostFactor(V.skills, ShieldSkill);
			S.Shield.block_half_arc_deg = FMath::Min(180.f, S.Shield.block_half_arc_deg + V.skills.block_arc_deg_per_level * ShieldSkill);
		}
		return S;
	}
}
