// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KzSpawnPoint.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UKzSensorComponent;
class UKzSpawnComponent;

/**
 * A point in the level where actors spawn, such as enemies coming into a fight. Placed on the ground, they stand on it facing its arrow.
 * Its sensor is the one Wait For Sensor waits for, and only shows with it.
 */
UCLASS(Blueprintable, HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD))
class KZGAMEPLAY_API AKzSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AKzSpawnPoint();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<UKzSpawnComponent> SpawnComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<UKzSensorComponent> SensorComponent;

#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent = nullptr;
#endif
};