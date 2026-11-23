// Copyright 2026 kirzo

#include "Camera/KzCameraZone.h"
#include "Components/ArrowComponent.h"
#include "Components/KzShapeComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/CollisionProfile.h"
#include "Math/Geometry/Shapes/KzBox.h"

AKzCameraZone::AKzCameraZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ShapeComponent = CreateDefaultSubobject<UKzShapeComponent>(TEXT("Shape"));
	ShapeComponent->Shape.InitializeAs<FKzBox>(FVector(500.0f, 500.0f, 300.0f));
	ShapeComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	RootComponent = ShapeComponent;

	Blend.BlendTime = 1.0f;
	Blend.BlendFunction = VTBlend_EaseInOut;

	SetCanBeDamaged(false);
	bReplicates = false;

#if WITH_EDITORONLY_DATA
	CameraArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("CameraArrow"));
	if (CameraArrow)
	{
		CameraArrow->ArrowColor = FColor(245, 165, 36);
		CameraArrow->SetUsingAbsoluteScale(true);
		CameraArrow->SetupAttachment(ShapeComponent);
	}
#endif
}

bool AKzCameraZone::Contains(const FVector& Location) const
{
	if (!ShapeComponent || !ShapeComponent->Shape.IsValid()) return false;

	const FKzShapeInstance Shape = ShapeComponent->Shape * ShapeComponent->GetComponentScale();
	return Shape.IntersectsPoint(ShapeComponent->GetComponentLocation(), ShapeComponent->GetComponentQuat(), Location);
}

double AKzCameraZone::GetSize() const
{
	return ShapeComponent ? ShapeComponent->Bounds.GetBox().GetVolume() : 0.0;
}

void AKzCameraZone::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AKzCameraZone::UnregisterAllComponents(bool bForReregister)
{
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Unregister(this);
	}

	Super::UnregisterAllComponents(bForReregister);
}

void AKzCameraZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
	if (CameraArrow)
	{
		const FVector ToCamera = Camera ? Camera->GetActorLocation() - CameraArrow->GetComponentLocation() : FVector::ZeroVector;
		CameraArrow->SetVisibility(!ToCamera.IsNearlyZero());
		if (!ToCamera.IsNearlyZero())
		{
			CameraArrow->SetWorldRotation(ToCamera.Rotation());
			CameraArrow->SetArrowLength(ToCamera.Size());
		}
	}
#endif
}