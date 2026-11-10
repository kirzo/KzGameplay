// Copyright 2026 kirzo

#include "Engagement/KzEngagementComponent.h"
#include "AIController.h"
#include "Combat/KzCombatTags.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagAssetInterface.h"
#include "HAL/IConsoleManager.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationData.h"

namespace
{
	TAutoConsoleVariable<bool> CVarDrawEngagement(TEXT("kz.Engagement.Draw"), false, TEXT("Draws the engagement areas of characters with attackers engaged: what each sector holds, red when crowded, and where each attacker heads."));

	constexpr int32 CircleSegments = 64;

	/** How often an attacker on its way looks for a better place, or for another it should swap places with. */
	constexpr double ReconsiderInterval = 0.5;

	/** How much cheaper another sector has to be to change course for it. */
	constexpr float SwitchMargin = 0.15f;

	/** How much shorter two trips have to get to swap places. */
	constexpr double SwapMargin = 100.0;

	/** How far ahead along the arc an attacker going around the target aims. */
	constexpr double ArcStep = 250.0;

	/** How far from its place an attacker still counts as being at it: more than where a move stops short of its goal. */
	constexpr double PlaceTolerance = 150.0;

	/** How much freer another point has to be to give up a place someone stands on. */
	constexpr double ClearanceMargin = 30.0;

	double GetYawDegrees(const FVector& Offset)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
	}

	const FLinearColor ForbiddenColor(1.0f, 0.2f, 0.2f);
	const FLinearColor CrowdedColor(1.0f, 0.1f, 0.1f);

	FLinearColor GetAreaColor(int32 Area)
	{
		static const FLinearColor Colors[] = { FLinearColor(0.3f, 0.85f, 0.4f), FLinearColor(1.0f, 0.65f, 0.25f), FLinearColor(0.35f, 0.6f, 1.0f), FLinearColor(0.8f, 0.45f, 1.0f) };
		return Colors[Area % UE_ARRAY_COUNT(Colors)];
	}
}

float FKzEngagementSettings::GetInnerRadius(int32 Area) const
{
	float Radius = bForbidden ? ForbiddenRadius : 0.0f;
	for (int32 Index = 0; Index < Area && Index < Areas.Num(); ++Index)
	{
		Radius += Areas[Index].Width;
	}
	return Radius;
}

UKzEngagementComponent::UKzEngagementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	FKzEngagementArea Close;
	Close.Width = 250.0f;
	Close.Capacity = 3;
	Close.SectorWeight = 10.0f;

	FKzEngagementArea Far;
	Far.Width = 400.0f;
	Far.SectorWeight = 20.0f;

	Settings.Areas = { Close, Far };
	Settings.ExemptTags.AddTag(KzTags::Combat::State_Attacking);
}

void UKzEngagementComponent::Engage(AActor* Attacker, float Weight, FGameplayTag PreferredArea)
{
	if (!Attacker)
	{
		return;
	}

	FEngager* Engager = FindEngager(Attacker);
	if (!Engager)
	{
		Engager = &Engagers.AddDefaulted_GetRef();
		Engager->Actor = Attacker;
	}
	Engager->Weight = Weight;
	Engager->PreferredArea = PreferredArea;
	UpdateTick();
}

void UKzEngagementComponent::Disengage(AActor* Attacker)
{
	Engagers.RemoveAll([Attacker](const FEngager& Engager) { return !Engager.Actor.IsValid() || Engager.Actor.Get() == Attacker; });
	UpdateTick();
}

bool UKzEngagementComponent::FindPlace(AActor* Attacker)
{
	FEngager* Engager = FindEngager(Attacker);
	if (!Engager || Settings.Areas.IsEmpty())
	{
		return false;
	}

	// It keeps a place that is still right for it; else it picks another, such as when the target left it behind
	if (!Engager->Place.IsSet() || IsWrongPlace(*Engager, Engager->Place.GetValue()))
	{
		// From the place it leaves when it stands at it, so it picks from the same sector that made the place wrong
		FVector From = Attacker->GetActorLocation();
		if (Engager->Place.IsSet() && FVector::Dist2D(From, Engager->Place.GetValue()) <= PlaceTolerance)
		{
			From = Engager->Place.GetValue();
		}

		// Its own claim doesn't count against it
		Engager->Place.Reset();

		const FSectorChoice Choice = ChooseSector(*Engager, ChooseArea(*Engager), GetBearing(From));
		Engager->Place = PickPoint(*Engager, Choice.Area, Choice.Sector, From);
	}
	Engager->bTravelling = true;

	const UWorld* World = GetWorld();
	Engager->NextReconsiderTime = (World ? World->GetTimeSeconds() : 0.0) + ReconsiderInterval;
	UpdateTick();
	return true;
}

