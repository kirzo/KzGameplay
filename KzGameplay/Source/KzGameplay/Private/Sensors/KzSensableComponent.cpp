// Copyright 2026 kirzo

#include "Sensors/KzSensableComponent.h"
#include "Sensors/KzSpatialSenseSubsystem.h"
#include "Components/KzShapeComponent.h"
#include "Math/Geometry/Shapes/KzPoint.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UKzSensableComponent::UKzSensableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Shape = FKzShapeInstance::Make<FKzPoint>();
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
	CachedComponent = Component.GetComponent(this);
}

FKzShapeInstance UKzSensableComponent::GetShapeInstance() const
{
	// A component without a shape of its own, a plain scene component, still has Shape there
	if (bUseComponentShape)
	{
		if (const UKzShapeComponent* KzShapeComp = Cast<UKzShapeComponent>(CachedComponent.Get()))
		{
			return KzShapeComp->Shape * KzShapeComp->GetComponentScale();
		}
		if (const UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(CachedComponent.Get()))
		{
			return FKzShapeInstance::MakeFromCollisionShape(PrimComp->GetCollisionShape());
		}
	}
	return Shape;
}

FBox UKzSensableComponent::GetBounds() const
{
	const UPrimitiveComponent* PrimComp = bUseComponentShape ? Cast<UPrimitiveComponent>(CachedComponent.Get()) : nullptr;
	return PrimComp ? PrimComp->Bounds.GetBox() : Shape.GetBoundingBox(GetShapeLocation(), GetShapeRotation());
}

FVector UKzSensableComponent::GetShapeLocation() const
{
	if (const USceneComponent* SceneComp = CachedComponent.Get()) return SceneComp->GetComponentLocation();
	return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

FQuat UKzSensableComponent::GetShapeRotation() const
{
	if (const USceneComponent* SceneComp = CachedComponent.Get()) return SceneComp->GetComponentQuat();
	return GetOwner() ? GetOwner()->GetActorQuat() : FQuat::Identity;
}