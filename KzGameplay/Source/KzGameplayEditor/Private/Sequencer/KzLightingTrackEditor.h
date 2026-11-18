// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneTrackEditor.h"

struct FAssetData;
class UKzLightingProfile;
class UKzLightingTrack;

/**
 * Sequencer's Lighting track: added from the track menu, or by dropping a lighting profile into a sequence.
 * Its "+" button adds a section with the profile picked, from the current time to the end of the playback range.
 */
class FKzLightingTrackEditor : public FMovieSceneTrackEditor
{
public:
	explicit FKzLightingTrackEditor(TSharedRef<ISequencer> InSequencer) : FMovieSceneTrackEditor(InSequencer) {}

	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> InSequencer) { return MakeShared<FKzLightingTrackEditor>(InSequencer); }

	virtual FText GetDisplayName() const override;
	virtual void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override;
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
	virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;
	virtual const FSlateBrush* GetIconBrush() const override;

private:
	UKzLightingTrack* AddLightingTrack();
	void AddSection(UKzLightingTrack& Track, UKzLightingProfile* Profile);
	TSharedRef<SWidget> BuildProfilePicker(TWeakObjectPtr<UKzLightingTrack> WeakTrack);
	void OnProfilePicked(const FAssetData& AssetData, TWeakObjectPtr<UKzLightingTrack> WeakTrack);
};