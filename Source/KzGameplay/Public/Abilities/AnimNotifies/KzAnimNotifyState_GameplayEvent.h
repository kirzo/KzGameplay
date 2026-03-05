// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "KzAnimNotifyState_GameplayEvent.generated.h"

/** Animation notify state that sends Gameplay Events at the beginning and end of its duration. */
UCLASS(meta = (DisplayName = "Gameplay Event State"))
class KZGAMEPLAY_API UKzAnimNotifyState_GameplayEvent : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_GameplayEvent();

	/** The gameplay tag to send when the notify state begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	FGameplayTag BeginEventTag;

	/** The gameplay tag to send when the notify state ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	FGameplayTag EndEventTag;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};