// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Containers/KzPriorityStack.h"
#include "Camera/KzCameraTypes.h"
#include "UObject/ObjectKey.h"
#include "KzCameraRigComponent.generated.h"

class UCameraComponent;
class UKzCameraModule;
class UKzSplineFollowerComponent;

/**
 * Drives the camera attached under it with camera modules: its own, plus the layers that volumes, markers and Blueprint add on top.
 * It goes between the actor's root and the camera, so the camera keeps its offset; a spline follower on the actor moves the root.
 * It owns the lens: the camera's field of view is overwritten every frame. It also runs in the editor, as a preview.
 */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzCameraRigComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UKzCameraRigComponent();

	/** The base layer, always on. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Camera Rig")
	TArray<TObjectPtr<UKzCameraModule>> Modules;

	/** Before any module changes it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig", meta = (UIMin = "5", UIMax = "170", Units = "Degrees"))
	float FieldOfView = 90.0f;

	/** For layers that don't set their own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig", meta = (ClampMin = "0", Units = "Seconds"))
	float DefaultBlendTime = 1.0f;

	/** Makes the actor the first local player's view target when play starts. The player controller must not manage its camera target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig")
	bool bAutoActivateForPlayer = false;

	/** Adds a layer, or updates the one Source already has. Snap skips the blend. */
	UFUNCTION(BlueprintCallable, Category = "Camera Rig", meta = (AdvancedDisplay = "bSnap"))
	void PushLayer(UObject* Source, const FKzCameraLayerSettings& Settings, bool bSnap = false);

	/** Blends out the layer of Source and removes it. */
	UFUNCTION(BlueprintCallable, Category = "Camera Rig", meta = (AdvancedDisplay = "bSnap"))
	void PopLayer(UObject* Source, bool bSnap = false);

	/** Scales the layer of Source, as a volume does with its alpha. */
	UFUNCTION(BlueprintCallable, Category = "Camera Rig")
	void SetLayerAlpha(UObject* Source, float Alpha);

	/** What the camera follows: the follower's projected actor, or the player pawn in game. */
	UFUNCTION(BlueprintPure, Category = "Camera Rig")
	AActor* GetTarget() const;

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

private:
	struct FLayer
	{
		TWeakObjectPtr<UObject> Source;
		TArray<TObjectPtr<UKzCameraModule>> Modules;
		float Alpha = 1.0f;
		float Blend = 0.0f;
		float BlendTime = 0.0f;

		/** Alpha and blend, minus what newer clearing layers hide. */
		float Weight = 0.0f;

		/** When it was activated, for clearing layers. */
		uint64 Order = 0;

		bool bClearPrevious = false;
		bool bRemoving = false;
	};

	Kz::TPriorityStack<FLayer, false, TObjectKey<UObject>, false> Layers;
	uint64 LastLayerOrder = 0;

	TWeakObjectPtr<UKzSplineFollowerComponent> Follower;
	TWeakObjectPtr<UCameraComponent> Camera;

	bool bPendingTeleport = true;

	UCameraComponent* FindCamera();

	/** Adds, scales and removes the layers of the camera volumes for the target's location. */
	void UpdateVolumes(const FVector& TargetLocation, bool bSnap);

	/** Advances the time blends and drops the layers that finished blending out or lost their source. */
	void UpdateLayerBlends(float DeltaTime, bool bSnap);

	/** Newest first, each clearing layer hides the ones activated before it by its own weight. */
	static void UpdateLayerWeights(const TArray<FLayer*>& InLayers);

	/** OrderedLayers go from the lowest priority to the highest. */
	FKzCameraPose EvaluatePose(FKzCameraContext& Context, const FTransform& CameraRelative, const TArray<FLayer*>& OrderedLayers) const;

	/** Runs every module of one kind, from the base to the top layer, each blended by its layer's weight. */
	FKzCameraPose EvaluateSlot(const UClass* SlotClass, const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers) const;

	/** Hangs the pose from the rail point at its distance. */
	void PlaceOnRail(FKzCameraContext& Context, FKzCameraPose& Pose, const FTransform& CameraRelative) const;

	void ApplyPose(const FKzCameraPose& Pose, UCameraComponent* CameraComponent);
};
