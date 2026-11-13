// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "KzEngagementComponent.generated.h"

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

	/** Attackers with any of these are never asked to make room, such as State.Attacking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement")
	FGameplayTagContainer ExemptTags;

	float GetInnerRadius(int32 Area) const;
	float GetOuterRadius(int32 Area) const { return GetInnerRadius(Area) + (Areas.IsValidIndex(Area) ? Areas[Area].Width : 0.0f); }
};

/**
 * Where attackers place themselves around its owner: a forbidden area too close to it, then areas that fit so many attackers, each split in
 * sectors that hold so much weight. Passive: it only works when attackers ask, so every character can carry one and be surrounded, whoever drives it.
 * Attackers engage it with their weight and the area they prefer. A place one heads for counts as its own from the moment it sets off.
 */
UCLASS(ClassGroup = (KzAI), meta = (BlueprintSpawnableComponent))
class KZAI_API UKzEngagementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzEngagementComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engagement", meta = (ShowOnlyInnerProperties))
	FKzEngagementSettings Settings;

	/** Starts counting an attacker, until Disengage. Engaging again updates its weight and preferred area. */
	void Engage(AActor* Attacker, float Weight, FGameplayTag PreferredArea);
	void Disengage(AActor* Attacker);

	/**
	 * Sets the attacker on its way to its place. It keeps the one it has while that is still right for it; else it picks one: in the area it
	 * prefers or, if full, the next one out with room; in the sector at its bearing or one beside it, the one with room that weighs least
	 * how far it is against how full it and the ones beside it are (Spread). The place is its own from then on: it counts there, and those
	 * there that have to make room for it start being out of place. It is a spot on the ground: it doesn't follow the target, so one the
	 * target leaves in the wrong area or sector is picked again from where the attacker is then. False if it isn't engaged or there are no areas.
	 */
	bool FindPlace(AActor* Attacker);

	/**
	 * Where the attacker should head now for its place: straight to it, or a point ahead around the target when the straight way passes too
	 * close to it. Every so often it also takes a clearly better place, or swaps places with another on its way when both trips get shorter.
	 * False without a place.
	 */
	bool UpdatePlace(AActor* Attacker, FVector& OutLocation);

	/**
	 * The attacker is no longer on its way. Close enough to its place, it keeps it and counts there, wherever exactly it stopped, so counts
	 * don't flicker at the edges of areas and sectors. Stopped short, such as by a hit, where it stands becomes its place.
	 */
	void ReleasePlace(AActor* Attacker);

	/** Sets the attacker on its way to a place it chose itself, such as the end of a move around the target: it counts there from now on. False if it isn't engaged. */
	bool ClaimPlace(AActor* Attacker, const FVector& Location);

	/** False until the attacker first finds or claims a place. */
	bool HasPlace(const AActor* Attacker) const;

	/** Whether the attacker's place is no longer right for it, wherever the attacker is: the area or the sector, as in Is Out Of Place. */
	bool IsInWrongPlace(const AActor* Attacker) const;

	bool IsInForbiddenArea(const AActor* Attacker) const;

	/**
	 * Whether the attacker should move: it has no place, is away from it, such as pushed by a hit, or in the forbidden area; or its place is no
	 * longer right for it, such as when the target moved: in an area it doesn't prefer while a closer one it prefers has room, among those that have to make
	 * room in an area or sector holding too much (the settled ones that are not exempt, nearest to where there is room first), or beside a
	 * sector that is clearly better for it, by Spread.
	 */
	bool IsOutOfPlace(const AActor* Attacker) const;

	/** The area a location is in: INDEX_NONE inside the forbidden area, the number of areas past the last one. */
	int32 GetAreaAt(const FVector& Location) const;

	/** The sector of an area a location is in. */
	int32 GetSectorAt(int32 Area, const FVector& Location) const;

	/** The yaw sectors start from: the view of whoever drives the owner, else its facing, turned by Sector Offset. */
	float GetReferenceYaw() const;

	/** Its areas and sectors as lines on the ground, for the editor's visualizer and the debug view. */
	void ForEachDebugLine(TFunctionRef<void(const FVector& Start, const FVector& End, const FLinearColor& Color)> Line) const;

	/** The weight of each sector and the capacity of each area as labels; with bLoad, how much each sector holds too. */
	void ForEachDebugLabel(bool bLoad, TFunctionRef<void(const FVector& Location, const FString& Text, const FLinearColor& Color)> Label) const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FEngager
	{
		TWeakObjectPtr<AActor> Actor;
		float Weight = 0.0f;
		FGameplayTag PreferredArea;

		/** Its place, if it has one: a spot on the ground, which stays behind when the owner moves. It counts there, on its way or settled at it. */
		TOptional<FVector> Place;

		/** On its way to its place. Settled ones are the ones asked to make room. */
		bool bTravelling = false;

		/** When it next looks for a better place on its way. */
		double NextReconsiderTime = 0.0;
	};

	struct FSectorChoice
	{
		int32 Area = INDEX_NONE;
		int32 Sector = INDEX_NONE;
		float Cost = 0.0f;
		bool bHasRoom = false;
	};

	FEngager* FindEngager(const AActor* Attacker);
	const FEngager* FindEngager(const AActor* Attacker) const;

	/** Where an engager counts: its place, else where it is. */
	FVector GetCountedLocation(const FEngager& Engager) const;

	/** Whether a place is no longer right for the engager: the wrong area, or a sector it has to leave or would clearly do better beside. */
	bool IsWrongPlace(const FEngager& Engager, const FVector& Place) const;

	/** How far a sector is from a bearing against how much the area would hold around it with the engager, the nearer around the more, by Spread. */
	float GetSectorCost(const FEngager& Engager, int32 Area, int32 Sector, float Bearing) const;

	/** Of the sector of an area at a bearing and the ones beside it, the cheapest with room for the engager, or the cheapest if none has room. */
	FSectorChoice ChooseSector(const FEngager& Engager, int32 Area, float Bearing) const;

	/** A point in a sector: the bearing from From kept inside it, at a random distance in the area, away from the others. */
	FVector PickPoint(const FEngager& Engager, int32 Area, int32 Sector, const FVector& From) const;

	/** How far a point is from the nearest other engager: from its place and, for one that is settled, from where it stands too. */
	double GetClearance(const FEngager& Engager, const FVector& Point) const;

	/** Swaps places with another on its way when both trips get shorter, else takes a clearly better place. */
	void Reconsider(FEngager& Engager);

	/** Straight to the place, or a point ahead around the owner when the straight way passes too close to it. */
	FVector GetPathPoint(const FVector& From, const FVector& Place) const;

	float GetSectorLoad(int32 Area, int32 Sector, const FEngager* Ignore) const;
	int32 GetAreaCount(int32 Area, const FEngager* Ignore) const;
	bool HasRoom(int32 Area, const FEngager* Ignore) const;
	bool IsExempt(const FEngager& Engager) const;

	int32 GetPreferredArea(const FEngager& Engager) const;

	/** The area an engager should be in: its preferred one or the next one out with room, else the last. */
	int32 ChooseArea(const FEngager& Engager) const;

	/** Whether the engager is one of those that have to leave an area with more attackers than it fits: the farthest out first. */
	bool IsAreaLeaver(const FEngager& Engager, int32 Area) const;

	/** Whether the engager is one of those that have to leave a sector holding too much weight: the nearest to a sector with room first. */
	bool IsSectorLeaver(const FEngager& Engager, int32 Area, int32 Sector) const;

	/** Degrees from the reference yaw to a location, in [0, 360). */
	float GetBearing(const FVector& Location) const;

	/** A location around the owner, at a bearing and a distance. */
	FVector GetLocationAt(float Bearing, float Radius) const;

	FVector GetCenter() const;

	/** Keeps the debug view ticking only while it is on and someone is engaged. */
	void UpdateTick();

	TArray<FEngager> Engagers;
};