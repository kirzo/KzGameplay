// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "KzGameplayCueNotify_Burst.generated.h"

/**
 * A GCN Burst that can also shake the mesh of the actor it plays on, along the blow, such as a body taking a hit. Not the attacker's: its own cues,
 * and those of hits on what has no ability system, play on it and do not shake. Nor one with State.SuperArmor, which holds steady.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kz Burst", ShortTooltip = "A GCN Burst that can also shake the target's mesh."))
class KZGAMEPLAY_API UKzGameplayCueNotify_Burst : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:
	/** How far the mesh moves at first; zero does not shake it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh Shake", meta = (DisplayName = "Amplitude", ClampMin = "0", Units = "Centimeters"))
	float MeshShakeAmplitude = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh Shake", meta = (DisplayName = "Duration", ClampMin = "0", Units = "Seconds"))
	float MeshShakeDuration = 0.15f;

	/** How many times a second it goes back and forth: low sways, high trembles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh Shake", meta = (DisplayName = "Frequency", ClampMin = "0", Units = "Hertz"))
	float MeshShakeFrequency = 30.0f;

	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
};