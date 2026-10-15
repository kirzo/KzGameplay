// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Equipment/KzEquipListener.h"
#include "GameplayTagContainer.h"
#include "Items/KzItemFragment.h"
#include "KzItemFragment_LimbicPose.generated.h"

class ULimbicPuppeteer;

/**
 * Holds a Limbic pose while the item is equipped, such as both hands gripping a hose: plays a puppeteer in a slot of the Limbic
 * node of the skeletal mesh the item hangs from, blending in as it is equipped and out as it is put away, on every machine.
 * The character's puppeteer needs a Slot node with the same tag.
 */
UCLASS(ClassGroup = "Animation", DisplayName = "Limbic Pose")
class KZGAMEPLAYLIMBIC_API UKzItemFragment_LimbicPose : public UKzItemFragment, public IKzEquipListener
{
	GENERATED_BODY()

public:
	UKzItemFragment_LimbicPose();

	/** Slot of the character's puppeteer the pose plays in. */
	UPROPERTY(EditAnywhere, Category = "Pose", meta = (Categories = "Limbic.Slot"))
	FGameplayTag Slot;

	/** Puppeteer held while the item is equipped. */
	UPROPERTY(EditAnywhere, Category = "Pose")
	TSoftObjectPtr<ULimbicPuppeteer> Puppeteer;

	UPROPERTY(EditAnywhere, Category = "Pose", meta = (ClampMin = "0", Units = "Seconds"))
	float BlendIn = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Pose", meta = (ClampMin = "0", Units = "Seconds"))
	float BlendOut = 0.25f;

	virtual void GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const override;
	virtual void OnEquipped_Implementation(const FKzEquipEvent& Event) const override;
	virtual void OnUnequipped_Implementation(const FKzEquipEvent& Event) const override;
};