// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "KzSensableComponent.generated.h"

/** Component that registers an Actor to the Spatial Sense Subsystem. */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzSensableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzSensableComponent();

	/** The tags that define what this object is (e.g. "Creature.Orc", "Item.Weapon") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kz Sensable")
	FGameplayTagContainer SenseTags;

	/** Should this object update its position in the grid every frame? (False for static objects) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kz Sensable")
	bool bIsDynamic = true;

	/** Shape definition (sphere, box, capsule, etc.) */
	UPROPERTY(EditAnywhere, Category = "Kz Sensable")
	FKzShapeInstance FallbackShape;

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

	/** Force updates the shape cache. Useful if the primitive changes significantly at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Kz Sensable")
	void CacheShapeComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Cached reference to the shape we are tracking */
	TWeakObjectPtr<UPrimitiveComponent> CachedPrimitive;
};