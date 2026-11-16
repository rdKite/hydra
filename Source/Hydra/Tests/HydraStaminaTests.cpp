// Automation test for the GAS helper layer and stamina: runs the real effects on a real
// ability system component in a small throwaway world, with the numbers from data/values.json.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/HydraAttributeSet.h"
#include "Abilities/HydraEffects.h"
#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Data/HydraDataLoader.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/HydraTestHelpers.h"

namespace HydraTests
{
	/** A game world without a map, ticked by hand. */
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("HydraTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
		}

		~FTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		/** Advances the world in 1/60 s steps. */
		void Advance(float Seconds)
		{
			const float Step = 1.f / 60.f;
			for (float T = 0.f; T < Seconds - KINDA_SMALL_NUMBER; T += Step)
			{
				// The timer manager ticks once per engine frame; the test runs inside one frame.
				++GFrameCounter;
				World->Tick(LEVELTICK_All, Step);
			}
		}

		UAbilitySystemComponent* SpawnCharacterLike()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor);
			ASC->RegisterComponent();
			ASC->AddSpawnedAttribute(NewObject<UHydraAttributeSet>(Actor));
			ASC->InitAbilityActorInfo(Actor, Actor);
			return ASC;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraStaminaTest, "Hydra.Stamina.CostRegenDelay", HydraTests::Flags)
