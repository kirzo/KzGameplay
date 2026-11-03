// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KzMeshShakeSubsystem.generated.h"

/** Shakes meshes back and forth for a moment, such as a body taking a hit. Runs on the world's time, so a body frozen by a hit stop still shakes. */
UCLASS()
class KZGAMEPLAY_API UKzMeshShakeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Shakes the actor's mesh, the character's own or its first one, fading out over Duration. A new shake on a mesh still shaking replaces it.
	 * @param Direction The way it moves first, such as the way the hit pushes.
	 * @param Amplitude How far it moves at first, in centimeters.
	 * @param Frequency How many times a second it goes back and forth.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	static void ShakeMesh(AActor* Actor, FVector Direction, float Amplitude = 3.0f, float Duration = 0.15f, float Frequency = 30.0f);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	struct FShake
	{
		/** Where the mesh sits still, given back at the end. */
		FVector BaseLocation = FVector::ZeroVector;
		FVector Direction = FVector::ZeroVector;
		float Amplitude = 0.0f;
		float Duration = 0.0f;
		float Frequency = 0.0f;
		float Time = 0.0f;
	};

	TMap<TWeakObjectPtr<USceneComponent>, FShake> Shakes;
};
