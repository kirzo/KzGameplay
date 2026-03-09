// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Misc/KzTransformSource.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "AbilityTask_WaitTraceCollision.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitTraceCollisionDelegate, const FHitResult&, HitResult);

/**
 * Ability Task that performs a continuous volumetric sweep using a generic Transform Source.
 * It ticks every frame to catch fast movements and maintains an internal list of ignored actors
 * to prevent multi-hitting the same target during a single activation.
 */
UCLASS()
class KZGAMEPLAY_API UAbilityTask_WaitTraceCollision : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_WaitTraceCollision(const FObjectInitializer& ObjectInitializer);

	/** Delegate fired whenever a new valid target is hit during the sweep. */
	UPROPERTY(BlueprintAssignable)
	FWaitTraceCollisionDelegate OnHitDetected;

	/**
	 * Starts a continuous collision sweep using a specific shape and a dynamic transform source.
	 * @param OwningAbility The Gameplay Ability executing the task.
	 * @param TaskInstanceName Name of the task for debugging.
	 * @param TransformSource The source providing the world transform (location & rotation) every frame.
	 * @param CollisionShape The geometric shape to sweep (Box, Sphere, Capsule). Exposed to Blueprint via FKzShapeInstance.
	 * @param ObjectTypes Object channel types to collide with (e.g., Pawn, Destructible).
	 * @param bDebugTrace If true, draws debug shapes in the world.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitTraceCollision* WaitTraceCollision(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		FKzShapeInstance CollisionShape,
		FKzTransformSource TransformSource,
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		bool bDebugTrace = false
	);

	/** Adds an actor to the ignore list dynamically during the task execution. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks")
	void AddIgnoredActor(AActor* ActorToIgnore);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	// --- Task Configuration ---
	FKzTransformSource TransformSource;
	FKzShapeInstance TraceShape;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	bool bDebugTrace;

	// --- Runtime State ---
	FTransform PreviousTransform;
	TArray<TWeakObjectPtr<AActor>> IgnoredActors;
};