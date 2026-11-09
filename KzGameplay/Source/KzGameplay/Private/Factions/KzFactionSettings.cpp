// Copyright 2026 kirzo

#include "Factions/KzFactionSettings.h"

EKzAttitude UKzFactionSettings::GetAttitude(const FGameplayTag& A, const FGameplayTag& B) const
{
	if (!A.IsValid() || !B.IsValid())
	{
		return EKzAttitude::Neutral;
	}

	if (A == B)
	{
		return EKzAttitude::Friendly;
	}

	const FKzFactionRelation* Relation = Relations.FindByPredicate([&A, &B](const FKzFactionRelation& Entry)
	{
		return (Entry.A == A && Entry.B == B) || (Entry.A == B && Entry.B == A);
	});
	return Relation ? Relation->Attitude : EKzAttitude::Neutral;
}
