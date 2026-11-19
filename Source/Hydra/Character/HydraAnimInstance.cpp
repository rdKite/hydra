#include "Character/HydraAnimInstance.h"

#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"

namespace HydraAnim
{
	/** Cross-fade between two clips (looks only; the timing of actions comes from values.json). */
	constexpr float BlendS = 0.08f;

	// Bones of the humanoid skeleton (names of the Unreal mannequin) whose length the leg scale changes.
	const FName Pelvis(TEXT("pelvis"));
	const FName LowerLegs[] = { FName(TEXT("calf_l")), FName(TEXT("calf_r")) };
	const FName Feet[] = { FName(TEXT("foot_l")), FName(TEXT("foot_r")) };
}

void FHydraAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	Input = CastChecked<UHydraAnimInstance>(InAnimInstance)->PendingInput;
}

void FHydraAnimInstanceProxy::Update(float DeltaSeconds)
{
	if (Input.Clip != CurrentClip || Input.Serial != CurrentSerial)
	{
		// A new clip (or the same one from its start): fade from where the old one was left.
		PreviousClip = CurrentClip;
		PreviousTime = CurrentTime;
		bPreviousLoop = bCurrentLoop;
		CurrentClip = Input.Clip;
		CurrentSerial = Input.Serial;
		BlendAlpha = PreviousClip ? 0.f : 1.f;
	}
	CurrentTime = Input.Time;
	bCurrentLoop = Input.bLoop;
	if (BlendAlpha < 1.f)
	{
		BlendAlpha = FMath::Min(1.f, BlendAlpha + DeltaSeconds / HydraAnim::BlendS);
		if (BlendAlpha >= 1.f)
		{
			PreviousClip = nullptr;
		}
	}
}

void FHydraAnimInstanceProxy::SampleClip(const UAnimSequence* Clip, float Time, bool bLoop, FPoseContext& Out) const
{
	FAnimationPoseData PoseData(Out);
	Clip->GetAnimationPose(PoseData, FAnimExtractContext(static_cast<double>(Time), false, FDeltaTimeRecord(), bLoop));
}

bool FHydraAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	if (CurrentClip)
	{
		SampleClip(CurrentClip, CurrentTime, bCurrentLoop, Output);
	}
	else
	{
		Output.ResetToRefPose();
	}
	if (PreviousClip && BlendAlpha < 1.f)
	{
		FPoseContext From(Output);
		SampleClip(PreviousClip, PreviousTime, bPreviousLoop, From);
		FAnimationPoseData OutData(Output);
		const FAnimationPoseData FromData(From);
		// Weight of the first pose (the new clip) = BlendAlpha.
		FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, FromData, BlendAlpha);
	}
	if (!FMath::IsNearlyEqual(Input.LegScale, 1.f))
	{
		ScaleLegs(Output);
	}
	return true;
}

void FHydraAnimInstanceProxy::ScaleLegs(FPoseContext& Output) const
{
	// A bone's length is the offset of its child. Thigh and calf get shorter (or longer) by
	// moving calf and foot along their own offsets; the pelvis comes down by what the legs
	// lost, so the feet stay on the floor. Nothing is scaled, so the limbs keep their girth.
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	auto Find = [&Bones](FName Name)
	{
		const int32 PoseIndex = Bones.GetPoseBoneIndexForBoneName(Name);
		return PoseIndex != INDEX_NONE ? Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(PoseIndex)) : FCompactPoseBoneIndex(INDEX_NONE);
	};
	const float Scale = FMath::Clamp(Input.LegScale, 0.3f, 2.f);
	float LegLength = 0.f;
	for (const FName* Group : { HydraAnim::LowerLegs, HydraAnim::Feet })
	{
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FCompactPoseBoneIndex Index = Find(Group[Side]);
			if (Index.GetInt() == INDEX_NONE || !Output.Pose.IsValidIndex(Index))
			{
				return; // not the humanoid skeleton
			}
			if (Side == 0)
			{
				LegLength += Output.Pose.GetRefPose(Index).GetTranslation().Size();
			}
			FTransform& Bone = Output.Pose[Index];
			Bone.SetTranslation(Bone.GetTranslation() * Scale);
		}
	}
	const FCompactPoseBoneIndex PelvisIndex = Find(HydraAnim::Pelvis);
	if (PelvisIndex.GetInt() != INDEX_NONE && Output.Pose.IsValidIndex(PelvisIndex))
	{
		// The pelvis hangs straight above the root: move it along its reference offset.
		const FVector Up = Output.Pose.GetRefPose(PelvisIndex).GetTranslation().GetSafeNormal();
		FTransform& PelvisBone = Output.Pose[PelvisIndex];
		PelvisBone.SetTranslation(PelvisBone.GetTranslation() - Up * LegLength * (1.f - Scale));
	}
}
