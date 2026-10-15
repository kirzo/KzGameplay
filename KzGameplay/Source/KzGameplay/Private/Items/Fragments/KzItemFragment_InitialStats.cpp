// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_InitialStats.h"
#include "Items/KzItemInstance.h"

void UKzItemFragment_InitialStats::OnInstanceCreated(FKzItemInstance& Instance) const
{
	Super::OnInstanceCreated(Instance);

	for (const TTuple<FGameplayTag, float>& Stat : InitialStats)
	{
		Instance.Stats.SetStat(Stat.Key, Stat.Value);
	}
}