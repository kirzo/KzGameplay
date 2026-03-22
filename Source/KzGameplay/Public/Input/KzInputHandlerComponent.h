// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Containers/KzPriorityStack.h"
#include "Input/KzInputModifierStack.h"
#include "KzInputHandlerComponent.generated.h"

class UKzInputProfile;
class UEnhancedInputComponent;
class APawn;
struct FInputActionValue;
enum class ETriggerEvent : uint8;

// Delegate for routing analog values
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKzInputAxisDelegate, FGameplayTag, InputTag, const FInputActionValue&, Value, ETriggerEvent, TriggerEvent);

/**
 * Component responsible for translating Enhanced Input Actions into Gameplay Tags
 * and injecting them into the Gameplay Ability System (GAS) or broadcasting them.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzInputHandlerComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	/** The default input profile to use. Can be overridden at runtime. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UKzInputProfile> DefaultInputProfile;

	/** Default modifiers applied automatically upon initialization. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TMap<FGameplayTag, FKzInputModifierContainer> DefaultModifiers;

public:
	/** Broadcasts whenever an Analog action is triggered, started, or completed. */
	UPROPERTY(BlueprintAssignable, Category = "Input")
	FKzInputAxisDelegate OnInputAxis;

private:
	/** The currently active input profile. */
	UPROPERTY(Transient)
	TObjectPtr<UKzInputProfile> ActiveInputProfile;

	/** Stores the handles of current bindings so we can cleanly remove them on profile swaps. */
	TArray<uint32> BindHandles;

	/** Map of ignore stacks, keyed by the specific Gameplay Tag of the input. */
	TMap<FGameplayTag, Kz::TPriorityStack<bool, false, FName, false>> IgnoreInputStacks;

	/** Map of modifier stacks, keyed by the specific Gameplay Tag of the input. */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FKzInputModifierStack> ModifierStacks;

public:
	UKzInputHandlerComponent();

	/** Manually re-initializes input with a new profile (e.g., when swapping control schemes). */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void InitializeInput(UKzInputProfile* OverrideProfile);

	/**
	 * Pushes a new input ignore state to the stack for a specific Gameplay Tag.
	 * @param InputTag The specific input to block (e.g., Input.Move)
	 * @param SourceID Unique identifier for the source applying the block
	 * @param bIgnoreInput True to block the input, false to explicitly allow it
	 * @param Priority Stack priority
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Control")
	void PushInputIgnore(FGameplayTag InputTag, FName SourceID, bool bIgnoreInput, int32 Priority);

	/** Removes a previously applied input ignore state for a specific Gameplay Tag. */
	UFUNCTION(BlueprintCallable, Category = "Input|Control")
	void RemoveInputIgnore(FGameplayTag InputTag, FName SourceID);

	/** Checks if a specific input tag is currently blocked by the priority stack. */
	UFUNCTION(BlueprintCallable, Category = "Input|Control")
	bool IsInputIgnored(FGameplayTag InputTag) const;

	/** Adds a new modifier instance to a specific input tag's stack. */
	UFUNCTION(BlueprintCallable, Category = "Input|Modifiers")
	void PushInputModifier(FGameplayTag InputTag, UKzInputModifier* Modifier);

	/** Removes a specific modifier instance from a specific input tag's stack. */
	UFUNCTION(BlueprintCallable, Category = "Input|Modifiers")
	void RemoveInputModifier(FGameplayTag InputTag, UKzInputModifier* Modifier);

	/** Processes a raw input vector through the specific tag's modifier stack. */
	UFUNCTION(BlueprintCallable, Category = "Input|Processing")
	FVector ProcessInput(FGameplayTag InputTag, const FVector& RawInput) const;

protected:
	virtual void BeginPlay() override;

private:
	/** Called by the Pawn when it has been locally restarted and the InputComponent is ready. */
	UFUNCTION()
	void OnPawnRestarted(APawn* Pawn);

	/** Internal helper to perform the actual binding. */
	void TryBindInput(APawn* Pawn, UKzInputProfile* ProfileToUse = nullptr);

	/** Internal callback for when an input action is pressed. */
	void Input_ActionPressed(FGameplayTag InputTag, FGameplayTag EventTag);

	/** Internal callback for when an input action is released. */
	void Input_ActionReleased(FGameplayTag InputTag, FGameplayTag EventTag);

	/** Internal execution of release, bypassing ignore checks. Used to force releases when input gets blocked. */
	void ExecuteActionReleased(FGameplayTag InputTag, FGameplayTag EventTag);

	/** Internal callback to handle analog values with custom payload. */
	void Input_Axis(const FInputActionValue& Value, FGameplayTag InputTag, ETriggerEvent TriggerEvent);
};