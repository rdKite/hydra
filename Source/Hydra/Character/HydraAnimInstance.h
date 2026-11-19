#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "HydraAnimInstance.generated.h"

class UAnimSequence;

/** What the character wants shown this frame; handed from the game thread to the proxy. */
struct FHydraAnimInput
{
	/** The clip to show and where in it (seconds). Null = reference pose. */
	TObjectPtr<const UAnimSequence> Clip = nullptr;
	float Time = 0.f;
	bool bLoop = false;
	/** Changes when the same clip starts again (a second attack right after the first). */
	int32 Serial = 0;
	/** Length factor of the leg bones (1 = as built). The pipeline test for character creation. */
	float LegScale = 1.f;
};

/**
 * The "anim graph" in C++ (no Animation Blueprint, which would be a binary editor asset):
 * one clip per thing the game shows, with a short cross-fade from the pose before, and
 * bone lengths that can be scaled at runtime (legs).
 */
struct FHydraAnimInstanceProxy : public FAnimInstanceProxy
{
	FHydraAnimInstanceProxy() = default;
	explicit FHydraAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void Update(float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	void SampleClip(const UAnimSequence* Clip, float Time, bool bLoop, FPoseContext& Out) const;
	void ScaleLegs(FPoseContext& Output) const;

	FHydraAnimInput Input;
	// The clip shown now and the one it fades from (frozen at the time it was left).
	TObjectPtr<const UAnimSequence> CurrentClip = nullptr;
	int32 CurrentSerial = 0;
	float CurrentTime = 0.f;
	bool bCurrentLoop = false;
	TObjectPtr<const UAnimSequence> PreviousClip = nullptr;
	float PreviousTime = 0.f;
	bool bPreviousLoop = false;
	/** 1 = only the current clip. */
	float BlendAlpha = 1.f;
};

/**
 * Animation of every form that has a model (docs/tech/blender-pipeline.md). The character
 * decides which clip and where in it (AHydraCharacter::UpdateMeshAnimation: clips of actions
 * are time-scaled to the action's duration from values.json); this class blends and
 * evaluates. Animation logic stays in C++, as CLAUDE.md asks.
 */
UCLASS(Transient)
class HYDRA_API UHydraAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Game thread: what to show from the next update on. */
	void SetInput(const FHydraAnimInput& InInput) { PendingInput = InInput; }

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FHydraAnimInstanceProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }

private:
	friend struct FHydraAnimInstanceProxy;
	FHydraAnimInput PendingInput;
};
