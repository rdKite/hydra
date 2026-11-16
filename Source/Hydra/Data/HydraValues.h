#pragma once

#include "CoreMinimal.h"
#include "HydraValues.generated.h"

// Mirrors data/values.json: every gameplay number (decision 2026-11-16: one file). General
// rules are sections of their own; the numbers of one form, weapon or shield are in the
// sections forms / weapons / shields, keyed by the ID from forms.json / weapons.json.
// Member names are snake_case on purpose, so that FJsonObjectConverter maps the JSON keys
// 1:1 without a hand-written parser (this deviates from Unreal naming). Units as in the
// file: times in ms, rates per second, distances in metres, angles in degrees (arcs as half
// angles); "weight ratio" = weight / strength.
// Adding a value: data/values.json (bump version, changelog line) plus a member here; the
// automation tests Hydra.Values.* fail if the two drift apart.

/** Numbers of one form. body_weight and strength share a unit with the weapon weights. */
USTRUCT()
struct FHydraFormValues
{
	GENERATED_BODY()
	/** Weight of the body (weight = body plus load). */
	UPROPERTY() float body_weight = 0.f;
	UPROPERTY() float strength = 0.f;
	UPROPERTY() float hp_max = 0.f;
	UPROPERTY() float stamina_max = 0.f;
	UPROPERTY() float stamina_regen_per_s = 0.f;
};

/** Light attack of a weapon: press the hand's button. */
USTRUCT()
struct FHydraLightAttackValues
{
	GENERATED_BODY()
	UPROPERTY() float damage = 0.f;
	UPROPERTY() float windup_ms = 0.f;
	UPROPERTY() float hit_ms = 0.f;
	UPROPERTY() float recovery_ms = 0.f;
	UPROPERTY() float hit_stop_ms = 0.f;
	UPROPERTY() float reach_m = 0.f;
	UPROPERTY() float half_arc_deg = 0.f;
};

/** Heavy attack: hold the button; it strikes on release between min_windup_ms and max_charge_ms. */
USTRUCT()
struct FHydraHeavyAttackValues
{
	GENERATED_BODY()
	/** Damage at min_windup_ms; grows linearly to x full_charge_damage_factor at max_charge_ms. */
	UPROPERTY() float damage = 0.f;
	UPROPERTY() float min_windup_ms = 0.f;
	UPROPERTY() float max_charge_ms = 0.f;
	UPROPERTY() float full_charge_damage_factor = 0.f;
	UPROPERTY() float hit_ms = 0.f;
	UPROPERTY() float recovery_ms = 0.f;
	UPROPERTY() float hit_stop_ms = 0.f;
	UPROPERTY() float reach_m = 0.f;
	UPROPERTY() float half_arc_deg = 0.f;
};

USTRUCT()
struct FHydraWeaponValues
{
	GENERATED_BODY()
	UPROPERTY() float weight = 0.f;
	UPROPERTY() FHydraLightAttackValues light;
	UPROPERTY() FHydraHeavyAttackValues heavy;
};

USTRUCT()
struct FHydraShieldValues
{
	GENERATED_BODY()
	UPROPERTY() float weight = 0.f;
	/** Directional block: half angle of the block arc and the share of damage it takes away. */
	UPROPERTY() float block_half_arc_deg = 0.f;
	UPROPERTY() float block_damage_reduction = 0.f;
};

USTRUCT()
struct FHydraCharacterValues
{
	GENERATED_BODY()
	UPROPERTY() float respawn_delay_ms = 0.f;
};

USTRUCT()
struct FHydraMovementValues
{
	GENERATED_BODY()
	UPROPERTY() float walk_speed_m_s = 0.f;
	UPROPERTY() float run_speed_m_s = 0.f;
	UPROPERTY() float strafe_speed_m_s = 0.f;
	/** How high a jump lifts the feet. */
	UPROPERTY() float jump_height_m = 0.f;
};

