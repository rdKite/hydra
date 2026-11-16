#pragma once

#include "CoreMinimal.h"

/**
 * What one character did, counted on the server for the metrics CSV (tuning later). The
 * counters run from spawn; the game mode writes the difference since its last row.
 */
struct FHydraMetricCounters
{
	/** Weapon and body attacks started; casts are counted in Casts. */
	int32 Attacks = 0;
	/** Own attacks that landed unblocked / landed in a block / hit nobody. */
	int32 Hits = 0;
	int32 HitsBlocked = 0;
	int32 Misses = 0;
	/** Hits taken unblocked (weapon, body and bolt). */
	int32 HitsTaken = 0;
	/** Hits it took on its block, and blocks that broke. */
	int32 Blocks = 0;
	int32 BlockBreaks = 0;
	/** Dodge steps, and those that took it out of an attack that would have hit. */
	int32 Dodges = 0;
	int32 DodgesAvoided = 0;
	int32 Casts = 0;
	int32 BoltHits = 0;
	float DamageDealt = 0.f;
	float DamageTaken = 0.f;
	float StaminaUsed = 0.f;
	float ManaUsed = 0.f;
	int32 Deaths = 0;
	int32 Kills = 0;

	FHydraMetricCounters& operator+=(const FHydraMetricCounters& O)
	{
		Attacks += O.Attacks; Hits += O.Hits; HitsBlocked += O.HitsBlocked; Misses += O.Misses; HitsTaken += O.HitsTaken;
		Blocks += O.Blocks; BlockBreaks += O.BlockBreaks; Dodges += O.Dodges; DodgesAvoided += O.DodgesAvoided;
		Casts += O.Casts; BoltHits += O.BoltHits; DamageDealt += O.DamageDealt; DamageTaken += O.DamageTaken;
		StaminaUsed += O.StaminaUsed; ManaUsed += O.ManaUsed; Deaths += O.Deaths; Kills += O.Kills;
		return *this;
	}

	FHydraMetricCounters operator-(const FHydraMetricCounters& O) const
	{
		FHydraMetricCounters R = *this;
		R.Attacks -= O.Attacks; R.Hits -= O.Hits; R.HitsBlocked -= O.HitsBlocked; R.Misses -= O.Misses; R.HitsTaken -= O.HitsTaken;
		R.Blocks -= O.Blocks; R.BlockBreaks -= O.BlockBreaks; R.Dodges -= O.Dodges; R.DodgesAvoided -= O.DodgesAvoided;
		R.Casts -= O.Casts; R.BoltHits -= O.BoltHits; R.DamageDealt -= O.DamageDealt; R.DamageTaken -= O.DamageTaken;
		R.StaminaUsed -= O.StaminaUsed; R.ManaUsed -= O.ManaUsed; R.Deaths -= O.Deaths; R.Kills -= O.Kills;
		return R;
	}

	static const TCHAR* CsvHeader()
	{
		return TEXT("attacks,hits,hits_blocked,misses,hits_taken,blocks,block_breaks,dodges,dodges_avoided,casts,bolt_hits,damage_dealt,damage_taken,stamina_used,mana_used,deaths,kills");
	}

	FString ToCsv() const
	{
		return FString::Printf(TEXT("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.1f,%.1f,%.1f,%.1f,%d,%d"),
			Attacks, Hits, HitsBlocked, Misses, HitsTaken, Blocks, BlockBreaks, Dodges, DodgesAvoided, Casts, BoltHits,
			DamageDealt, DamageTaken, StaminaUsed, ManaUsed, Deaths, Kills);
	}
};
