// Copyright 2026 kirzo

#include "Combat/KzTimeDilationSubsystem.h"
#include "GameFramework/Actor.h"

namespace
{
	// Zero would stop the actor's tick from moving anything on, timers included.
	constexpr float MinDilation = 0.001f;
}

void UKzTimeDilationSubsystem::Dilate(AActor* Actor, float Dilation, float Duration)
{
	if (!Actor || Duration <= 0.0f || Dilation >= 1.0f)
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

	FRequest Request;
	Request.Dilation = FMath::Max(Dilation, MinDilation);
	Request.TimeLeft = Duration;
	Dilated->Requests.Add(Request);

	float Slowest = 1.0f;
	for (const FRequest& Each : Dilated->Requests)
	{
		Slowest = FMath::Min(Slowest, Each.Dilation);
	}
	Actor->CustomTimeDilation = Dilated->BaseDilation * Slowest;
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
		float Slowest = 1.0f;
		for (int32 Index = Dilated.Requests.Num() - 1; Index >= 0; --Index)
		{
			FRequest& Request = Dilated.Requests[Index];
			Request.TimeLeft -= DeltaTime;
			if (Request.TimeLeft <= 0.0f)
			{
				Dilated.Requests.RemoveAtSwap(Index);
			}
			else
			{
				Slowest = FMath::Min(Slowest, Request.Dilation);
			}
		}

		Actor->CustomTimeDilation = Dilated.BaseDilation * Slowest;
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