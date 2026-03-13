// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Input/KzInputModifier.h"
#include "Misc/KzTransformSource.h"
#include "KzInputModifier_Tether.generated.h"

/**
 * Input modifier that acts as a physical tether.
 * Applies centripetal correction to the input when high velocity threatens to breach the boundary,
 * ensuring smooth orbital movement around the anchor.
 */
UCLASS(meta = (DisplayName = "Tether Constraint (Centripetal)"))
class KZGAMEPLAY_API UKzInputModifier_Tether : public UKzInputModifier
{
	GENERATED_BODY()

public:
	/** The anchor point the player is tethered to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tether", meta = (ExposeOnSpawn = "true"))
	FKzTransformSource AnchorSource;

	/** The maximum allowed absolute distance from the anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tether", meta = (ExposeOnSpawn = "true", ClampMin = 0.0))
	float MaxDistance = 150.0f;

protected:
	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const override;
};