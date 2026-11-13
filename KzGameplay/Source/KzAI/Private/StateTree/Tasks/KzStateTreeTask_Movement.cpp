// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_Movement.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

EStateTreeRunStatus FKzStateTreeTask_SetMovementMode::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		if (InstanceData.Character)
		{
			if (UCharacterMovementComponent* MoveComp = InstanceData.Character->GetCharacterMovement())
			{
				MoveComp->SetMovementMode(InstanceData.MovementMode);
			}
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FKzStateTreeTask_SetAvoidanceWeight::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (UCharacterMovementComponent* Movement = InstanceData.Character ? InstanceData.Character->GetCharacterMovement() : nullptr)
	{
		InstanceData.PreviousWeight = Movement->AvoidanceWeight;
		Movement->SetRVOAvoidanceWeight(InstanceData.Weight);
	}
	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_SetAvoidanceWeight::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (UCharacterMovementComponent* Movement = InstanceData.Character ? InstanceData.Character->GetCharacterMovement() : nullptr)
	{
		Movement->SetRVOAvoidanceWeight(InstanceData.PreviousWeight);
	}
}

#if WITH_EDITOR
FText FKzStateTreeTask_SetMovementMode::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	if (InstanceData)
	{
		UEnum* MovementModeEnum = StaticEnum<EMovementMode>();
		if (MovementModeEnum)
		{
			FString ModeName = MovementModeEnum->GetDisplayNameTextByValue(InstanceData->MovementMode.GetValue()).ToString();
			return FText::Format(FText::FromString("<b>Set Movement:</b> {0}"), FText::FromString(ModeName));
		}
	}
	return FText::FromString("<b>Set Movement Mode</b>");
}
#endif // WITH_EDITOR