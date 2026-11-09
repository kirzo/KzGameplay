// Copyright 2026 kirzo

#include "Factions/ScriptableCondition_Attitude.h"
#include "Factions/KzFactionLibrary.h"
#include "GameFramework/Actor.h"

bool UScriptableCondition_Attitude::Evaluate_Implementation() const
{
	return IsValid(SourceActor) && IsValid(TargetActor) && UKzFactionLibrary::GetAttitude(SourceActor, TargetActor) == Attitude;
}

#if WITH_EDITOR
FText UScriptableCondition_Attitude::GetDisplayTitle() const
{
	FString SourceName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_Attitude, SourceActor), SourceName))
	{
		SourceName = SourceActor ? SourceActor->GetActorLabel() : TEXT("None");
	}

	FString TargetName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_Attitude, TargetActor), TargetName))
	{
		TargetName = TargetActor ? TargetActor->GetActorLabel() : TEXT("None");
	}

	return FText::Format(INVTEXT("Is {0} {1} to {2}?"), FText::FromString(TargetName), StaticEnum<EKzAttitude>()->GetDisplayValueAsText(Attitude), FText::FromString(SourceName));
}
#endif
