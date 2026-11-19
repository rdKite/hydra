// Automation test for the GAS helper layer and stamina: runs the real effects on a real
// ability system component in a small throwaway world, with the numbers from data/values.json.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/HydraAttributeSet.h"
#include "Abilities/HydraEffects.h"
#include "Abilities/HydraGas.h"
#include "Abilities/HydraTags.h"
#include "AbilitySystemComponent.h"
#include "Character/HydraCombatMath.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraStaminaTest, "Hydra.Stamina.CostAndRegeneration", HydraTests::Flags)
bool FHydraStaminaTest::RunTest(const FString& Parameters)
{
	FHydraValues V;
	FHydraDefinitions Defs;
	TArray<FString> Problems;
	if (!TestTrue(TEXT("data loads"), HydraDataLoader::LoadValues(HydraTests::DataDir(), V, Problems) && HydraDataLoader::LoadDefinitions(HydraTests::DataDir(), Defs, Problems)))
	{
		return false;
	}
	// The bars of the humanoid at level 0 as the game derives them (10 per attribute point,
	// regeneration as a share of the maximum per second).
	struct FBar { float max; float regen_per_s; };
	const FHydraCombatStats Human = HydraCombatMath::BuildStats(Defs, V, TEXT("humanoid"));
	const FBar S{ Human.StaminaMax, Human.StaminaRegenPerS };
	const float Tick = S.regen_per_s * UHydraGE_StaminaRegen::TickS;   // regeneration per tick
	const float Tolerance = 2.f * Tick + 0.01f;                        // regeneration comes in ticks

	HydraTests::FTestWorld TestWorld;
	UAbilitySystemComponent* ASC = TestWorld.SpawnCharacterLike();
	FActiveGameplayEffectHandle Regen;
	HydraGas::ApplyStaminaValues(ASC, S.max, S.regen_per_s, Regen, true);

	// Start: full bar, and it stays at the maximum.
	TestEqual(TEXT("max from the endurance of the form"), HydraGas::GetMaxStamina(ASC), S.max);
	TestEqual(TEXT("starts full"), HydraGas::GetStamina(ASC), S.max);
	TestWorld.Advance(0.5f);
	TestEqual(TEXT("never above max"), HydraGas::GetStamina(ASC), S.max);

	// Cost, then regeneration at once: there is no delay after an action any more.
	const float Cost = S.max * 0.4f;
	TestTrue(TEXT("can afford 40 % of the bar"), HydraGas::CanAfford(ASC, Cost));
	HydraGas::SpendStamina(ASC, Cost);
	TestEqual(TEXT("cost taken"), HydraGas::GetStamina(ASC), S.max - Cost);
	const float RegenS = 0.5f;
	TestWorld.Advance(RegenS);
	const float Expected = S.max - Cost + S.regen_per_s * RegenS;
	TestTrue(FString::Printf(TEXT("regenerates at once with the share of the form (expected about %.1f, is %.1f)"), Expected, HydraGas::GetStamina(ASC)),
		FMath::Abs(HydraGas::GetStamina(ASC) - Expected) <= Tolerance);

	// No regeneration during an action: the character sets this tag while an attack, a dodge
	// or a raised block runs.
	const float Before = HydraGas::GetStamina(ASC);
	ASC->SetLooseGameplayTagCount(HydraTags::State_StaminaRegenBlocked, 1);
	TestWorld.Advance(0.5f);
	TestEqual(TEXT("no regeneration while an action runs"), HydraGas::GetStamina(ASC), Before);
	ASC->SetLooseGameplayTagCount(HydraTags::State_StaminaRegenBlocked, 0);
	TestWorld.Advance(0.3f);
	TestTrue(TEXT("regeneration goes on when the action is over"), HydraGas::GetStamina(ASC) > Before);

	// Back to full, not beyond.
	TestWorld.Advance(S.max / S.regen_per_s + 1.f);
	TestEqual(TEXT("full again"), HydraGas::GetStamina(ASC), S.max);

	// Empty bar: cannot afford, never below zero.
	HydraGas::SpendStamina(ASC, S.max * 0.9f);
	TestFalse(TEXT("cannot afford more than is left"), HydraGas::CanAfford(ASC, S.max * 0.2f));
	HydraGas::SpendStamina(ASC, S.max);
	TestEqual(TEXT("never below zero"), HydraGas::GetStamina(ASC), 0.f);

	// Reload with other numbers: new maximum, bar clamped, new rate.
	const FBar Changed{ S.max * 0.5f, S.regen_per_s * 2.f };
	HydraGas::ApplyStaminaValues(ASC, Changed.max, Changed.regen_per_s, Regen, false);
	TestEqual(TEXT("new max after reload"), HydraGas::GetMaxStamina(ASC), Changed.max);
	const float BeforeReload = HydraGas::GetStamina(ASC);
	TestWorld.Advance(0.25f);
	const float Gained = HydraGas::GetStamina(ASC) - BeforeReload;
	TestTrue(FString::Printf(TEXT("new rate after reload (expected about %.1f, gained %.1f)"), Changed.regen_per_s * 0.25f, Gained),
		FMath::Abs(Gained - Changed.regen_per_s * 0.25f) <= 4.f * Tolerance);
	TestWorld.Advance(Changed.max / Changed.regen_per_s + 1.f);
	TestEqual(TEXT("full at the new max"), HydraGas::GetStamina(ASC), Changed.max);

	// Health: damage goes through the meta attribute, never below zero.
	HydraGas::ApplyHealthValues(ASC, Human.HpMax, true);
	TestEqual(TEXT("health starts full"), HydraGas::GetHealth(ASC), Human.HpMax);
	HydraGas::DealDamage(ASC, Human.HpMax * 0.3f, nullptr);
	TestEqual(TEXT("damage taken from health"), HydraGas::GetHealth(ASC), Human.HpMax * 0.7f, 0.01f);
	HydraGas::DealDamage(ASC, Human.HpMax * 5.f, nullptr);
	TestEqual(TEXT("health never below zero"), HydraGas::GetHealth(ASC), 0.f);

	// Mana (the slime's bar): cost, regeneration, none while a bolt is wound up or charged, clamp.
	const FHydraCombatStats Slime = HydraCombatMath::BuildStats(Defs, V, TEXT("slime"));
	FActiveGameplayEffectHandle ManaRegen;
	HydraGas::ApplyManaValues(ASC, Slime.ManaMax, Slime.ManaRegenPerS, ManaRegen, true);
	TestEqual(TEXT("mana starts full"), HydraGas::GetMana(ASC), Slime.ManaMax);
	TestTrue(TEXT("can afford a bolt"), HydraGas::CanAffordMana(ASC, V.bolt.mana_cost));
	HydraGas::SpendMana(ASC, V.bolt.mana_cost);
	TestEqual(TEXT("mana cost taken"), HydraGas::GetMana(ASC), Slime.ManaMax - V.bolt.mana_cost);
	ASC->SetLooseGameplayTagCount(HydraTags::State_ManaRegenBlocked, 1);
	TestWorld.Advance(0.5f);
	TestEqual(TEXT("no mana regeneration while casting"), HydraGas::GetMana(ASC), Slime.ManaMax - V.bolt.mana_cost);
	ASC->SetLooseGameplayTagCount(HydraTags::State_ManaRegenBlocked, 0);
	TestWorld.Advance(1.f);
	TestTrue(FString::Printf(TEXT("mana regenerates with its share per second (is %.1f)"), HydraGas::GetMana(ASC)),
		FMath::Abs(HydraGas::GetMana(ASC) - (Slime.ManaMax - V.bolt.mana_cost + Slime.ManaRegenPerS)) <= 2.f * Slime.ManaRegenPerS * UHydraGE_StaminaRegen::TickS + 0.01f);
	TestWorld.Advance(Slime.ManaMax / FMath::Max(1.f, Slime.ManaRegenPerS) + 1.f);
	TestEqual(TEXT("mana full again, not beyond"), HydraGas::GetMana(ASC), Slime.ManaMax);
	HydraGas::SpendMana(ASC, Slime.ManaMax * 3.f);
	TestEqual(TEXT("mana never below zero"), HydraGas::GetMana(ASC), 0.f);
	TestFalse(TEXT("cannot afford a bolt with empty mana"), HydraGas::CanAffordMana(ASC, V.bolt.mana_cost));
	// A form without mana (the humanoid): no bar, no regeneration.
	HydraGas::ApplyManaValues(ASC, 0.f, 0.f, ManaRegen, true);
	TestWorld.Advance(0.5f);
	TestEqual(TEXT("no mana bar stays empty"), HydraGas::GetMana(ASC), 0.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
