// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Input/KzInputModifier.h"
#include "KzInputModifier_PrimitiveSweep.generated.h"

/**
 * Input modifier that performs a predictive sweep using a primitive component.
 * If the sweep hits an obstacle, it modifies the input to prevent penetration
 * and allow sliding along the surface.
 */
UCLASS(meta = (DisplayName = "Primitive Sweep Constraint"))
class KZGAMEPLAY_API UKzInputModifier_PrimitiveSweep : public UKzInputModifier
{
	GENERATED_BODY()

public:
	/** The primitive component that will act as the "collision sensor". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sweep", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UPrimitiveComponent> SweepPrimitive;

	/** Seconds of movement to look ahead. The reach in cm is this times the avatar current top speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sweep", meta = (ExposeOnSpawn = "true"))
	float LookAheadTime = 0.1f;

	/** Additional actors to ignore during the sweep. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sweep", meta = (ExposeOnSpawn = "true"))
	TArray<TObjectPtr<AActor>> IgnoredActors;

#if WITH_EDITORONLY_DATA
	/** Draws the sweep shape, the swept path and any hit normals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sweep")
	bool bDrawDebug = false;
#endif

protected:
	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) override;
};