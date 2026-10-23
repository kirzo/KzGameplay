// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/KzDatabaseComponent.h"
#include "Items/KzItemDefinition.h"
#include "Items/KzItemFragment.h"
#include "KzItemFragment_Database.generated.h"

/**
 * The item's own tables, for anything it brings with it: the pose to hold it in, its sounds, its looks. A query about the item made by a user goes to
 * these and to the user's database component, with the item's ItemTags and the user's state as context; the user's rows win only when more specific.
 */
UCLASS(ClassGroup = "Data", DisplayName = "Database")
class KZGAMEPLAY_API UKzItemFragment_Database : public UKzItemFragment, public IKzDatabaseOwner
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database", meta = (ShowOnlyInnerProperties))
	FKzDatabaseSet Databases;

	//~ IKzDatabaseOwner
	virtual const FKzDatabaseSet& GetDatabaseSet() const override { return Databases; }

	/** The item's ItemTags. */
	virtual void AppendDatabaseTags(FGameplayTagContainer& OutTags) const override
	{
		if (const UKzItemDefinition* Item = GetTypedOuter<UKzItemDefinition>())
		{
			OutTags.AppendTags(Item->ItemTags);
		}
	}
	/** Resolves a query about the item made by a user; Default when the query asks nothing or neither answers. */
	template <typename T>
	static T ResolveFor(const UKzItemDefinition* Item, const AActor* User, const FKzDatabaseQuery& Query, const T& Default)
	{
		const UKzItemFragment_Database* Fragment = Item ? Item->FindFragmentByClass<UKzItemFragment_Database>() : nullptr;
		return UKzDatabaseComponent::ResolveFor<T>(Fragment ? &Fragment->Databases : nullptr, Item ? Item->ItemTags : FGameplayTagContainer(), User, Query, Default);
	}
};