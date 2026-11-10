// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_FindTarget.h"
#include "StateTreeExecutionContext.h"
#include "Sensors/KzSensorComponent.h"
#include "Factions/KzFactionLibrary.h"

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
			return EStateTreeRunStatus::Running;
		}

		const double DistSq = FVector::DistSquared(Location, Candidate->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			Nearest = Candidate;
			NearestDistSq = DistSq;
		}
	}

	InstanceData.Target = Nearest;
	return EStateTreeRunStatus::Running;
}
