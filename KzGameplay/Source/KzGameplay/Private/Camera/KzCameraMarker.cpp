// Copyright 2026 kirzo

#include "Camera/KzCameraMarker.h"
#include "Camera/KzCameraRigComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/KzSplineFollowerComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FColor CameraMarkerColor(245, 165, 36);
	const FColor ClearingMarkerColor(169, 112, 255);
}

AKzCameraMarker::AKzCameraMarker()
{
#if WITH_EDITORONLY_DATA
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> MarkerTexture;
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> ClearingTexture;
		FConstructorStatics()
			: MarkerTexture(TEXT("/KzGameplay/EditorResources/CameraMarker"))
			, ClearingTexture(TEXT("/KzGameplay/EditorResources/CameraMarkerClear"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	MarkerSprite = ConstructorStatics.MarkerTexture.Get();
	ClearingSprite = ConstructorStatics.ClearingTexture.Get();

	if (SpriteComponent)
	{
		SpriteComponent->Sprite = MarkerSprite;
		SpriteComponent->SpriteInfo.Category = TEXT("Cameras");
		SpriteComponent->SpriteInfo.DisplayName = NSLOCTEXT("KzSpriteCategory", "Cameras", "Cameras");
	}

	if (ProjectionArrow)
	{
		ProjectionArrow->ArrowColor = CameraMarkerColor;
	}
#endif
}

void AKzCameraMarker::NotifyCrossed(UKzSplineFollowerComponent* Follower, bool bForward, bool bTeleport)
{
	const AActor* FollowerOwner = Follower ? Follower->GetOwner() : nullptr;
	if (UKzCameraRigComponent* Rig = FollowerOwner ? FollowerOwner->FindComponentByClass<UKzCameraRigComponent>() : nullptr)
	{
		if (bForward)
		{
			Rig->PushLayer(this, Layer, bTeleport);
		}
		else
		{
			Rig->PopLayer(this, bTeleport);
		}
	}

	Super::NotifyCrossed(Follower, bForward, bTeleport);
}

void AKzCameraMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
	if (SpriteComponent)
	{
		SpriteComponent->SetSprite(Layer.bClearPrevious ? ClearingSprite : MarkerSprite);
	}

	if (ProjectionArrow)
	{
		ProjectionArrow->SetArrowColor(Layer.bClearPrevious ? ClearingMarkerColor : CameraMarkerColor);
	}
#endif
}
