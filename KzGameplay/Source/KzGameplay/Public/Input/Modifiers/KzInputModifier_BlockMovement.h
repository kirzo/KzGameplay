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

public:
	/** Blocking the movement does not change what the player asks for. */
	virtual FVector PreviewInput(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) override { return CurrentInput; }

protected:
	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) override
	{
		return FVector::ZeroVector;
	}
};