bool UKzEngagementComponent::UpdatePlace(AActor* Attacker, FVector& OutLocation)
{
	FEngager* Engager = FindEngager(Attacker);
	if (!Engager || !Engager->Place.IsSet())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now >= Engager->NextReconsiderTime)
	{
		Engager->NextReconsiderTime = Now + ReconsiderInterval;
		Reconsider(*Engager);
	}

	OutLocation = GetPathPoint(Attacker->GetActorLocation(), Engager->Place.GetValue());
	return true;
}

void UKzEngagementComponent::ReleasePlace(AActor* Attacker)
{
	FEngager* Engager = FindEngager(Attacker);
	if (!Engager)
	{
		return;
	}

	// Arrived, the place stays its own: it counts there, not wherever it stopped. Stopped on the way, it counts where it is
	Engager->bTravelling = false;
	if (Engager->Place.IsSet() && FVector::Dist2D(Attacker->GetActorLocation(), Engager->Place.GetValue()) > PlaceTolerance)
	{
		Engager->Place.Reset();
	}
}

bool UKzEngagementComponent::IsInForbiddenArea(const AActor* Attacker) const
{
	return Attacker && Settings.bForbidden && FVector::Dist2D(Attacker->GetActorLocation(), GetCenter()) < Settings.ForbiddenRadius;
}

bool UKzEngagementComponent::IsOutOfPlace(const AActor* Attacker) const
{
	const FEngager* Engager = FindEngager(Attacker);
	if (!Engager || Settings.Areas.IsEmpty())
	{
		return false;
	}

	// Without a place, away from it, such as after a hit, or too close to the target
	if (!Engager->Place.IsSet() || IsInForbiddenArea(Attacker))
	{
		return true;
	}

	const FVector Place = Engager->Place.GetValue();
	return FVector::Dist2D(Attacker->GetActorLocation(), Place) > PlaceTolerance || IsWrongPlace(*Engager, Place);
}

bool UKzEngagementComponent::IsWrongPlace(const FEngager& Engager, const FVector& Place) const
{
	const int32 Area = GetAreaAt(Place);
	if (!Settings.Areas.IsValidIndex(Area))
	{
		return true;
	}

	const int32 Preferred = GetPreferredArea(Engager);
	if (Area < Preferred)
	{
		return true;
	}
	for (int32 Closer = Preferred; Closer < Area; ++Closer)
	{
		if (HasRoom(Closer, &Engager))
		{
			return true;
		}
	}

	if (IsAreaLeaver(Engager, Area))
	{
		return true;
	}
	if (IsExempt(Engager))
	{
		return false;
	}

	// A sector beside its own is better: any better one when it has to make room, one with room otherwise
	const int32 Sector = GetSectorAt(Area, Place);
	const float Bearing = GetBearing(Place);
	const FSectorChoice Best = ChooseSector(Engager, Area, Bearing);
	if (Best.Sector == Sector)
	{
		return false;
	}

	const float Gain = GetSectorCost(Engager, Area, Sector, Bearing) - Best.Cost;
	if (IsSectorLeaver(Engager, Area, Sector))
	{
		return Best.bHasRoom || Gain > 0.0f;
	}

	// Half of what a step gains between two others of its weight left two sectors uneven: under that, it is as even as sectors allow
	const FKzEngagementArea& AreaSettings = Settings.Areas[Area];
	const float Margin = FMath::Clamp(Settings.Spread, 0.0f, 1.0f) * Engager.Weight / FMath::Max(AreaSettings.GetSectorWeight(Sector), UE_KINDA_SMALL_NUMBER) * 4.0f / FMath::Square(AreaSettings.GetSectorCount());
	return Best.bHasRoom && Gain > Margin;
}

