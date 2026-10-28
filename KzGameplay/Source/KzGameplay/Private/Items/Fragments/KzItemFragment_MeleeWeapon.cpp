// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_MeleeWeapon.h"
#include "Combat/KzComboGraph.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"

void UKzItemFragment_MeleeWeapon::GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const
{
	OutRequired.Add(UKzItemFragment_Equippable::StaticClass());
}

void UKzItemFragment_MeleeWeapon::GetPreloadPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if (!Combo.IsNull())
	{
		OutPaths.Add(Combo.ToSoftObjectPath());
	}
}