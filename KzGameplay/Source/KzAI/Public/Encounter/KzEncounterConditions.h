// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "ScriptableConditions/ScriptableCondition.h"
#include "ScriptableConditions/ScriptableCondition_Compare.h"
#include "KzEncounterConditions.generated.h"

class UKzEncounterComponent;

UENUM(BlueprintType)
enum class EKzEncounterSince : uint8
{
	/** Since the last of its Waves came, or since it started if none has. */
	LastWave UMETA(DisplayName = "Last Wave"),
	Start
};

UENUM(BlueprintType)
enum class EKzEncounterEnemies : uint8
{
	All,

	/** Those of the last of its Waves to come, or the Initial ones before any. */
	LastWave UMETA(DisplayName = "Last Wave")
};

UENUM(BlueprintType)
enum class EKzEncounterAttributeMode : uint8
{
	/** The average of all of them. */
	Group,

	/** The one with the lowest value. */
	Lower
};

/** Base of the conditions that read the encounter they belong to, such as in its Start When and End When. False anywhere else. */
UCLASS(Abstract)
class KZAI_API UKzEncounterCondition : public UScriptableCondition
{
	GENERATED_BODY()

protected:
	const UKzEncounterComponent* GetEncounter() const;
};

/** So many of its enemies defeated, such as a set number of kills, or the one tagged Encounter.Boss. */
UCLASS(DisplayName = "Encounter Defeated", meta = (ConditionCategory = "Gameplay|Encounter"))
class KZAI_API UKzEncounterCondition_Defeated : public UKzEncounterCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Encounter", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	EKzEncounterSince Since = EKzEncounterSince::LastWave;

	/** Only enemies whose spawn has any of these. Empty counts them all. */
	UPROPERTY(EditAnywhere, Category = "Encounter")
	FGameplayTagContainer Tags;

protected:
	virtual bool Evaluate_Implementation() const override;
};

/** So long since its last wave came, or since it started. */
UCLASS(DisplayName = "Encounter Time", meta = (ConditionCategory = "Gameplay|Encounter"))
class KZAI_API UKzEncounterCondition_Time : public UKzEncounterCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Encounter", meta = (ClampMin = "0", Units = "Seconds"))
	float Seconds = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	EKzEncounterSince Since = EKzEncounterSince::LastWave;

protected:
	virtual bool Evaluate_Implementation() const override;
};

/** So many of its Waves defeated whole. The Initial enemies aren't one. */
UCLASS(DisplayName = "Encounter Waves", meta = (ConditionCategory = "Gameplay|Encounter"))
class KZAI_API UKzEncounterCondition_Waves : public UKzEncounterCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Encounter", meta = (ClampMin = "1"))
	int32 Count = 1;

protected:
	virtual bool Evaluate_Implementation() const override;
};

/**
 * So few of its enemies alive or still to spawn: such as one left, to bring the next wave before the last one is gone, or none of the last
 * wave, while some of an earlier one still fight.
 */
UCLASS(DisplayName = "Encounter Alive", meta = (ConditionCategory = "Gameplay|Encounter"))
class KZAI_API UKzEncounterCondition_Alive : public UKzEncounterCondition
{
	GENERATED_BODY()

public:
	/** At most this many. */
	UPROPERTY(EditAnywhere, Category = "Encounter", meta = (ClampMin = "0"))
	int32 Count = 0;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	EKzEncounterEnemies Of = EKzEncounterEnemies::All;

	/** Only enemies whose spawn has any of these. Empty counts them all. */
	UPROPERTY(EditAnywhere, Category = "Encounter")
	FGameplayTagContainer Tags;

protected:
	virtual bool Evaluate_Implementation() const override;
};

/** An attribute of its enemies alive reaches a value, such as their health: their average, or the lowest of them. False without any. */
UCLASS(DisplayName = "Encounter Attribute", meta = (ConditionCategory = "Gameplay|Encounter"))
class KZAI_API UKzEncounterCondition_Attribute : public UKzEncounterCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Encounter")
	FGameplayAttribute Attribute;

	/** Compares Attribute as a fraction of this one, such as health of max health. */
	UPROPERTY(EditAnywhere, Category = "Encounter")
	FGameplayAttribute MaxAttribute;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	EKzEncounterAttributeMode Mode = EKzEncounterAttributeMode::Group;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	EScriptableComparisonOp Operation = EScriptableComparisonOp::LessOrEqual;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float Value = 0.5f;

	/** Only enemies whose spawn has any of these. Empty takes them all. */
	UPROPERTY(EditAnywhere, Category = "Encounter")
	FGameplayTagContainer Tags;

protected:
	virtual bool Evaluate_Implementation() const override;
};