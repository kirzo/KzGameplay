// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Core/KzMovementTypes.h"
#include "Misc/KzTransformSource.h"
#include "AbilityTask_MoveToLocationUsingInput.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoveToLocationInputDelegate, bool, bWasTeleported);

/**
 * Moves the avatar to a target by simulating movement input, following it if it moves,
 * and optionally rotates to face the target's direction upon arrival.
 */
UCLASS()
class KZGAMEPLAY_API UAbilityTask_MoveToLocationUsingInput : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_MoveToLocationUsingInput(const FObjectInitializer& ObjectInitializer);

	/** Delegate fired when the character successfully reaches the target location. */
	UPROPERTY(BlueprintAssignable)
	FMoveToLocationInputDelegate OnTargetReached;

	/**
	 * Moves the avatar to a target by adding movement input, optionally rotating upon arrival.
	 * @param Target Where to go, resolved every frame so a moving target is followed; a plain location or transform converts to it.
	 * @param VerticalAlignment Controls how the Z-axis (height) is handled.
	 * @param AcceptanceRadius How close the character needs to be to consider the target reached.
	 * @param Timeout Maximum time in seconds to attempt movement/rotation. If exceeded, the character is teleported.
	 * @param bBlockPlayerInput If true, the player's movement input is ignored during the task.
	 * @param bUseTargetRotation If true, the character will rotate to the target's yaw after reaching it.
	 * @param InterpolationDuration Time in seconds to interpolate to the exact location (and rotation) once the AcceptanceRadius is reached.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_MoveToLocationUsingInput* MoveToLocationUsingInput(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		FKzTransformSource Target,
		EKzTargetVerticalAlignment VerticalAlignment = EKzTargetVerticalAlignment::AlignFeetToTarget,
		float AcceptanceRadius = 0.0f,
		float Timeout = 4.0f,
		bool bBlockPlayerInput = true,
		bool bUseTargetRotation = false,
		float InterpolationDuration = 0.1f
	);

	virtual void Activate() override;

private:
	/** The handler that owns the input we block, if the avatar has one. */
	class UKzInputHandlerComponent* GetInputHandler() const;

	/** Identifies our block on the ignore stack. */
	FName GetBlockSourceID() const;

	/** Refreshes TargetLocation and TargetRotation from wherever the target is now. */
	void ResolveTarget();

public:
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool AbilityIsEnding) override;

protected:
	UPROPERTY(Replicated)
	FKzTransformSource Target;

	UPROPERTY(Replicated)
	EKzTargetVerticalAlignment VerticalAlignment;

	/** Where the target resolved to this frame. */
	FVector TargetLocation;
	FRotator TargetRotation;

	/** Off once the avatar gives up on a target it cannot reach, so it settles where it got stuck. */
	bool bTrackTarget = true;

	UPROPERTY(Replicated)
	bool bUseTargetRotation;

	UPROPERTY(Replicated)
	float AcceptanceRadius;

	UPROPERTY(Replicated)
	float Timeout;

	UPROPERTY(Replicated)
	float InterpolationDuration;

	UPROPERTY(Replicated)
	bool bBlockInput;

	float TimeStarted;
	bool bIsFinished;

	// --- Anti-Stuck State ---
	FVector LastLocation;
	float StuckTime;

	// --- Interpolation Phase State ---
	bool bIsInterpolatingPhase;
	float TimeInterpolationStarted;
	FVector StartLocationForInterpolation;
	FQuat StartRotationQuat;
	FQuat TargetRotationQuat;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};