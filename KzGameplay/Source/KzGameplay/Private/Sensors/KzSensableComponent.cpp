// Copyright 2026 kirzo

#include "Sensors/KzSensableComponent.h"
#include "Sensors/KzSpatialSenseSubsystem.h"
#include "Sensors/KzSensorComponent.h"
#include "Components/KzShapeComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UKzSensableComponent::UKzSensableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKzSensableComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheShapeComponent();

	if (UKzSpatialSenseSubsystem* Subsystem = GetWorld()->GetSubsystem<UKzSpatialSenseSubsystem>())
	{
		Subsystem->RegisterSensable(this);
	}
}

void UKzSensableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UKzSpatialSenseSubsystem* Subsystem = World->GetSubsystem<UKzSpatialSenseSubsystem>())
		{
			Subsystem->UnregisterSensable(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UKzSensableComponent::CacheShapeComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 1. Prioritize the Root Component if it's a Primitive (KzShapeComponent inherits from Primitive)
	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
	{
		CachedPrimitive = RootPrim;
		return;
	}

	// 2. Fallback to a KzShapeComponent explicitly, but NEVER pick a Sensor
	TArray<UKzShapeComponent*> ShapeComponents;
	Owner->GetComponents<UKzShapeComponent>(ShapeComponents);

	for (UKzShapeComponent* Shape : ShapeComponents)
	{
		if (!Shape->IsA(UKzSensorComponent::StaticClass()))
		{
			CachedPrimitive = Shape;
			return;
		}
	}

	// 3. Fallback to the first Primitive Component, again ignoring Sensors
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* Prim : PrimitiveComponents)
	{
		if (!Prim->IsA(UKzSensorComponent::StaticClass()))
		{
			CachedPrimitive = Prim;
			return;
		}
	}
}

FKzShapeInstance UKzSensableComponent::GetShapeInstance() const
{
	if (UPrimitiveComponent* PrimComp = CachedPrimitive.Get())
	{
		// Is it already our native Shape?
		if (UKzShapeComponent* KzShapeComp = Cast<UKzShapeComponent>(PrimComp))
		{
			return KzShapeComp->Shape * KzShapeComp->GetComponentScale();
		}

		return FKzShapeInstance::MakeFromCollisionShape(PrimComp->GetCollisionShape());
	}
	return FallbackShape;
}

FBox UKzSensableComponent::GetBounds() const
{
	if (UPrimitiveComponent* PrimComp = CachedPrimitive.Get()) return PrimComp->Bounds.GetBox();

	AActor* Owner = GetOwner();
	return Owner ? FallbackShape.GetBoundingBox(Owner->GetActorLocation(), Owner->GetActorQuat()) : FBox();
}

FVector UKzSensableComponent::GetShapeLocation() const
{
	if (UPrimitiveComponent* PrimComp = CachedPrimitive.Get()) return PrimComp->GetComponentLocation();
	return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

FQuat UKzSensableComponent::GetShapeRotation() const
{
	if (UPrimitiveComponent* PrimComp = CachedPrimitive.Get()) return PrimComp->GetComponentQuat();
	return GetOwner() ? GetOwner()->GetActorQuat() : FQuat::Identity;
}