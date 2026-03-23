// Copyright 2026 kirzo

#include "Animation/ScriptableTask_LinkAnimLayers.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

void UScriptableTask_LinkAnimLayers::BeginTask()
{
	if (IsValid(TargetActor) && !AnimLayerClass.IsNull())
	{
		if (USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
			{
				if (UClass* LoadedLayerClass = AnimLayerClass.LoadSynchronous())
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
		if (USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
			{
				if (UClass* LoadedLayerClass = AnimLayerClass.LoadSynchronous())
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