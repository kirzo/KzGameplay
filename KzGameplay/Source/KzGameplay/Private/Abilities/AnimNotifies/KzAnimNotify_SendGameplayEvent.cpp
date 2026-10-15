// Copyright 2026 kirzo

#include "Abilities/AnimNotifies/KzAnimNotify_SendGameplayEvent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"

UKzAnimNotify_SendGameplayEvent::UKzAnimNotify_SendGameplayEvent()
{
#if WITH_EDITORONLY_DATA
	// Default color for GAS-related notifies (a nice light blue)
	NotifyColor = FColor(100, 149, 237, 255);
#endif
}

void UKzAnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && EventTag.IsValid())
	{
		if (AActor* OwnerActor = MeshComp->GetOwner())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
			{
				FGameplayEventData Payload;
				Payload.EventTag = EventTag;
				Payload.Instigator = OwnerActor;
				Payload.Target = OwnerActor;
				Payload.OptionalObject = Animation; // We pass the animation as optional context

				// Send the event directly to the ASC
				ASC->HandleGameplayEvent(EventTag, &Payload);
			}
		}
	}
}

FString UKzAnimNotify_SendGameplayEvent::GetNotifyName_Implementation() const
{
	if (EventTag.IsValid())
	{
		return EventTag.ToString();
	}

	return Super::GetNotifyName_Implementation();
}