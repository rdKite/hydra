#pragma once

#include "NativeGameplayTags.h"

// Native gameplay tags: declared in C++, no tag table asset. Add new tags here and in
// HydraTags.cpp.
namespace HydraTags
{
	// States on a character (granted by effects or abilities).
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_StaminaRegenBlocked);
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);          // attack ability running (wind-up to end of recovery)
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_AttackCommitted);    // wind-up and strike: the attack binds, slow turning
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Blocking);
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dodging);            // dodge step and its recovery
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Burning);

	// Abilities.
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Block);
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cast);

	// SetByCaller keys: the magnitudes every Hydra effect reads from its spec.
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Amount);
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration);
}
