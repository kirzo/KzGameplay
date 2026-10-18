// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_Capture.h"

UKzInputModifier_Capture::UKzInputModifier_Capture()
{
	InstancingPolicy = EKzInputModifierInstancing::PerOwner;
}

FVector UKzInputModifier_Capture::GetIntent() const
{
	return GFrameCounter - IntentFrame <= 1 ? Intent : FVector::ZeroVector;
}

FVector UKzInputModifier_Capture::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput)
{
	Intent = FVector(CurrentInput.X, CurrentInput.Y, 0.0);
	IntentFrame = GFrameCounter;
	return FVector::ZeroVector;
}