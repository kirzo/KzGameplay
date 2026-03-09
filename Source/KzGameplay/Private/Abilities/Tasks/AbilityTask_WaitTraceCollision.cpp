// Copyright 2026 kirzo

#include "Abilities/Tasks/AbilityTask_WaitTraceCollision.h"
#include "Engine/World.h"
#include "KismetTraceUtils.h"

UE_DISABLE_OPTIMIZATION

UAbilityTask_WaitTraceCollision::UAbilityTask_WaitTraceCollision(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
	bDebugTrace = false;
}

UAbilityTask_WaitTraceCollision* UAbilityTask_WaitTraceCollision::WaitTraceCollision(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	FKzShapeInstance InCollisionShape,
	FKzTransformSource InTransformSource,
	const TArray<TEnumAsByte<EObjectTypeQuery>>& InObjectTypes,
	bool bInDebugTrace)
{
	UAbilityTask_WaitTraceCollision* MyObj = NewAbilityTask<UAbilityTask_WaitTraceCollision>(OwningAbility, TaskInstanceName);

	MyObj->TransformSource = InTransformSource;
	MyObj->TraceShape = InCollisionShape;
	MyObj->ObjectTypes = InObjectTypes;
	MyObj->bDebugTrace = bInDebugTrace;

	return MyObj;
}

void UAbilityTask_WaitTraceCollision::AddIgnoredActor(AActor* ActorToIgnore)
{
	if (ActorToIgnore)
	{
		IgnoredActors.AddUnique(ActorToIgnore);
	}
}

void UAbilityTask_WaitTraceCollision::Activate()
{
	Super::Activate();

	if (!TransformSource.IsValid() || !TraceShape.IsValid())
	{
		EndTask();
		return;
	}

	// Always ignore the Avatar running the ability
	if (AActor* AvatarActor = GetAvatarActor())
	{
		AddIgnoredActor(AvatarActor);
	}

	// If the transform source is attached to a specific actor (like a weapon), ignore that actor too
	if (const AActor* SourceActor = TransformSource.GetActor())
	{
		AddIgnoredActor(const_cast<AActor*>(SourceActor));
	}

	// Cache the initial starting transform
	PreviousTransform = TransformSource.GetTransform();
}

void UAbilityTask_WaitTraceCollision::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!TransformSource.IsValid() || !TraceShape.IsValid())
	{
		return;
	}

	FTransform CurrentTransform = TransformSource.GetTransform();

	// Avoid tracing if the source hasn't moved physically
	if (CurrentTransform.Equals(PreviousTransform, 0.1f))
	{
		return;
	}

	// Convert Blueprint Object Types to Native Collision Params
	FCollisionObjectQueryParams ObjectQueryParams;
	for (TEnumAsByte<EObjectTypeQuery> ObjType : ObjectTypes)
	{
		ObjectQueryParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjType));
	}

	// Rebuild Native Ignore List
	FCollisionQueryParams Params(TEXT("AbilityTraceCollisionSweep"), true);
	for (TWeakObjectPtr<AActor> IgnoredActor : IgnoredActors)
	{
		if (IgnoredActor.IsValid())
		{
			Params.AddIgnoredActor(IgnoredActor.Get());
		}
	}

	const UWorld* World = GetWorld();
	FHitResult HitResult;

	FCollisionShape NativeShape = TraceShape.ToCollisionShape();

	// Perform the sweep from the previous frame to the current frame
	bool bHit = World->SweepSingleByObjectType(
		HitResult,
		PreviousTransform.GetLocation(),
		CurrentTransform.GetLocation(),
		CurrentTransform.GetRotation(),
		ObjectQueryParams,
		NativeShape,
		Params
	);

#if ENABLE_DRAW_DEBUG
	EDrawDebugTrace::Type DrawType = bDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	if (NativeShape.IsCapsule())
	{
		DrawDebugCapsuleTraceSingle(World, PreviousTransform.GetLocation(), CurrentTransform.GetLocation(), NativeShape.GetCapsuleRadius(), NativeShape.GetCapsuleHalfHeight(), CurrentTransform.Rotator(), DrawType, bHit, HitResult, FLinearColor::Red, FLinearColor::Green, 2.0f);
	}
	else if (NativeShape.IsBox())
	{
		DrawDebugBoxTraceSingle(World, PreviousTransform.GetLocation(), CurrentTransform.GetLocation(), NativeShape.GetBox(), CurrentTransform.Rotator(), DrawType, bHit, HitResult, FLinearColor::Red, FLinearColor::Green, 2.0f);
	}
	else if (NativeShape.IsSphere())
	{
		DrawDebugSphereTraceSingle(World, PreviousTransform.GetLocation(), CurrentTransform.GetLocation(), NativeShape.GetSphereRadius(), DrawType, bHit, HitResult, FLinearColor::Red, FLinearColor::Green, 2.0f);
	}
#endif

	if (bHit && HitResult.GetActor())
	{
		// 1. Add to ignore list immediately so we don't hit the same target next tick
		AddIgnoredActor(HitResult.GetActor());

		// 2. Broadcast the hit to the Gameplay Ability graph
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnHitDetected.Broadcast(HitResult);
		}
	}

	// Cache current transform for the next tick
	PreviousTransform = CurrentTransform;
}

void UAbilityTask_WaitTraceCollision::OnDestroy(bool bInOwnerFinished)
{
	IgnoredActors.Empty();
	Super::OnDestroy(bInOwnerFinished);
}

UE_ENABLE_OPTIMIZATION