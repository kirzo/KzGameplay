// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_PushSteering.h"
#include "Steering/KzSteeringComponent.h"
#include "Steering/KzSteeringBehavior.h"
#include "Steering/KzSteeringProfile.h"
#include "StateTreeExecutionContext.h"

////////////////////////////////////////////////////////////////////
// PUSH BEHAVIOR TASK
////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FKzStateTreeTask_PushBehavior::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// Have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		if (InstanceData.SteeringComponent && InstanceData.LayerTag.IsValid() && InstanceData.Behaviors.Num() > 0)
		{
			// The first behavior creates the layer, the rest will append to it
			for (UKzSteeringBehavior* Behavior : InstanceData.Behaviors)
			{
				if (Behavior)
				{
					InstanceData.SteeringComponent->PushBehavior(Behavior, InstanceData.LayerTag, InstanceData.Priority);
				}
			}
		}
		else if (!InstanceData.SteeringComponent)
		{
			return EStateTreeRunStatus::Failed;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_PushBehavior::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// Are we transitioning to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		if (InstanceData.SteeringComponent)
		{
			InstanceData.SteeringComponent->RemoveLayer(InstanceData.LayerTag);
		}
	}
}

#if WITH_EDITOR
FText FKzStateTreeTask_PushBehavior::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	if (InstanceData && InstanceData->Behaviors.Num() > 0)
	{
		// If there is only one behavior, show its specific name
		if (InstanceData->Behaviors.Num() == 1 && InstanceData->Behaviors[0])
		{
			return FText::Format(FText::FromString("<b>Push Behavior:</b> {0}"), FText::FromString(InstanceData->Behaviors[0]->GetClass()->GetDisplayNameText().ToString()));
		}

		// If there are multiple, summarize them
		return FText::Format(FText::FromString("<b>Push {0} Behaviors</b>"), InstanceData->Behaviors.Num());
	}

	return FText::FromString("<b>Push Steering Behaviors</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////
// PUSH PROFILE TASK
////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FKzStateTreeTask_PushProfile::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// Have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		if (InstanceData.SteeringComponent && InstanceData.Profile && InstanceData.LayerTag.IsValid())
		{
			InstanceData.SteeringComponent->PushProfile(InstanceData.Profile, InstanceData.LayerTag, InstanceData.Priority);
		}
		else if (!InstanceData.SteeringComponent)
		{
			return EStateTreeRunStatus::Failed;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_PushProfile::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// Are we transitioning to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		if (InstanceData.SteeringComponent)
		{
			InstanceData.SteeringComponent->RemoveLayer(InstanceData.LayerTag);
		}
	}
}

#if WITH_EDITOR
FText FKzStateTreeTask_PushProfile::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	if (InstanceData && InstanceData->Profile)
	{
		return FText::Format(FText::FromString("<b>Push Profile:</b> {0}"), FText::FromString(InstanceData->Profile->GetName()));
	}

	return FText::FromString("<b>Push Steering Profile</b>");
}
#endif // WITH_EDITOR