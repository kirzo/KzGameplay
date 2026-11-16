// Copyright 2026 kirzo

#include "Animation/KzAnimationNotifyState.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/KzAnimationSource.h"

float FKzAnimTime::ToSeconds(const UAnimSequenceBase* Animation) const
{
	if (Unit == EKzAnimTimeUnit::Seconds)
	{
		return Value;
	}

	const FFrameRate FrameRate = Animation && Animation->GetSamplingFrameRate().IsValid() ? Animation->GetSamplingFrameRate() : FFrameRate(30, 1);
	return static_cast<float>(FrameRate.AsSeconds(FFrameTime::FromDecimal(Value)));
}

void UKzAnimationNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	FKzAnimationPlay::RouteNotify(MeshComp, EventReference, this, true, TotalDuration);
}

void UKzAnimationNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	FKzAnimationPlay::RouteNotify(MeshComp, EventReference, this, false, 0.0f);
}