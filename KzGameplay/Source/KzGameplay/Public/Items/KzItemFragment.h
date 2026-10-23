// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KzItemFragment.generated.h"

struct FKzItemInstance;

/**
 * Base class for modular item data fragments.
 * Fragments define specific properties or behaviors for an item definition.
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract, Blueprintable)
class KZGAMEPLAY_API UKzItemFragment : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Called when a new runtime instance of this item is created (e.g., added to inventory).
	 * Allows the fragment to inject initial state, like default stats, into the instance.
	 * @param Instance The newly created item instance (mutable so we can add stats to it).
	 */
	virtual void OnInstanceCreated(FKzItemInstance& Instance) const;

	/**
	 * Fragment classes this one cannot work without on the same item, as Unity's RequireComponent.
	 * The item adds them when this one is added, the item editor will not remove them while this one stays, and validation fails if one is missing.
	 */
	virtual void GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const {}

	/** Soft assets the item uses in play, such as the pose it is held in; its world actor loads them in the background, so using the item doesn't hitch. */
	virtual void GetPreloadPaths(TArray<FSoftObjectPath>& OutPaths) const {}

protected:
	/** Blueprint-implementable event for OnInstanceCreated. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Item Fragment", meta = (DisplayName = "On Instance Created"))
	void K2_OnInstanceCreated(UPARAM(ref) FKzItemInstance& Instance) const;
};