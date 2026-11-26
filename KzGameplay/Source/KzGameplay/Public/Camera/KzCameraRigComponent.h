// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Containers/KzBlendLayers.h"
#include "Camera/KzCameraTypes.h"
#include "UObject/ObjectKey.h"
#include "KzCameraRigComponent.generated.h"

class AKzCameraVolume;
class AKzStandIn;
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

	/** What the camera follows in the editor, and in game when it doesn't follow the player pawn. Empty in the editor, the nearest Stand-In. */
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

	/**
	 * Starts updating, snapped to where it should be, and updates at once so a blend towards it starts from the right view.
	 * The camera keeps its own layers, and leaves the volumes the target is already in to the camera that saw it enter them: it only
	 * takes the ones it enters from now on. With bTargetJumped (start, respawn), it takes every volume where the target is.
	 */
	void Wake(bool bTargetJumped = false);

	/** Stops updating. In game, rigs sleep until the camera director needs them, so cameras out of view cost nothing. */
	void Sleep();

	bool IsAwake() const { return IsComponentTickEnabled(); }

	/** How far Location is from where this camera would hang for it: its spline's point for it, by its own projection rule, or its anchor. */
	float GetDistanceTo(const FVector& Location) const;

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	struct FLayer : Kz::FBlendLayer
	{
		TArray<TObjectPtr<UKzCameraModule>> Modules;
	};

	/** The layers from the lowest priority to the highest, with their weights. For the camera debugger. */
	TArray<FLayer*> GetLayers() { return Layers.GetOrdered(); }

	/** Volumes the target was already in when the rig woke, left to another camera until the target leaves them. For the camera debugger. */
	const TSet<TObjectKey<AKzCameraVolume>>& GetIgnoredVolumes() const { return IgnoredVolumes; }

	/** Frame the camera debugger last drew on: the rigs draw their debug the next one too, as with Draw Debug. */
	static uint64 DebuggerFrame;

private:
	Kz::TBlendLayers<FLayer> Layers;

	TWeakObjectPtr<UKzSplineFollowerComponent> Follower;
	TWeakObjectPtr<UCameraComponent> Camera;

	TWeakObjectPtr<const AActor> LastTarget;
	FVector LastTargetLocation = FVector::ZeroVector;
	FKzCameraPose LastPose;
	FRotator LastRest = FRotator::ZeroRotator;
	FMinimalViewInfo LastView;

	bool bPendingTeleport = true;

	TSet<TObjectKey<AKzCameraVolume>> IgnoredVolumes;

	/** Woke without the target jumping: the next volume update ignores the volumes the target is in. */
	bool bWokeClean = false;

	UCameraComponent* FindCamera();

	/** One frame of the rig: focus, anchor, the rest of the modules, and the pose on the camera. */
	void UpdateRig(float DeltaTime);

	/** Adds, scales and removes the layers of the camera volumes for the target's location. Reread applies them by the location alone. */
	void UpdateVolumes(const FVector& TargetLocation, bool bSnap, bool bReread);

	/** What a kind of module did this frame, for the debug drawing: its top module that weighs, and the pose before and after the kind. */
	struct FDebugSlot
	{
		const UKzCameraModule* Module = nullptr;
		FKzCameraPose In;
		FKzCameraPose Out;
	};

	/** Runs every kind of module whose stage is between First and Last, by stage and then in the order they first appear. */
	void EvaluateStages(EKzCameraStage First, EKzCameraStage Last, const FKzCameraContext& Context, FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers, TArray<FDebugSlot>* DebugSlots) const;

	/** Runs every module of one kind, from the base to the top layer, each blended by its layer's weight. OutTop is the top one that weighs. */
	FKzCameraPose EvaluateSlot(const UClass* SlotClass, const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers, const UKzCameraModule*& OutTop) const;

	void ApplyPose(const FKzCameraPose& Pose, UCameraComponent* CameraComponent);

	/** In the editor, gives a followed stand-in's preview this camera's view while it is the one that would show the stand-in in game. */
	void UpdateStandInPreview(AKzStandIn* StandIn, UCameraComponent* CameraComponent) const;

	/** In the editor while selected, with Draw Debug, or with the camera debugger. */
	bool ShouldDrawDebug() const;

	void DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FDebugSlot>& DebugSlots) const;
};