bool FHydraStaminaTest::RunTest(const FString& Parameters)
{
	FHydraValues V;
	TArray<FString> Problems;
	if (!TestTrue(TEXT("values.json loads"), HydraDataLoader::LoadValues(HydraTests::DataDir(), V, Problems)))
	{
		return false;
	}
	// The bar of the humanoid (values.json forms.humanoid) with the general regeneration delay.
	struct FBar { float max; float regen_per_s; };
	const FHydraFormValues& Form = V.forms.FindChecked(TEXT("humanoid"));
	const FBar S{ Form.stamina_max, Form.stamina_regen_per_s };
	const FHydraStaminaValues& Rules = V.stamina;
	const float DelayS = Rules.regen_delay_after_action_ms / 1000.f;
	const float Tick = S.regen_per_s * UHydraGE_StaminaRegen::TickS;   // regeneration per tick
	const float Tolerance = 2.f * Tick + 0.01f;                        // regeneration comes in ticks

	HydraTests::FTestWorld TestWorld;
	UAbilitySystemComponent* ASC = TestWorld.SpawnCharacterLike();
	FActiveGameplayEffectHandle Regen;
	HydraGas::ApplyStaminaValues(ASC, S.max, S.regen_per_s, Regen, true);

	// Start: full bar, and it stays at the maximum.
	TestEqual(TEXT("max from values.json"), HydraGas::GetMaxStamina(ASC), S.max);
	TestEqual(TEXT("starts full"), HydraGas::GetStamina(ASC), S.max);
	TestWorld.Advance(0.5f);
	TestEqual(TEXT("never above max"), HydraGas::GetStamina(ASC), S.max);

	// Cost.
	const float Cost = S.max * 0.4f;
	TestTrue(TEXT("can afford 40 % of the bar"), HydraGas::CanAfford(ASC, Cost));
	HydraGas::SpendStamina(ASC, Cost, Rules);
	TestEqual(TEXT("cost taken"), HydraGas::GetStamina(ASC), S.max - Cost);
	TestTrue(TEXT("regeneration blocked after the action"), ASC->HasMatchingGameplayTag(HydraTags::State_StaminaRegenBlocked));

	// Regeneration delay: nothing comes back until it is over.
	TestWorld.Advance(DelayS * 0.75f);
	TestEqual(TEXT("no regeneration during the delay"), HydraGas::GetStamina(ASC), S.max - Cost);

	// A second action inside the delay restarts it.
	const float SmallCost = S.max * 0.1f;
	HydraGas::SpendStamina(ASC, SmallCost, Rules);
	TestWorld.Advance(DelayS * 0.75f);
	TestEqual(TEXT("second action restarts the delay"), HydraGas::GetStamina(ASC), S.max - Cost - SmallCost);

	// Regeneration: delay over after another 0.25 x delay, then regen_per_s.
	const float RegenS = 0.5f;
	TestWorld.Advance(DelayS * 0.25f + RegenS);
	TestFalse(TEXT("delay over"), ASC->HasMatchingGameplayTag(HydraTags::State_StaminaRegenBlocked));
	const float Expected = S.max - Cost - SmallCost + S.regen_per_s * RegenS;
	TestTrue(FString::Printf(TEXT("regenerates with regen_per_s (expected about %.1f, is %.1f)"), Expected, HydraGas::GetStamina(ASC)),
		FMath::Abs(HydraGas::GetStamina(ASC) - Expected) <= Tolerance);

	// Back to full, not beyond.
	TestWorld.Advance(S.max / S.regen_per_s + 1.f);
	TestEqual(TEXT("full again"), HydraGas::GetStamina(ASC), S.max);

	// Empty bar: cannot afford, never below zero.
	HydraGas::SpendStamina(ASC, S.max * 0.9f, Rules);
	TestFalse(TEXT("cannot afford more than is left"), HydraGas::CanAfford(ASC, S.max * 0.2f));
	HydraGas::SpendStamina(ASC, S.max, Rules);
	TestEqual(TEXT("never below zero"), HydraGas::GetStamina(ASC), 0.f);

	// Reload with other numbers: new maximum, bar clamped, new rate.
	const FBar Changed{ S.max * 0.5f, S.regen_per_s * 2.f };
	TestWorld.Advance(DelayS + 0.05f);
	HydraGas::ApplyStaminaValues(ASC, Changed.max, Changed.regen_per_s, Regen, false);
	TestEqual(TEXT("new max after reload"), HydraGas::GetMaxStamina(ASC), Changed.max);
	const float Before = HydraGas::GetStamina(ASC);
	TestWorld.Advance(0.5f);
	const float Gained = HydraGas::GetStamina(ASC) - Before;
	TestTrue(FString::Printf(TEXT("new rate after reload (expected about %.1f, gained %.1f)"), Changed.regen_per_s * 0.5f, Gained),
		FMath::Abs(Gained - Changed.regen_per_s * 0.5f) <= 2.f * Tolerance);
	TestWorld.Advance(Changed.max / Changed.regen_per_s + 1.f);
	TestEqual(TEXT("full at the new max"), HydraGas::GetStamina(ASC), Changed.max);

	// Health: damage goes through the meta attribute, never below zero.
	HydraGas::ApplyHealthValues(ASC, Form.hp_max, true);
	TestEqual(TEXT("health starts full"), HydraGas::GetHealth(ASC), Form.hp_max);
	HydraGas::DealDamage(ASC, Form.hp_max * 0.3f, nullptr);
	TestEqual(TEXT("damage taken from health"), HydraGas::GetHealth(ASC), Form.hp_max * 0.7f);
	HydraGas::DealDamage(ASC, Form.hp_max * 5.f, nullptr);
	TestEqual(TEXT("health never below zero"), HydraGas::GetHealth(ASC), 0.f);

	// Mana (the slime's bar): cost, regeneration without a delay, clamp.
	const FHydraFormValues& Slime = V.forms.FindChecked(TEXT("slime"));
	FActiveGameplayEffectHandle ManaRegen;
	HydraGas::ApplyManaValues(ASC, Slime.mana_max, Slime.mana_regen_per_s, ManaRegen, true);
	TestEqual(TEXT("mana starts full"), HydraGas::GetMana(ASC), Slime.mana_max);
	TestTrue(TEXT("can afford a bolt"), HydraGas::CanAffordMana(ASC, V.bolt.mana_cost));
	HydraGas::SpendMana(ASC, V.bolt.mana_cost);
	TestEqual(TEXT("mana cost taken"), HydraGas::GetMana(ASC), Slime.mana_max - V.bolt.mana_cost);
	TestWorld.Advance(1.f);
	TestTrue(FString::Printf(TEXT("mana regenerates with mana_regen_per_s (is %.1f)"), HydraGas::GetMana(ASC)),
		FMath::Abs(HydraGas::GetMana(ASC) - (Slime.mana_max - V.bolt.mana_cost + Slime.mana_regen_per_s)) <= 2.f * Slime.mana_regen_per_s * UHydraGE_StaminaRegen::TickS + 0.01f);
	TestWorld.Advance(Slime.mana_max / FMath::Max(1.f, Slime.mana_regen_per_s) + 1.f);
	TestEqual(TEXT("mana full again, not beyond"), HydraGas::GetMana(ASC), Slime.mana_max);
	HydraGas::SpendMana(ASC, Slime.mana_max * 3.f);
	TestEqual(TEXT("mana never below zero"), HydraGas::GetMana(ASC), 0.f);
	TestFalse(TEXT("cannot afford a bolt with empty mana"), HydraGas::CanAffordMana(ASC, V.bolt.mana_cost));
	// A form without mana (the humanoid): no bar, no regeneration.
	HydraGas::ApplyManaValues(ASC, 0.f, 0.f, ManaRegen, true);
	TestWorld.Advance(0.5f);
	TestEqual(TEXT("no mana bar stays empty"), HydraGas::GetMana(ASC), 0.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
