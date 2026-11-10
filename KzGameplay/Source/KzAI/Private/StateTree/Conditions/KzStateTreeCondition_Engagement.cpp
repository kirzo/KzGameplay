// Copyright 2026 kirzo

#include "StateTree/Conditions/KzStateTreeCondition_Engagement.h"
#include "StateTreeExecutionContext.h"
#include "Engagement/KzEngagementComponent.h"

bool FKzStateTreeCondition_Engagement::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const UKzEngagementComponent* Engagement = InstanceData.Target ? InstanceData.Target->FindComponentByClass<UKzEngagementComponent>() : nullptr;
	if (!Engagement || !InstanceData.Actor)
	{
		return false ^ bInvert;
	}

	const bool bResult = Check == EKzEngagementCheck::InForbiddenArea ? Engagement->IsInForbiddenArea(InstanceData.Actor) : Engagement->IsOutOfPlace(InstanceData.Actor);
	return bResult ^ bInvert;
}