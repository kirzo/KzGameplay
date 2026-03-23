// Copyright 2026 kirzo

#include "Animation/KzAnimLayerComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

UKzAnimLayerComponent::UKzAnimLayerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKzAnimLayerComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedMesh = TargetMeshReference.GetComponent<USkeletalMeshComponent>(this);

	// Fallback: If no valid reference was provided, grab the first one on the owner
	if (!CachedMesh)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			CachedMesh = OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
		}
	}

	if (DefaultLayer)
	{
		PushLayer(DefaultLayer, DefaultLayerPriority);
	}
}

void UKzAnimLayerComponent::PushLayer(TSubclassOf<UAnimInstance> LayerClass, int32 Priority)
{
	if (!LayerClass) return;

	LayerStack.Push(LayerClass, Priority);
	UpdateLinkedLayers();
}

void UKzAnimLayerComponent::PopLayer(TSubclassOf<UAnimInstance> LayerClass)
{
	if (!LayerClass) return;

	if (LayerStack.Pop(LayerClass))
	{
		UpdateLinkedLayers(LayerClass);
	}
}

void UKzAnimLayerComponent::UpdateLinkedLayers(TSubclassOf<UAnimInstance> LayerToUnlink)
{
	// Early return if we didn't find a mesh during initialization
	if (!CachedMesh) return;

	UAnimInstance* MainAnimInstance = CachedMesh->GetAnimInstance();
	if (!MainAnimInstance) return;

	// Always explicitly unlink the popped layer to ensure the slot is cleared properly
	if (LayerToUnlink)
	{
		MainAnimInstance->UnlinkAnimClassLayers(LayerToUnlink);
	}

	// If the stack has layers, re-link the top one.
	if (!LayerStack.IsEmpty())
	{
		MainAnimInstance->LinkAnimClassLayers(LayerStack.Top());
	}
}