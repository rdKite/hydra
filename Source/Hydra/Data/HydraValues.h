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

/**
 * Resistance per damage type: the share of that damage a form shrugs off (0.5 = half).
 * Negative = vulnerable (-0.25 = a quarter more).
 */
USTRUCT()
struct FHydraResistanceValues
{
	GENERATED_BODY()
	UPROPERTY() float slash = 0.f;
	UPROPERTY() float pierce = 0.f;
	UPROPERTY() float blunt = 0.f;
	UPROPERTY() float fire = 0.f;
};

/** Numbers of one form. body_weight and strength share a unit with the weapon weights. */
USTRUCT()
struct FHydraFormValues
{
	GENERATED_BODY()
	/** Weight of the body (weight = body plus load). */
	UPROPERTY() float body_weight = 0.f;
	UPROPERTY() float strength = 0.f;
	/** Size of the body for movement and hit areas (a capsule). */
	UPROPERTY() float body_radius_m = 0.f;
	UPROPERTY() float body_height_m = 0.f;
	UPROPERTY() float hp_max = 0.f;
	UPROPERTY() float stamina_max = 0.f;
	UPROPERTY() float stamina_regen_per_s = 0.f;
	/** 0 = the form has no mana bar. */
	UPROPERTY() float mana_max = 0.f;
	UPROPERTY() float mana_regen_per_s = 0.f;
	UPROPERTY() FHydraResistanceValues resistance;
	/** Highest character level of the form. */
	UPROPERTY() int32 level_cap = 0;
	/** Arena rule: a player starts at this level with every skill of the form at start_skill_level. */
	UPROPERTY() int32 start_level = 0;
	UPROPERTY() int32 start_skill_level = 0;
	/** Element body: damage of the body's element a melee hit against this form deals back (0 = none). */
	UPROPERTY() float element_body_damage = 0.f;
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
	/** Share of a bolt's damage that still gets through a plain shield. */
	UPROPERTY() float projectile_damage_through = 0.f;
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

/** How an AI plays: the practice opponent (ai_practice) and every enemy type (enemies.<id>.ai). */
USTRUCT()
struct FHydraAiValues
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
	/** A form with the bolt casts it at targets inside this range (0 = never) ... */
	UPROPERTY() float cast_range_m = 0.f;
	/** ... and waits this long between two casts. */
	UPROPERTY() float cast_pause_ms = 0.f;
};

/** Numbers of one enemy type of the waves (enemies.json). */
USTRUCT()
struct FHydraEnemyValues
{
	GENERATED_BODY()
	UPROPERTY() int32 level = 0;
	/** Level of every skill of its form. */
	UPROPERTY() int32 skill_level = 0;
	/** nXP for defeating it = nxp_base x (1 + progression.nxp_bonus_per_enemy_level x its level). */
	UPROPERTY() float nxp_base = 0.f;
	UPROPERTY() FHydraAiValues ai;
};

USTRUCT()
struct FHydraWaveGroupValues
{
	GENERATED_BODY()
	/** enemies.json ID. */
	UPROPERTY() FName enemy;
	UPROPERTY() int32 count = 0;
};

USTRUCT()
struct FHydraWaveValues
{
	GENERATED_BODY()
	UPROPERTY() TArray<FHydraWaveGroupValues> groups;
};

/** The waves of the arena, in order; after the last one they start again with stronger enemies. */
USTRUCT()
struct FHydraWavesValues
{
	GENERATED_BODY()
	UPROPERTY() float first_delay_ms = 0.f;
	/** Pause between a cleared wave and the next. */
	UPROPERTY() float break_ms = 0.f;
	/** Levels every enemy gains per completed pass through the list. */
	UPROPERTY() int32 level_bonus_per_loop = 0;
	UPROPERTY() TArray<FHydraWaveValues> list;
};

/** Character level. nXP needed for the step from level L to L + 1 = xp_base + xp_per_level x L. */
USTRUCT()
struct FHydraProgressionValues
{
	GENERATED_BODY()
	UPROPERTY() float xp_base = 0.f;
	UPROPERTY() float xp_per_level = 0.f;
	UPROPERTY() float nxp_bonus_per_enemy_level = 0.f;
	/** Each level: this share more maximum HP and base damage. */
	UPROPERTY() float hp_bonus_per_level = 0.f;
	UPROPERTY() float damage_bonus_per_level = 0.f;
	UPROPERTY() int32 milestone_every_levels = 0;
};

/**
 * Skills (grow by use). Skill XP for the step from skill level L to L + 1 = xp_base +
 * xp_per_level x L. What a level does: more damage and lower cost for the skill's actions,
 * and per kind a wider block arc, a longer dodge step, a bigger element field.
 */
