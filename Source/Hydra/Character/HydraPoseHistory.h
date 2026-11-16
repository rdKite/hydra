#pragma once

#include "CoreMinimal.h"

/** Where a character was, where it faced and whether it blocked, at one server time. */
struct FHydraPoseSample
{
	double Time = 0.0;
	FVector Location = FVector::ZeroVector;
	float Yaw = 0.f;
	bool bBlocking = false;
};

/**
 * Lag compensation: the server's short memory of a character's poses. When the server
 * checks a hit, it looks up where the target was at the moment the attacker saw
 * (HydraCombatMath::RewindSeconds) - position for the hit area, facing and block state for
 * the block arc and the weak point. Plain data, no world access.
 */
class FHydraPoseHistory
{
public:
	/** Adds a sample (times must not go backwards) and drops samples older than KeepSeconds. */
	void Add(const FHydraPoseSample& Sample, double KeepSeconds)
	{
		if (Samples.Num() > 0 && Sample.Time < Samples.Last().Time)
		{
			Samples.Reset(); // time went backwards (should not happen): start over
		}
		Samples.Add(Sample);
		int32 FirstNeeded = 0;
		// Keep one sample older than the window, so the start of the window can be interpolated.
		while (FirstNeeded + 1 < Samples.Num() && Samples[FirstNeeded + 1].Time < Sample.Time - KeepSeconds)
		{
			++FirstNeeded;
		}
		if (FirstNeeded > 0)
		{
			Samples.RemoveAt(0, FirstNeeded, EAllowShrinking::No);
		}
	}

	/**
	 * The pose at Time: interpolated between the two samples around it; the oldest or newest
	 * sample if Time is outside the memory. The block state is the one that held at Time
	 * (from the sample before it). Returns false if there are no samples.
	 */
	bool Get(double Time, FHydraPoseSample& Out) const
	{
		if (Samples.Num() == 0)
		{
			return false;
		}
		if (Time <= Samples[0].Time)
		{
			Out = Samples[0];
			return true;
		}
		if (Time >= Samples.Last().Time)
		{
			Out = Samples.Last();
			return true;
		}
		for (int32 i = Samples.Num() - 2; i >= 0; --i)
		{
			if (Samples[i].Time <= Time)
			{
				const FHydraPoseSample& A = Samples[i];
				const FHydraPoseSample& B = Samples[i + 1];
				const double Span = B.Time - A.Time;
				const float Alpha = Span > UE_SMALL_NUMBER ? static_cast<float>((Time - A.Time) / Span) : 0.f;
				Out.Time = Time;
				Out.Location = FMath::Lerp(A.Location, B.Location, Alpha);
				Out.Yaw = FRotator::NormalizeAxis(A.Yaw + FMath::FindDeltaAngleDegrees(A.Yaw, B.Yaw) * Alpha);
				Out.bBlocking = A.bBlocking;
				return true;
			}
		}
		Out = Samples[0];
		return true;
	}

	/** Forget everything (teleport, respawn: the old poses must not be hit). */
	void Reset() { Samples.Reset(); }

	int32 Num() const { return Samples.Num(); }

private:
	TArray<FHydraPoseSample> Samples;
};
