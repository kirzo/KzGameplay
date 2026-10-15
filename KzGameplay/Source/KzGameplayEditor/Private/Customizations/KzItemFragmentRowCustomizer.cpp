// Copyright 2026 kirzo

#include "Customizations/KzItemFragmentRowCustomizer.h"
#include "Items/KzItemDefinition.h"
#include "Items/KzItemFragment.h"
#include "PropertyHandle.h"

bool FKzItemFragmentRowCustomizer::CanRemoveRows(const TArray<TSharedPtr<IPropertyHandle>>& Handles, FText& OutReason) const
{
	const UKzItemDefinition* Item = nullptr;
	TSet<const UKzItemFragment*> Leaving;
	for (const TSharedPtr<IPropertyHandle>& Handle : Handles)
	{
		if (const UKzItemFragment* Fragment = ResolveObject<UKzItemFragment>(Handle))
		{
			Leaving.Add(Fragment);
		}

		if (!Item && Handle.IsValid())
		{
			TArray<UObject*> Outers;
			Handle->GetOuterObjects(Outers);
			Item = Outers.IsEmpty() ? nullptr : Cast<UKzItemDefinition>(Outers[0]);
		}
	}

	if (!Item || Leaving.IsEmpty())
	{
		return true;
	}

	for (const UKzItemFragment* Staying : Item->Fragments)
	{
		if (!Staying || Leaving.Contains(Staying))
		{
			continue;
		}

		TArray<TSubclassOf<UKzItemFragment>> Required;
		Staying->GetRequiredFragments(Required);
		for (const TSubclassOf<UKzItemFragment>& RequiredClass : Required)
		{
			if (!RequiredClass)
			{
				continue;
			}

			// Only a removal that takes away the last fragment meeting the requirement is refused; one already unmet is validation's to report.
			const UKzItemFragment* Leaver = nullptr;
			for (const UKzItemFragment* Fragment : Leaving)
			{
				if (Fragment->IsA(RequiredClass))
				{
					Leaver = Fragment;
					break;
				}
			}

			const bool bStillMet = Item->Fragments.ContainsByPredicate([&](const UKzItemFragment* Other) { return Other && !Leaving.Contains(Other) && Other->IsA(RequiredClass); });
			if (Leaver && !bStillMet)
			{
				OutReason = FText::Format(INVTEXT("{0} cannot be removed: {1} requires it."), Leaver->GetClass()->GetDisplayNameText(), Staying->GetClass()->GetDisplayNameText());
				return false;
			}
		}
	}

	return true;
}