USTRUCT()
struct FHydraSkillsValues
{
	GENERATED_BODY()
	UPROPERTY() float xp_base = 0.f;
	UPROPERTY() float xp_per_level = 0.f;
	/** Skill XP per landed weapon or body attack, per hit taken on the block, per dodge that avoided a hit, per bolt that hit. */
	UPROPERTY() float xp_per_hit = 0.f;
	UPROPERTY() float xp_per_block = 0.f;
	UPROPERTY() float xp_per_dodge = 0.f;
	UPROPERTY() float xp_per_bolt_hit = 0.f;
	UPROPERTY() float damage_bonus_per_level = 0.f;
	/** Cost factor = 1 - this x level, never below min_cost_factor. */
	UPROPERTY() float cost_reduction_per_level = 0.f;
	UPROPERTY() float min_cost_factor = 0.f;
	UPROPERTY() float block_arc_deg_per_level = 0.f;
	UPROPERTY() float dodge_distance_bonus_per_level = 0.f;
	UPROPERTY() float field_radius_bonus_per_level = 0.f;
};

/** Imprint: an acquired alignment to an element. Progress comes from contact with the element. */
USTRUCT()
struct FHydraImprintValues
{
	GENERATED_BODY()
	UPROPERTY() int32 max_level = 0;
	/** Progress per point of that element's damage taken / dealt. */
	UPROPERTY() float progress_per_damage_taken = 0.f;
	UPROPERTY() float progress_per_damage_dealt = 0.f;
	/** Progress needed for the first level and for each further one (the first is clearly easier). */
	UPROPERTY() float progress_first_level = 0.f;
	UPROPERTY() float progress_per_further_level = 0.f;
	/** Per imprint level: resistance against the element and damage with it. */
	UPROPERTY() float resistance_per_level = 0.f;
	UPROPERTY() float damage_bonus_per_level = 0.f;
};

/** Instinct after an evolution, scaled by (old skill level / old form's level cap). */
USTRUCT()
struct FHydraInstinctValues
{
	GENERATED_BODY()
	/** Permanent bonus on the skill, in skill levels, at ratio 1. */
	UPROPERTY() float skill_level_bonus_max = 0.f;
	/** Extra share of skill XP for that skill at ratio 1 (it is relearned faster). */
	UPROPERTY() float xp_bonus_max = 0.f;
};

/**
 * The spell form bolt (Phase 1: the slime's projectile). Press = cast after the wind-up,
 * hold = charge: stronger and costlier up to max_charge_ms. Without an element it deals
 * blunt damage; with fire it deals fire damage, adds burning and leaves a fire field.
 */
USTRUCT()
struct FHydraBoltValues
{
	GENERATED_BODY()
	UPROPERTY() float mana_cost = 0.f;
	UPROPERTY() float damage = 0.f;
	UPROPERTY() float windup_ms = 0.f;
	UPROPERTY() float max_charge_ms = 0.f;
	UPROPERTY() float full_charge_damage_factor = 0.f;
	UPROPERTY() float full_charge_cost_factor = 0.f;
	UPROPERTY() float recovery_ms = 0.f;
	UPROPERTY() float speed_m_s = 0.f;
	UPROPERTY() float range_m = 0.f;
	UPROPERTY() float radius_m = 0.f;
};

/** The fire element field a fire bolt leaves where it lands. */
USTRUCT()
struct FHydraFireFieldValues
{
	GENERATED_BODY()
	UPROPERTY() float radius_m = 0.f;
	UPROPERTY() float duration_ms = 0.f;
};

/**
 * Burning (stacking debuff of fire). In a fire field the stack rises by one every
 * stack_interval_ms (the first at once); outside it drops by one per interval. Fire
 * resistance lengthens the interval. Damage over time per stack.
 */
USTRUCT()
struct FHydraBurningValues
{
	GENERATED_BODY()
	UPROPERTY() float damage_per_s_per_stack = 0.f;
	UPROPERTY() int32 max_stacks = 0;
	UPROPERTY() float stack_interval_ms = 0.f;
	/** Stacks a fire bolt adds on a hit. */
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
	UPROPERTY() FHydraAiValues ai_practice;
	/** Per ID from enemies.json. */
	UPROPERTY() TMap<FName, FHydraEnemyValues> enemies;
	UPROPERTY() FHydraWavesValues waves;
	UPROPERTY() FHydraProgressionValues progression;
	UPROPERTY() FHydraSkillsValues skills;
	UPROPERTY() FHydraImprintValues imprint;
	UPROPERTY() FHydraInstinctValues instinct;
	UPROPERTY() FHydraBoltValues bolt;
	UPROPERTY() FHydraFireFieldValues fire_field;
	UPROPERTY() FHydraBurningValues burning;
	UPROPERTY() FHydraNetSimulationValues net_simulation;
};
