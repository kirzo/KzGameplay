// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KzSpawnPoint.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UKzSpawnComponent;

/** A point in the level where actors spawn, such as enemies coming into a fight. Placed on the ground, they stand on it facing its arrow. */
UCLASS(Blueprintable, HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD))
class KZGAMEPLAY_API AKzSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AKzSpawnPoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<UKzSpawnComponent> SpawnComponent;

#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent = nullptr;
#endif
};