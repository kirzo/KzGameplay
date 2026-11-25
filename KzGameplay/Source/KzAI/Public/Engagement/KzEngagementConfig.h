// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KzEngagementConfig.generated.h"

/** One ring of places around the target, from where the one inside it ends outwards. */
USTRUCT(BlueprintType)
struct KZAI_API FKzEngagementArea
{
	GENERATED_BODY()

	/** How trees name it, such as the area an attacker prefers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement")
	FGameplayTag Tag;

	/** How far it reaches past the area inside it, or past the forbidden area for the first one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (ClampMin = "0", Units = "Centimeters"))
	float Width = 300.0f;

	/** How many attackers fit; 0 has no limit. Those that don't fit go to the next area out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (ClampMin = "0"))
	int32 Capacity = 0;

	/** Equal slices around the target, so attackers spread instead of piling up on one side. 1 has no slices. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (ClampMin = "1", UIMin = "1", UIMax = "16"))
	int32 SectorCount = 8;

	/** The attacker weight a sector holds before it is crowded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (ClampMin = "0"))
	float SectorWeight = 10.0f;

	/** Per sector, clockwise from the first: its own weight instead of Sector Weight. Missing entries use Sector Weight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement")
	TArray<float> SectorWeights;

	int32 GetSectorCount() const { return FMath::Max(1, SectorCount); }
	float GetSectorWeight(int32 Sector) const { return SectorWeights.IsValidIndex(Sector) ? SectorWeights[Sector] : SectorWeight; }
};

/** How attackers place themselves around a target: rings with room for so many, split in sectors that hold so much weight. */
USTRUCT(BlueprintType)
struct KZAI_API FKzEngagementSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (InlineEditConditionToggle))
	bool bForbidden = true;

	/** Too close to the target: attackers that end up here, after attacking or being hit, leave as soon as they can move. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (EditCondition = "bForbidden", ClampMin = "0", Units = "Centimeters"))
	float ForbiddenRadius = 150.0f;

	/** From the inside out, each starting where the one inside it ends, or at the target without a forbidden area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (TitleProperty = "Tag"))
	TArray<FKzEngagementArea> Areas;

	/** Turns the sectors: the first one starts straight ahead of the view of whoever drives the target, the camera for a player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (Units = "Degrees"))
	float SectorOffset = 0.0f;

	/**
	 * How much attackers even out around the target. An attacker only takes the sector at its bearing or one beside it, so nobody crosses the
	 * ring: those in the way move on a sector instead, towards where there is more room. 0 only moves them out of sectors holding too much,
	 * 1 keeps them moving until they are evenly spread all the way around. Inside its sector, the more it is, the nearer the middle an
	 * attacker stands instead of taking its shortest way in.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (ClampMin = "0", ClampMax = "1"))
	float Spread = 1.0f;

	/** How far apart the places attackers head for stay, so they don't bump and slide along each other. Also how far around the forbidden area they go. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (ClampMin = "0", Units = "Centimeters"))
	float MinSeparation = 120.0f;

	/** Attackers with any of these are never asked to make room, such as State.Attacking, or State.AttackGranted on the way to an attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement")
	FGameplayTagContainer ExemptTags;

	float GetInnerRadius(int32 Area) const;
	float GetOuterRadius(int32 Area) const { return GetInnerRadius(Area) + (Areas.IsValidIndex(Area) ? Areas[Area].Width : 0.0f); }
};

/** Where attackers place themselves around a target, for its Engagement Component, or for an encounter to put in place while it lasts. */
UCLASS(BlueprintType)
class KZAI_API UKzEngagementConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UKzEngagementConfig();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engagement", meta = (ShowOnlyInnerProperties))
	FKzEngagementSettings Settings;
};