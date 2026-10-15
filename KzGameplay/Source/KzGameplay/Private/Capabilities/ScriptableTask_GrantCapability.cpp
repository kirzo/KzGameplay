// Copyright 2026 kirzo

#include "Capabilities/ScriptableTask_GrantCapability.h"
#include "Capabilities/KzCapabilityComponent.h"
#include "GameFramework/Actor.h"

void UScriptableTask_GrantCapability::BeginTask()
{
	if (IsValid(TargetActor) && Capability.IsValid())
	{
		if (UKzCapabilityComponent* Capabilities = TargetActor->FindComponentByClass<UKzCapabilityComponent>())
		{
			// Granting what the target already has is a no-op there, so remember whether it was ours to undo
			bGranted = !Capabilities->HasCapability(Capability);
			Capabilities->GrantCapability(Capability, SourceSet);
		}
	}

	Finish();
}

void UScriptableTask_GrantCapability::ResetTask()
{
	if (!bRevertOnReset || !bGranted || !IsValid(TargetActor))
	{
		return;
	}

	if (UKzCapabilityComponent* Capabilities = TargetActor->FindComponentByClass<UKzCapabilityComponent>())
	{
		Capabilities->RevokeCapability(Capability);
	}

	bGranted = false;
}

#if WITH_EDITOR
FText UScriptableTask_GrantCapability::GetDisplayTitle() const
{
	FString TargetName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_GrantCapability, TargetActor), TargetName))
	{
		TargetName = TargetActor ? TargetActor->GetActorLabel() : TEXT("None");
	}

	if (Capability.IsValid())
	{
		return FText::Format(INVTEXT("Grant [{0}] to {1}"), FText::FromName(Capability.GetTagName()), FText::FromString(TargetName));
	}

	return INVTEXT("Grant Capability");
}
#endif
