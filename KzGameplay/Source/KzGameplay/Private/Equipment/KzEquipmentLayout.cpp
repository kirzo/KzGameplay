// Copyright 2026 kirzo

#include "Equipment/KzEquipmentLayout.h"

#if WITH_EDITOR
#include "Misc/MessageDialog.h"
#endif

FGameplayTag UKzEquipmentLayout::ResolveSlotID(const FGameplayTag& InSlotID) const
{
	// 1. Check local aliases first (allows a child to change MainHand from Right to Left)
	if (const FGameplayTag* TargetTag = SlotAliases.Find(InSlotID))
	{
		return *TargetTag;
	}

	// 2. If not found locally, check if the parent layout knows this alias
	if (ParentLayout && ParentLayout != this)
	{
		return ParentLayout->ResolveSlotID(InSlotID);
	}

	// 3. If it's not an alias anywhere, assume it's already a physical Slot ID
	return InSlotID;
}

bool UKzEquipmentLayout::FindSlotDefinition(const FGameplayTag& InSlotID, FKzEquipmentSlotDefinition& OutDefinition) const
{
	// First, resolve the alias (MainHand -> RightHand)
	const FGameplayTag ResolvedID = ResolveSlotID(InSlotID);

	// 1. Check local slots first (this allows child layouts to override parent socket names or display names)
	for (const FKzEquipmentSlotDefinition& SlotDef : Slots)
	{
		if (SlotDef.SlotID == ResolvedID)
		{
			OutDefinition = SlotDef;
			return true;
		}
	}

	// 2. If not found locally, delegate to the parent layout
	if (ParentLayout && ParentLayout != this)
	{
		return ParentLayout->FindSlotDefinition(ResolvedID, OutDefinition);
	}

	return false;
}

FName UKzEquipmentLayout::GetSocketForSlot(const FGameplayTag& InSlotID) const
{
	FKzEquipmentSlotDefinition FoundDef;
	if (FindSlotDefinition(InSlotID, FoundDef))
	{
		return FoundDef.DefaultSocketName;
	}

	return NAME_None;
}

void UKzEquipmentLayout::GetAllSlotDefinitions(TArray<FKzEquipmentSlotDefinition>& OutAllSlots) const
{
	// Start with parent slots if available
	if (ParentLayout && ParentLayout != this)
	{
		ParentLayout->GetAllSlotDefinitions(OutAllSlots);
	}

	// Add or override with local slots
	for (const FKzEquipmentSlotDefinition& LocalSlot : Slots)
	{
		// Try to find if this slot already exists from the parent
		int32 ExistingIndex = OutAllSlots.IndexOfByPredicate([&LocalSlot](const FKzEquipmentSlotDefinition& S)
			{
				return S.SlotID == LocalSlot.SlotID;
			});

		if (ExistingIndex != INDEX_NONE)
		{
			// Override parent's definition
			OutAllSlots[ExistingIndex] = LocalSlot;
		}
		else
		{
			// Add as a new slot
			OutAllSlots.Add(LocalSlot);
		}
	}
}

#if WITH_EDITOR
void UKzEquipmentLayout::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// Validate changes to the Parent Layout
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UKzEquipmentLayout, ParentLayout))
	{
		if (ParentLayout)
		{
			// 1. Prevent Self-Reference
			if (ParentLayout == this)
			{
				ParentLayout = nullptr;
				FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("An Equipment Layout cannot be its own parent."));
				return;
			}

			// 2. Prevent Circular Dependencies (A -> B -> C -> A)
			if (HasCircularDependency(ParentLayout))
			{
				ParentLayout = nullptr;
				FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Circular dependency detected! This layout is already a parent somewhere in that hierarchy."));
				return;
			}
		}
	}
}

bool UKzEquipmentLayout::HasCircularDependency(const UKzEquipmentLayout* PotentialParent) const
{
	const UKzEquipmentLayout* CurrentNode = PotentialParent;

	// Walk up the chain to see if we eventually hit 'this'
	while (CurrentNode)
	{
		if (CurrentNode == this)
		{
			return true;
		}
		CurrentNode = CurrentNode->ParentLayout;
	}

	return false;
}
#endif