int32 UKzEngagementComponent::GetAreaAt(const FVector& Location) const
{
	const double Distance = FVector::Dist2D(Location, GetCenter());
	if (Settings.bForbidden && Distance < Settings.ForbiddenRadius)
	{
		return INDEX_NONE;
	}

	for (int32 Area = 0; Area < Settings.Areas.Num(); ++Area)
	{
		if (Distance < Settings.GetOuterRadius(Area))
		{
			return Area;
		}
	}
	return Settings.Areas.Num();
}

int32 UKzEngagementComponent::GetSectorAt(int32 Area, const FVector& Location) const
{
	if (!Settings.Areas.IsValidIndex(Area))
	{
		return INDEX_NONE;
	}

	const int32 Count = Settings.Areas[Area].GetSectorCount();
	return FMath::Min(FMath::FloorToInt32(GetBearing(Location) * Count / 360.0f), Count - 1);
}

float UKzEngagementComponent::GetReferenceYaw() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return Settings.SectorOffset;
	}

	double Yaw = Owner->GetActorRotation().Yaw;
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			FVector ViewLocation;
			FRotator ViewRotation;
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
			Yaw = ViewRotation.Yaw;
		}
	}
	return static_cast<float>(Yaw) + Settings.SectorOffset;
}

void UKzEngagementComponent::ForEachDebugLine(TFunctionRef<void(const FVector& Start, const FVector& End, const FLinearColor& Color)> Line) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// On the ground, under the middle of the owner
	const FVector Ground = Owner->GetActorLocation() - FVector(0.0, 0.0, Owner->GetSimpleCollisionHalfHeight());
	const float ReferenceYaw = GetReferenceYaw();
	auto PointAt = [&Ground, ReferenceYaw](float Bearing, float Radius)
	{
		const float Yaw = FMath::DegreesToRadians(ReferenceYaw + Bearing);
		return Ground + FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.0f) * Radius;
	};
	auto Circle = [&PointAt, &Line](float Radius, const FLinearColor& Color)
	{
		for (int32 Segment = 0; Segment < CircleSegments; ++Segment)
		{
			Line(PointAt(360.0f * Segment / CircleSegments, Radius), PointAt(360.0f * (Segment + 1) / CircleSegments, Radius), Color);
		}
	};

	if (Settings.bForbidden)
	{
		Circle(Settings.ForbiddenRadius, ForbiddenColor);
	}

	for (int32 Area = 0; Area < Settings.Areas.Num(); ++Area)
	{
		const FLinearColor Color = GetAreaColor(Area);
		const float Inner = Settings.GetInnerRadius(Area);
		const float Outer = Settings.GetOuterRadius(Area);
		Circle(Outer, Color);

		const int32 Count = Settings.Areas[Area].GetSectorCount();
		for (int32 Sector = 0; Count > 1 && Sector < Count; ++Sector)
		{
			const float Bearing = 360.0f * Sector / Count;
			Line(PointAt(Bearing, Inner), PointAt(Bearing, Outer), Color);
		}
	}

	// Straight ahead of the view
	Line(Ground, PointAt(-Settings.SectorOffset, Settings.GetOuterRadius(Settings.Areas.Num() - 1)), FLinearColor::White);
}

void UKzEngagementComponent::ForEachDebugLabel(bool bLoad, TFunctionRef<void(const FVector& Location, const FString& Text, const FLinearColor& Color)> Label) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FVector Ground = Owner->GetActorLocation() - FVector(0.0, 0.0, Owner->GetSimpleCollisionHalfHeight());
	const float ReferenceYaw = GetReferenceYaw();
	auto PointAt = [&Ground, ReferenceYaw](float Bearing, float Radius)
	{
		const float Yaw = FMath::DegreesToRadians(ReferenceYaw + Bearing);
		return Ground + FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.0f) * Radius;
	};

	for (int32 Area = 0; Area < Settings.Areas.Num(); ++Area)
	{
		const FKzEngagementArea& AreaSettings = Settings.Areas[Area];
		const FLinearColor Color = GetAreaColor(Area);
		const float Inner = Settings.GetInnerRadius(Area);
		const float Outer = Settings.GetOuterRadius(Area);

		const int32 Count = AreaSettings.GetSectorCount();
		for (int32 Sector = 0; Sector < Count; ++Sector)
		{
			const float Weight = AreaSettings.GetSectorWeight(Sector);
			const float Load = bLoad ? GetSectorLoad(Area, Sector, nullptr) : 0.0f;
			const FString Text = bLoad ? FString::Printf(TEXT("%g/%g"), Load, Weight) : FString::Printf(TEXT("%g"), Weight);
			Label(PointAt(360.0f * (Sector + 0.5f) / Count, (Inner + Outer) * 0.5f), Text, Load > Weight ? CrowdedColor : Color);
		}

		// Its name and how many fit, at its outer edge straight ahead of the view
		FString AreaText = AreaSettings.Tag.IsValid() ? AreaSettings.Tag.ToString() : FString::Printf(TEXT("Area %d"), Area);
		if (AreaSettings.Capacity > 0)
		{
			AreaText += bLoad ? FString::Printf(TEXT(" %d/%d"), GetAreaCount(Area, nullptr), AreaSettings.Capacity) : FString::Printf(TEXT(" (%d)"), AreaSettings.Capacity);
		}
		Label(PointAt(-Settings.SectorOffset, Outer), AreaText, Color);
	}
}

void UKzEngagementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Engagers.RemoveAll([](const FEngager& Engager) { return !Engager.Actor.IsValid(); });
	UpdateTick();
	if (!IsComponentTickEnabled())
	{
		return;
	}

	const UWorld* World = GetWorld();
	ForEachDebugLine([World](const FVector& Start, const FVector& End, const FLinearColor& Color)
	{
		DrawDebugLine(World, Start, End, Color.ToFColor(true), false, -1.0f, 0, 2.0f);
	});
	ForEachDebugLabel(true, [World](const FVector& Location, const FString& Text, const FLinearColor& Color)
	{
		// 0 lasts one frame: -1 would keep each label forever
		DrawDebugString(World, Location, Text, nullptr, Color.ToFColor(true), 0.0f, true);
	});

	for (const FEngager& Engager : Engagers)
	{
		const FVector From = Engager.Actor->GetActorLocation();
		if (Engager.Place.IsSet())
		{
			// Its place; on its way to it, where it heads now too
			const FVector Place = Engager.Place.GetValue();
			DrawDebugSphere(World, Place, 20.0f, 8, Engager.bTravelling ? FColor::White : FColor::Silver);
			if (Engager.bTravelling)
			{
				DrawDebugLine(World, From, GetPathPoint(From, Place), FColor::White, false, -1.0f, 0, 1.0f);
			}
		}

		// The sector it counts in
		const FVector Counted = GetCountedLocation(Engager);
		const int32 Area = GetAreaAt(Counted);
		if (Settings.Areas.IsValidIndex(Area))
		{
			const FKzEngagementArea& AreaSettings = Settings.Areas[Area];
			const float Bearing = (GetSectorAt(Area, Counted) + 0.5f) * 360.0f / AreaSettings.GetSectorCount();
			const FVector Middle = GetLocationAt(Bearing, Settings.GetInnerRadius(Area) + AreaSettings.Width * 0.5f);
			DrawDebugLine(World, From, Middle, GetAreaColor(Area).ToFColor(true), false, -1.0f, 0, 2.0f);
		}

		// Where its controller is taking it: the end of the path it follows, if any
		const APawn* Pawn = Cast<APawn>(Engager.Actor.Get());
		const AAIController* Controller = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
		const UPathFollowingComponent* Following = Controller ? Controller->GetPathFollowingComponent() : nullptr;
		const FNavPathSharedPtr Path = Following && Following->GetStatus() != EPathFollowingStatus::Idle ? Following->GetPath() : FNavPathSharedPtr();
		if (Path.IsValid() && Path->IsValid())
		{
			DrawDebugLine(World, From, Path->GetEndLocation(), FColor::Orange, false, -1.0f, 0, 3.0f);
		}
	}
}

UKzEngagementComponent::FEngager* UKzEngagementComponent::FindEngager(const AActor* Attacker)
{
	return Attacker ? Engagers.FindByPredicate([Attacker](const FEngager& Engager) { return Engager.Actor.Get() == Attacker; }) : nullptr;
}

const UKzEngagementComponent::FEngager* UKzEngagementComponent::FindEngager(const AActor* Attacker) const
{
	return Attacker ? Engagers.FindByPredicate([Attacker](const FEngager& Engager) { return Engager.Actor.Get() == Attacker; }) : nullptr;
}

