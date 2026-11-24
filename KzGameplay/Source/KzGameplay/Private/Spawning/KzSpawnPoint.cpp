// Copyright 2026 kirzo

#include "Spawning/KzSpawnPoint.h"
#include "Spawning/KzSpawnComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AKzSpawnPoint::AKzSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnComponent = CreateDefaultSubobject<UKzSpawnComponent>(TEXT("SpawnComponent"));
	RootComponent = SpawnComponent;

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