USTRUCT()
struct FHydraTurnRateValues
{
	GENERATED_BODY()
	/** Turn rate at weight ratio 1: rate = base / ratio^exponent, then the factors, then floor and cap. */
	UPROPERTY() float base_deg_s = 0.f;
	UPROPERTY() float min_deg_s = 0.f;
	UPROPERTY() float max_deg_s = 0.f;
	UPROPERTY() float weight_strength_exponent = 0.f;
	UPROPERTY() float blocking_factor = 0.f;
	UPROPERTY() float attacking_factor = 0.f;
};

USTRUCT()
struct FHydraLockOnValues
{
	GENERATED_BODY()
	UPROPERTY() float range_m = 0.f;
	UPROPERTY() float max_angle_deg = 0.f;
	UPROPERTY() float break_range_m = 0.f;
};

USTRUCT()
struct FHydraAimAssistValues
{
	GENERATED_BODY()
	UPROPERTY() float angle_deg = 0.f;
	UPROPERTY() float range_m = 0.f;
	UPROPERTY() float strength = 0.f;
};

USTRUCT()
struct FHydraStaminaValues
{
	GENERATED_BODY()
	UPROPERTY() float regen_delay_after_action_ms = 0.f;
};

USTRUCT()
struct FHydraAttackValues
{
	GENERATED_BODY()
	/** Stamina of a light / heavy attack = this x weapon weight / strength (x skill factor, M4). */
	UPROPERTY() float light_stamina_per_weight_ratio = 0.f;
	UPROPERTY() float heavy_stamina_per_weight_ratio = 0.f;
	/** Movement speed factor during wind-up and strike. */
	UPROPERTY() float move_speed_ratio = 0.f;
};

USTRUCT()
struct FHydraChainValues
{
	GENERATED_BODY()
	UPROPERTY() float damage_bonus_per_hit = 0.f;
	UPROPERTY() float max_bonus = 0.f;
	UPROPERTY() float reset_after_ms = 0.f;
};

USTRUCT()
struct FHydraInterruptValues
{
	GENERATED_BODY()
	/** A hit interrupts the target's attack if damage >= this x target weight (body plus load). */
	UPROPERTY() float damage_threshold_per_weight = 0.f;
	/** While attacking, the target's weapon weight counts this many times extra. */
	UPROPERTY() float attacking_weapon_weight_factor = 0.f;
};

USTRUCT()
struct FHydraBlockValues
{
	GENERATED_BODY()
	/** Stamina the blocker pays for a light attack by a weapon as heavy as the blocking item. */
	UPROPERTY() float stamina_cost_per_hit = 0.f;
	UPROPERTY() float heavy_attack_factor = 0.f;
	/** Limits for (attacking weapon weight / blocking item weight) in block cost and rebound. */
	UPROPERTY() float weight_ratio_min = 0.f;
	UPROPERTY() float weight_ratio_max = 0.f;
	UPROPERTY() bool no_regen_while_holding = false;
	UPROPERTY() float move_speed_ratio = 0.f;
	/** Block with the weapon (no shield): not used by the Phase 1 loadout yet. */
	UPROPERTY() float weapon_half_arc_deg = 0.f;
	/** Rebound: share of its own attack cost the attacker loses extra, at equal weights. */
	UPROPERTY() float rebound_attacker_stamina_ratio = 0.f;
};

USTRUCT()
struct FHydraDodgeValues
{
	GENERATED_BODY()
	/** At weight ratio 1; scales with the weight ratio (body plus load). */
	UPROPERTY() float stamina_cost = 0.f;
	UPROPERTY() float distance_m = 0.f;
	UPROPERTY() float duration_ms = 0.f;
	UPROPERTY() float recovery_ms = 0.f;
};

USTRUCT()
struct FHydraWeakPointValues
{
	GENERATED_BODY()
	UPROPERTY() float back_half_arc_deg = 0.f;
	/** Multiplier for a weapon at reference_weapon_weight_ratio; lighter weapons get more bonus. */
	UPROPERTY() float damage_multiplier = 0.f;
	UPROPERTY() float reference_weapon_weight_ratio = 0.f;
	UPROPERTY() float max_damage_multiplier = 0.f;
};