FVector UKzEngagementComponent::GetCountedLocation(const FEngager& Engager) const
{
	if (Engager.Place.IsSet())
	{
		return Engager.Place.GetValue();
	}
	const AActor* Actor = Engager.Actor.Get();
	return Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
}

float UKzEngagementComponent::GetSectorCost(const FEngager& Engager, int32 Area, int32 Sector, float Bearing) const
{
	const FKzEngagementArea& AreaSettings = Settings.Areas[Area];
	const int32 Count = AreaSettings.GetSectorCount();
	const float SectorAngle = 360.0f / Count;
	const float Travel = FMath::Abs(FMath::FindDeltaAngleDegrees(Bearing, (Sector + 0.5f) * SectorAngle)) / 180.0f;

	// What the whole area holds counts, less the farther around from the sector and nothing across the ring, so attackers even out all the way around
	float Load = Engager.Weight;
	for (const FEngager& Other : Engagers)
	{
		if (&Other == &Engager || !Other.Actor.IsValid())
		{
			continue;
		}

		const FVector Location = GetCountedLocation(Other);
		if (GetAreaAt(Location) == Area)
		{
			const int32 Steps = FMath::Abs(GetSectorAt(Area, Location) - Sector);
			Load += Other.Weight * FMath::Square(1.0f - FMath::Min(Steps, Count - Steps) * 2.0f / Count);
		}
	}

	const float Spread = FMath::Clamp(Settings.Spread, 0.0f, 1.0f);
	return Travel * (1.0f - Spread) + Load / FMath::Max(AreaSettings.GetSectorWeight(Sector), UE_KINDA_SMALL_NUMBER) * Spread;
}

UKzEngagementComponent::FSectorChoice UKzEngagementComponent::ChooseSector(const FEngager& Engager, int32 Area, float Bearing) const
{
	FSectorChoice Best;
	Best.Area = Area;

	const FKzEngagementArea& AreaSettings = Settings.Areas[Area];
	const int32 Count = AreaSettings.GetSectorCount();
	const int32 Nearest = FMath::Min(FMath::FloorToInt32(Bearing * Count / 360.0f), Count - 1);

	// Only the sector at its bearing and the ones beside it: nobody crosses the ring for a place, those in the way make room instead
	for (const int32 Offset : { 0, -1, 1 })
	{
		if ((Offset != 0 && Count == 1) || (Offset == 1 && Count == 2))
		{
			continue;
		}

		const int32 Sector = (Nearest + Offset + Count) % Count;
		const bool bRoom = GetSectorLoad(Area, Sector, &Engager) + Engager.Weight <= AreaSettings.GetSectorWeight(Sector);
		const float Cost = GetSectorCost(Engager, Area, Sector, Bearing);
		if (Best.Sector == INDEX_NONE || (bRoom && !Best.bHasRoom) || (bRoom == Best.bHasRoom && Cost < Best.Cost))
		{
			Best.Sector = Sector;
			Best.Cost = Cost;
			Best.bHasRoom = bRoom;
		}
	}
	return Best;
}

FVector UKzEngagementComponent::PickPoint(const FEngager& Engager, int32 Area, int32 Sector, const FVector& From) const
{
	const FKzEngagementArea& AreaSettings = Settings.Areas[Area];
	const float SectorAngle = 360.0f / AreaSettings.GetSectorCount();
	const float Inner = Settings.GetInnerRadius(Area);
	// Clear of the area's edges by half the separation, so a place isn't on the line with the next area or the forbidden one
	const float RadiusMargin = FMath::Min(AreaSettings.Width * 0.5f, FMath::Max(AreaSettings.Width * 0.1f, Settings.MinSeparation * 0.5f));
	const float Middle = (Sector + 0.5f) * SectorAngle;
	const float HalfSpan = SectorAngle * 0.4f;

	// From its shortest way in to around the middle of the sector, by Spread: at the edges, those in sectors apart still end up together
	const float Nearest = FMath::Clamp(FMath::FindDeltaAngleDegrees(Middle, GetBearing(From)), -HalfSpan, HalfSpan);
	const float Centring = AreaSettings.GetSectorCount() > 1 ? FMath::Clamp(Settings.Spread, 0.0f, 1.0f) : 0.0f;
	const float Delta = FMath::Lerp(Nearest, FMath::FRandRange(-HalfSpan, HalfSpan) / 3.0f, Centring);

	// At a random distance in the area; around it in the sector while too close to the others
	FVector Best = GetLocationAt(Middle + Delta, FMath::FRandRange(Inner + RadiusMargin, Inner + AreaSettings.Width - RadiusMargin));
	double BestClearance = GetClearance(Engager, Best);
	for (int32 Try = 1; Try <= 8 && BestClearance < Settings.MinSeparation; ++Try)
	{
		// Alternating sides, farther each time
		const float Offset = (Try + 1) / 2 * HalfSpan * 0.5f * (Try % 2 ? 1.0f : -1.0f);
		const FVector Point = GetLocationAt(Middle + FMath::Clamp(Delta + Offset, -HalfSpan, HalfSpan), FMath::FRandRange(Inner + RadiusMargin, Inner + AreaSettings.Width - RadiusMargin));
		const double Clearance = GetClearance(Engager, Point);
		if (Clearance > BestClearance)
		{
			Best = Point;
			BestClearance = Clearance;
		}
	}
	return Best;
}

