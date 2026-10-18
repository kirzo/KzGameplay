// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Input/KzInputModifier.h"
#include "KzInputModifier_Capture.generated.h"

/**
 * Takes the avatar's movement input for something else to use, such as an object the player pushes or a cart they steer: keeps it as an
 * intent its owner reads, and lets none of it move the avatar. Goes last in the stack, to capture what the other modifiers leave.
 */
UCLASS(meta = (DisplayName = "Input Capture"))
class KZGAMEPLAY_API UKzInputModifier_Capture : public UKzInputModifier
{
	GENERATED_BODY()

public:
	UKzInputModifier_Capture();

	/** The input captured this frame or the last, flat and with its analog magnitude; zero once it stops coming, as a released stick sends nothing. */
	UFUNCTION(BlueprintPure, Category = "Input Modifier")
	FVector GetIntent() const;

protected:
	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) override;

private:
	FVector Intent = FVector::ZeroVector;
	uint64 IntentFrame = 0;
};