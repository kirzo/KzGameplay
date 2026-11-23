// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h"
#include "KzCameraZone.generated.h"

class UArrowComponent;
class UKzShapeComponent;

/**
 * A volume that picks the camera for the player inside it. Entering it switches the camera director to its camera;
 * starting or respawning inside it cuts straight to it. Leaving every zone keeps the current camera.
 */
UCLASS(Blueprintable, HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD))
class KZGAMEPLAY_API AKzCameraZone : public AActor
{
	GENERATED_BODY()

public:
	AKzCameraZone();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Zone")
	TObjectPtr<UKzShapeComponent> ShapeComponent;

	/** The camera for this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Zone")
	TObjectPtr<AActor> Camera;

	/** How the view blends to the camera on entering the zone. 0 seconds is a cut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Zone")
	FViewTargetTransitionParams Blend;

	/** Among overlapping zones the highest wins; among equals, the smallest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Zone")
	int32 Priority = 0;

	UFUNCTION(BlueprintPure, Category = "Camera Zone")
	bool Contains(const FVector& Location) const;

	/** Volume of the zone's bounds, to prefer the more specific of two overlapping zones. */
	double GetSize() const;

	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool bForReregister = false) override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
#if WITH_EDITORONLY_DATA
	/** Points at the zone's camera. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> CameraArrow = nullptr;
#endif
};