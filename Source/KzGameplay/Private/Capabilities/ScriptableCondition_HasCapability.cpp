// Copyright 2026 kirzo

#include "Capabilities/ScriptableCondition_HasCapability.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

bool UScriptableCondition_HasCapability::Evaluate_Implementation() const
{
	if (!IsValid(TargetActor) || !Capability.IsValid())
	{
		return false;
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
		return FText::Format(INVTEXT("Can {0} [{1}]"), FText::FromString(TargetName), FText::FromName(Capability.GetTagName()));
	}

	return INVTEXT("Has Capability");
}
#endif
