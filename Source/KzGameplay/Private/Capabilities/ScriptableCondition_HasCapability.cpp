// Copyright 2026 kirzo

#include "Capabilities/ScriptableCondition_HasCapability.h"
#include "Capabilities/KzCapabilityComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

bool UScriptableCondition_HasCapability::Evaluate_Implementation() const
{
	if (!IsValid(TargetActor) || !Capability.IsValid())
	{
		return false;
	}

	if (bMustBeUsableNow)
	{
		// Only the component knows what implements a capability, so only it can say whether that would run
		if (const UKzCapabilityComponent* Capabilities = TargetActor->FindComponentByClass<UKzCapabilityComponent>())
		{
			return Capabilities->CanUseCapability(Capability);
		}
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	return ASC && ASC->HasMatchingGameplayTag(Capability);
}

#if WITH_EDITOR
FText UScriptableCondition_HasCapability::GetDisplayTitle() const
{
	FString TargetName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_HasCapability, TargetActor), TargetName))
	{
		TargetName = TargetActor ? TargetActor->GetActorLabel() : TEXT("None");
	}

	if (Capability.IsValid())
	{
		const FText Verb = bMustBeUsableNow ? INVTEXT("Can {0} {1} right now?") : INVTEXT("Can {0} {1}?");
		return FText::Format(Verb, FText::FromString(TargetName), FText::FromName(Capability.GetTagLeafName()));
	}

	return INVTEXT("Has Capability...?");
}
#endif