#include "Abilities/HydraTags.h"

namespace HydraTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaminaRegenBlocked, "State.StaminaRegenBlocked", "Stamina does not regenerate: an action runs (attack, dodge, held block)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_ManaRegenBlocked, "State.ManaRegenBlocked", "Mana does not regenerate: a bolt is wound up or charged");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Spell, "State.Spell", "The cast ability runs (wind-up to end of recovery)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "An attack is running (wind-up, strike or recovery)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_AttackCommitted, "State.AttackCommitted", "Wind-up or strike: the attack binds, turning is slowed, no dodge");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Blocking, "State.Blocking", "Holding the directional block");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Casting, "State.Casting", "Winding up or charging the bolt: the caster follows the camera direction");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dodging, "State.Dodging", "Dodge step or its recovery");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "HP reached 0, waiting for the respawn");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Burning, "State.Burning", "At least one stack of burning");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cast, "Ability.Cast", "Cast the bolt (the slime's projectile)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack, "Ability.Attack", "Weapon attack (light or heavy)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Block, "Ability.Block", "Directional block");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dodge, "Ability.Dodge", "Dodge step");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Amount, "Data.Amount", "SetByCaller: amount added to the effect's attribute (negative = cost)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Duration, "Data.Duration", "SetByCaller: duration of the effect in seconds");
}