double UKzEngagementComponent::GetClearance(const FEngager& Engager, const FVector& Point) const
{
	double Clearance = TNumericLimits<double>::Max();
	for (const FEngager& Other : Engagers)
	{
		if (&Other == &Engager || !Other.Actor.IsValid())
		{
			continue;
		}

		Clearance = FMath::Min(Clearance, FVector::Dist2D(Point, GetCountedLocation(Other)));

		// A settled one stands where it stopped, which can be short of its place
		if (!Other.bTravelling)
		{
			Clearance = FMath::Min(Clearance, FVector::Dist2D(Point, Other.Actor->GetActorLocation()));
		}
	}
	return Clearance;
}

void UKzEngagementComponent::Reconsider(FEngager& Engager)
{
	const FVector From = Engager.Actor->GetActorLocation();

	// Another of the same weight on its way, nearer to this one's place while this one is nearer to its: they swap
	for (FEngager& Other : Engagers)
	{
		if (&Other == &Engager || !Other.Actor.IsValid() || !Other.bTravelling || !Other.Place.IsSet() || Other.Weight != Engager.Weight)
		{
			continue;
		}

		const FVector OtherFrom = Other.Actor->GetActorLocation();
		const FVector Mine = Engager.Place.GetValue();
		const FVector Theirs = Other.Place.GetValue();
		const double Current = FVector::Dist2D(From, Mine) + FVector::Dist2D(OtherFrom, Theirs);
		const double Swapped = FVector::Dist2D(From, Theirs) + FVector::Dist2D(OtherFrom, Mine);
		if (Swapped < Current - SwapMargin)
		{
			Swap(Engager.Place, Other.Place);
			return;
		}
	}

	// A place the target left behind, a sector clearly cheaper than the one it heads for, or room in an area it prefers more
	const FVector Place = Engager.Place.GetValue();
	const int32 PlaceArea = GetAreaAt(Place);
	const int32 PlaceSector = GetSectorAt(PlaceArea, Place);
	Engager.Place.Reset();

	const FSectorChoice Best = ChooseSector(Engager, ChooseArea(Engager), GetBearing(From));
	const bool bValidPlace = Settings.Areas.IsValidIndex(PlaceArea);
	const float CurrentCost = bValidPlace ? GetSectorCost(Engager, PlaceArea, PlaceSector, GetBearing(From)) : TNumericLimits<float>::Max();
	const bool bSame = Best.Area == PlaceArea && Best.Sector == PlaceSector;
	if (!bSame && (!bValidPlace || Best.Area < PlaceArea || Best.Cost < CurrentCost - SwitchMargin))
	{
		Engager.Place = PickPoint(Engager, Best.Area, Best.Sector, From);
	}
	else
	{
		Engager.Place = Place;

		// Someone stands on its place, such as one that settled there after it chose: another point of the same sector, if clearly freer
		const double Clearance = GetClearance(Engager, Place);
		if (bValidPlace && Clearance < Settings.MinSeparation * 0.75)
		{
			const FVector Point = PickPoint(Engager, PlaceArea, PlaceSector, From);
			if (GetClearance(Engager, Point) > Clearance + ClearanceMargin)
			{
				Engager.Place = Point;
			}
		}
	}
}

