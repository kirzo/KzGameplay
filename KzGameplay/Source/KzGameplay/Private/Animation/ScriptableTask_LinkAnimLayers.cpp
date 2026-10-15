// Copyright 2026 kirzo

#include "Animation/ScriptableTask_LinkAnimLayers.h"
#include "Animation/KzAnimLayerComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

void UScriptableTask_LinkAnimLayers::BeginTask()
{
	if (IsValid(TargetActor) && !AnimLayerClass.IsNull())
	{
		if (UClass* LoadedLayerClass = AnimLayerClass.LoadSynchronous())
		{
			// Try to use the custom priority stack component
			if (UKzAnimLayerComponent* LayerComp = TargetActor->FindComponentByClass<UKzAnimLayerComponent>())
			{
				LayerComp->PushLayer(LoadedLayerClass, LayerPriority);
			}
			// Fallback: Apply directly to the Skeletal Mesh
			else if (USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
				{
					AnimInstance->LinkAnimClassLayers(LoadedLayerClass);
				}
			}
		}
	}

	Finish();
}

void UScriptableTask_LinkAnimLayers::ResetTask()
{
	if (bRevertOnReset && IsValid(TargetActor) && !AnimLayerClass.IsNull())
	{
		if (UClass* LoadedLayerClass = AnimLayerClass.LoadSynchronous())
		{
			if (UKzAnimLayerComponent* LayerComp = TargetActor->FindComponentByClass<UKzAnimLayerComponent>())
			{
				LayerComp->PopLayer(LoadedLayerClass);
			}
			else if (USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
				{
					AnimInstance->UnlinkAnimClassLayers(LoadedLayerClass);
				}
			}
		}
	}
}

#if WITH_EDITOR
FText UScriptableTask_LinkAnimLayers::GetDisplayTitle() const
{
	FString TargetName;

	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_LinkAnimLayers, TargetActor), TargetName))
	{
		TargetName = TargetActor ? TargetActor->GetActorLabel() : TEXT("None");
	}

	if (!AnimLayerClass.IsNull())
	{
		FString LayerName = AnimLayerClass.GetAssetName();
		LayerName.RemoveFromEnd(TEXT("_C"));

		return FText::Format(INVTEXT("Link [{0}] on {1}"), FText::FromString(LayerName), FText::FromString(TargetName));
	}

	return INVTEXT("Link Anim Layers");
}
#endif