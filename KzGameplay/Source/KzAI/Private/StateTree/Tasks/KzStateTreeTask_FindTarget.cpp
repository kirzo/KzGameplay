// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_FindTarget.h"
#include "StateTreeExecutionContext.h"
#include "Sensors/KzSensorComponent.h"
#include "Factions/KzFactionLibrary.h"
#include "KzAITags.h"

namespace
{
	/** The current target while the sensor still has it with the attitude, else the nearest that has it. */
	AActor* PickTarget(const FKzStateTreeTask_FindTargetInstanceData& InstanceData)
	{
		const FVector Location = InstanceData.Actor->GetActorLocation();
		AActor* Nearest = nullptr;
		double NearestDistSq = TNumericLimits<double>::Max();

		for (const FKzOverlapResult& Overlap : InstanceData.Sensor->GetOverlapInfos())
		{
			AActor* Candidate = Overlap.Actor.Get();
			if (!Candidate || UKzFactionLibrary::GetAttitude(InstanceData.Actor, Candidate) != InstanceData.Attitude)
			{
				continue;
			}

			if (Candidate == InstanceData.Target)
			{
				return Candidate;
			}

			const double DistSq = FVector::DistSquared(Location, Candidate->GetActorLocation());
			if (DistSq < NearestDistSq)
			{
				Nearest = Candidate;
				NearestDistSq = DistSq;
			}
		}
		return Nearest;
	}
}

EStateTreeRunStatus FKzStateTreeTask_FindTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return Tick(Context, 0.0f);
}

EStateTreeRunStatus FKzStateTreeTask_FindTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Actor || !InstanceData.Sensor)
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = PickTarget(InstanceData);
	if (Target != InstanceData.Target)
	{
		InstanceData.Target = Target;
		Context.SendEvent(KzTags::AI::Target_Changed);
	}
	return EStateTreeRunStatus::Running;
}
