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

UHydraGE_ManaChange::UHydraGE_ManaChange()
{
	MakeInstant();
	AddAmountTo(UHydraAttributeSet::GetManaAttribute());
}

UHydraGE_ManaRegen::UHydraGE_ManaRegen()
{
	MakeInfinite(UHydraGE_StaminaRegen::TickS);
	AddAmountTo(UHydraAttributeSet::GetManaAttribute());
	PauseWhileTargetHas(HydraTags::State_ManaRegenBlocked);
}
