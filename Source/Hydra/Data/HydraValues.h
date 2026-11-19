#pragma once

#include "CoreMinimal.h"
#include "HydraValues.generated.h"

// Mirrors data/values.json: every gameplay number (decision 2026-11-16: one file). General
// rules are sections of their own; the numbers of one form, weapon or shield are in the
// sections forms / weapons / shields, keyed by the ID from forms.json / weapons.json.
// Member names are snake_case on purpose, so that FJsonObjectConverter maps the JSON keys
// 1:1 without a hand-written parser (this deviates from Unreal naming). Units as in the
// file: times in ms, rates per second, distances in metres, angles in degrees (arcs as half
// angles), weights in kg (strength in the same unit).
// Adding a value: data/values.json (bump version, changelog line) plus a member here; the
// automation tests Hydra.Values.* fail if the two drift apart.

/**
 * Resistance per damage type in points: reduction = points / (points + resistance.curve_constant),
 * so 100 points halve the damage at a constant of 100. Negative points = weakness:
 * damage x (1 + |points| / constant).
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

/**
 * Numbers of one form. The body attributes are base values at level 0 and grow with
 * progression.level_growth per level: 10 HP per vitality, 10 stamina per endurance, 10 mana
 * per spirit; strength (in kg) sets the handling, dexterity the weak-point bonus and the dodge.
 */
