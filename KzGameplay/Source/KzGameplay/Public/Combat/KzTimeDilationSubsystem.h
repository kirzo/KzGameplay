// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Curves/RichCurve.h"
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
	/** Runs Actor at Dilation of its speed for Duration seconds of the world's time; zero freezes it. */
	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	void Dilate(AActor* Actor, float Dilation, float Duration);

	/** Runs Actor along Curve for Duration seconds of the world's time: X from 0 to 1 through it, its speed on Y, from 0, frozen, to 1. */
	void DilateAlong(AActor* Actor, const FRichCurve& Curve, float Duration);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	struct FRequest
	{
		float Dilation = 1.0f;

		/** Takes Dilation's place when it has keys. */
		FRichCurve Curve;

		float Duration = 0.0f;
		float TimeLeft = 0.0f;

		float GetDilation() const;
	};

	struct FDilated
	{
		/** The actor's own dilation, given back at the end. */
		float BaseDilation = 1.0f;
		TArray<FRequest> Requests;
	};

	void AddRequest(AActor* Actor, FRequest&& Request);

	/** Sets the actor's dilation to the slowest of its requests. */
	static void Apply(AActor& Actor, const FDilated& Dilated);

	TMap<TWeakObjectPtr<AActor>, FDilated> Actors;
};