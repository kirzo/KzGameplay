// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "KzAnimNotify_StowItem.generated.h"

/** Puts the item in a slot away, or draws it back, at this frame: where the hand reaches the scabbard. Works in any animation, played by anything. */
UCLASS(meta = (DisplayName = "Stow Item"))
class KZGAMEPLAY_API UKzAnimNotify_StowItem : public UAnimNotify
{
	GENERATED_BODY()

public:
	UKzAnimNotify_StowItem();

	/** The slot the item is used from, such as the hand. */
	UPROPERTY(EditAnywhere, Category = "Stow Item", meta = (Categories = "Equipment.Slot"))
	FGameplayTag Slot;

	/** Draws the item back into Slot instead of putting it away. */
	UPROPERTY(EditAnywhere, Category = "Stow Item")
	bool bDraw = false;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
