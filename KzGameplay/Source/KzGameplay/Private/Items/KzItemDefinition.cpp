// Copyright 2026 kirzo

#include "Items/KzItemDefinition.h"
#include "Items/KzItemFragment.h"
#include "UObject/Interface.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "UObject/UnrealType.h"
#endif

#if WITH_EDITOR
EDataValidationResult UKzItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TSet<UClass*> EncounteredClasses;
	bool bHasDuplicates = false;

	for (const UKzItemFragment* Fragment : Fragments)
	{
		if (Fragment)
		{
			UClass* FragmentClass = Fragment->GetClass();

			// Check if we already have a fragment of this exact class
			if (EncounteredClasses.Contains(FragmentClass))
			{
				bHasDuplicates = true;
				Context.AddError(FText::Format(INVTEXT("Duplicate fragment type found: {0}. An item can only have one fragment of each type."), FText::FromString(FragmentClass->GetName())));
			}
			else
			{
				EncounteredClasses.Add(FragmentClass);
			}
		}
	}

	if (bHasDuplicates)
	{
		Result = EDataValidationResult::Invalid;
	}

	for (const UKzItemFragment* Fragment : Fragments)
	{
		if (!Fragment)
		{
			continue;
		}

		TArray<TSubclassOf<UKzItemFragment>> Required;
		Fragment->GetRequiredFragments(Required);
		for (const TSubclassOf<UKzItemFragment>& RequiredClass : Required)
		{
			if (RequiredClass && !HasFragment(RequiredClass))
			{
				Context.AddError(FText::Format(INVTEXT("{0} requires a {1} fragment on the same item."), Fragment->GetClass()->GetDisplayNameText(), RequiredClass->GetDisplayNameText()));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}

void UKzItemDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UKzItemDefinition, Fragments))
	{
		AddRequiredFragments();
	}
}

void UKzItemDefinition::AddRequiredFragments()
{
	// Fragments added here are visited too, so requirements of requirements come along.
	for (int32 Index = 0; Index < Fragments.Num(); ++Index)
	{
		if (!Fragments[Index])
		{
			continue;
		}

		TArray<TSubclassOf<UKzItemFragment>> Required;
		Fragments[Index]->GetRequiredFragments(Required);
		for (const TSubclassOf<UKzItemFragment>& RequiredClass : Required)
		{
			if (RequiredClass && !RequiredClass->HasAnyClassFlags(CLASS_Abstract) && !HasFragment(RequiredClass))
			{
				Modify();
				Fragments.Add(NewObject<UKzItemFragment>(this, RequiredClass, NAME_None, RF_Transactional));
			}
		}
	}
}
#endif

const UKzItemFragment* UKzItemDefinition::FindFragmentByClass(const TSubclassOf<UKzItemFragment> FragmentClass) const
{
	UKzItemFragment* FoundFragment = nullptr;

	if (UClass* TargetClass = FragmentClass.Get())
	{
		for (UKzItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(TargetClass))
			{
				FoundFragment = Fragment;
				break;
			}
		}
	}

	return FoundFragment;
}

const UKzItemFragment* UKzItemDefinition::GetFragmentByClass(TSubclassOf<UKzItemFragment> FragmentClass) const
{
	return FindFragmentByClass(FragmentClass);
}

const UKzItemFragment* UKzItemDefinition::FindFragmentByInterface(const TSubclassOf<UInterface> Interface) const
{
	UKzItemFragment* FoundFragment = nullptr;

	if (Interface)
	{
		for (UKzItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->GetClass()->ImplementsInterface(Interface))
			{
				FoundFragment = Fragment;
				break;
			}
		}
	}

	return FoundFragment;
}

const UKzItemFragment* UKzItemDefinition::GetFragmentByInterface(TSubclassOf<UInterface> Interface) const
{
	return FindFragmentByInterface(Interface);
}

bool UKzItemDefinition::HasFragment(TSubclassOf<UKzItemFragment> FragmentClass) const
{
	return FindFragmentByClass(FragmentClass) != nullptr;
}