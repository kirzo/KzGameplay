// Copyright 2026 kirzo

#include "Factions/KzFactionComponent.h"

UKzFactionComponent::UKzFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKzFactionComponent::SetFaction(FGameplayTag NewFaction)
{
	if (Faction != NewFaction)
	{
		Faction = NewFaction;
		OnFactionChanged.Broadcast(Faction);
	}
}

void UKzFactionComponent::SetAttitudeTowards(FGameplayTag OtherFaction, EKzAttitude Attitude)
{
	if (OtherFaction.IsValid())
	{
		Attitudes.Add(OtherFaction, Attitude);
	}
}

void UKzFactionComponent::ClearAttitudeTowards(FGameplayTag OtherFaction)
{
	Attitudes.Remove(OtherFaction);
}

bool UKzFactionComponent::FindAttitudeTowards(const FGameplayTag& OtherFaction, EKzAttitude& OutAttitude) const
{
	if (const EKzAttitude* Found = Attitudes.Find(OtherFaction))
	{
		OutAttitude = *Found;
		return true;
	}
	return false;
}
