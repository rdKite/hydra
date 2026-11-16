#pragma once

#include "NativeGameplayTags.h"

// Native gameplay tags: declared in C++, no tag table asset. Add new tags here and in
// HydraTags.cpp.
namespace HydraTags
{
	// States on a character (granted by effects or abilities).
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_StaminaRegenBlocked);

	// SetByCaller keys: the magnitudes every Hydra effect reads from its spec.
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Amount);
	HYDRA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration);
}
