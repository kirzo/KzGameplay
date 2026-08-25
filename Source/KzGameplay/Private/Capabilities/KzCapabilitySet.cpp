// Copyright 2026 kirzo

#include "Capabilities/KzCapabilitySet.h"

const FKzCapabilityGrant* UKzCapabilitySet::FindGrant(FGameplayTag Capability) const
{
	if (const FKzCapabilityGrant* Grant = Capabilities.Find(Capability))
	{
		return Grant;
	}

	if (!ParentSet || bWalking)
	{
		if (bWalking)
		{
			UE_LOG(LogTemp, Error, TEXT("Capability set %s inherits from itself through its parent chain."), *GetNameSafe(this));
		}

		return nullptr;
	}

	TGuardValue<bool> Guard(bWalking, true);
	return ParentSet->FindGrant(Capability);
}

bool UKzCapabilitySet::DefinesCapability(FGameplayTag Capability) const
{
	return FindGrant(Capability) != nullptr;
}

void UKzCapabilitySet::CollectCapabilities(TMap<FGameplayTag, FKzCapabilityGrant>& OutCapabilities) const
{
	if (bWalking)
	{
		UE_LOG(LogTemp, Error, TEXT("Capability set %s inherits from itself through its parent chain."), *GetNameSafe(this));
		return;
	}

	// Ancestors first, so each generation overwrites what it redefines
	if (ParentSet)
	{
		TGuardValue<bool> Guard(bWalking, true);
		ParentSet->CollectCapabilities(OutCapabilities);
	}

	OutCapabilities.Append(Capabilities);
}

FGameplayTagContainer UKzCapabilitySet::GetCapabilityTags() const
{
	TMap<FGameplayTag, FKzCapabilityGrant> Resolved;
	CollectCapabilities(Resolved);

	FGameplayTagContainer Tags;
	for (const TPair<FGameplayTag, FKzCapabilityGrant>& Pair : Resolved)
	{
		Tags.AddTag(Pair.Key);
	}

	return Tags;
}
