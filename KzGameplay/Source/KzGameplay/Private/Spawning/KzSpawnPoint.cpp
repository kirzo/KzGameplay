// Copyright 2026 kirzo

#include "Spawning/KzSpawnPoint.h"
#include "Spawning/KzSpawnComponent.h"
#include "Math/Geometry/Shapes/KzSphere.h"
#include "Sensors/KzSensorComponent.h"
#include "Sensors/KzSensorTags.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AKzSpawnPoint::AKzSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnComponent = CreateDefaultSubobject<UKzSpawnComponent>(TEXT("SpawnComponent"));
	RootComponent = SpawnComponent;

	// Off until Wait For Sensor turns it on
	SensorComponent = CreateDefaultSubobject<UKzSensorComponent>(TEXT("SensorComponent"));
	SensorComponent->SetupAttachment(SpawnComponent);
	SensorComponent->GameplayTags.AddTag(KzTags::Sensor::Spawn);
	SensorComponent->Shape = FKzShapeInstance::Make<FKzSphere>(1500.0f);
	SensorComponent->ScanInterval = 0.25f;
	SensorComponent->ShapeColor = FColor(245, 165, 36);
	SensorComponent->bAutoActivate = false;

	SetCanBeDamaged(false);
	bReplicates = false;

#if WITH_EDITORONLY_DATA
	// Raised off the ground the point marks, so the floor doesn't hide them
	const FVector EditorOffset(0.0f, 0.0f, 50.0f);

	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzGameplay/EditorResources/SpawnPoint"));
		SpriteComponent->Sprite = SpriteTexture.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = TEXT("Spawning");
		SpriteComponent->SpriteInfo.DisplayName = NSLOCTEXT("KzSpriteCategory", "Spawning", "Spawning");
		SpriteComponent->SetRelativeLocation(EditorOffset);
		SpriteComponent->SetupAttachment(SpawnComponent);
	}

	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor(245, 165, 36);
		ArrowComponent->SetRelativeLocation(EditorOffset);
		ArrowComponent->SetupAttachment(SpawnComponent);
	}
#endif
}

void AKzSpawnPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SensorComponent->SetVisibility(SpawnComponent->AutoSpawnClass && SpawnComponent->bWaitForSensor);
}