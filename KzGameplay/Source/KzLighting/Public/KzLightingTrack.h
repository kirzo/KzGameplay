// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneNameableTrack.h"
#include "Sections/MovieSceneHookSection.h"
#include "KzLightingTrack.generated.h"

class AKzLightingRig;
class UKzLightingProfile;

/**
 * A lighting profile over a stretch of a sequence, brought in as a layer of the world's lighting rig and weighted by the section's easing.
 * Sections of a track that overlap crossfade between their profiles. When the section stops evaluating its layer goes,
 * unless the section keeps state in game.
 */
UCLASS()
class KZLIGHTING_API UKzLightingSection : public UMovieSceneHookSection
{
	GENERATED_BODY()

public:
	UKzLightingSection(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, Category = "Lighting")
	TObjectPtr<UKzLightingProfile> Profile;

protected:
	virtual void Begin(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const override;
	virtual void Update(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const override;
	virtual void End(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const override;

private:
	/** The track's sections evaluated at Time, bottom to top: a later one blends over an earlier one. */
	TArray<const UKzLightingSection*, TInlineAllocator<4>> GetSectionsAt(FFrameTime Time) const;

	/**
	 * This section's layer weight at Time. Stacked bottom to top, the track's layers cover what lies below by their total easing,
	 * with a mix of their profiles in proportion to it, so overlapping sections crossfade instead of dipping towards what lies below.
	 */
	float GetLayerWeight(FFrameTime Time) const;

	void PushLayer(AKzLightingRig& Rig, FFrameTime Time) const;
};

/** Blends lighting profiles over a sequence, through the world's lighting rig. */
UCLASS()
class KZLIGHTING_API UKzLightingTrack : public UMovieSceneNameableTrack
{
	GENERATED_BODY()

public:
	UKzLightingTrack(const FObjectInitializer& ObjectInitializer);

	/** Where the track's layers go among the rig's others. Higher goes on top. */
	UPROPERTY(EditAnywhere, Category = "Lighting")
	int32 Priority = 100;

	virtual void AddSection(UMovieSceneSection& Section) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual bool SupportsMultipleRows() const override { return true; }
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
	virtual bool HasSection(const UMovieSceneSection& Section) const override;
	virtual bool IsEmpty() const override;
	virtual void RemoveSection(UMovieSceneSection& Section) override;
	virtual void RemoveSectionAt(int32 SectionIndex) override;
	virtual void RemoveAllAnimationData() override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDefaultDisplayName() const override;
#endif

private:
	UPROPERTY()
	TArray<TObjectPtr<UMovieSceneSection>> Sections;
};