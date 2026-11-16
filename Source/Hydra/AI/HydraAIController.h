#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HydraAIController.generated.h"

class AHydraCharacter;

/**
 * The simple AI opponent for practice (server only). It plays an ordinary AHydraCharacter
 * through the same inputs a player has - attack button, block button, dodge - so every rule
 * applies to it unchanged: stamina, turn rate, attacks that bind, block arc, no invisible
 * bonuses.
 *
 * Behaviour (numbers in values.json, ai_practice.*):
 *  - walks up to the nearest living player and keeps facing it (which also protects its
 *    back; it turns no faster than its turn rate, so it can be flanked);
 *  - attacks when the player is inside its reach and arc, light or heavy by chance, then
 *    pauses;
 *  - when it sees an attack start nearby it reacts after its reaction time - blocks or
 *    dodges by chance - or misses the moment altogether (error rate).
 *
 * No navigation mesh: it walks straight at its target (the arena is open). Smarter
 * behaviour and waves come with M4.
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
	void UpdateTarget(const AHydraCharacter* Self);
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
	EReaction PendingReaction = EReaction::None;
	double ReactAt = -1.0;
	TWeakObjectPtr<AHydraCharacter> ReactTo;
};
