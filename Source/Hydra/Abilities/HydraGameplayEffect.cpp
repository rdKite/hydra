#include "Abilities/HydraGameplayEffect.h"

#include "Abilities/HydraTags.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace
{
	FGameplayEffectModifierMagnitude SetByCaller(const FGameplayTag& Tag)
	{
		FSetByCallerFloat Value;
		Value.DataTag = Tag;
		return FGameplayEffectModifierMagnitude(Value);
	}
}

void UHydraGameplayEffect::MakeInstant()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
}

void UHydraGameplayEffect::MakeTimed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = SetByCaller(HydraTags::Data_Duration);
}

void UHydraGameplayEffect::MakeInfinite(float PeriodS)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	if (PeriodS > 0.f)
	{
		Period = FScalableFloat(PeriodS);
		bExecutePeriodicEffectOnApplication = false;
	}
}

void UHydraGameplayEffect::AddAmountTo(const FGameplayAttribute& Attribute)
{
	FGameplayModifierInfo Info;
	Info.Attribute = Attribute;
	Info.ModifierOp = EGameplayModOp::Additive;
	Info.ModifierMagnitude = SetByCaller(HydraTags::Data_Amount);
	Modifiers.Add(Info);
}

void UHydraGameplayEffect::GrantTag(const FGameplayTag& Tag)
{
	// GE components must be default subobjects created in the constructor (tech-test finding).
	if (!GrantedTagsComponent)
	{
		GrantedTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("GrantedTags"));
		GEComponents.Add(GrantedTagsComponent);
	}
	GrantedTags.AddTag(Tag);
	FInheritedTagContainer Container;
	Container.Added = GrantedTags;
	GrantedTagsComponent->SetAndApplyTargetTagChanges(Container);
}

void UHydraGameplayEffect::PauseWhileTargetHas(const FGameplayTag& Tag)
{
	if (!RequirementsComponent)
	{
		RequirementsComponent = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TagRequirements"));
		GEComponents.Add(RequirementsComponent);
	}
	RequirementsComponent->OngoingTagRequirements.IgnoreTags.AddTag(Tag);
}

void UHydraGameplayEffect::StackPerTarget(int32 Limit)
{
	// UGameplayEffect::SetStackingType() is not DLL-exported in 5.8 (link error), so the
	// deprecated member is set directly (tech-test finding).
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = Limit;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}
