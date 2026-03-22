// Copyright 2026 kirzo

#include "Input/KzInputProfile.h"

const FKzInputAction* UKzInputProfile::FindActionConfigForTag(const FGameplayTag& InputTag) const
{
	for (const FKzInputAction& Action : InputActions)
	{
		if (Action.InputTag == InputTag)
		{
			return &Action;
		}
	}
	return nullptr;
}

const UInputAction* UKzInputProfile::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FKzInputAction& Action : InputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find NativeInputAction for InputTag [%s] on InputProfile [%s]."), *InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}