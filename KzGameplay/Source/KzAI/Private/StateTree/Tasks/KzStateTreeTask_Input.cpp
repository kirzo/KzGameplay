// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_Input.h"
#include "StateTreeExecutionContext.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Input/KzInputHandlerComponent.h"

namespace
{
	UKzInputHandlerComponent* GetInputHandler(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	}
}

EStateTreeRunStatus FKzStateTreeTask_PressInput::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (Transition.ChangeType != EStateTreeStateChangeType::Changed)
	{
		return EStateTreeRunStatus::Running;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Pressed = 0;
	InstanceData.bMatched = false;

	if (!GetInputHandler(InstanceData.Actor))
	{
		return EStateTreeRunStatus::Failed;
	}

	PressNext(InstanceData);
	while (InstanceData.NextWhen.IsEmpty() && InstanceData.Pressed < InstanceData.Inputs.Num())
	{
		PressNext(InstanceData);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FKzStateTreeTask_PressInput::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.bLoop && InstanceData.Pressed >= InstanceData.Inputs.Num())
	{
		return EStateTreeRunStatus::Running;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstanceData.Actor);
	const bool bMatches = ASC && InstanceData.NextWhen.Matches(ASC->GetOwnedGameplayTags());
	if (bMatches && !InstanceData.bMatched)
	{
		PressNext(InstanceData);
	}
	InstanceData.bMatched = bMatches;

	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_PressInput::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (Transition.ChangeType != EStateTreeStateChangeType::Changed)
	{
		return;
	}

	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bHolding = InstanceData.bHoldLast && !InstanceData.bLoop && InstanceData.Pressed > 0 && InstanceData.Pressed == InstanceData.Inputs.Num();
	if (UKzInputHandlerComponent* InputHandler = bHolding ? GetInputHandler(InstanceData.Actor) : nullptr)
	{
		InputHandler->ReleaseInput(InstanceData.Inputs.Last());
	}
}

void FKzStateTreeTask_PressInput::PressNext(FInstanceDataType& InstanceData)
{
	UKzInputHandlerComponent* InputHandler = GetInputHandler(InstanceData.Actor);
	const int32 Num = InstanceData.Inputs.Num();
	if (!InputHandler || Num == 0 || (!InstanceData.bLoop && InstanceData.Pressed >= Num))
	{
		return;
	}

	const FGameplayTag Input = InstanceData.Inputs[InstanceData.Pressed++ % Num];
	InputHandler->PressInput(Input);

	const bool bHold = InstanceData.bHoldLast && !InstanceData.bLoop && InstanceData.Pressed == Num;
	if (!bHold)
	{
		InputHandler->ReleaseInput(Input);
	}
}

#if WITH_EDITOR
FText FKzStateTreeTask_PressInput::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	if (!InstanceData || InstanceData->Inputs.IsEmpty())
	{
		return FText::FromString("<b>Press Input</b>");
	}

	TArray<FString> Names;
	for (const FGameplayTag& Input : InstanceData->Inputs)
	{
		Names.Add(Input.ToString());
	}
	return FText::Format(FText::FromString("<b>{0}:</b> {1}"), FText::FromString(InstanceData->bLoop ? "Press (loop)" : InstanceData->bHoldLast ? "Hold" : "Press"), FText::FromString(FString::Join(Names, TEXT(" -> "))));
}
#endif // WITH_EDITOR
