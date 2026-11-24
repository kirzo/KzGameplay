// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "KzCameraDirector.generated.h"

class AKzCameraZone;
class APawn;
class APlayerController;

/** Something the cameras did, for the camera debugger's history. */
struct FKzCameraEvent
{
	double Time = 0.0;
	FString Text;
};

/**
 * Chooses the camera the first local player sees, and keeps camera rigs awake only while they are in view or blending in.
 * At play start, and after a respawn or a teleport, it cuts to the camera of the zone the player is in, or else the nearest one.
 * While playing, camera zones, camera markers and Blueprint switch cameras. The player controller must not manage its camera target.
 */
UCLASS()
class KZGAMEPLAY_API UKzCameraDirector : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** A player that moves farther than this in one frame is taken as respawned, and gets the camera for where it is now. 0 means never. */
	UPROPERTY(BlueprintReadWrite, Category = "Camera Director")
	float TeleportDistance = 1000.0f;

	static UKzCameraDirector* Get(const UObject* WorldContextObject);

	/**
	 * Shows Camera, blending from the current view. A camera with a camera rig also becomes the one ReturnToGameplay goes back to.
	 * Instigator is who asked, for the camera debugger; from Blueprint, the caller.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Director", meta = (DefaultToSelf = "Instigator", HidePin = "Instigator"))
	void SwitchTo(AActor* Camera, const FViewTargetTransitionParams& Blend, const UObject* Instigator = nullptr);

	/** Back to the gameplay camera, from a cutscene, a fixed shot or a finisher. */
	UFUNCTION(BlueprintCallable, Category = "Camera Director", meta = (DefaultToSelf = "Instigator", HidePin = "Instigator"))
	void ReturnToGameplay(const FViewTargetTransitionParams& Blend, const UObject* Instigator = nullptr);

	/** Cuts to the camera for where the player is now: its zone's, or else the nearest, with every volume the player is in. Play start and respawns do it on their own. */
	UFUNCTION(BlueprintCallable, Category = "Camera Director")
	void SelectCamera();

	UFUNCTION(BlueprintPure, Category = "Camera Director")
	AActor* GetGameplayCamera() const { return GameplayCamera.Get(); }

	const AKzCameraZone* GetCurrentZone() const { return CurrentZone.Get(); }

	/** Why the camera in view was chosen, and when. */
	const FKzCameraEvent& GetLastSwitch() const { return LastSwitch; }

	/** The latest events, oldest first. */
	const TArray<FKzCameraEvent>& GetHistory() const { return History; }

	/** Adds an event to the camera debugger's history and to the visual logger, under Subject. */
	static void RecordEvent(const UObject* Subject, const FString& Text);

	/** How the camera debugger names an object: an actor by its label, a component by its actor's. */
	static FString Describe(const UObject* Object);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	TWeakObjectPtr<AActor> GameplayCamera;
	TWeakObjectPtr<const AKzCameraZone> CurrentZone;

	TWeakObjectPtr<const APawn> LastPawn;
	FVector LastPawnLocation = FVector::ZeroVector;

	FKzCameraEvent LastSwitch;
	TArray<FKzCameraEvent> History;

	APlayerController* GetPlayerController() const;

	/** Switches to Camera and records Reason for the debugger. After a jump (start, respawn) the camera takes every volume the player is in. */
	void Show(AActor* Camera, const FViewTargetTransitionParams& Blend, const FString& Reason, bool bPlayerJumped = false);

	/** Whether the player sees the gameplay camera, or is blending to it, rather than a cutscene or a fixed shot. */
	bool IsShowingGameplay(const APlayerCameraManager* CameraManager) const;

	/** Wakes the rigs in view or blending in, and puts the rest to sleep. */
	void UpdateRigs(const APlayerCameraManager* CameraManager) const;

	/** The zone Location is in: the highest priority among overlapping ones, then the smallest. */
	const AKzCameraZone* FindZone(const FVector& Location) const;

	/** The camera that would hang nearest to Location. */
	AActor* FindNearestCamera(const FVector& Location) const;
};