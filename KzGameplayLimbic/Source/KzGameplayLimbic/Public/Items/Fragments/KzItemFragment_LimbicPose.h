// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzDatabase.h"
#include "Equipment/KzEquipListener.h"
#include "GameplayTagContainer.h"
#include "Items/KzItemFragment.h"
#include "KzItemFragment_LimbicPose.generated.h"

class ULimbicPuppeteer;

/**
 * Holds a Limbic pose while the item is equipped, such as both hands gripping a hose: plays a puppeteer in a slot of the Limbic
 * node of the skeletal mesh the item hangs from, blending in as it is equipped and out as it is put away, on every machine.
 * The character's puppeteer needs a Slot node with the same tag. Subclasses choose the puppeteer.
 */
UCLASS(Abstract)
class KZGAMEPLAYLIMBIC_API UKzItemFragment_LimbicPoseBase : public UKzItemFragment, public IKzEquipListener
{
	GENERATED_BODY()

public:
	UKzItemFragment_LimbicPoseBase();

	/** Slot of the character's puppeteer the pose plays in. */
	UPROPERTY(EditAnywhere, Category = "Pose", meta = (Categories = "Limbic.Slot"))
	FGameplayTag Slot;

	UPROPERTY(EditAnywhere, Category = "Pose", meta = (ClampMin = "0", Units = "Seconds"))
	float BlendIn = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Pose", meta = (ClampMin = "0", Units = "Seconds"))
	float BlendOut = 0.25f;

	virtual void GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const override;
	virtual void OnEquipped_Implementation(const FKzEquipEvent& Event) const override;
	virtual void OnUnequipped_Implementation(const FKzEquipEvent& Event) const override;

protected:
	/** The puppeteer to hold for this equip. */
	virtual ULimbicPuppeteer* GetPuppeteer(const FKzEquipEvent& Event) const PURE_VIRTUAL(UKzItemFragment_LimbicPoseBase::GetPuppeteer, return nullptr;);
};

/** Holds the puppeteer set here. */
UCLASS(ClassGroup = "Animation", DisplayName = "Limbic Pose")
class KZGAMEPLAYLIMBIC_API UKzItemFragment_LimbicPose : public UKzItemFragment_LimbicPoseBase
{
	GENERATED_BODY()

public:
	/** Puppeteer held while the item is equipped. */
	UPROPERTY(EditAnywhere, Category = "Pose")
	TSoftObjectPtr<ULimbicPuppeteer> Puppeteer;

protected:
	virtual ULimbicPuppeteer* GetPuppeteer(const FKzEquipEvent& Event) const override;
};

/** Holds the puppeteer a query picks: from the item's Database fragment, or from the holder's database when it has a more specific one. */
UCLASS(ClassGroup = "Animation", DisplayName = "Limbic Pose (By Query)")
class KZGAMEPLAYLIMBIC_API UKzItemFragment_LimbicPoseByQuery : public UKzItemFragment_LimbicPoseBase
{
	GENERATED_BODY()

public:
	/** What to ask, such as RequireTags Pose.Hold, from tables of soft Limbic Puppeteer references. The item's ItemTags and the holder's state are the context. */
	UPROPERTY(EditAnywhere, Category = "Pose")
	FKzDatabaseQuery Query;

protected:
	virtual ULimbicPuppeteer* GetPuppeteer(const FKzEquipEvent& Event) const override;
};