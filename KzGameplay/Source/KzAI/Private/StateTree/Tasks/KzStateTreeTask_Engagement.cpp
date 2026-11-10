// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_Engagement.h"
#include "StateTreeExecutionContext.h"
#include "Engagement/KzEngagementComponent.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"

//----------------------------------------------------------------------//
//  FKzStateTreeTask_EngageTarget
//----------------------------------------------------------------------//

EStateTreeRunStatus FKzStateTreeTask_EngageTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return Tick(Context, 0.0f);
}

EStateTreeRunStatus FKzStateTreeTask_EngageTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Actor)
	{
		return EStateTreeRunStatus::Failed;
	}

	UKzEngagementComponent* Engagement = InstanceData.Target ? InstanceData.Target->FindComponentByClass<UKzEngagementComponent>() : nullptr;
	if (Engagement != InstanceData.Engagement.Get())
	{
		if (UKzEngagementComponent* Previous = InstanceData.Engagement.Get())
		{
			Previous->Disengage(InstanceData.Actor);
		}
		InstanceData.Engagement = Engagement;
	}

	if (Engagement)
	{
		Engagement->Engage(InstanceData.Actor, InstanceData.Weight, InstanceData.PreferredArea);
	}
	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_EngageTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (UKzEngagementComponent* Engagement = InstanceData.Engagement.Get())
	{
		Engagement->Disengage(InstanceData.Actor);
	}
	InstanceData.Engagement.Reset();
}

//----------------------------------------------------------------------//
//  FKzStateTreeTask_FindEngagementPoint
//----------------------------------------------------------------------//

namespace
{
	/** Onto the navmesh, where a Move To puts its goal: it compares both to tell whether the destination moved. */
	void ProjectToNavigation(const AActor& Actor, FVector& Location)
	{
		const APawn* Pawn = Cast<APawn>(&Actor);
		UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Actor.GetWorld());
		FNavLocation Projected;
		if (Pawn && Navigation && Navigation->ProjectPointToNavigation(Location, Projected, INVALID_NAVEXTENT, &Pawn->GetNavAgentPropertiesRef()))
		{
			Location = Projected.Location;
		}
		else
		{
			Location.Z = Actor.GetActorLocation().Z - Actor.GetSimpleCollisionHalfHeight();
		}
	}
}

EStateTreeRunStatus FKzStateTreeTask_FindEngagementPoint::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UKzEngagementComponent* Engagement = InstanceData.Target ? InstanceData.Target->FindComponentByClass<UKzEngagementComponent>() : nullptr;
	if (!InstanceData.Actor || !Engagement || !Engagement->FindPlace(InstanceData.Actor) || !Engagement->UpdatePlace(InstanceData.Actor, InstanceData.Location))
	{
		return EStateTreeRunStatus::Failed;
	}

	ProjectToNavigation(*InstanceData.Actor, InstanceData.Location);
	InstanceData.Engagement = Engagement;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FKzStateTreeTask_FindEngagementPoint::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UKzEngagementComponent* Engagement = InstanceData.Engagement.Get();
	if (!Engagement || !Engagement->UpdatePlace(InstanceData.Actor, InstanceData.Location))
	{
		return EStateTreeRunStatus::Failed;
	}

	ProjectToNavigation(*InstanceData.Actor, InstanceData.Location);
	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_FindEngagementPoint::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (UKzEngagementComponent* Engagement = InstanceData.Engagement.Get())
	{
		Engagement->ReleasePlace(InstanceData.Actor);
	}
	InstanceData.Engagement.Reset();
}