USTRUCT()
struct FHydraFormValues
{
	GENERATED_BODY()
	/** Weight of the body in kg (weight = body plus load). Costs are measured against it. */
	UPROPERTY() float body_weight = 0.f;
	UPROPERTY() float vitality = 0.f;
	UPROPERTY() float endurance = 0.f;
	UPROPERTY() float spirit = 0.f;
	UPROPERTY() float strength = 0.f;
	UPROPERTY() float dexterity = 0.f;
	/** Size of the body for movement and hit areas (a capsule). */
	UPROPERTY() float body_radius_m = 0.f;
	UPROPERTY() float body_height_m = 0.f;
	/** Regeneration as a share of the maximum per second; none while an action runs. */
	UPROPERTY() float stamina_regen_share_per_s = 0.f;
	UPROPERTY() float mana_regen_share_per_s = 0.f;
	/** Stamina of a dodge step for the bare body; x total weight / body weight. */
	UPROPERTY() float dodge_stamina_cost = 0.f;
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

/**
 * A weapon schematic or a body attack. Base damage (light.damage, heavy.damage) follows
 * from the cost (damage.efficiency_*) and is stored here, not computed at runtime.
 */
USTRUCT()
struct FHydraWeaponValues
{
	GENERATED_BODY()
	/** In kg; counts as load. 0 for a body attack. */
	UPROPERTY() float weight = 0.f;
	/** Body attacks: used in place of weight / body weight for the stamina cost. 0 = a weapon (cost from its weight). */
	UPROPERTY() float cost_ratio = 0.f;
	/** Factor on the stamina cost (rarity); 1 in Phase 1. */
	UPROPERTY() float demand = 0.f;
	/** Share of the attribute bonus that comes from strength; the rest comes from dexterity. */
	UPROPERTY() float strength_share = 0.f;
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
struct FHydraAttackValues
{
	GENERATED_BODY()
	/** Stamina of a light / heavy attack = this x weapon weight / body weight x demand (body attacks: x cost_ratio). */
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
	/** A hit interrupts the target's attack if its damage is at least this share of the target's maximum HP. */
	UPROPERTY() float threshold_share_of_max_hp = 0.f;
};

USTRUCT()
struct FHydraBlockValues
{
	GENERATED_BODY()
	/** Stamina per second while the block is raised = this x shield weight / body weight. */
	UPROPERTY() float hold_stamina_per_s_per_weight_ratio = 0.f;
	/**
	 * A blocked hit costs the blocker (blocked damage x share) stamina, share = blocker_share x
	 * reference_shield_ratio / (shield weight / body weight), inside share_min .. share_max:
	 * a heavier shield takes hits more cheaply (and costs more to hold).
	 */
	UPROPERTY() float blocker_share = 0.f;
	UPROPERTY() float reference_shield_ratio = 0.f;
	UPROPERTY() float share_min = 0.f;
	UPROPERTY() float share_max = 0.f;
	/** Rebound: the attacker loses this share of the blocked damage as stamina (not for bolts). */
	UPROPERTY() float rebound_share = 0.f;
	UPROPERTY() float move_speed_ratio = 0.f;
	/** Block with the weapon (no shield): not used by the Phase 1 loadout yet. */
	UPROPERTY() float weapon_half_arc_deg = 0.f;
	/** Share of a bolt's damage that still gets through a plain shield. */
	UPROPERTY() float projectile_damage_through = 0.f;
};

USTRUCT()
struct FHydraDodgeValues
{
	GENERATED_BODY()
	UPROPERTY() float distance_m = 0.f;
	UPROPERTY() float duration_ms = 0.f;
	UPROPERTY() float recovery_ms = 0.f;
	/** Dexterity: distance x (1 + this x gain), duration / (1 + speed_per_dexterity_gain x gain). */
	UPROPERTY() float distance_per_dexterity_gain = 0.f;
	UPROPERTY() float speed_per_dexterity_gain = 0.f;
};

USTRUCT()
struct FHydraWeakPointValues
{
	GENERATED_BODY()
	UPROPERTY() float back_half_arc_deg = 0.f;
	/** Multiplier = damage_multiplier + multiplier_per_dexterity_gain x the attacker's dexterity gain, capped. */
	UPROPERTY() float damage_multiplier = 0.f;
	UPROPERTY() float multiplier_per_dexterity_gain = 0.f;
	UPROPERTY() float max_damage_multiplier = 0.f;
};

/**
 * Damage = base x (1 + progress bonuses added up) x situational factors multiplied. The
 * attribute bonus: strength share x strength_bonus_per_gain x strength gain + dexterity share
 * x dexterity_bonus_per_gain x dexterity gain (gain = what the level added on top of the base
 * value, as a share of it). efficiency_*: how the stored base damage of a weapon follows from
 * its cost - light damage = efficiency_base x (weight / efficiency_reference_kg) ^
 * efficiency_exponent x light cost of a human at level 0 (checked by an automation test).
 */
USTRUCT()
struct FHydraDamageValues
{
	GENERATED_BODY()
	UPROPERTY() float strength_bonus_per_gain = 0.f;
	UPROPERTY() float dexterity_bonus_per_gain = 0.f;
	UPROPERTY() float efficiency_base = 0.f;
	UPROPERTY() float efficiency_reference_kg = 0.f;
	UPROPERTY() float efficiency_exponent = 0.f;
};

USTRUCT()
struct FHydraResistanceRuleValues
{
	GENERATED_BODY()
	/** Points at which the damage is halved. */
	UPROPERTY() float curve_constant = 0.f;
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
	/** hXP = nXP x this (at least 1): the stand-in for the threat rating. */
	UPROPERTY() float hxp_factor = 0.f;
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

/**
 * Character level. nXP needed for the step from level L to L + 1 = xp_base + xp_per_level x L.
 * Each level raises every body attribute by level_growth of the form's base value.
 */
USTRUCT()
struct FHydraProgressionValues
{
	GENERATED_BODY()
	UPROPERTY() float xp_base = 0.f;
	UPROPERTY() float xp_per_level = 0.f;
	UPROPERTY() float nxp_bonus_per_enemy_level = 0.f;
	UPROPERTY() float level_growth = 0.f;
	UPROPERTY() int32 milestone_every_levels = 0;
};

/** What a skill level does (effect, never lower costs). */
USTRUCT()
struct FHydraSkillsValues
{
	GENERATED_BODY()
	/** Progress bonus on the damage of the skill's actions, per level (added to the other progress bonuses). */
	UPROPERTY() float damage_bonus_per_level = 0.f;
	UPROPERTY() float block_arc_deg_per_level = 0.f;
	UPROPERTY() float field_radius_bonus_per_level = 0.f;
};

/** Automatic skill points at a level-up = base + factor x log2(hardship rating), rating = hXP / nXP of the level (at least 1). */
USTRUCT()
struct FHydraSkillPointsValues
{
	GENERATED_BODY()
	UPROPERTY() float base = 0.f;
	UPROPERTY() float factor = 0.f;
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
	/** Per imprint level: resistance points against the element and a progress bonus on damage with it. */
	UPROPERTY() float resistance_points_per_level = 0.f;
	UPROPERTY() float damage_bonus_per_level = 0.f;
};

/** Instinct after an evolution, scaled by (old skill level / old form's level cap). */
USTRUCT()
struct FHydraInstinctValues
{
	GENERATED_BODY()
	/** Permanent bonus on the skill, in skill levels, at ratio 1. */
	UPROPERTY() float skill_level_bonus_max = 0.f;
	/** The skill's share of the hXP counts this much more at ratio 1 when skill points are split (it is relearned faster). */
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
	UPROPERTY() FHydraAttackValues attack;
	UPROPERTY() FHydraChainValues chain;
	UPROPERTY() FHydraInterruptValues interrupt;
	UPROPERTY() FHydraBlockValues block;
	UPROPERTY() FHydraDodgeValues dodge;
	UPROPERTY() FHydraWeakPointValues weak_point;
	UPROPERTY() FHydraDamageValues damage;
	UPROPERTY() FHydraResistanceRuleValues resistance;
	UPROPERTY() FHydraLagCompensationValues lag_compensation;
	UPROPERTY() FHydraAiValues ai_practice;
	/** Per ID from enemies.json. */
	UPROPERTY() TMap<FName, FHydraEnemyValues> enemies;
	UPROPERTY() FHydraWavesValues waves;
	UPROPERTY() FHydraProgressionValues progression;
	UPROPERTY() FHydraSkillsValues skills;
	UPROPERTY() FHydraSkillPointsValues skill_points;
	UPROPERTY() FHydraImprintValues imprint;
	UPROPERTY() FHydraInstinctValues instinct;
	UPROPERTY() FHydraBoltValues bolt;
	UPROPERTY() FHydraFireFieldValues fire_field;
	UPROPERTY() FHydraBurningValues burning;
	UPROPERTY() FHydraNetSimulationValues net_simulation;
};
