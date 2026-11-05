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
 * Each frame it works out a focus from its target (focus stages), hangs the camera from an anchor (anchor stage) and then aims it.
 * A spline follower in Driven mode on the actor takes the focus onto its spline and becomes the anchor; without one, the camera
 * hangs from the rig's parent unless a Follow module takes it to the focus.
 * It goes between the actor's root and the camera, so the camera keeps its offset. It owns the lens, and runs in the editor as a preview.
 */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzCameraRigComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UKzCameraRigComponent();

	/** What the camera follows in the editor, and in game when it doesn't follow the player pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig")
	TObjectPtr<AActor> Target;

	/** In game, follow the first local player's pawn instead of Target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig")
	bool bTargetPlayerPawn = true;

	/** A target that moves farther than this in one frame teleports the camera instead of moving it. 0 means never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig", meta = (ClampMin = "0", Units = "Centimeters"))
	float TeleportDistance = 1000.0f;

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

	/** Draws the target, the focus and what the modules do, in game too. In the editor they show while the camera is selected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig|Debug")
	bool bDrawDebug = false;

	/** Adds a layer, or updates the one Source already has. Snap skips the blend. */
	UFUNCTION(BlueprintCallable, Category = "Camera Rig", meta = (AdvancedDisplay = "bSnap"))
	void PushLayer(UObject* Source, const FKzCameraLayerSettings& Settings, bool bSnap = false);

	/** Blends out the layer of Source and removes it. */
	UFUNCTION(BlueprintCallable, Category = "Camera Rig", meta = (AdvancedDisplay = "bSnap"))
	void PopLayer(UObject* Source, bool bSnap = false);

	/** Scales the layer of Source, as a volume does with its alpha. */
	UFUNCTION(BlueprintCallable, Category = "Camera Rig")
	void SetLayerAlpha(UObject* Source, float Alpha);

	/** What the camera follows: the player pawn in game when bTargetPlayerPawn, Target otherwise. */
	UFUNCTION(BlueprintPure, Category = "Camera Rig")
	AActor* GetTarget() const;

	/** The point the camera followed last frame. */
	UFUNCTION(BlueprintPure, Category = "Camera Rig")
	FVector GetFocus() const { return LastPose.Focus; }

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

	TWeakObjectPtr<const AActor> LastTarget;
	FVector LastTargetLocation = FVector::ZeroVector;
	FKzCameraPose LastPose;
	FRotator LastRest = FRotator::ZeroRotator;
	FMinimalViewInfo LastView;

	bool bPendingTeleport = true;

	UCameraComponent* FindCamera();

	/** Adds, scales and removes the layers of the camera volumes for the target's location. */
	void UpdateVolumes(const FVector& TargetLocation, bool bSnap);

	/** Advances the time blends and drops the layers that finished blending out or lost their source. */
	void UpdateLayerBlends(float DeltaTime, bool bSnap);

	/** The layers from the lowest priority to the highest, with their weights up to date. */
	TArray<FLayer*> GetOrderedLayers();

	/** Newest first, each clearing layer hides the ones activated before it by its own weight. */
	static void UpdateLayerWeights(const TArray<FLayer*>& InLayers);

	/** Runs every kind of module whose stage is between First and Last, by stage and then in the order they first appear. */
	void EvaluateStages(EKzCameraStage First, EKzCameraStage Last, const FKzCameraContext& Context, FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers) const;

	/** Runs every module of one kind, from the base to the top layer, each blended by its layer's weight. */
	FKzCameraPose EvaluateSlot(const UClass* SlotClass, const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers) const;

	void ApplyPose(const FKzCameraPose& Pose, UCameraComponent* CameraComponent);

	void DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers) const;
};
