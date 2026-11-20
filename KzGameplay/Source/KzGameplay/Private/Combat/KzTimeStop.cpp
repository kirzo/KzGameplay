// Copyright 2026 kirzo

#include "Combat/KzTimeStop.h"
#include "Combat/KzTimeDilationSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	UKzTimeDilationSubsystem* FindTimeDilation(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext && GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
		return World ? World->GetSubsystem<UKzTimeDilationSubsystem>() : nullptr;
	}
}

void UKzHitStop::Apply(AActor* Instigator, TConstArrayView<AActor*> Receivers, float InDuration) const
{
	UKzTimeDilationSubsystem* TimeDilation = FindTimeDilation(Instigator ? Instigator : Receivers.IsEmpty() ? nullptr : Receivers[0]);
	if (!TimeDilation || InDuration <= 0.0f)
	{
		return;
	}

	if (Scope != EKzHitStopScope::Receiver)
	{
		TimeDilation->Hold(Instigator, *this, InDuration, CancelableBy);
	}
	if (Scope != EKzHitStopScope::Instigator)
	{
		for (AActor* Receiver : Receivers)
		{
			TimeDilation->Hold(Receiver, *this, InDuration, CancelableBy);
		}
	}
}

UKzSlowMotion::UKzSlowMotion()
{
	Duration = 0.5f;
	bRepeatInAttack = false;
}

void UKzSlowMotion::Apply(const UObject* WorldContext, float InDuration) const
{
	if (UKzTimeDilationSubsystem* TimeDilation = FindTimeDilation(WorldContext))
	{
		TimeDilation->HoldWorld(*this, InDuration);
	}
}