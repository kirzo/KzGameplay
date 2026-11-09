// Copyright 2026 kirzo

#include "Factions/KzFactionLibrary.h"
#include "Factions/KzFactionComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	const UKzFactionComponent* FindOwnComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UKzFactionComponent>() : nullptr;
	}

	const UKzFactionComponent* FindControllerComponent(const AActor* Actor)
	{
		const APawn* Pawn = Cast<APawn>(Actor);
		const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
		return Controller ? Controller->FindComponentByClass<UKzFactionComponent>() : nullptr;
	}

	bool FindException(const AActor* Actor, const FGameplayTag& OtherFaction, EKzAttitude& OutAttitude)
	{
		const UKzFactionComponent* Own = FindOwnComponent(Actor);
		if (Own && Own->FindAttitudeTowards(OtherFaction, OutAttitude))
		{
			return true;
		}

		const UKzFactionComponent* FromController = FindControllerComponent(Actor);
		return FromController && FromController->FindAttitudeTowards(OtherFaction, OutAttitude);
	}
}

FGameplayTag UKzFactionLibrary::GetFaction(const AActor* Actor)
{
	const UKzFactionComponent* FromController = FindControllerComponent(Actor);
	if (FromController && FromController->GetFaction().IsValid())
	{
		return FromController->GetFaction();
	}

	const UKzFactionComponent* Own = FindOwnComponent(Actor);
	return Own ? Own->GetFaction() : FGameplayTag();
}

EKzAttitude UKzFactionLibrary::GetAttitude(const AActor* Actor, const AActor* Other)
{
	if (!Actor || !Other)
	{
		return EKzAttitude::Neutral;
	}

	if (Actor == Other)
	{
		return EKzAttitude::Friendly;
	}

	const FGameplayTag ActorFaction = GetFaction(Actor);
	const FGameplayTag OtherFaction = GetFaction(Other);

	EKzAttitude Attitude;
	if (FindException(Actor, OtherFaction, Attitude) || FindException(Other, ActorFaction, Attitude))
	{
		return Attitude;
	}

	return UKzFactionSettings::Get()->GetAttitude(ActorFaction, OtherFaction);
}
