// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "KzStateTreeTask_Input.generated.h"

USTRUCT()
struct FKzStateTreeTask_PressInputInstanceData
{
	GENERATED_BODY()

	/** The pawn whose inputs it presses, through its input handler. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** Pressed in order: the first on entering, each next one as the pawn starts matching Next When. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "Input"))
	TArray<FGameplayTag> Inputs;

	/** What the pawn must match for the next press, such as State.ComboWindow to chain a combo. Empty presses them all at once. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTagQuery NextWhen;

	/**
	 * Starts the list again after its last input, on each new match of Next When. With Input.Attack and State.ComboWindow, it chains
	 * for as long as the combo goes on, without the tree knowing how long the combo is.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bLoop = false;

	/** Keeps the last input held until the state exits, as for sprinting. Off, every press is a tap. Not with Loop. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "!bLoop"))
	bool bHoldLast = false;

	int32 Pressed = 0;

	/** Whether the pawn matched Next When on the last tick, so each press waits for it to start matching again. */
	bool bMatched = false;
};

/**
 * Presses a pawn's inputs as a player would, so an AI uses the same abilities, combos and restrictions. It keeps running
 * until the state exits: leave it with a transition, such as when the tag of the ability it started goes away.
 */
USTRUCT(meta = (DisplayName = "Press Input", Category = "KzAI|Input"))
struct KZAI_API FKzStateTreeTask_PressInput : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeTask_PressInputInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

private:
	/** Presses the next input, holding it if it is the last and Hold Last is on. */
	static void PressNext(FInstanceDataType& InstanceData);
};
