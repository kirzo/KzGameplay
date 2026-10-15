// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "KzAnimNotify_SendGameplayEvent.generated.h"

/**
 * Animation notify that sends a Gameplay Event to the owner's Ability System Component.
 * Ideal for combo windows, footstep events, or triggering ability logic from animations.
 */
UCLASS(meta = (DisplayName = "Send Gameplay Event"))
class KZGAMEPLAY_API UKzAnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UKzAnimNotify_SendGameplayEvent();

	/** The gameplay tag to send as the event type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	FGameplayTag EventTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};