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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UKzInputProfile> DefaultInputProfile;

private:
	/** Stores the handles of current bindings so we can cleanly remove them on profile swaps. */
	TArray<uint32> BindHandles;

	/** Stacks to handle conflicting input block requests (e.g., UI open vs Interaction playing). */
	Kz::TPriorityStack<bool, false, FName, false> IgnoreMoveInputStack;
	Kz::TPriorityStack<bool, false, FName, false> IgnoreLookInputStack;

	/** Stacks of active input modifiers (e.g., for recoil, slowdowns, forced movement). */
	UPROPERTY(Transient)
	FKzInputModifierStack MoveModifierStack;

	UPROPERTY(Transient)
	FKzInputModifierStack LookModifierStack;

public:
	UKzInputHandlerComponent();

	/** Manually re-initializes input with a new profile (e.g., when swapping control schemes). */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void InitializeInput(UKzInputProfile* OverrideProfile);

	/** Pushes a new move input ignore state to the stack. */
	UFUNCTION(BlueprintCallable, Category = "Input|Control")
	void PushMoveInputIgnore(FName SourceID, bool bIgnoreMoveInput, int32 Priority);

	/** Removes a previously applied move input ignore state. */
	UFUNCTION(BlueprintCallable, Category = "Input|Control")
	void RemoveMoveInputIgnore(FName SourceID);

	/** Pushes a new look input ignore state to the stack. */
	UFUNCTION(BlueprintCallable, Category = "Input|Control")
	void PushLookInputIgnore(FName SourceID, bool bIgnoreLookInput, int32 Priority);

	/** Removes a previously applied look input ignore state. */
	UFUNCTION(BlueprintCallable, Category = "Input|Control")
	void RemoveLookInputIgnore(FName SourceID);

	/** Adds a new modifier instance to the movement stack */
	UFUNCTION(BlueprintCallable, Category = "Input|Modifiers")
	void PushMoveModifier(UKzInputModifier* Modifier);

	/** Removes a specific modifier instance from the movement stack */
	UFUNCTION(BlueprintCallable, Category = "Input|Modifiers")
	void RemoveMoveModifier(UKzInputModifier* Modifier);

	/** Processes a raw movement vector through the stack */
	UFUNCTION(BlueprintCallable, Category = "Input|Processing")
	FVector ProcessMoveInput(const FVector& RawInput) const;

	/** Adds a new modifier instance to the look stack */
	UFUNCTION(BlueprintCallable, Category = "Input|Modifiers")
	void PushLookModifier(UKzInputModifier* Modifier);

	/** Removes a specific modifier instance from the look stack */
	UFUNCTION(BlueprintCallable, Category = "Input|Modifiers")
	void RemoveLookModifier(UKzInputModifier* Modifier);

	/** Processes a raw look vector through the stack */
	UFUNCTION(BlueprintCallable, Category = "Input|Processing")
	FVector ProcessLookInput(const FVector& RawInput) const;

protected:
	virtual void BeginPlay() override;

private:
	/** Called by the Pawn when it has been locally restarted and the InputComponent is ready. */
	UFUNCTION()
	void OnPawnRestarted(APawn* Pawn);

	/** Internal helper to perform the actual binding. */
	void TryBindInput(APawn* Pawn, UKzInputProfile* ProfileToUse = nullptr);

	/** Internal callback for when an input action is pressed. */
	void Input_ActionPressed(FGameplayTag InputTag);

	/** Internal callback for when an input action is released. */
	void Input_ActionReleased(FGameplayTag InputTag);

	/** Internal functions to apply the top of the stack to the Controller. */
	void UpdateMoveInputIgnore();
	void UpdateLookInputIgnore();
};