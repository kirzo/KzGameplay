// Copyright 2026 kirzo

#include "Camera/KzCameraLimit.h"
#include "Components/KzShapeComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/CollisionProfile.h"
#include "Math/Geometry/Shapes/KzBox.h"

AKzCameraLimit::AKzCameraLimit()
{
	PrimaryActorTick.bCanEverTick = false;

	ShapeComponent = CreateDefaultSubobject<UKzShapeComponent>(TEXT("Shape"));
	ShapeComponent->Shape.InitializeAs<FKzBox>(FVector(200.0f));
	ShapeComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	RootComponent = ShapeComponent;

	SetCanBeDamaged(false);
	bReplicates = false;
}

FKzShapeInstance AKzCameraLimit::GetShape(bool bWithMargin) const
{
	FKzShapeInstance Shape = ShapeComponent->Shape;
	if (!Shape.IsValid()) return Shape;

	Shape *= ShapeComponent->GetComponentScale();
	if (bWithMargin)
	{
		Shape += Mode == EKzCameraLimitMode::Block ? Margin : -Margin;
	}
	return Shape;
}

FBox AKzCameraLimit::GetBounds() const
{
	// Shrinking keeps a confining shape inside its own bounds. Grown along its own axes, a turned shape reaches up to root 3 times the margin further along the world's.
	const FBox Bounds = ShapeComponent->Bounds.GetBox();
	return Mode == EKzCameraLimitMode::Block ? Bounds.ExpandBy(Margin * UE_DOUBLE_SQRT_3) : Bounds;
}

bool AKzCameraLimit::IsConfining(const FVector& Location) const
{
	return Mode == EKzCameraLimitMode::Confine
		&& GetBounds().IsInsideOrOn(Location)
		&& GetShape(false).IntersectsPoint(ShapeComponent->GetComponentLocation(), ShapeComponent->GetComponentQuat(), Location);
}

void AKzCameraLimit::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AKzCameraLimit::UnregisterAllComponents(bool bForReregister)
{
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Unregister(this);
	}

	Super::UnregisterAllComponents(bForReregister);
}

void AKzCameraLimit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ShapeComponent->ShapeColor = Mode == EKzCameraLimitMode::Block ? FColor::Red : FColor::Green;
	ShapeComponent->MarkRenderStateDirty();
}