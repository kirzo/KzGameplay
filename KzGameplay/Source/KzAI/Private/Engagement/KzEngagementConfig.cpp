// Copyright 2026 kirzo

#include "Engagement/KzEngagementConfig.h"
#include "Combat/KzCombatTags.h"
#include "KzAITags.h"

float FKzEngagementSettings::GetInnerRadius(int32 Area) const
{
	float Radius = bForbidden ? ForbiddenRadius : 0.0f;
	for (int32 Index = 0; Index < Area && Index < Areas.Num(); ++Index)
	{
		Radius += Areas[Index].Width;
	}
	return Radius;
}

UKzEngagementConfig::UKzEngagementConfig()
{
	FKzEngagementArea Close;
	Close.Width = 250.0f;
	Close.Capacity = 3;
	Close.SectorWeight = 10.0f;

	FKzEngagementArea Far;
	Far.Width = 400.0f;
	Far.SectorWeight = 20.0f;

	Settings.Areas = { Close, Far };
	Settings.ExemptTags.AddTag(KzTags::Combat::State_Attacking);
	Settings.ExemptTags.AddTag(KzTags::AI::State_AttackGranted);
}