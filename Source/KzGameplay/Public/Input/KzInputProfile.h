// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KzInputProfile.generated.h"

class UInputAction;

/**
 * Struct used to map an Enhanced Input Action to a Gameplay Tag.
 */
USTRUCT(BlueprintType)
struct FKzInputAction
{
	GENERATED_BODY()

public:
	/** The physical input action (e.g., Left Click, Spacebar, 'E' key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<const UInputAction> InputAction = nullptr;

	/** The logical gameplay tag associated with this action (e.g., Input.Action.Interact). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * Data Asset that contains a collection of Input Actions mapped to Gameplay Tags.
 * Swap these profiles at runtime to completely change the control scheme.
 */
UCLASS(BlueprintType, Const)
class KZGAMEPLAY_API UKzInputProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** List of input actions used by this profile and their corresponding tags. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (TitleProperty = "InputTag"))
	TArray<FKzInputAction> InputActions;

	/**
	 * Returns the first Input Action associated with a given Gameplay Tag.
	 * @param InputTag The tag to search for.
	 * @return The associated Input Action, or nullptr if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = false) const;
};