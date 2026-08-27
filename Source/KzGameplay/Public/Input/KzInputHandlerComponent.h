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
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (Categories = "Input"))
	TMap<FGameplayTag, FKzInputModifierContainer> DefaultModifiers;

public:
	/** Broadcasts whenever an Analog action is triggered, started, or completed. */
	UPROPERTY(BlueprintAssignable, Category = "Input")
	FKzInputAxisDelegate OnInputAxis;

private:
	/**
	 * One layer of input: a profile, whatever it bound, and whether its context went on.
	 * Kept so removing a layer takes away exactly what it brought and nothing else.
	 */
	struct FActiveProfile
	{
		TObjectPtr<UKzInputProfile> Profile;
		TArray<uint32> BindHandles;
		bool bAppliedContext = false;
	};

	/**
	 * Active profiles, base first. Anything the player picks up can add its own on top, so the character
	 * never has to declare inputs that belong to something it might be holding.
	 */
	TArray<FActiveProfile> ActiveProfiles;

	/** Always climbing, so a context added later outranks every one already on. */
	int32 NextContextPriority = 0;

	/** Map of ignore stacks, keyed by the specific Gameplay Tag of the input. */
	TMap<FGameplayTag, Kz::TPriorityStack<bool, false, FName, false>> IgnoreInputStacks;

	/** Map of modifier stacks, keyed by the specific Gameplay Tag of the input. */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FKzInputModifierStack> ModifierStacks;

	/** Last value seen per input, before and after the stack. */
	TMap<FGameplayTag, FVector> RawInputs;
	TMap<FGameplayTag, FVector> ProcessedInputs;

public:
	UKzInputHandlerComponent();

	/** Manually re-initializes input with a new profile (e.g., when swapping control schemes). */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void InitializeInput(UKzInputProfile* OverrideProfile);

	/**
	 * Adds a profile on top of the current ones: binds its actions and applies its keys.
	 * Made for things that grant controls while held, so an item ships its own inputs and takes them with
	 * it. A tag declared here shadows the same tag lower down, which is how an item retunes what exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void PushInputProfile(UKzInputProfile* Profile);

	/** Takes a profile back off, unbinding exactly what it brought. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RemoveInputProfile(UKzInputProfile* Profile);

	/** The action config for a tag, searched from the topmost profile down. */
	const struct FKzInputAction* FindActionConfig(const FGameplayTag& InputTag) const;

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

	/**
	 * Adds a modifier from its class, honouring its instancing policy: shared modifiers are pushed as they
	 * are, per-owner ones are copied first so their state belongs to this handler alone.
	 *  The instance that ended up on the stack, to hand back to RemoveInputModifier later.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Modifiers")
	UKzInputModifier* PushInputModifierOfClass(FGameplayTag InputTag, TSubclassOf<UKzInputModifier> ModifierClass);

	/** Removes a specific modifier instance from a specific input tag's stack. */
	UFUNCTION(BlueprintCallable, Category = "Input|Modifiers")
	void RemoveInputModifier(FGameplayTag InputTag, UKzInputModifier* Modifier);

	/** Processes a raw input vector through the specific tag's modifier stack. */
	UFUNCTION(BlueprintCallable, Category = "Input|Processing")
	FVector ProcessInput(FGameplayTag InputTag, const FVector& RawInput);

	/** What the player asked for, before any modifier touched it. */
	UFUNCTION(BlueprintPure, Category = "Input|Processing")
	FVector GetRawInput(FGameplayTag InputTag) const;

	/**
	 * What came out of the modifier stack, which is what actually drove the avatar.
	 * Kept so anything needing the player's intent can ask here instead of digging it out of the pawn.
	 */
	UFUNCTION(BlueprintPure, Category = "Input|Processing")
	FVector GetProcessedInput(FGameplayTag InputTag) const;

protected:
	virtual void BeginPlay() override;

private:
	/** Called by the Pawn when it has been locally restarted and the InputComponent is ready. */
	UFUNCTION()
	void OnPawnRestarted(APawn* Pawn);

	/** Internal helper to perform the actual binding. */
	void TryBindInput(APawn* Pawn, UKzInputProfile* ProfileToUse = nullptr);

	/** Binds one profile's actions and applies its context, recording both so they can be undone. */
	void BindProfileLayer(APawn* Pawn, UKzInputProfile* Profile);

	/** Undoes exactly what BindProfileLayer did for this layer. */
	void UnbindProfileLayer(APawn* Pawn, FActiveProfile& Layer);

	/** The subsystem that owns the key mappings, or null for anything without a local player. */
	class UEnhancedInputLocalPlayerSubsystem* GetLocalPlayerInput(const APawn* Pawn) const;

	/** Internal callback for when an input action is pressed. */
	void Input_ActionPressed(FGameplayTag InputTag, FGameplayTag EventTag);

	/** Internal callback for when an input action is released. */
	void Input_ActionReleased(FGameplayTag InputTag, FGameplayTag EventTag);

	/** Internal execution of release, bypassing ignore checks. Used to force releases when input gets blocked. */
	void ExecuteActionReleased(FGameplayTag InputTag, FGameplayTag EventTag);

	/** Internal callback to handle analog values with custom payload. */
	void Input_Axis(const FInputActionValue& Value, FGameplayTag InputTag, ETriggerEvent TriggerEvent);

	/** Returns the instance to push: the modifier itself when shared, a copy of it when per-owner. */
	UKzInputModifier* ResolveModifierInstance(UKzInputModifier* Modifier);
};