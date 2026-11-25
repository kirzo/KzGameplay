// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "Components/KzComponentReference.h"
#include "Pooling/KzActorPoolSubsystem.h"
#include "KzSensableComponent.generated.h"

class AController;
class APawn;

/**
 * Registers its actor with the Spatial Sense Subsystem, for sensors to find. It sits where Component is, as a point unless given a Shape, so a
 * sensor finds it by whether that point lies inside the sensor's shape. While its actor is in an actor pool, sensors don't find it.
 * On a pawn, its Sense Tags have Character.Player while a player drives it.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzSensableComponent : public UActorComponent, public IKzPoolable
{
	GENERATED_BODY()

public:
	UKzSensableComponent();

	/** The tags that define what this object is (e.g. "Creature.Orc", "Item.Weapon") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kz Sensable")
	FGameplayTagContainer SenseTags;

	/** Whether this object can move and must be re-indexed when it does. Set to FALSE for props that never move: static sensables pay zero per-frame cost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kz Sensable")
	bool bIsDynamic = true;

	/** Where it is: a scene component of its actor, the root if none. */
	UPROPERTY(EditAnywhere, Category = "Kz Sensable")
	FKzComponentReference Component;

	/** Takes Component's own shape instead of Shape: its Shape if a KzShapeComponent, its collision shape if a primitive. For when sensors have to meet its volume. */
	UPROPERTY(EditAnywhere, Category = "Kz Sensable")
	bool bUseComponentShape = false;

	/** Its shape, at Component. A point by default, the cheapest for a sensor to test. */
	UPROPERTY(EditAnywhere, Category = "Kz Sensable", meta = (EditCondition = "!bUseComponentShape"))
	FKzShapeInstance Shape;

	// --- Runtime API ---

	/** Returns the exact shape instance being used for detection. */
	UFUNCTION(BlueprintPure, Category = "Kz Sensable")
	FKzShapeInstance GetShapeInstance() const;

	/** Returns the world-space bounding box of this sensable object. */
	UFUNCTION(BlueprintPure, Category = "Kz Sensable")
	FBox GetBounds() const;

	/** Returns the world location of the shape. */
	UFUNCTION(BlueprintPure, Category = "Kz Sensable")
	FVector GetShapeLocation() const;

	/** Returns the world rotation of the shape. */
	UFUNCTION(BlueprintPure, Category = "Kz Sensable")
	FQuat GetShapeRotation() const;

	/** Finds Component again, for when it changes at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Kz Sensable")
	void CacheShapeComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnAcquiredFromPool_Implementation() override;
	virtual void OnReleasedToPool_Implementation() override;

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	/** Component, found once: queries ask for it every time. */
	TWeakObjectPtr<USceneComponent> CachedComponent;
};