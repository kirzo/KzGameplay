// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "KzFactionSettings.generated.h"

/** How one side sees another. */
UENUM(BlueprintType)
enum class EKzAttitude : uint8
{
	Friendly,
	Neutral,
	Hostile
};

/** How two factions see each other, the same both ways. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzFactionRelation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (Categories = "Faction"))
	FGameplayTag A;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (Categories = "Faction"))
	FGameplayTag B;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	EKzAttitude Attitude = EKzAttitude::Hostile;
};

/**
 * How factions see each other by default, under Project Settings -> Plugins -> Kz Factions. Who belongs to which faction, and the
 * exceptions to these relations, live in UKzFactionComponent.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kz Factions"))
class KZGAMEPLAY_API UKzFactionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UKzFactionSettings* Get() { return GetDefault<UKzFactionSettings>(); }

	/** Pairs of factions and how they see each other, both ways. A faction is friendly to itself, and a pair left out is neutral. */
	UPROPERTY(Config, EditAnywhere, Category = "Factions", meta = (TitleProperty = "{A} - {B}: {Attitude}"))
	TArray<FKzFactionRelation> Relations;

	/** How faction A sees faction B by default. Without a faction, neutral. */
	EKzAttitude GetAttitude(const FGameplayTag& A, const FGameplayTag& B) const;
};
