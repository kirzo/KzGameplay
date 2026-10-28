// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KzTimeDilationSubsystem.generated.h"

/**
 * Slows or freezes single actors for a while, such as both sides of a hit or a parried attacker. Overlapping requests on an actor keep the slowest,
 * and the actor gets its own dilation back once they all run out, whoever started them.
 */
UCLASS()
class KZGAMEPLAY_API UKzTimeDilationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Runs Actor at Dilation of its speed for Duration seconds of the world's time. */
	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	void Dilate(AActor* Actor, float Dilation, float Duration);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	struct FRequest
	{
		float Dilation = 1.0f;
		float TimeLeft = 0.0f;
	};

	struct FDilated
	{
		/** The actor's own dilation, given back at the end. */
		float BaseDilation = 1.0f;
		TArray<FRequest> Requests;
	};

	TMap<TWeakObjectPtr<AActor>, FDilated> Actors;
};