// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KzEncounter.generated.h"

class UBillboardComponent;
class UKzEncounterComponent;
class UKzSensorComponent;

/**
 * A fight placed in the level, such as an arena: its spawn points, waves and limits are set on its Encounter Component.
 * Its sensor is the one Start On Sensor waits for, and only shows with it.
 */
UCLASS(Blueprintable, HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD))
class KZAI_API AKzEncounter : public AActor
{
	GENERATED_BODY()

public:
	AKzEncounter();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<UKzEncounterComponent> EncounterComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<UKzSensorComponent> SensorComponent;

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
#endif
};