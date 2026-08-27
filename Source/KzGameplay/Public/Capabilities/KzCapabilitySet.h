// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KzCapabilitySet.generated.h"

class UGameplayAbility;
class UKzInputProfile;

/**
 * What backs a capability. An empty Ability is meaningful: the owner can do the thing and needs no ability
 * to do it, which is how a scripted actor holds a capability the player exercises through GAS.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzCapabilityGrant
{
	GENERATED_BODY()

	/**
	 * Whether the owner starts with this, or has to be given it by something.
	 *
	 * The set says what every capability is made of, including ones the owner may never hold: attacking
	 * is defined here so the axe does not have to know about abilities, but you only get it by picking
	 * the axe up.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capability")
	bool bGrantedByDefault = false;

	/** Ability granted alongside the capability tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capability")
	TSubclassOf<UGameplayAbility> Ability;

	/** Level the ability is granted at. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capability", meta = (ClampMin = 1))
	int32 Level = 1;

	/**
	 * Controls that come with the capability. Knowing how to do something and having the buttons for it
	 * are the same fact, so whatever grants the capability never has to mention inputs at all.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capability")
	TArray<TObjectPtr<UKzInputProfile>> InputProfiles;
};

/**
 * Maps capabilities to what implements them, with inheritance so a set can extend another and override
 * single entries.
 *
 * Capabilities are the vocabulary the world asks in: an object requires Capability.Push and never learns
 * which ability, if any, answers it. An ability implements a capability, not the other way round.
 */
UCLASS(BlueprintType, Const)
class KZGAMEPLAY_API UKzCapabilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Set to inherit from. Entries here override entries with the same tag anywhere up the chain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capabilities")
	TObjectPtr<UKzCapabilitySet> ParentSet;

	/** Capabilities defined by this set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capabilities", meta = (Categories = "Capability"))
	TMap<FGameplayTag, FKzCapabilityGrant> Capabilities;

	/** What implements a capability, searching up the chain. Null if nothing defines it. */
	const FKzCapabilityGrant* FindGrant(FGameplayTag Capability) const;

	/** True if this set or an ancestor defines the capability. */
	UFUNCTION(BlueprintPure, Category = "Capabilities")
	bool DefinesCapability(FGameplayTag Capability) const;

	/** Flattens the chain into OutCapabilities, closest definition winning. */
	void CollectCapabilities(TMap<FGameplayTag, FKzCapabilityGrant>& OutCapabilities) const;

	/** Every capability tag this set resolves to. */
	UFUNCTION(BlueprintPure, Category = "Capabilities")
	FGameplayTagContainer GetCapabilityTags() const;

private:
	/** Guards against a parent chain that loops back on itself. */
	mutable bool bWalking = false;
};
