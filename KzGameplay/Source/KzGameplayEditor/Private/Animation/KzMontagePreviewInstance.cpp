// Copyright 2026 kirzo

#include "Animation/KzMontagePreviewInstance.h"
#include "Animation/AnimMontage.h"
#include "AnimationRuntime.h"

namespace
{
	/** The montage's pose at Time, read off its first slot as the montage's slot node would, root locked when it has root motion. */
	void EvaluateMontage(const UAnimMontage& Montage, float Time, FPoseContext& Output)
	{
		if (Montage.SlotAnimTracks.IsEmpty())
		{
			Output.ResetToRefPose();
			return;
		}

		FAnimationPoseData PoseData(Output);
		Montage.SlotAnimTracks[0].AnimTrack.GetAnimationPose(PoseData, FAnimExtractContext(static_cast<double>(Time), Montage.HasRootMotion()));
	}
}

void FKzMontagePreviewProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	const UKzMontagePreviewInstance* Instance = CastChecked<UKzMontagePreviewInstance>(InAnimInstance);
	Montage = Instance->Montage;
	Time = Instance->Time;
	FromMontage = Instance->FromMontage;
	FromTime = Instance->FromTime;
	FromWeight = Instance->FromWeight;
}

bool FKzMontagePreviewProxy::Evaluate(FPoseContext& Output)
{
	if (!Montage)
	{
		Output.ResetToRefPose();
		return true;
	}

	if (!FromMontage || FromWeight <= 0.0f)
	{
		EvaluateMontage(*Montage, Time, Output);
		return true;
	}

	FPoseContext From(Output);
	FPoseContext To(Output);
	EvaluateMontage(*FromMontage, FromTime, From);
	EvaluateMontage(*Montage, Time, To);

	FAnimationPoseData OutputData(Output);
	FAnimationRuntime::BlendTwoPosesTogether(FAnimationPoseData(From), FAnimationPoseData(To), FromWeight, OutputData);
	return true;
}

void UKzMontagePreviewInstance::Play(UAnimMontage* InMontage)
{
	Montage = InMontage;
	Time = 0.0f;
	bPlaying = true;
	SetBlendFrom(nullptr, 0.0f, 0.0f);
}

void UKzMontagePreviewInstance::SetBlendFrom(UAnimMontage* InMontage, float InTime, float InWeight)
{
	FromMontage = InWeight > 0.0f ? InMontage : nullptr;
	FromTime = InTime;
	FromWeight = InWeight;
}

FAnimInstanceProxy* UKzMontagePreviewInstance::CreateAnimInstanceProxy()
{
	return new FKzMontagePreviewProxy(this);
}

void UKzMontagePreviewInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Played on its own it loops, as the single node instance it stands in for did.
	const float Length = Montage ? Montage->GetPlayLength() : 0.0f;
	if (bPlaying && Length > 0.0f)
	{
		Time = FMath::Fmod(Time + DeltaSeconds * PlayRate, Length);
	}
}
