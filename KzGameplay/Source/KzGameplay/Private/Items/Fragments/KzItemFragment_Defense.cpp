// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_Defense.h"
#include "Combat/KzCombatTags.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"

void UKzItemFragment_Defense::GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const
{
	OutRequired.Add(UKzItemFragment_Equippable::StaticClass());
}

void UKzItemFragment_Defense::GetEquippedTags(FGameplayTagContainer& OutTags) const
{
	if (BlockAngle > 0.0f)
	{
		OutTags.AddTag(KzTags::Combat::Equipment_Defense_Block);
	}
	if (ParryAngle > 0.0f)
	{
		OutTags.AddTag(KzTags::Combat::Equipment_Defense_Parry);
	}
}