// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KzItemDefinition.generated.h"

class AActor;
class UTexture2D;
class UKzItemFragment;

/**
 * Defines the core, immutable data and rules for an item in the game.
 */
UCLASS(BlueprintType, Const)
class KZGAMEPLAY_API UKzItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	// ==========================================
	// VISUALS & UI
	// ==========================================

	/** The localized name of the item to display in UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FText DisplayName;

	/** The localized description of the item. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals", meta = (MultiLine = true))
	FText Description;

	/** The icon used in inventories or HUDs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	// ==========================================
	// STORAGE & RULES
	// ==========================================

	/** The physical actor class to spawn if this item is dropped from the inventory into the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftClassPtr<AActor> WorldActorClass;

	/**
	 * Gameplay Tags describing this item's properties or categories.
	 * e.g., "Item.Weapon.Axe", "Item.Throwable", "Item.Material.Wood".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FGameplayTagContainer ItemTags;

	// ==========================================
	// FRAGMENTS
	// ==========================================

	/** List of modular fragments defining specific behaviors for this item. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Item")
	TArray<TObjectPtr<UKzItemFragment>> Fragments;

	/** Searches fragments array and returns first encountered fragment of the specified class, native version of GetFragmentByClass */
	const UKzItemFragment* FindFragmentByClass(const TSubclassOf<UKzItemFragment> FragmentClass) const;

	/** Searches fragments array and returns first encountered fragment of the specified class */
	UFUNCTION(BlueprintCallable, Category = "Item", meta = (DeterminesOutputType = "FragmentClass"))
	const UKzItemFragment* GetFragmentByClass(TSubclassOf<UKzItemFragment> FragmentClass) const;

	/** Searches fragments array and returns first encountered fragment that implements the given interface. */
	const UKzItemFragment* FindFragmentByInterface(const TSubclassOf<UInterface> Interface) const;

	/** Gets all the fragments that implements the given interface. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	const UKzItemFragment* GetFragmentByInterface(TSubclassOf<UInterface> Interface) const;

	/** Returns true if the item definition contains a fragment of the specified class. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool HasFragment(TSubclassOf<UKzItemFragment> FragmentClass) const;

	/** Templatized version of FindFragmentByClass that handles casting for you */
	template<class T>
	const T* FindFragmentByClass() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UKzItemFragment>::Value, "'T' template parameter to FindFragmentByClass must be derived from UKzItemFragment");

		return (T*)FindFragmentByClass(T::StaticClass());
	}

	/** Templatized version of FindFragmentByInterface that handles casting for you */
	template<class T UE_REQUIRES(TIsIInterface<T>::Value)>
	const T* FindFragmentByInterface() const
	{
		return Cast<T>(FindFragmentByInterface(T::UClassType::StaticClass()));
	}

	/** Templatized version of HasFragment that handles casting for you */
	template <class T>
	bool HasFragment() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UKzItemFragment>::Value, "'T' template parameter to HasFragment must be derived from UKzItemFragment");

		return HasFragment(T::StaticClass());
	}
};