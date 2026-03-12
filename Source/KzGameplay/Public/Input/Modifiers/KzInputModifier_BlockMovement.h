// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Input/KzInputModifier.h"
#include "KzInputModifier_BlockMovement.generated.h"

/** Modifies the input to be completely zero. */
UCLASS(meta = (DisplayName = "Block Movement Constraint"))
class KZGAMEPLAY_API UKzInputModifier_BlockMovement : public UKzInputModifier
{
	GENERATED_BODY()

protected:
	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const override
	{
		return FVector::ZeroVector;
	}
};