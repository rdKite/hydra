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
	/** Credited skill XP towards the next level; it waits for the next character level-up. */
	UPROPERTY() float Xp = 0.f;
	/** Earned in the running fight; credited at its end, only on victory. */
	UPROPERTY() float PendingXp = 0.f;
	/** Instinct from earlier forms: how high the skill was relative to the old level cap (0..1). */
	UPROPERTY() float InstinctRatio = 0.f;
};

/**
 * How a character grows (Phase 1 design, section 17), decided on the server and replicated:
 *
 *  - Character level from nXP for defeated enemies. Each level: a little more HP and damage.
 *  - Skills grow by use. Skill XP is earned in a fight (pending), credited at its end on
 *    victory, and turned into skill levels only at a character level-up - one level per
 *    level-up and skill, never above the character level.
 *  - One imprint (fire): progress from contact with the element. At a milestone (every
 *    milestone_every_levels levels) the player may start or raise it, if earned.
 *  - Evolution at a milestone or at the level cap, if the new form's imprint is there or
 *    earned: level back to 0, skills back to 0 with an instinct per skill.
 *
 * The numbers this leads to (damage, costs, HP, ...) are worked out in
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

	/** nXP for a defeated enemy; level-ups raise skills that have enough skill XP. */
	void ServerAddXp(float Nxp);

	/** Skill XP earned by an action in the running fight (pending until the fight is won). */
	void ServerAddSkillXp(FName SkillId, float Amount);

	/** The fight is won: pending skill XP is credited. Lost: it is dropped. */
	void ServerEndFight(bool bVictory);

	/** Contact with fire (damage dealt or taken, already weighted). */
	void ServerAddImprintProgress(float Amount);

	/** Dev: set the imprint level directly. */
	void ServerSetImprintLevel(int32 NewLevel);

	/** The player's choice at a milestone: "imprint" or "evolve". False if it is not on offer. */
	bool ServerChoose(FName Choice);

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
