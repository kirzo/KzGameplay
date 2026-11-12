// Copyright 2026 kirzo

#include "Validation/KzItemDefinitionValidators.h"
#include "Items/KzItemDefinition.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Equipment/KzEquipmentLayout.h"
#include "Capabilities/KzCapabilitySet.h"
#include "AssetRegistry/IAssetRegistry.h"

#define LOCTEXT_NAMESPACE "KzItemDefinitionValidators"

namespace
{
	/** Whether any asset of the class in the project passes the predicate. Loads them. */
	template <typename T>
	bool AnyAssetInProject(TFunctionRef<bool(const T&)> Predicate)
	{
		TArray<FAssetData> Assets;
		IAssetRegistry::GetChecked().GetAssetsByClass(T::StaticClass()->GetClassPathName(), Assets, true);
		return Assets.ContainsByPredicate([&Predicate](const FAssetData& Data)
			{
				const T* Loaded = Cast<T>(Data.GetAsset());
				return Loaded && Predicate(*Loaded);
			});
	}
}

bool UKzItemDefinitionValidator::CanValidate_Implementation(const UObject* Asset) const
{
	return Asset && Asset->IsA<UKzItemDefinition>();
}

void UKzItemDefinitionValidator::Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const
{
	const UKzItemDefinition* Item = Cast<UKzItemDefinition>(Asset);
	if (!Item) { return; }

	const FName Id = GetValidatorId();

	for (int32 i = 0; i < Item->Fragments.Num(); ++i)
	{
		const UKzItemFragment_Equippable* Equippable = Cast<UKzItemFragment_Equippable>(Item->Fragments[i]);
		if (!Equippable) { continue; }

		if (!Equippable->TargetSlot.IsValid())
		{
			OutIssues.Add(FKzValidationIssue::WithContextIndex(EKzValidationSeverity::Error, LOCTEXT("NoTargetSlot", "Equippable has no Target Slot."), Id, i));
		}

		for (const FGameplayTag& Slot : { Equippable->TargetSlot, Equippable->StowSlot })
		{
			const bool bDefined = !Slot.IsValid() || AnyAssetInProject<UKzEquipmentLayout>([&Slot](const UKzEquipmentLayout& Layout)
				{
					FKzEquipmentSlotDefinition Definition;
					return Layout.FindSlotDefinition(Slot, Definition);
				});
			if (!bDefined)
			{
				OutIssues.Add(FKzValidationIssue::WithContextIndex(
					EKzValidationSeverity::Warning,
					FText::Format(LOCTEXT("SlotUndefined", "No Equipment Layout defines {0}."), FText::FromString(Slot.ToString())),
					Id, i));
			}
		}

		for (const FGameplayTag& Capability : Equippable->GrantedCapabilities)
		{
			const bool bDefined = AnyAssetInProject<UKzCapabilitySet>([&Capability](const UKzCapabilitySet& Set) { return Set.DefinesCapability(Capability); });
			if (!bDefined)
			{
				OutIssues.Add(FKzValidationIssue::WithContextIndex(
					EKzValidationSeverity::Warning,
					FText::Format(LOCTEXT("CapabilityUndefined", "No Capability Set defines {0}: equipping this grants nothing for it."), FText::FromString(Capability.ToString())),
					Id, i));
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE