// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "KzAnimationNotifyState.generated.h"

/** Notify state handed to the animation source play that fired it, which tells whoever started that play. Nothing hears it without one, as in the animation editors. */
UCLASS(Abstract)
class KZGAMEPLAY_API UKzAnimationNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};