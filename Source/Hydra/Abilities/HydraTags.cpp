#include "Abilities/HydraTags.h"

namespace HydraTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaminaRegenBlocked, "State.StaminaRegenBlocked", "Stamina does not regenerate (regeneration delay after an action, or holding a block)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "An attack is running (wind-up, strike or recovery)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_AttackCommitted, "State.AttackCommitted", "Wind-up or strike: the attack binds, turning is slowed, no dodge");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Blocking, "State.Blocking", "Holding the directional block");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dodging, "State.Dodging", "Dodge step or its recovery");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "HP reached 0, waiting for the respawn");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack, "Ability.Attack", "Weapon attack (light or heavy)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Block, "Ability.Block", "Directional block");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dodge, "Ability.Dodge", "Dodge step");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Amount, "Data.Amount", "SetByCaller: amount added to the effect's attribute (negative = cost)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Duration, "Data.Duration", "SetByCaller: duration of the effect in seconds");
}
