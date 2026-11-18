// Copyright 2026 kirzo

#include "Combat/KzCombatTypes.h"
#include "Combat/KzCombatSettings.h"
#include "Combat/KzMeshShakeSubsystem.h"
#include "UObject/PropertyTag.h"

const FVector* FKzHitDirection::FindPreset(const FGameplayTag& Tag)
{
	return UKzCombatSettings::Get()->HitDirections.Find(Tag);
}

void FKzHitDirection::PostSerialize(const FArchive& Ar)
{
	// Saved before the presets moved to Project Settings: the same preset, now by tag. None was the default then, and is never saved now.
	if (!Ar.IsLoading() || Preset_DEPRECATED == EKzHitDirectionPreset::None)
	{
		return;
	}

	using namespace KzTags::Combat;
	switch (Preset_DEPRECATED)
	{
	case EKzHitDirectionPreset::HitboxMotion:
		Mode = EKzHitDirectionMode::HitboxMotion;
		break;
	case EKzHitDirectionPreset::Custom:
		Mode = EKzHitDirectionMode::Custom;
		break;
	default:
		Mode = EKzHitDirectionMode::Preset;
		PresetTag = Preset_DEPRECATED == EKzHitDirectionPreset::RightToLeft ? HitDirection_RightToLeft.GetTag()
			: Preset_DEPRECATED == EKzHitDirectionPreset::LeftToRight ? HitDirection_LeftToRight.GetTag()
			: Preset_DEPRECATED == EKzHitDirectionPreset::Overhead ? HitDirection_Overhead.GetTag()
			: Preset_DEPRECATED == EKzHitDirectionPreset::Uppercut ? HitDirection_Uppercut.GetTag()
			: HitDirection_Thrust.GetTag();
		break;
	}
	Preset_DEPRECATED = EKzHitDirectionPreset::None;
}

FVector FKzHitDirection::ToWorld(const FVector& Attacker, const FVector& Target, const FVector& Motion) const
{
	FVector Forward = (Target - Attacker).GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}

	if (Mode == EKzHitDirectionMode::HitboxMotion)
	{
		return Motion.IsNearlyZero(1.0) ? Forward : Motion.GetSafeNormal();
	}

	const FVector* Preset = Mode == EKzHitDirectionMode::Preset ? FindPreset(PresetTag) : nullptr;
	const FVector Local = Mode == EKzHitDirectionMode::Custom ? Vector : Preset ? *Preset : FVector::ForwardVector;
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

void FKzHitShake::Shake(AActor* Target, const FVector& Direction, float HitStop) const
{
	UKzMeshShakeSubsystem::ShakeMesh(Target, Direction, Amplitude, Duration > 0.0f ? Duration : HitStop, Frequency, Start * HitStop);
}

bool FKzHitShake::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	if (Tag.Type == NAME_FloatProperty)
	{
		Slot << Amplitude;
		return true;
	}
	return false;
}