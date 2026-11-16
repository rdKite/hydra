#include "Abilities/HydraTags.h"

namespace HydraTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaminaRegenBlocked, "State.StaminaRegenBlocked", "Stamina does not regenerate (regeneration delay after an action; later: while blocking)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Amount, "Data.Amount", "SetByCaller: amount added to the effect's attribute (negative = cost)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Duration, "Data.Duration", "SetByCaller: duration of the effect in seconds");
}
