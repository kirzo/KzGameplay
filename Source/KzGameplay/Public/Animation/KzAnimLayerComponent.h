// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Containers/KzPriorityStack.h"
#include "Components/KzComponentSocketReference.h"
#include "KzAnimLayerComponent.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;

/**
 * Manages the injection of Animation Layers using a priority stack.
 * Ensures that if a high-priority layer is removed, the system correctly falls back
 * to the previous layer instead of breaking the animation state.
 */
UCLASS(ClassGroup = (Animation), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzAnimLayerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzAnimLayerComponent();

	/**
	 * The specific skeletal mesh component that should receive the animation layers.
	 * If left empty, it will automatically fallback to the first SkeletalMeshComponent found on the Owner.
	 */
	UPROPERTY(EditAnywhere, Category = "Animation Layers", meta = (NoSocket, NoOffset, AllowedClasses = "/Script/Engine.SkeletalMeshComponent"))
	FKzComponentSocketReference TargetMeshReference;

	/** The default layer pushed when the component begins play (e.g., Unarmed locomotion). */
	UPROPERTY(EditAnywhere, Category = "Animation Layers")
	TSubclassOf<UAnimInstance> DefaultLayer;

	/** The priority for the default layer. Usually 0. */
	UPROPERTY(EditAnywhere, Category = "Animation Layers", meta = (EditCondition = "DefaultLayer != nullptr"))
	int32 DefaultLayerPriority = 0;

	/**
	 * Pushes a new animation layer class to the stack and updates the mesh if it has the highest priority.
	 * @param LayerClass The AnimInstance class to link.
	 * @param Priority Higher numbers take precedence.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Layers")
	void PushLayer(TSubclassOf<UAnimInstance> LayerClass, int32 Priority);

	/**
	 * Removes a specific animation layer class from the stack.
	 * If the removed layer was the active one, it automatically links the next one in the stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Layers")
	void PopLayer(TSubclassOf<UAnimInstance> LayerClass);

protected:
	virtual void BeginPlay() override;

private:
	Kz::TPriorityStack<TSubclassOf<UAnimInstance>, false, TSubclassOf<UAnimInstance>, false> LayerStack;

	/** Cached reference to the owner's primary skeletal mesh. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMesh;

	void UpdateLinkedLayers(TSubclassOf<UAnimInstance> LayerToUnlink = nullptr);
};