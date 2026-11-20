// Copyright 2026 kirzo

#include "Combat/KzTimeDilationSubsystem.h"
#include "Combat/KzTimeStop.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"

namespace
{
	// A frozen actor still ticks this slowly, a millisecond each second: a zero delta breaks what divides by it, such as an AnimBlueprint's turn rate.
	constexpr float MinDilation = 0.001f;
}

void UKzTimeDilationSubsystem::AdvanceRequests(TArray<FRequest>& Requests, float Delta)
{
	for (int32 Index = Requests.Num() - 1; Index >= 0; --Index)
	{
		if (!Requests[Index].Advance(Delta))
		{
			Requests.RemoveAtSwap(Index);
		}
	}
}

float UKzTimeDilationSubsystem::FRequest::GetDilation() const
{
	if (DelayLeft > 0.0f)
	{
		return 1.0f;
	}
	const float Value = Curve.IsEmpty() || Duration <= 0.0f ? Dilation : 1.0f - Curve.Eval(FMath::Clamp(1.0f - TimeLeft / Duration, 0.0f, 1.0f));
	return FMath::Clamp(Value, MinDilation, 1.0f);
}

bool UKzTimeDilationSubsystem::FRequest::Advance(float Delta)
{
	if (DelayLeft > 0.0f)
	{
		DelayLeft -= Delta;
		return true;
	}
	TimeLeft -= Delta;
	return TimeLeft > 0.0f;
}

UKzTimeDilationSubsystem::FRequest UKzTimeDilationSubsystem::MakeRequest(const UKzTimeStop& Stop, float Duration)
{
	FRequest Request;
	Request.Dilation = 0.0f;
	if (const FRichCurve* Curve = Stop.Curve.GetRichCurveConst())
	{
		Request.Curve = *Curve;
	}
	Request.Duration = Duration;
	Request.TimeLeft = Duration;
	Request.DelayLeft = Stop.Delay;
	return Request;
}

void UKzTimeDilationSubsystem::Dilate(AActor* Actor, float Dilation, float Duration)
{
	if (Dilation >= 1.0f)
	{
		return;
	}

	FRequest Request;
	Request.Dilation = Dilation;
	Request.Duration = Duration;
	Request.TimeLeft = Duration;
	AddRequest(Actor, MoveTemp(Request));
}

void UKzTimeDilationSubsystem::Hold(AActor* Actor, const UKzTimeStop& Stop, float Duration, const FGameplayTagContainer& CancelableBy)
{
	FRequest Request = MakeRequest(Stop, Duration);
	Request.CancelableBy = CancelableBy;
	AddRequest(Actor, MoveTemp(Request));
}

void UKzTimeDilationSubsystem::HoldWorld(const UKzTimeStop& Stop, float Duration)
{
	if (Duration > 0.0f)
	{
		WorldRequests.Add(MakeRequest(Stop, Duration));
		ApplyWorld();
	}
}

void UKzTimeDilationSubsystem::SetWorldDilation(const UObject* Source, const UObject* Owner, float Dilation)
{
	WorldDilations.Add(TPair<FObjectKey, FObjectKey>(Source, Owner), FMath::Clamp(Dilation, MinDilation, 1.0f));
	ApplyWorld();
}

void UKzTimeDilationSubsystem::ClearWorldDilation(const UObject* Source, const UObject* Owner)
{
	if (WorldDilations.Remove(TPair<FObjectKey, FObjectKey>(Source, Owner)) > 0)
	{
		ApplyWorld();
	}
}

void UKzTimeDilationSubsystem::Cancel(AActor* Actor, const FGameplayTagContainer& AbilityTags)
{
	FDilated* Dilated = Actor && !AbilityTags.IsEmpty() ? Actors.Find(Actor) : nullptr;
	if (!Dilated)
	{
		return;
	}

	// Matched as abilities block one another: the ability's tags, parents included, against those listed
	const int32 Removed = Dilated->Requests.RemoveAllSwap([&AbilityTags](const FRequest& Request)
	{
		return !Request.CancelableBy.IsEmpty() && AbilityTags.HasAny(Request.CancelableBy);
	});
	if (Removed > 0)
	{
		Apply(*Actor, *Dilated);
		if (Dilated->Requests.IsEmpty())
		{
			Actors.Remove(Actor);
		}
	}
}

