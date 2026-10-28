// Copyright 2026 kirzo

#include "Animation/KzAnimationNotifyState.h"
#include "Animation/KzAnimationSource.h"

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