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

	bool bResult = false;
	switch (Check)
	{
	case EKzEngagementCheck::InForbiddenArea:
		bResult = Engagement->IsInForbiddenArea(InstanceData.Actor);
		break;
	case EKzEngagementCheck::OutOfPlace:
		bResult = Engagement->IsOutOfPlace(InstanceData.Actor);
		break;
	case EKzEngagementCheck::NoPlace:
		bResult = !Engagement->HasPlace(InstanceData.Actor);
		break;
	case EKzEngagementCheck::WrongPlace:
		bResult = Engagement->IsInWrongPlace(InstanceData.Actor);
		break;
	}
	return bResult ^ bInvert;
}