// Automation tests for the pure positional rules (HydraCombatMath.h).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/HydraCombatMath.h"
#include "Tests/HydraTestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraTargetingMathTest, "Hydra.Camera.TargetingMath", HydraTests::Flags)
bool FHydraTargetingMathTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	const FVector Origin(0.f, 0.f, 0.f);
	const FVector Forward(1.f, 0.f, 0.f);
	float Score = 0.f;

	// Cone: range and half angle.
	TestTrue(TEXT("straight ahead in range"), ScoreTargetInCone(Origin, Forward, FVector(1000.f, 0.f, 0.f), 1500.f, 60.f, Score));
	TestFalse(TEXT("beyond range"), ScoreTargetInCone(Origin, Forward, FVector(1600.f, 0.f, 0.f), 1500.f, 60.f, Score));
	TestFalse(TEXT("behind"), ScoreTargetInCone(Origin, Forward, FVector(-500.f, 0.f, 0.f), 1500.f, 60.f, Score));
	TestTrue(TEXT("45 degrees off, inside 60"), ScoreTargetInCone(Origin, Forward, FVector(500.f, 500.f, 0.f), 1500.f, 60.f, Score));
	TestFalse(TEXT("45 degrees off, outside 30"), ScoreTargetInCone(Origin, Forward, FVector(500.f, 500.f, 0.f), 1500.f, 30.f, Score));
	TestTrue(TEXT("height is ignored"), ScoreTargetInCone(Origin, Forward, FVector(1000.f, 0.f, 800.f), 1500.f, 60.f, Score));

	// Score: centre of the cone first, then distance.
	float Centre = 0.f, Side = 0.f, Near = 0.f, Far = 0.f;
	ScoreTargetInCone(Origin, Forward, FVector(1000.f, 0.f, 0.f), 1500.f, 60.f, Centre);
	ScoreTargetInCone(Origin, Forward, FVector(700.f, 700.f, 0.f), 1500.f, 60.f, Side);
	TestTrue(TEXT("target in the centre beats one at the side"), Centre < Side);
	ScoreTargetInCone(Origin, Forward, FVector(300.f, 0.f, 0.f), 1500.f, 60.f, Near);
	ScoreTargetInCone(Origin, Forward, FVector(1200.f, 0.f, 0.f), 1500.f, 60.f, Far);
	TestTrue(TEXT("near target beats far target"), Near < Far);

	// Aim assist: partial turn along the shorter way.
	TestEqual(TEXT("strength 0.5 turns half way"), AimAssistYaw(0.f, 40.f, 0.5f), 20.f);
	TestEqual(TEXT("strength 0 does nothing"), AimAssistYaw(10.f, 40.f, 0.f), 10.f);
	TestEqual(TEXT("strength 1 faces the target"), AimAssistYaw(10.f, 40.f, 1.f), 40.f);
	TestEqual(TEXT("shorter way across 180"), AimAssistYaw(170.f, -170.f, 0.5f), 180.f);
	TestEqual(TEXT("strength is clamped"), AimAssistYaw(0.f, 40.f, 3.f), 40.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraTurnRateMathTest, "Hydra.Camera.TurnRate", HydraTests::Flags)
bool FHydraTurnRateMathTest::RunTest(const FString& Parameters)
{
	FHydraTurnRateValues V;
	V.min_deg_s = 120.f;
	V.max_deg_s = 720.f;
	V.base_deg_s = 540.f;
	TestEqual(TEXT("base inside floor and cap"), HydraCombatMath::TurnRateDegS(V), 540.f);
	V.base_deg_s = 50.f;
	TestEqual(TEXT("floor: nobody is helpless"), HydraCombatMath::TurnRateDegS(V), 120.f);
	V.base_deg_s = 5000.f;
	TestEqual(TEXT("cap: nobody turns instantly"), HydraCombatMath::TurnRateDegS(V), 720.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHydraNetSimMathTest, "Hydra.Net.EmulationMath", HydraTests::Flags)
bool FHydraNetSimMathTest::RunTest(const FString& Parameters)
{
	using namespace HydraCombatMath;
	TestEqual(TEXT("round-trip step: half per side"), OutgoingLagMs(100, true), 50);
	TestEqual(TEXT("one-way step: all per side"), OutgoingLagMs(100, false), 100);
	TestEqual(TEXT("no negative lag"), OutgoingLagMs(-5, true), 0);
	TestEqual(TEXT("2 % loss"), PacketLossPercent(0.02f), 2);
	TestEqual(TEXT("loss is clamped"), PacketLossPercent(3.f), 100);

	const TArray<int32> Steps = { 0, 50, 100, 150 };
	TestEqual(TEXT("next step"), NextLatencyStep(Steps, 50), 100);
	TestEqual(TEXT("wraps to the first"), NextLatencyStep(Steps, 150), 0);
	TestEqual(TEXT("unknown value starts at the first"), NextLatencyStep(Steps, 75), 0);
	TestEqual(TEXT("no steps"), NextLatencyStep(TArray<int32>(), 50), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
