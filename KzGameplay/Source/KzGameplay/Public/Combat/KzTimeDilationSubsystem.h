// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Curves/RichCurve.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "KzTimeDilationSubsystem.generated.h"

class UKzTimeStop;

/**
 * Slows or freezes single actors for a while, such as both sides of a hit, or the whole world, such as a slow motion. Overlapping requests keep the
 * slowest, and the actor or the world gets its own dilation back once they all run out, whoever started them.
 */
UCLASS()
class KZGAMEPLAY_API UKzTimeDilationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Runs Actor at Dilation of its speed for Duration seconds of the world's time; zero freezes it. */
	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	void Dilate(AActor* Actor, float Dilation, float Duration);

	/** Holds Actor along Stop's curve for Duration of the world's time, after its delay. An ability with any of CancelableBy lets go of it. */
	void Hold(AActor* Actor, const UKzTimeStop& Stop, float Duration, const FGameplayTagContainer& CancelableBy = FGameplayTagContainer());

	/** Slows the whole world along Stop's curve for Duration of real time, after its delay, so the slowdown does not stretch itself. */
	void HoldWorld(const UKzTimeStop& Stop, float Duration);

	/** Slows the whole world to Dilation for as long as Source keeps it on Owner, such as an animation window on its mesh, until cleared. */
	void SetWorldDilation(const UObject* Source, const UObject* Owner, float Dilation);
	void ClearWorldDilation(const UObject* Source, const UObject* Owner);

	/** Lets go of Actor's holds that an ability with AbilityTags cancels, as it activates. */
	void Cancel(AActor* Actor, const FGameplayTagContainer& AbilityTags);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	struct FRequest
	{
		/** For the whole of it, without curve keys. */
		float Dilation = 1.0f;

		/** How strongly it holds along the way, from 1, frozen, to 0, not at all; takes Dilation's place when it has keys. */
		FRichCurve Curve;

		float Duration = 0.0f;
		float TimeLeft = 0.0f;

		/** Until it runs out, the request holds nothing. */
		float DelayLeft = 0.0f;

		FGameplayTagContainer CancelableBy;

		float GetDilation() const;

		/** Counts Delta off its delay, then off its time; false once over. */
		bool Advance(float Delta);
	};

	struct FDilated
	{
		/** The actor's own dilation, given back at the end. */
		float BaseDilation = 1.0f;
		TArray<FRequest> Requests;
	};

	static FRequest MakeRequest(const UKzTimeStop& Stop, float Duration);

	/** Counts Delta off each request, dropping those over. */
	static void AdvanceRequests(TArray<FRequest>& Requests, float Delta);

	void AddRequest(AActor* Actor, FRequest&& Request);

	/** Sets the actor's dilation to the slowest of its requests. */
	static void Apply(AActor& Actor, const FDilated& Dilated);

	/** Sets the world's dilation to the slowest of its requests and windows, or gives back its own once there are none. */
	void ApplyWorld();

	TMap<TWeakObjectPtr<AActor>, FDilated> Actors;

	TArray<FRequest> WorldRequests;
	TMap<TPair<FObjectKey, FObjectKey>, float> WorldDilations;

	/** The world's own dilation while this one dilates it, given back at the end. */
	TOptional<float> BaseWorldDilation;
};