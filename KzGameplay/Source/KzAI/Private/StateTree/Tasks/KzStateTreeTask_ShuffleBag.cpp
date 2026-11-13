// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_ShuffleBag.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FKzStateTreeTask_ShuffleBagDraw::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	FKzShuffleBag* Bag = InstanceData.Bag.GetMutablePtr(Context);
	InstanceData.Index = Bag ? Bag->Draw() : INDEX_NONE;
	if (InstanceData.Index == INDEX_NONE)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FKzVariant& Value = Bag->Entries[InstanceData.Index].Value;
	InstanceData.Tag = Value.Is<FGameplayTag>() ? Value.Get<FGameplayTag>() : FGameplayTag();
	return EStateTreeRunStatus::Succeeded;
}