FVector UKzEngagementComponent::GetPathPoint(const FVector& From, const FVector& Place) const
{
	const FVector Center = GetCenter();
	const FVector ToFrom = (From - Center) * FVector(1.0, 1.0, 0.0);
	const FVector ToPlace = (Place - Center) * FVector(1.0, 1.0, 0.0);
	const double MinRadius = (Settings.bForbidden ? Settings.ForbiddenRadius : 0.0f) + Settings.MinSeparation;

	// Straight while that keeps clear of the target
	if (FMath::PointDistToSegment(FVector::ZeroVector, ToFrom, ToPlace) >= MinRadius)
	{
		return Place;
	}

	const double FromRadius = ToFrom.Size();
	const double Delta = FMath::FindDeltaAngleDegrees(GetYawDegrees(ToFrom), GetYawDegrees(ToPlace));
	const double StepAngle = FMath::RadiansToDegrees(FMath::Atan2(ArcStep, FMath::Max(FromRadius, MinRadius)));
	if (FMath::Abs(Delta) <= StepAngle)
	{
		return Place;
	}

	// Around it, easing toward the place's distance: a step along the tangent, as chasing a point on the arc itself spirals inside it
	const double Radius = FMath::Max(MinRadius, FMath::Lerp(FromRadius, ToPlace.Size(), StepAngle / FMath::Abs(Delta)));
	const FVector Outward = ToFrom.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector Tangent = FVector(-Outward.Y, Outward.X, 0.0) * FMath::Sign(Delta);
	const FVector Point = Center + Outward * Radius + Tangent * ArcStep;
	return FVector(Point.X, Point.Y, Place.Z);
}

float UKzEngagementComponent::GetSectorLoad(int32 Area, int32 Sector, const FEngager* Ignore) const
{
	float Load = 0.0f;
	for (const FEngager& Engager : Engagers)
	{
		if (&Engager == Ignore || !Engager.Actor.IsValid())
		{
			continue;
		}

		const FVector Location = GetCountedLocation(Engager);
		if (GetAreaAt(Location) == Area && GetSectorAt(Area, Location) == Sector)
		{
			Load += Engager.Weight;
		}
	}
	return Load;
}

int32 UKzEngagementComponent::GetAreaCount(int32 Area, const FEngager* Ignore) const
{
	int32 Count = 0;
	for (const FEngager& Engager : Engagers)
	{
		if (&Engager != Ignore && Engager.Actor.IsValid() && GetAreaAt(GetCountedLocation(Engager)) == Area)
		{
			++Count;
		}
	}
	return Count;
}

bool UKzEngagementComponent::HasRoom(int32 Area, const FEngager* Ignore) const
{
	const int32 Capacity = Settings.Areas[Area].Capacity;
	return Capacity <= 0 || GetAreaCount(Area, Ignore) < Capacity;
}

bool UKzEngagementComponent::IsExempt(const FEngager& Engager) const
{
	const IGameplayTagAssetInterface* Tags = Cast<IGameplayTagAssetInterface>(Engager.Actor.Get());
	return Tags && Tags->HasAnyMatchingGameplayTags(Settings.ExemptTags);
}

int32 UKzEngagementComponent::GetPreferredArea(const FEngager& Engager) const
{
	if (Engager.PreferredArea.IsValid())
	{
		const int32 Index = Settings.Areas.IndexOfByPredicate([&Engager](const FKzEngagementArea& Area) { return Area.Tag == Engager.PreferredArea; });
		if (Index != INDEX_NONE)
		{
			return Index;
		}
	}
	return 0;
}

int32 UKzEngagementComponent::ChooseArea(const FEngager& Engager) const
{
	for (int32 Area = GetPreferredArea(Engager); Area < Settings.Areas.Num(); ++Area)
	{
		if (HasRoom(Area, &Engager))
		{
			return Area;
		}
	}
	return Settings.Areas.Num() - 1;
}

