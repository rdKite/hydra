#pragma once

#include "CoreMinimal.h"
#include "HydraValues.generated.h"

// Mirrors data/values.json. Member names are snake_case on purpose, so that
// FJsonObjectConverter maps the JSON keys 1:1 without a hand-written parser (this deviates
// from Unreal naming). Units as in the file: times in ms, rates per second, distances in
// metres, angles in degrees. Adding a value: data/values.json (bump version, changelog line)
// plus a member here; the automation tests Hydra.Values.* fail if the two drift apart.

USTRUCT()
struct FHydraCharacterValues
{
	GENERATED_BODY()
	UPROPERTY() float hp_max = 0.f;
	UPROPERTY() float respawn_delay_ms = 0.f;
	UPROPERTY() float hit_react_ms = 0.f;
};

USTRUCT()
struct FHydraMovementValues
{
	GENERATED_BODY()
	UPROPERTY() float walk_speed_m_s = 0.f;
	UPROPERTY() float run_speed_m_s = 0.f;
	UPROPERTY() float strafe_speed_m_s = 0.f;
};

USTRUCT()
struct FHydraTurnRateValues
{
	GENERATED_BODY()
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
	UPROPERTY() float max = 0.f;
	UPROPERTY() float regen_per_s = 0.f;
	UPROPERTY() float regen_delay_after_action_ms = 0.f;
};

USTRUCT()
struct FHydraLightAttackValues
{
	GENERATED_BODY()
	UPROPERTY() float stamina_cost = 0.f;
	UPROPERTY() float damage = 0.f;
	UPROPERTY() float windup_ms = 0.f;
	UPROPERTY() float hit_ms = 0.f;
	UPROPERTY() float recovery_ms = 0.f;
	UPROPERTY() float hit_stop_ms = 0.f;
	UPROPERTY() float reach_m = 0.f;
	UPROPERTY() float arc_deg = 0.f;
};

USTRUCT()
struct FHydraHeavyAttackValues
{
	GENERATED_BODY()
	UPROPERTY() float stamina_cost = 0.f;
	UPROPERTY() float damage = 0.f;
	UPROPERTY() float min_windup_ms = 0.f;
	UPROPERTY() float max_charge_ms = 0.f;
	UPROPERTY() float hit_ms = 0.f;
	UPROPERTY() float recovery_ms = 0.f;
	UPROPERTY() float hit_stop_ms = 0.f;
	UPROPERTY() float reach_m = 0.f;
	UPROPERTY() float arc_deg = 0.f;
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
	UPROPERTY() float damage_threshold_per_weight = 0.f;
};

USTRUCT()
struct FHydraBlockValues
{
	GENERATED_BODY()
	UPROPERTY() float damage_reduction = 0.f;
	UPROPERTY() float stamina_cost_per_hit = 0.f;
	UPROPERTY() bool no_regen_while_holding = false;
	UPROPERTY() float move_speed_ratio = 0.f;
	UPROPERTY() float round_shield_half_arc_deg = 0.f;
	UPROPERTY() float weapon_half_arc_deg = 0.f;
	UPROPERTY() float rebound_attacker_stamina_ratio = 0.f;
};

USTRUCT()
struct FHydraDodgeValues
{
	GENERATED_BODY()
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
	UPROPERTY() float damage_multiplier = 0.f;
};

USTRUCT()
struct FHydraLagCompensationValues
{
	GENERATED_BODY()
	UPROPERTY() float max_rewind_ms = 0.f;
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
	UPROPERTY() FHydraCharacterValues character;
	UPROPERTY() FHydraMovementValues movement;
	UPROPERTY() FHydraTurnRateValues turn_rate;
	UPROPERTY() FHydraLockOnValues lock_on;
	UPROPERTY() FHydraAimAssistValues aim_assist;
	UPROPERTY() FHydraStaminaValues stamina;
	UPROPERTY() FHydraLightAttackValues light_attack;
	UPROPERTY() FHydraHeavyAttackValues heavy_attack;
	UPROPERTY() FHydraChainValues chain;
	UPROPERTY() FHydraInterruptValues interrupt;
	UPROPERTY() FHydraBlockValues block;
	UPROPERTY() FHydraDodgeValues dodge;
	UPROPERTY() FHydraWeakPointValues weak_point;
	UPROPERTY() FHydraLagCompensationValues lag_compensation;
	UPROPERTY() FHydraManaValues mana;
	UPROPERTY() FHydraFireBoltValues fire_bolt;
	UPROPERTY() FHydraBurningValues burning;
	UPROPERTY() FHydraNetSimulationValues net_simulation;
};
