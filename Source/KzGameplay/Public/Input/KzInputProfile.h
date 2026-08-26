// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KzInputProfile.generated.h"

class UInputAction;
class UInputMappingContext;

/** Defines how the handler routes an input action's events. */
UENUM(BlueprintType)
enum class EKzInputRouting : uint8
{
	/** Digital: Started/Completed drive GAS input tags, plus optional gameplay events. */
	Ability,
	/** Analog: value runs through the modifier stack and broadcasts on OnInputAxis across its lifecycle (Started/Triggered/Completed/Canceled). */
	Analog
};

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

	/** How the handler routes this action: as a digital ability input, or as an analog value. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	EKzInputRouting Routing = EKzInputRouting::Ability;

	/** Gameplay Event sent to the ASC when this action Starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Events", meta = (EditCondition = "Routing == EKzInputRouting::Ability"))
	FGameplayTag OnStartedEvent;

	/** Gameplay Event sent to the ASC when this action Completes or Cancels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Events", meta = (EditCondition = "Routing == EKzInputRouting::Ability"))
	FGameplayTag OnCompletedEvent;
};

/**
 * Everything about the inputs one thing has: which keys, which tags, how each is routed and what it
 * announces. A character has one; so does anything that hands the player controls it did not have before,
 * like a nozzle, and pushing that profile brings its keys along instead of asking a second asset to agree.
 */
UCLASS(BlueprintType, Const)
class KZGAMEPLAY_API UKzInputProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Keys for these actions, applied while the profile is active. Without one the actions are declared
	 * but nothing can trigger them, which is a valid way to describe inputs another profile provides keys for.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	/** Enhanced Input priority for the context above. Higher wins when two contexts map the same key. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 ContextPriority = 0;

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