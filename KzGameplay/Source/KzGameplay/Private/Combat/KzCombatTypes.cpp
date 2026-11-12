// Copyright 2026 kirzo

#include "Combat/KzCombatTypes.h"

FVector FKzHitDirection::GetPresetVector(EKzHitDirectionPreset InPreset)
{
	switch (InPreset)
	{
	case EKzHitDirectionPreset::RightToLeft:
		return FVector::LeftVector;
	case EKzHitDirectionPreset::LeftToRight:
		return FVector::RightVector;
	case EKzHitDirectionPreset::Overhead:
		return FVector::DownVector;
	case EKzHitDirectionPreset::Uppercut:
		return FVector::UpVector;
	default:
		return FVector::ForwardVector;
	}
}

FVector FKzHitDirection::ToWorld(const FVector& Attacker, const FVector& Target, const FVector& Motion) const
{
	FVector Forward = (Target - Attacker).GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}

	if (Preset == EKzHitDirectionPreset::HitboxMotion)
	{
		return Motion.IsNearlyZero(1.0) ? Forward : Motion.GetSafeNormal();
	}

	const FVector Local = Preset == EKzHitDirectionPreset::Custom ? Vector : GetPresetVector(Preset);
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FVector World = Forward * Local.X + Right * Local.Y + FVector::UpVector * Local.Z;
	return World.IsNearlyZero() ? Forward : World.GetSafeNormal();
}

bool FKzHitReactionTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayAbilityTargetData_SingleTargetHit::NetSerialize(Ar, Map, bOutSuccess);
	Ar << Direction;
	Ar << Knockback;
	return true;
}