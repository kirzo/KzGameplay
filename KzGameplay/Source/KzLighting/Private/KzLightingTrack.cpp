// Copyright 2026 kirzo

#include "KzLightingTrack.h"
#include "Engine/World.h"
#include "EntitySystem/MovieSceneSharedPlaybackState.h"
#include "KzLightingRig.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneSequence.h"

#define LOCTEXT_NAMESPACE "KzLightingTrack"

namespace
{
	AKzLightingRig* FindRig(const TSharedRef<UE::MovieScene::FSharedPlaybackState>& SharedPlaybackState)
	{
		const UObject* PlaybackContext = SharedPlaybackState->GetPlaybackContext();
		return PlaybackContext ? AKzLightingRig::Get(PlaybackContext) : nullptr;
	}

	FFrameNumber StartOf(const UMovieSceneSection& Section)
	{
		const TRange<FFrameNumber> Range = Section.GetRange();
		return Range.HasLowerBound() ? Range.GetLowerBoundValue() : FFrameNumber(TNumericLimits<int32>::Lowest());
	}
}

UKzLightingSection::UKzLightingSection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bRequiresRangedHook = true;
}

TArray<const UKzLightingSection*, TInlineAllocator<4>> UKzLightingSection::GetSectionsAt(FFrameTime Time) const
{
	TArray<const UKzLightingSection*, TInlineAllocator<4>> Result;
	if (const UMovieSceneTrack* Track = GetTypedOuter<UMovieSceneTrack>())
	{
		for (const UMovieSceneSection* Section : Track->GetAllSections())
		{
			const UKzLightingSection* LightingSection = Cast<UKzLightingSection>(Section);
			if (LightingSection && LightingSection->IsActive() && LightingSection->GetRange().Contains(Time.FrameNumber))
			{
				Result.Add(LightingSection);
			}
		}
	}

	Result.StableSort([](const UKzLightingSection& A, const UKzLightingSection& B) { return StartOf(A) < StartOf(B); });
	return Result;
}

float UKzLightingSection::GetLayerWeight(FFrameTime Time) const
{
	const TArray<const UKzLightingSection*, TInlineAllocator<4>> Sections = GetSectionsAt(Time);
	const int32 Index = Sections.IndexOfByKey(this);
	if (Index == INDEX_NONE) return 0.0f;

	float Total = 0.0f;
	float Above = 0.0f;
	for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
	{
		const float SectionEasing = Sections[SectionIndex]->EvaluateEasing(Time);
		Total += SectionEasing;
		if (SectionIndex > Index)
		{
			Above += SectionEasing;
		}
	}
	if (Total <= 0.0f) return 0.0f;

	// Layer i of n weighs Coverage * Share(i) / (1 - Coverage * Share(above i)): stacked, they give Coverage * Share(i) of each profile.
	const float Coverage = FMath::Min(Total, 1.0f);
	const float Remaining = 1.0f - Coverage * Above / Total;
	return Remaining > UE_SMALL_NUMBER ? FMath::Clamp(Coverage * EvaluateEasing(Time) / Total / Remaining, 0.0f, 1.0f) : 0.0f;
}

void UKzLightingSection::PushLayer(AKzLightingRig& Rig, FFrameTime Time) const
{
	const UKzLightingTrack* Track = GetTypedOuter<UKzLightingTrack>();

	FKzLightingLayerSettings Settings;
	Settings.Profile = Profile;
	Settings.Priority = Track ? Track->Priority : 0;
	Settings.bOverrideBlendTime = true;
	Settings.BlendTime = 0.0f;

	// The section is the layer's source: the rig only reads it.
	UKzLightingSection* Source = const_cast<UKzLightingSection*>(this);
	Rig.PushLayer(Source, Settings, true);
	Rig.SetLayerAlpha(Source, GetLayerWeight(Time));
}

void UKzLightingSection::Begin(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const
{
	if (AKzLightingRig* Rig = FindRig(SharedPlaybackState))
	{
		// Every section here is pushed again in order, so the layers stack as the sections do whichever began first.
		const FFrameTime Time = Params.Context.GetTime();
		for (const UKzLightingSection* Section : GetSectionsAt(Time))
		{
			Section->PushLayer(*Rig, Time);
		}
	}
}

void UKzLightingSection::Update(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const
{
	if (AKzLightingRig* Rig = FindRig(SharedPlaybackState))
	{
		Rig->SetLayerAlpha(const_cast<UKzLightingSection*>(this), GetLayerWeight(Params.Context.GetTime()));
	}
}

void UKzLightingSection::End(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const
{
	AKzLightingRig* Rig = FindRig(SharedPlaybackState);
	if (!Rig) return;

	EMovieSceneCompletionMode CompletionMode = GetCompletionMode();
	if (CompletionMode == EMovieSceneCompletionMode::ProjectDefault)
	{
		const UMovieSceneSequence* Sequence = GetTypedOuter<UMovieSceneSequence>();
		CompletionMode = Sequence ? Sequence->DefaultCompletionMode : EMovieSceneCompletionMode::RestoreState;
	}

	// Keeping state leaves the layer as it was, in game only: the editor always cleans up.
	if (CompletionMode == EMovieSceneCompletionMode::KeepState && Rig->GetWorld()->IsGameWorld()) return;

	Rig->PopLayer(const_cast<UKzLightingSection*>(this), true);
}

UKzLightingTrack::UKzLightingTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(245, 197, 66, 65);
#endif

	// Sequencer draws the easing handles only on sections with a blend type; this one isn't used to evaluate.
	SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
}

void UKzLightingTrack::AddSection(UMovieSceneSection& Section)
{
	if (Section.IsA<UKzLightingSection>())
	{
		Sections.Add(&Section);
	}
}

bool UKzLightingTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UKzLightingSection::StaticClass();
}

UMovieSceneSection* UKzLightingTrack::CreateNewSection()
{
	UKzLightingSection* Section = NewObject<UKzLightingSection>(this, NAME_None, RF_Transactional);
	Section->SetBlendType(EMovieSceneBlendType::Absolute);
	return Section;
}

const TArray<UMovieSceneSection*>& UKzLightingTrack::GetAllSections() const
{
	return Sections;
}

bool UKzLightingTrack::HasSection(const UMovieSceneSection& Section) const
{
	return Sections.Contains(&Section);
}

bool UKzLightingTrack::IsEmpty() const
{
	return Sections.IsEmpty();
}

void UKzLightingTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
}

void UKzLightingTrack::RemoveSectionAt(int32 SectionIndex)
{
	Sections.RemoveAt(SectionIndex);
	MovieSceneHelpers::SortConsecutiveSections(MutableView(Sections));
}

void UKzLightingTrack::RemoveAllAnimationData()
{
	Sections.Empty();
}

#if WITH_EDITORONLY_DATA
FText UKzLightingTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("TrackName", "Lighting");
}
#endif

#undef LOCTEXT_NAMESPACE