USTRUCT()
struct FHydraLagCompensationValues
{
	GENERATED_BODY()
	UPROPERTY() float max_rewind_ms = 0.f;
};

USTRUCT()
struct FHydraAiPracticeValues
{
	GENERATED_BODY()
	UPROPERTY() float engage_range_m = 0.f;
	/** Delay before it reacts to an attack it sees starting. */
	UPROPERTY() float reaction_time_ms = 0.f;
	/** Chance that it does not react to an attack at all. */
	UPROPERTY() float error_rate = 0.f;
	/** Pause between its own attacks. */
	UPROPERTY() float attack_pause_ms = 0.f;
	UPROPERTY() float heavy_attack_chance = 0.f;
	/** When it reacts: chance to block, chance to dodge; otherwise it takes the hit. */
	UPROPERTY() float block_chance = 0.f;
	UPROPERTY() float dodge_chance = 0.f;
	UPROPERTY() float block_hold_ms = 0.f;
};

USTRUCT()
struct FHydraManaValues
{
	GENERATED_BODY()
	UPROPERTY() float max = 0.f;
	UPROPERTY() float regen_per_s = 0.f;
};

USTRUCT()
struct FHydraFireBoltValues
{
	GENERATED_BODY()
	UPROPERTY() float mana_cost = 0.f;
	UPROPERTY() float damage = 0.f;
	UPROPERTY() float speed_m_s = 0.f;
	UPROPERTY() float range_m = 0.f;
	UPROPERTY() float field_radius_m = 0.f;
	UPROPERTY() float field_duration_ms = 0.f;
};

USTRUCT()
struct FHydraBurningValues
{
	GENERATED_BODY()
	UPROPERTY() float damage_per_s_per_stack = 0.f;
	UPROPERTY() int32 max_stacks = 0;
	UPROPERTY() float duration_ms = 0.f;
	UPROPERTY() int32 stacks_per_hit = 0;
	UPROPERTY() float tick_interval_ms = 0.f;
};

USTRUCT()
struct FHydraNetSimulationValues
{
	GENERATED_BODY()
	UPROPERTY() TArray<int32> latency_steps_ms;
	UPROPERTY() float packet_loss_ratio = 0.f;
	UPROPERTY() bool latency_is_round_trip = true;
	UPROPERTY() bool packet_loss_both_directions = true;
};

USTRUCT()
struct FHydraValues
{
	GENERATED_BODY()
	UPROPERTY() int32 version = 0;
	/** Per ID from forms.json. */
	UPROPERTY() TMap<FName, FHydraFormValues> forms;
	/** Per ID from weapons.json (entries with blocks = false). */
	UPROPERTY() TMap<FName, FHydraWeaponValues> weapons;
	/** Per ID from weapons.json (entries with blocks = true). */
	UPROPERTY() TMap<FName, FHydraShieldValues> shields;
	UPROPERTY() FHydraCharacterValues character;
	UPROPERTY() FHydraMovementValues movement;
	UPROPERTY() FHydraTurnRateValues turn_rate;
	UPROPERTY() FHydraLockOnValues lock_on;
	UPROPERTY() FHydraAimAssistValues aim_assist;
	UPROPERTY() FHydraStaminaValues stamina;
	UPROPERTY() FHydraAttackValues attack;
	UPROPERTY() FHydraChainValues chain;
	UPROPERTY() FHydraInterruptValues interrupt;
	UPROPERTY() FHydraBlockValues block;
	UPROPERTY() FHydraDodgeValues dodge;
	UPROPERTY() FHydraWeakPointValues weak_point;
	UPROPERTY() FHydraLagCompensationValues lag_compensation;
	UPROPERTY() FHydraAiPracticeValues ai_practice;
	UPROPERTY() FHydraManaValues mana;
	UPROPERTY() FHydraFireBoltValues fire_bolt;
	UPROPERTY() FHydraBurningValues burning;
	UPROPERTY() FHydraNetSimulationValues net_simulation;
};
