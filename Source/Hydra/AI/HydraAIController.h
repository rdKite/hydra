#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HydraAIController.generated.h"

class AHydraCharacter;

/**
 * The AI of the practice opponent and of every wave enemy (server only). It plays an ordinary AHydraCharacter
 * through the same inputs a player has - attack button, block button, dodge - so every rule
 * applies to it unchanged: stamina, turn rate, attacks that bind, block arc, no invisible
 * bonuses.
 *
 * Behaviour (numbers in values.json: ai_practice.* for the practice opponent, enemies.<id>.ai.*
 * for a wave enemy - reaction time and error rate per enemy type):
 *  - walks up to the nearest living player and keeps facing it (which also protects its
 *    back; it turns no faster than its turn rate, so it can be flanked);
 *  - attacks when the player is inside its reach and arc, light or heavy by chance, then
 *    pauses;
 *  - when it sees an attack start nearby it reacts after its reaction time - blocks or
 *    dodges by chance - or misses the moment altogether (error rate);
 *  - a form with the bolt (and cast_range_m > 0) keeps its distance while it has mana and
 *    casts at the target, aimed where the target stands (no leading: a moving target is missed).
 *
 * No navigation mesh: it walks straight at its target and steps around an obstacle that is
 * in the way (a short probe ahead, then sideways past it). A caster only casts with a clear
 * line to its target; otherwise it comes closer.
 *
 * Passive mode (console Hydra.AI 0, or -HydraAIPassive on the command line): it does nothing
 * by itself and only follows the HydraAI dev commands - the stand-in for a training dummy
 * and for scripted runs.
 */
UCLASS()
class HYDRA_API AHydraAIController : public AAIController
{
	GENERATED_BODY()

public:
	AHydraAIController();

	virtual void Tick(float DeltaSeconds) override;

	/** Server: Attacker just started an attack (called for every AI controller). */
	void OnAttackSeen(AHydraCharacter* Attacker);

	void SetPassive(bool bInPassive);
	bool IsPassive() const { return bPassive; }

	// Dev commands (scripted runs): drive the inputs directly.
	void CommandBlock(bool bHold);
	void CommandAttack(float HoldMs);

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	AHydraCharacter* GetHydraCharacter() const;
	/** The numbers this AI plays by. */
	const struct FHydraAiValues& GetAiValues() const;
	void UpdateTarget(const AHydraCharacter* Self);
	/** Is something of the level in the way from From along Direction (the body as a sphere)? */
	bool IsWayBlocked(const AHydraCharacter* Self, const FVector& From, const FVector& Direction, float DistanceCm) const;
	/** Direction to walk in to get to the target: straight, or sideways around an obstacle. */
	FVector SteerTowards(const AHydraCharacter* Self, const FVector& ToTarget, double Now);
	void StopActing(AHydraCharacter* Self);
	void React(AHydraCharacter* Self);

	enum class EReaction : uint8 { None, Block, Dodge };

	TWeakObjectPtr<AHydraCharacter> Target;
	bool bPassive = false;
	double NextTargetUpdate = 0.0;
	double NextAttackTime = 0.0;
	/** > 0: the attack button is held and will be released at this time. */
	double ReleaseAttackAt = -1.0;
	/** > 0: the block button is held and will be released at this time. */
	double ReleaseBlockAt = -1.0;
	/** > 0: the cast button is held and will be released at this time. */
	double ReleaseCastAt = -1.0;
	double NextCastTime = 0.0;
	bool bHadPawn = false;
	/** Stepping around an obstacle: direction, until when, and the side taken last (+1 left, -1 right). */
	FVector DetourDirection = FVector::ZeroVector;
	double DetourUntil = 0.0;
	float DetourSide = 1.f;
	EReaction PendingReaction = EReaction::None;
	double ReactAt = -1.0;
	TWeakObjectPtr<AHydraCharacter> ReactTo;
};
