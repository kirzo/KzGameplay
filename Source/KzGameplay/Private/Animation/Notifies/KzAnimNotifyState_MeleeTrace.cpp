// Copyright 2026 kirzo

#include "Animation/Notifies/KzAnimNotifyState_MeleeTrace.h"
#include "Weapons/KzMeleeWeaponComponent.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Items/KzItemInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

UKzAnimNotifyState_MeleeTrace::UKzAnimNotifyState_MeleeTrace()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 69, 0, 255); // Red-Orange
#endif
}

UKzMeleeWeaponComponent* UKzAnimNotifyState_MeleeTrace::FindMeleeComponent(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp || !MeshComp->GetOwner() || !EquipmentSlot.IsValid())
	{
		return nullptr;
	}

	AActor* OwnerActor = MeshComp->GetOwner();

	if (UKzEquipmentComponent* EquipComp = OwnerActor->FindComponentByClass<UKzEquipmentComponent>())
	{
		if (const FKzItemInstance* ItemInstance = EquipComp->FindItemInSlot(EquipmentSlot))
		{
			if (ItemInstance->IsValid() && ItemInstance->SpawnedActor)
			{
				return ItemInstance->SpawnedActor->FindComponentByClass<UKzMeleeWeaponComponent>();
			}
		}
	}

	return nullptr;
}

void UKzAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UKzMeleeWeaponComponent* MeleeComp = FindMeleeComponent(MeshComp))
	{
		MeleeComp->StartMeleeTrace(BaseSocketName, TipSocketName, TraceRadius);
	}
}

void UKzAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (UKzMeleeWeaponComponent* MeleeComp = FindMeleeComponent(MeshComp))
	{
		MeleeComp->SweepMeleeTrace(HitEventTag);
	}
}

void UKzAnimNotifyState_MeleeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UKzMeleeWeaponComponent* MeleeComp = FindMeleeComponent(MeshComp))
	{
		MeleeComp->EndMeleeTrace();
	}
}