bool UKzEngagementComponent::IsAreaLeaver(const FEngager& Engager, int32 Area) const
{
	const int32 Capacity = Settings.Areas[Area].Capacity;
	const int32 Excess = Capacity > 0 ? GetAreaCount(Area, nullptr) - Capacity : 0;
	if (Excess <= 0)
	{
		return false;
	}

	const FVector Center = GetCenter();
	TArray<TPair<double, const FEngager*>> Candidates;
	for (const FEngager& Other : Engagers)
	{
		if (Other.Actor.IsValid() && !Other.bTravelling && !IsExempt(Other) && GetAreaAt(GetCountedLocation(Other)) == Area)
		{
			Candidates.Emplace(FVector::DistSquared2D(GetCountedLocation(Other), Center), &Other);
		}
	}
	Candidates.Sort([](const TPair<double, const FEngager*>& A, const TPair<double, const FEngager*>& B) { return A.Key > B.Key; });

	for (int32 Index = 0; Index < Excess && Index < Candidates.Num(); ++Index)
	{
		if (Candidates[Index].Value == &Engager)
		{
			return true;
		}
	}
	return false;
}

bool UKzEngagementComponent::IsSectorLeaver(const FEngager& Engager, int32 Area, int32 Sector) const
{
	const FKzEngagementArea& AreaSettings = Settings.Areas[Area];
	const int32 Count = AreaSettings.GetSectorCount();
	const float Weight = AreaSettings.GetSectorWeight(Sector);
	float Load = GetSectorLoad(Area, Sector, nullptr);
	if (Count <= 1 || Load <= Weight)
	{
		return false;
	}

	const float SectorAngle = 360.0f / Count;
	const float Start = Sector * SectorAngle;
	const int32 Previous = (Sector + Count - 1) % Count;
	const int32 Next = (Sector + 1) % Count;
	const float PreviousLoad = GetSectorLoad(Area, Previous, nullptr);
	const float NextLoad = GetSectorLoad(Area, Next, nullptr);

	// How far each settled one that isn't exempt is from a side with room for it, else from the lighter side
	TArray<TPair<float, const FEngager*>> Candidates;
	for (const FEngager& Other : Engagers)
	{
		if (!Other.Actor.IsValid() || Other.bTravelling || IsExempt(Other))
		{
			continue;
		}

		const FVector Location = GetCountedLocation(Other);
		if (GetAreaAt(Location) != Area || GetSectorAt(Area, Location) != Sector)
		{
			continue;
		}

		const float Bearing = GetBearing(Location);
		const float ToPrevious = Bearing - Start;
		const float ToNext = Start + SectorAngle - Bearing;
		const bool bPreviousRoom = PreviousLoad + Other.Weight <= AreaSettings.GetSectorWeight(Previous);
		const bool bNextRoom = NextLoad + Other.Weight <= AreaSettings.GetSectorWeight(Next);

		float Distance;
		if (bPreviousRoom && bNextRoom)
		{
			Distance = FMath::Min(ToPrevious, ToNext);
		}
		else if (bPreviousRoom || bNextRoom)
		{
			Distance = bPreviousRoom ? ToPrevious : ToNext;
		}
		else
		{
			Distance = PreviousLoad - AreaSettings.GetSectorWeight(Previous) <= NextLoad - AreaSettings.GetSectorWeight(Next) ? ToPrevious : ToNext;
		}
		Candidates.Emplace(Distance, &Other);
	}
	Candidates.Sort([](const TPair<float, const FEngager*>& A, const TPair<float, const FEngager*>& B) { return A.Key < B.Key; });

	for (const TPair<float, const FEngager*>& Candidate : Candidates)
	{
		if (Load <= Weight)
		{
			break;
		}
		if (Candidate.Value == &Engager)
		{
			return true;
		}
		Load -= Candidate.Value->Weight;
	}
	return false;
}

float UKzEngagementComponent::GetBearing(const FVector& Location) const
{
	const FVector Offset = Location - GetCenter();
	return static_cast<float>(FRotator::ClampAxis(FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X)) - GetReferenceYaw()));
}

FVector UKzEngagementComponent::GetLocationAt(float Bearing, float Radius) const
{
	const float Yaw = FMath::DegreesToRadians(GetReferenceYaw() + Bearing);
	return GetCenter() + FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.0f) * Radius;
}

FVector UKzEngagementComponent::GetCenter() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

void UKzEngagementComponent::UpdateTick()
{
	SetComponentTickEnabled(CVarDrawEngagement.GetValueOnGameThread() && !Engagers.IsEmpty());
}