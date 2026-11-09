// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_Focus.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"

EStateTreeRunStatus FKzStateTreeTask_SetFocus::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.AIController->SetFocus(InstanceData.Target, EAIFocusPriority::Gameplay);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FKzStateTreeTask_SetFocus::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// The bound target can change while the state runs, such as when the AI picks another one
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController && InstanceData.AIController->GetFocusActorForPriority(EAIFocusPriority::Gameplay) != InstanceData.Target)
	{
		InstanceData.AIController->SetFocus(InstanceData.Target, EAIFocusPriority::Gameplay);
	}

	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_SetFocus::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (Transition.ChangeType != EStateTreeStateChangeType::Changed)
	{
		return;
	}

	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController)
	{
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
