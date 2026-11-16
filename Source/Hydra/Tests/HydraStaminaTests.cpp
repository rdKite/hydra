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
	const FHydraStaminaValues& S = V.stamina;
	const float DelayS = S.regen_delay_after_action_ms / 1000.f;
	const float Tick = S.regen_per_s * UHydraGE_StaminaRegen::TickS;   // regeneration per tick
	const float Tolerance = 2.f * Tick + 0.01f;                        // regeneration comes in ticks

	HydraTests::FTestWorld TestWorld;
	UAbilitySystemComponent* ASC = TestWorld.SpawnCharacterLike();
	FActiveGameplayEffectHandle Regen;
	HydraGas::ApplyStaminaValues(ASC, S, Regen, true);

	// Start: full bar, and it stays at the maximum.
	TestEqual(TEXT("max from values.json"), HydraGas::GetMaxStamina(ASC), S.max);
	TestEqual(TEXT("starts full"), HydraGas::GetStamina(ASC), S.max);
	TestWorld.Advance(0.5f);
	TestEqual(TEXT("never above max"), HydraGas::GetStamina(ASC), S.max);

	// Cost.
	const float Cost = S.max * 0.4f;
	TestTrue(TEXT("can afford 40 % of the bar"), HydraGas::CanAfford(ASC, Cost));
	HydraGas::SpendStamina(ASC, Cost, S);
	TestEqual(TEXT("cost taken"), HydraGas::GetStamina(ASC), S.max - Cost);
	TestTrue(TEXT("regeneration blocked after the action"), ASC->HasMatchingGameplayTag(HydraTags::State_StaminaRegenBlocked));

	// Regeneration delay: nothing comes back until it is over.
	TestWorld.Advance(DelayS * 0.75f);
	TestEqual(TEXT("no regeneration during the delay"), HydraGas::GetStamina(ASC), S.max - Cost);

	// A second action inside the delay restarts it.
	const float SmallCost = S.max * 0.1f;
	HydraGas::SpendStamina(ASC, SmallCost, S);
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
	HydraGas::SpendStamina(ASC, S.max * 0.9f, S);
	TestFalse(TEXT("cannot afford more than is left"), HydraGas::CanAfford(ASC, S.max * 0.2f));
	HydraGas::SpendStamina(ASC, S.max, S);
	TestEqual(TEXT("never below zero"), HydraGas::GetStamina(ASC), 0.f);

	// Reload with other numbers: new maximum, bar clamped, new rate.
	FHydraStaminaValues Changed = S;
	Changed.max = S.max * 0.5f;
	Changed.regen_per_s = S.regen_per_s * 2.f;
	TestWorld.Advance(DelayS + 0.05f);
	HydraGas::ApplyStaminaValues(ASC, Changed, Regen, false);
	TestEqual(TEXT("new max after reload"), HydraGas::GetMaxStamina(ASC), Changed.max);
	const float Before = HydraGas::GetStamina(ASC);
	TestWorld.Advance(0.5f);
	const float Gained = HydraGas::GetStamina(ASC) - Before;
	TestTrue(FString::Printf(TEXT("new rate after reload (expected about %.1f, gained %.1f)"), Changed.regen_per_s * 0.5f, Gained),
		FMath::Abs(Gained - Changed.regen_per_s * 0.5f) <= 2.f * Tolerance);
	TestWorld.Advance(Changed.max / Changed.regen_per_s + 1.f);
	TestEqual(TEXT("full at the new max"), HydraGas::GetStamina(ASC), Changed.max);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
