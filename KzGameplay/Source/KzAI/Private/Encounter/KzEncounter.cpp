// Copyright 2026 kirzo

#include "Encounter/KzEncounter.h"
#include "Encounter/KzEncounterComponent.h"
#include "Math/Geometry/Shapes/KzBox.h"
#include "Sensors/KzSensorComponent.h"
#include "Sensors/KzSensorTags.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AKzEncounter::AKzEncounter()
{
	PrimaryActorTick.bCanEverTick = false;

	EncounterComponent = CreateDefaultSubobject<UKzEncounterComponent>(TEXT("EncounterComponent"));
	RootComponent = EncounterComponent;

	// Off until Start On Sensor turns it on
	SensorComponent = CreateDefaultSubobject<UKzSensorComponent>(TEXT("SensorComponent"));
	SensorComponent->SetupAttachment(EncounterComponent);
	SensorComponent->GameplayTags.AddTag(KzTags::Sensor::Encounter);
	SensorComponent->Shape = FKzShapeInstance::Make<FKzBox>(FVector(500.0f, 500.0f, 200.0f));
	SensorComponent->ScanInterval = 0.25f;
	SensorComponent->ShapeColor = FColor(245, 165, 36);
	SensorComponent->bAutoActivate = false;

	SetCanBeDamaged(false);
	bReplicates = false;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzGameplay/EditorResources/Encounter"));
		SpriteComponent->Sprite = SpriteTexture.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = TEXT("Spawning");
		SpriteComponent->SpriteInfo.DisplayName = NSLOCTEXT("KzSpriteCategory", "Spawning", "Spawning");
		SpriteComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
		SpriteComponent->SetupAttachment(EncounterComponent);
	}
#endif
}

void AKzEncounter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SensorComponent->SetVisibility(EncounterComponent->bStartOnSensor);
}