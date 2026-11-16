#pragma once

#include "CoreMinimal.h"
#include "Data/HydraDefinitions.h"
#include "Data/HydraValues.h"

/**
 * What a character is made of for combat: its form with the loadout resolved and the
 * numbers that follow from weight and strength. Built from forms.json (loadout) and
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

	float HpMax = 0.f;
	float StaminaMax = 0.f;
	float StaminaRegenPerS = 0.f;

	bool bHasWeapon = false;
	FHydraWeaponValues Weapon;
	bool bHasShield = false;
	FHydraShieldValues Shield;

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

	// --- Stats ------------------------------------------------------------------------------

	/** Resolves a form and its loadout and derives the weight-based numbers. bValid is false for an unknown form or one without numbers. */
	inline FHydraCombatStats BuildStats(const FHydraDefinitions& Defs, const FHydraValues& V, FName FormId)
	{
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
		S.HpMax = Numbers->hp_max;
		S.StaminaMax = Numbers->stamina_max;
		S.StaminaRegenPerS = Numbers->stamina_regen_per_s;
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
				}
			}
			else if (const FHydraShieldValues* Sh = V.shields.Find(Item))
			{
				S.TotalWeight += Sh->weight;
				if (!S.bHasShield)
				{
					S.bHasShield = true;
					S.Shield = *Sh;
				}
			}
		}
		S.WeightRatio = WeightRatio(S.TotalWeight, S.Strength);
		S.TurnRateDegS = TurnRateDegS(V.turn_rate, S.WeightRatio);
		S.DodgeStaminaCost = DodgeStaminaCost(V.dodge, S.WeightRatio);
		if (S.bHasWeapon)
		{
			S.WeaponWeightRatio = WeightRatio(S.Weapon.weight, S.Strength);
			S.LightStaminaCost = AttackStaminaCost(V.attack.light_stamina_per_weight_ratio, S.Weapon.weight, S.Strength);
			S.HeavyStaminaCost = AttackStaminaCost(V.attack.heavy_stamina_per_weight_ratio, S.Weapon.weight, S.Strength);
		}
		return S;
	}
}
