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
		// 1. Try to find the Database Component to resolve the query
		if (UKzDatabaseComponent* DatabaseComp = TargetActor->FindComponentByClass<UKzDatabaseComponent>())
		{
			// We define the exact type we expect the database to return
			TSoftClassPtr<UAnimInstance> ResolvedSoftClass;

			// 2. Ask the component to resolve the query. It will automatically route it to the correct Asset in O(1).
			if (DatabaseComp->Resolve<TSoftClassPtr<UAnimInstance>>(Query, ResolvedSoftClass))
			{
				if (UClass* LoadedClass = ResolvedSoftClass.LoadSynchronous())
				{
					// Cache the result for the Reset phase
					ResolvedLayerClass = LoadedClass;

					// 3. Try to use the custom priority stack component
					if (UKzAnimLayerComponent* LayerComp = TargetActor->FindComponentByClass<UKzAnimLayerComponent>())
					{
						LayerComp->PushLayer(ResolvedLayerClass, LayerPriority);
					}
					// 4. Fallback: Apply directly to the Skeletal Mesh
					else if (USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
					{
						if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
						{
							AnimInstance->LinkAnimClassLayers(ResolvedLayerClass);
						}
					}
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