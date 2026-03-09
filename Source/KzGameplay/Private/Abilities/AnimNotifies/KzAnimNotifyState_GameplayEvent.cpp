// Copyright 2026 kirzo

#include "Abilities/AnimNotifies/KzAnimNotifyState_GameplayEvent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"

UKzAnimNotifyState_GameplayEvent::UKzAnimNotifyState_GameplayEvent()
{
#if WITH_EDITORONLY_DATA
	// Default color for GAS-related notifies (a nice light blue)
	NotifyColor = FColor(100, 149, 237, 255);
#endif
}

void UKzAnimNotifyState_GameplayEvent::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && BeginEventTag.IsValid())
	{
		if (AActor* OwnerActor = MeshComp->GetOwner())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
			{
				FGameplayEventData Payload;
				Payload.EventTag = BeginEventTag;
				Payload.Instigator = OwnerActor;
				Payload.Target = OwnerActor;
				Payload.OptionalObject = Animation; // Optional context

				ASC->HandleGameplayEvent(BeginEventTag, &Payload);
			}
		}
	}
}

void UKzAnimNotifyState_GameplayEvent::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && EndEventTag.IsValid())
	{
		if (AActor* OwnerActor = MeshComp->GetOwner())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
			{
				FGameplayEventData Payload;
				Payload.EventTag = EndEventTag;
				Payload.Instigator = OwnerActor;
				Payload.Target = OwnerActor;
				Payload.OptionalObject = Animation;

				ASC->HandleGameplayEvent(EndEventTag, &Payload);
			}
		}
	}
}

FString UKzAnimNotifyState_GameplayEvent::GetNotifyName_Implementation() const
{
	if (BeginEventTag.IsValid() || EndEventTag.IsValid())
	{
		// Formats the name to look like "Event.Start -> Event.End" in the timeline
		FString BeginStr = BeginEventTag.IsValid() ? BeginEventTag.ToString() : TEXT("None");
		FString EndStr = EndEventTag.IsValid() ? EndEventTag.ToString() : TEXT("None");
		return FString::Printf(TEXT("%s -> %s"), *BeginStr, *EndStr);
	}

	return Super::GetNotifyName_Implementation();
}