// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InputTriggers.h"
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

	/** The logical gameplay tag associated with this action (e.g., Input.Move). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "Input"))
	FGameplayTag InputTag;

	/** Bitmask to define which trigger states we want to listen to for this action. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Bitmask, BitmaskEnum = "/Script/EnhancedInput.ETriggerEvent"))
	int32 TriggerEvents = uint8(ETriggerEvent::Started) | uint8(ETriggerEvent::Completed);

	/** Gameplay Event sent to the ASC when this action Starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Events")
	FGameplayTag OnStartedEvent;

	/** Gameplay Event sent to the ASC when this action Completes or Cancels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Events")
	FGameplayTag OnCompletedEvent;
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

	/** Returns the full action configuration for a given Gameplay Tag. */
	const FKzInputAction* FindActionConfigForTag(const FGameplayTag& InputTag) const;

	/**
	 * Returns the first Input Action associated with a given Gameplay Tag.
	 * @param InputTag The tag to search for.
	 * @return The associated Input Action, or nullptr if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = false) const;
};