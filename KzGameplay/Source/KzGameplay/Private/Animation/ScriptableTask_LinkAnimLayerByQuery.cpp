// Copyright 2026 kirzo

#include "Animation/ScriptableTask_LinkAnimLayerByQuery.h"
#include "Components/KzDatabaseComponent.h"
#include "Animation/KzAnimLayerComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

void UScriptableTask_LinkAnimLayerByQuery::BeginTask()
{
	if (IsValid(TargetActor))
	{
		if (UClass* LayerClass = UKzDatabaseComponent::ResolveFor(TargetActor.Get(), Query, TSoftClassPtr<UAnimInstance>()).LoadSynchronous())
		{
			// Cached so ResetTask unlinks this exact class.
			ResolvedLayerClass = LayerClass;

			if (UKzAnimLayerComponent* LayerComp = TargetActor->FindComponentByClass<UKzAnimLayerComponent>())
			{
				LayerComp->PushLayer(ResolvedLayerClass, LayerPriority);
			}
			else if (USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
				{
					AnimInstance->LinkAnimClassLayers(ResolvedLayerClass);
				}
			}
		}
	}

	Finish();
}

void UScriptableTask_LinkAnimLayerByQuery::ResetTask()
{
	if (bRevertOnReset && IsValid(TargetActor) && ResolvedLayerClass)
	{
		// 1. Try to use the custom priority stack component
		if (UKzAnimLayerComponent* LayerComp = TargetActor->FindComponentByClass<UKzAnimLayerComponent>())
		{
			LayerComp->PopLayer(ResolvedLayerClass);
		}
		// 2. Fallback: Unlink directly from the Skeletal Mesh
		else if (USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
			{
				AnimInstance->UnlinkAnimClassLayers(ResolvedLayerClass);
			}
		}

		// Clear cache
		ResolvedLayerClass = nullptr;
	}
}

#if WITH_EDITOR
FText UScriptableTask_LinkAnimLayerByQuery::GetDisplayTitle() const
{
	FString TargetName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_LinkAnimLayerByQuery, TargetActor), TargetName))
	{
		TargetName = TargetActor ? TargetActor->GetActorLabel() : TEXT("None");
	}

	// Create a clean hint from the query (e.g., show the first required tag)
	FString QueryHint = TEXT("Empty Query");
	if (!Query.RequireTags.IsEmpty())
	{
		QueryHint = Query.RequireTags.First().ToString();
	}

	return FText::Format(INVTEXT("Link Layer [{0}] on {1}"), FText::FromString(QueryHint), FText::FromString(TargetName));
}
#endif