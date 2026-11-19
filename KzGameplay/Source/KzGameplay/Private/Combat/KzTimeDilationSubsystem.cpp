// Copyright 2026 kirzo

#include "Combat/KzTimeDilationSubsystem.h"
#include "GameFramework/Actor.h"

namespace
{
	// A frozen actor still ticks this slowly, a millisecond each second: a zero delta breaks what divides by it, such as an AnimBlueprint's turn rate.
	constexpr float MinDilation = 0.001f;
}

float UKzTimeDilationSubsystem::FRequest::GetDilation() const
{
	const float Value = Curve.IsEmpty() || Duration <= 0.0f ? Dilation : Curve.Eval(FMath::Clamp(1.0f - TimeLeft / Duration, 0.0f, 1.0f));
	return FMath::Clamp(Value, MinDilation, 1.0f);
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
	AddRequest(Actor, MoveTemp(Request));
}

void UKzTimeDilationSubsystem::DilateAlong(AActor* Actor, const FRichCurve& Curve, float Duration)
{
	FRequest Request;
	Request.Curve = Curve;
	Request.Duration = Duration;
	AddRequest(Actor, MoveTemp(Request));
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

	Request.TimeLeft = Request.Duration;
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
		for (int32 Index = Dilated.Requests.Num() - 1; Index >= 0; --Index)
		{
			FRequest& Request = Dilated.Requests[Index];
			Request.TimeLeft -= DeltaTime;
			if (Request.TimeLeft <= 0.0f)
			{
				Dilated.Requests.RemoveAtSwap(Index);
			}
		}

		Apply(*Actor, Dilated);
		if (Dilated.Requests.IsEmpty())
		{
			It.RemoveCurrent();
		}
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
	Super::Deinitialize();
}