#pragma once

#include "CoreMinimal.h"
#include "Data/HydraValues.h"

/**
 * Pure functions for positional rules (no world access), so they can be unit-tested and
 * used identically on server and client. Distances in cm, angles in degrees.
 */
namespace HydraCombatMath
{
	/**
	 * Turn rate of a character in deg/s. M1: the base rate inside floor and cap. M2 adds
	 * weight / strength and the slowdown while blocking and attacking.
	 */
	inline float TurnRateDegS(const FHydraTurnRateValues& V)
	{
		return FMath::Clamp(V.base_deg_s, V.min_deg_s, FMath::Max(V.min_deg_s, V.max_deg_s));
	}

	/**
	 * Is Target inside the cone from Origin along Direction (2D, half angle MaxAngleDeg, length
	 * RangeCm)? OutScore: lower is better - near the centre of the cone first, then near.
	 */
	inline bool ScoreTargetInCone(const FVector& Origin, const FVector& Direction, const FVector& Target, float RangeCm, float MaxAngleDeg, float& OutScore)
	{
		const FVector Delta = Target - Origin;
		const float Dist = Delta.Size2D();
		if (Dist > RangeCm || RangeCm <= 0.f)
		{
			return false;
		}
		const float Cos = Dist > KINDA_SMALL_NUMBER ? FVector::DotProduct(Delta.GetSafeNormal2D(), Direction.GetSafeNormal2D()) : 1.f;
		if (Cos < FMath::Cos(FMath::DegreesToRadians(MaxAngleDeg)))
		{
			return false;
		}
		OutScore = (1.f - Cos) * 4.f + Dist / RangeCm;
		return true;
	}

	/**
	 * Soft aim assist: the yaw after turning from CurrentYaw towards TargetYaw by Strength
	 * (0 = no help, 1 = face the target exactly), along the shorter way round.
	 */
	inline float AimAssistYaw(float CurrentYaw, float TargetYaw, float Strength)
	{
		return CurrentYaw + FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw) * FMath::Clamp(Strength, 0.f, 1.f);
	}

	/** Network emulation: delay each side adds to its outgoing packets for a latency step. */
	inline int32 OutgoingLagMs(int32 LatencyStepMs, bool bStepIsRoundTrip)
	{
		return FMath::Max(0, bStepIsRoundTrip ? LatencyStepMs / 2 : LatencyStepMs);
	}

	/** Network emulation: the engine takes packet loss as a whole percent. */
	inline int32 PacketLossPercent(float Ratio)
	{
		return FMath::Clamp(FMath::RoundToInt(Ratio * 100.f), 0, 100);
	}

	/** Next entry of the latency steps after Current (wraps; unknown Current starts at the first). */
	inline int32 NextLatencyStep(const TArray<int32>& Steps, int32 Current)
	{
		if (Steps.Num() == 0)
		{
			return 0;
		}
		return Steps[(Steps.IndexOfByKey(Current) + 1) % Steps.Num()];
	}
}
