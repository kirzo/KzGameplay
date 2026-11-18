// Copyright 2026 kirzo

#include "Sequencer/KzLightingTrackEditor.h"
#include "ContentBrowserModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IContentBrowserSingleton.h"
#include "ISequencer.h"
#include "ISequencerSection.h"
#include "KzLightingProfile.h"
#include "KzLightingTrack.h"
#include "MovieScene.h"
#include "MovieSceneSequence.h"
#include "MovieSceneTimeHelpers.h"
#include "MovieSceneToolHelpers.h"
#include "MVVM/Views/ViewUtilities.h"
#include "ScopedTransaction.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "KzLightingTrackEditor"

namespace
{
	/** A section shows the name of its profile. */
	class FKzLightingSectionInterface : public FSequencerSection
	{
	public:
		using FSequencerSection::FSequencerSection;

		virtual FText GetSectionTitle() const override
		{
			const UKzLightingSection* Section = Cast<UKzLightingSection>(WeakSection.Get());
			return Section && Section->Profile ? FText::FromName(Section->Profile->GetFName()) : LOCTEXT("NoProfile", "No Profile");
		}
	};
}

FText FKzLightingTrackEditor::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Lighting");
}

void FKzLightingTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddTrack", "Lighting Track"),
		LOCTEXT("AddTrackTooltip", "Blends lighting profiles in and out over the sequence, through the level's lighting rig."),
		FSlateIconFinder::FindIconForClass(UKzLightingProfile::StaticClass()),
		FUIAction(FExecuteAction::CreateLambda([this]() { AddLightingTrack(); })));
}

TSharedPtr<SWidget> FKzLightingTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	TWeakObjectPtr<UKzLightingTrack> WeakTrack = Cast<UKzLightingTrack>(Track);
	return UE::Sequencer::MakeAddButton(LOCTEXT("AddSection", "Profile"), FOnGetContent::CreateSP(this, &FKzLightingTrackEditor::BuildProfilePicker, WeakTrack), Params.ViewModel);
}

TSharedRef<ISequencerSection> FKzLightingTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	return MakeShared<FKzLightingSectionInterface>(SectionObject);
}

bool FKzLightingTrackEditor::HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid)
{
	UKzLightingProfile* Profile = Cast<UKzLightingProfile>(Asset);
	UMovieScene* MovieScene = GetFocusedMovieScene();
	if (!Profile || TargetObjectGuid.IsValid() || !MovieScene || MovieScene->IsReadOnly()) return false;

	const FScopedTransaction Transaction(LOCTEXT("AddProfileTransaction", "Add Lighting Profile"));

	// Onto the sequence's lighting track, made if there is none.
	UKzLightingTrack* Track = nullptr;
	for (UMovieSceneTrack* Existing : MovieScene->GetTracks())
	{
		if (UKzLightingTrack* LightingTrack = Cast<UKzLightingTrack>(Existing))
		{
			Track = LightingTrack;
			break;
		}
	}

	if (!Track)
	{
		Track = AddLightingTrack();
	}

	if (Track)
	{
		AddSection(*Track, Profile);
	}
	return Track != nullptr;
}

bool FKzLightingTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return Type == UKzLightingTrack::StaticClass();
}

bool FKzLightingTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	return InSequence && InSequence->IsTrackSupported(UKzLightingTrack::StaticClass()) != ETrackSupport::NotSupported;
}

const FSlateBrush* FKzLightingTrackEditor::GetIconBrush() const
{
	return FSlateIconFinder::FindIconBrushForClass(UKzLightingProfile::StaticClass());
}

UKzLightingTrack* FKzLightingTrackEditor::AddLightingTrack()
{
	UMovieScene* MovieScene = GetFocusedMovieScene();
	if (!MovieScene || MovieScene->IsReadOnly()) return nullptr;

	const FScopedTransaction Transaction(LOCTEXT("AddTrackTransaction", "Add Lighting Track"));
	MovieScene->Modify();

	UKzLightingTrack* Track = MovieScene->AddTrack<UKzLightingTrack>();
	if (TSharedPtr<ISequencer> Sequencer = GetSequencer())
	{
		Sequencer->OnAddTrack(Track, FGuid());
	}
	return Track;
}

void FKzLightingTrackEditor::AddSection(UKzLightingTrack& Track, UKzLightingProfile* Profile)
{
	TSharedPtr<ISequencer> Sequencer = GetSequencer();
	UMovieScene* MovieScene = GetFocusedMovieScene();
	if (!Sequencer || !MovieScene || MovieScene->IsReadOnly()) return;

	const FScopedTransaction Transaction(LOCTEXT("AddSectionTransaction", "Add Lighting Section"));
	Track.Modify();

	UKzLightingSection* Section = CastChecked<UKzLightingSection>(Track.CreateNewSection());
	Section->Profile = Profile;

	// From now to the end of the playback range, or a few seconds when that is already behind.
	const FFrameNumber Start = Sequencer->GetLocalTime().Time.FrameNumber;
	const FFrameNumber PlaybackEnd = UE::MovieScene::DiscreteExclusiveUpper(MovieScene->GetPlaybackRange());
	const FFrameNumber End = PlaybackEnd > Start ? PlaybackEnd : Start + MovieScene->GetTickResolution().AsFrameNumber(5.0);
	Section->SetRange(TRange<FFrameNumber>(Start, End));
	Section->SetRowIndex(MovieSceneToolHelpers::FindAvailableRowIndex(&Track, Section));
	Track.AddSection(*Section);

	Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	Sequencer->EmptySelection();
	Sequencer->SelectSection(Section);
	Sequencer->ThrobSectionSelection();
}

TSharedRef<SWidget> FKzLightingTrackEditor::BuildProfilePicker(TWeakObjectPtr<UKzLightingTrack> WeakTrack)
{
	FAssetPickerConfig Config;
	Config.Filter.ClassPaths.Add(UKzLightingProfile::StaticClass()->GetClassPathName());
	Config.Filter.bRecursiveClasses = true;
	Config.OnAssetSelected = FOnAssetSelected::CreateSP(this, &FKzLightingTrackEditor::OnProfilePicked, WeakTrack);
	Config.bAllowNullSelection = false;
	Config.InitialAssetViewType = EAssetViewType::List;
	Config.SaveSettingsName = TEXT("SequencerAssetPicker");

	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	return SNew(SBox)
		.WidthOverride(300.0f)
		.HeightOverride(300.0f)
		[
			ContentBrowser.Get().CreateAssetPicker(Config)
		];
}

void FKzLightingTrackEditor::OnProfilePicked(const FAssetData& AssetData, TWeakObjectPtr<UKzLightingTrack> WeakTrack)
{
	FSlateApplication::Get().DismissAllMenus();

	UKzLightingTrack* Track = WeakTrack.Get();
	if (UKzLightingProfile* Profile = Cast<UKzLightingProfile>(AssetData.GetAsset()); Track && Profile)
	{
		AddSection(*Track, Profile);
	}
}

#undef LOCTEXT_NAMESPACE