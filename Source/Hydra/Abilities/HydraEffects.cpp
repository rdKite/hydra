#include "Abilities/HydraEffects.h"

#include "Abilities/HydraAttributeSet.h"
#include "Abilities/HydraTags.h"

UHydraGE_StaminaChange::UHydraGE_StaminaChange()
{
	MakeInstant();
	AddAmountTo(UHydraAttributeSet::GetStaminaAttribute());
}

UHydraGE_Damage::UHydraGE_Damage()
{
	MakeInstant();
	AddAmountTo(UHydraAttributeSet::GetIncomingDamageAttribute());
}

UHydraGE_StaminaRegen::UHydraGE_StaminaRegen()
{
	MakeInfinite(TickS);
	AddAmountTo(UHydraAttributeSet::GetStaminaAttribute());
	PauseWhileTargetHas(HydraTags::State_StaminaRegenBlocked);
}

UHydraGE_StaminaRegenDelay::UHydraGE_StaminaRegenDelay()
{
	MakeTimed();
	GrantTag(HydraTags::State_StaminaRegenBlocked);
	StackPerTarget(1);
}
