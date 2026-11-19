#pragma once

#include "CoreMinimal.h"
#include "Character/HydraCombatMath.h"
#include "Components/ActorComponent.h"
#include "HydraProgressionComponent.generated.h"

class AHydraCharacter;

/** One skill of a character. */
USTRUCT()
struct FHydraSkillState
{
	GENERATED_BODY()

	/** skills.json ID. */
	UPROPERTY() FName Id;
	UPROPERTY() int32 Level = 0;
	/** Progress bar towards the next level: the fraction of a skill point that is left (0..1; more only while the skill is at its limit). */
	UPROPERTY() float Progress = 0.f;
	/** hXP this skill got in the current character level; decides its share of the skill points at the level-up. */
	UPROPERTY() float LevelHxp = 0.f;
	/** Stamina and mana its actions spent since the last defeated enemy; decides its share of that enemy's hXP. */
	UPROPERTY() float Use = 0.f;
	/** Instinct from earlier forms: how high the skill was relative to the old level cap (0..1). */
	UPROPERTY() float InstinctRatio = 0.f;
};

/**
 * How a character grows (Phase 1 design, sections 17 and 26), decided on the server and
 * replicated:
 *
 *  - Character level from nXP for defeated enemies. Each level raises the body attributes.
 *  - Skill growth: a defeated enemy also gives hXP, split over the skills in proportion to
 *    the stamina and mana their actions spent since the last defeated enemy. At a level-up
 *    the character gets automatic skill points (more for a high hardship rating = hXP / nXP
 *    of the level), split in proportion to the hXP each skill got in that level. Fractions
 *    stay on a progress bar per skill; no skill rises above the character level.
 *  - One imprint (fire): progress from contact with the element. At a milestone (every
 *    milestone_every_levels levels) the player may start or raise it, if earned.
 *  - Evolution at a milestone or at the level cap, if the new form's imprint is there or
 *    earned: level back to 0, skills back to 0 with an instinct per skill.
 *
 * The numbers this leads to (bars, damage, ...) are worked out in
 * HydraCombatMath::BuildStats from MakeView().
 */
UCLASS()
class HYDRA_API UHydraProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHydraProgressionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetLevel() const { return Level; }
	float GetXp() const { return Xp; }
	float GetXpForNextLevel() const;
	/** hXP / nXP of the current level, at least 1. */
	float GetHardship() const { return HydraCombatMath::HardshipRating(LevelHxp, LevelNxp); }
	int32 GetImprintLevel() const { return ImprintLevel; }
	float GetImprintProgress() const { return ImprintProgress; }
	float GetImprintProgressNeeded() const;
	int32 GetOpenMilestones() const { return OpenMilestones; }
	const TArray<FHydraSkillState>& GetSkills() const { return Skills; }
	const FHydraSkillState* FindSkill(FName Id) const;

	/** Level, effective skill levels and imprint for BuildStats. */
	FHydraProgressView MakeView() const;

	// --- What a milestone offers (the same answer on server and client) --------------------

	/** An open milestone and enough progress: the imprint can be started or raised. */
	bool CanRaiseImprint() const;

	/** The form it can evolve into now, or None. */
	FName GetEvolutionOffer() const;

	// --- Server ------------------------------------------------------------------------------

	/** Start state: character level and the level of every skill of the form. Clears everything else. */
	void ServerInit(int32 InLevel, int32 InSkillLevel, int32 InImprintLevel);

	/** An action of a skill spent stamina or mana (counts towards its share of the next enemy's hXP). */
	void ServerAddUse(FName SkillId, float Amount);

	/** An enemy was defeated: its hXP go to the skills by use, its nXP to the level; level-ups give skill points. */
	void ServerEnemyDefeated(float Nxp, float Hxp);

	/** The character was defeated: the use counted since the last defeated enemy is lost. */
	void ServerDropUse();

	/** Contact with fire (damage dealt or taken, already weighted). */
	void ServerAddImprintProgress(float Amount);

	/** The player's choice at a milestone: "imprint" or "evolve". False if it is not on offer. */
	bool ServerChoose(FName Choice);

	// Test tools (debug panel and dev commands): set the state directly.
	void ServerSetLevel(int32 NewLevel);
	void ServerSetSkill(FName SkillId, int32 NewLevel, float NewProgress);
	void ServerSetImprintLevel(int32 NewLevel);
	void ServerOpenMilestone();

protected:
	UFUNCTION()
	void OnRep_Progress();

	AHydraCharacter* GetCharacter() const;
	FHydraSkillState& EnsureSkill(FName Id);
	void AwakenElementSkill();
	void Changed();

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	int32 Level = 0;

	UPROPERTY(Replicated)
	float Xp = 0.f;

	/** hXP and nXP earned in the current level (hardship rating = their ratio). */
	UPROPERTY(Replicated)
	float LevelHxp = 0.f;

	UPROPERTY(Replicated)
	float LevelNxp = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	TArray<FHydraSkillState> Skills;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	int32 ImprintLevel = 0;

	UPROPERTY(Replicated)
	float ImprintProgress = 0.f;

	/** Milestones reached and not used yet (assumption: a milestone stays open until it is used). */
	UPROPERTY(Replicated)
	int32 OpenMilestones = 0;
};