void UKzTimeDilationSubsystem::AddRequest(AActor* Actor, FRequest&& Request)
{
	if (!Actor || Request.Duration <= 0.0f)
	{
		return;
	}

	FDilated* Dilated = Actors.Find(Actor);
	if (!Dilated)
	{
		FDilated New;
		New.BaseDilation = Actor->CustomTimeDilation;
		Dilated = &Actors.Add(Actor, MoveTemp(New));
	}

	Dilated->Requests.Add(MoveTemp(Request));
	Apply(*Actor, *Dilated);
}

void UKzTimeDilationSubsystem::Apply(AActor& Actor, const FDilated& Dilated)
{
	float Slowest = 1.0f;
	for (const FRequest& Request : Dilated.Requests)
	{
		Slowest = FMath::Min(Slowest, Request.GetDilation());
	}
	Actor.CustomTimeDilation = Dilated.BaseDilation * Slowest;
}

void UKzTimeDilationSubsystem::ApplyWorld()
{
	AWorldSettings* WorldSettings = GetWorld() ? GetWorld()->GetWorldSettings(false, false) : nullptr;
	if (!WorldSettings)
	{
		return;
	}

	if (WorldRequests.IsEmpty() && WorldDilations.IsEmpty())
	{
		if (BaseWorldDilation.IsSet())
		{
			WorldSettings->SetTimeDilation(BaseWorldDilation.GetValue());
			BaseWorldDilation.Reset();
		}
		return;
	}

	float Slowest = 1.0f;
	for (const FRequest& Request : WorldRequests)
	{
		Slowest = FMath::Min(Slowest, Request.GetDilation());
	}
	for (const TPair<TPair<FObjectKey, FObjectKey>, float>& Pair : WorldDilations)
	{
		Slowest = FMath::Min(Slowest, Pair.Value);
	}

	if (!BaseWorldDilation.IsSet())
	{
		BaseWorldDilation = WorldSettings->TimeDilation;
	}
	WorldSettings->SetTimeDilation(BaseWorldDilation.GetValue() * Slowest);
}

void UKzTimeDilationSubsystem::Tick(float DeltaTime)
{
	for (auto It = Actors.CreateIterator(); It; ++It)
	{
		AActor* Actor = It.Key().Get();
		if (!Actor)
		{
			It.RemoveCurrent();
			continue;
		}

		FDilated& Dilated = It.Value();
		AdvanceRequests(Dilated.Requests, DeltaTime);

		Apply(*Actor, Dilated);
		if (Dilated.Requests.IsEmpty())
		{
			It.RemoveCurrent();
		}
	}

	if (!WorldRequests.IsEmpty())
	{
		// DeltaTime comes slowed by the world's dilation: undone, the world's own requests last their real time
		const AWorldSettings* WorldSettings = GetWorld() ? GetWorld()->GetWorldSettings(false, false) : nullptr;
		const float WorldDilation = WorldSettings ? WorldSettings->GetEffectiveTimeDilation() : 1.0f;
		const float RealDelta = WorldDilation > UE_SMALL_NUMBER ? DeltaTime / WorldDilation : DeltaTime;
		AdvanceRequests(WorldRequests, RealDelta);
		ApplyWorld();
	}
}

TStatId UKzTimeDilationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzTimeDilationSubsystem, STATGROUP_Tickables);
}

void UKzTimeDilationSubsystem::Deinitialize()
{
	for (const TPair<TWeakObjectPtr<AActor>, FDilated>& Pair : Actors)
	{
		if (AActor* Actor = Pair.Key.Get())
		{
			Actor->CustomTimeDilation = Pair.Value.BaseDilation;
		}
	}
	Actors.Reset();

	WorldRequests.Reset();
	WorldDilations.Reset();
	if (BaseWorldDilation.IsSet())
	{
		ApplyWorld();
	}
	Super::Deinitialize();
}