// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Actors/KzSplineMarker.h"
#include "Camera/KzCameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "KzCameraMarker.generated.h"

class UTexture2D;

/**
 * Adds a layer to the camera rig whose follower crosses it forwards, and removes it when crossed back; crossed forwards, it can also switch cameras.
 * Jumping past it (start, respawn) applies the change without blending. A marker whose layer clears the previous ones is drawn in violet.
 */
UCLASS(Blueprintable)
class KZGAMEPLAY_API AKzCameraMarker : public AKzSplineMarker
{
	GENERATED_BODY()

public:
	AKzCameraMarker();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FKzCameraLayerSettings Layer;

	/** When the camera crosses the marker forwards, the view switches to this camera. Jumping past it (start, respawn) doesn't switch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<AActor> SwitchToCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (EditCondition = "SwitchToCamera != nullptr"))
	FViewTargetTransitionParams SwitchBlend;

	virtual void NotifyCrossed(UKzSplineFollowerComponent* Follower, bool bForward, bool bTeleport) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> MarkerSprite = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ClearingSprite = nullptr;
#endif
};
