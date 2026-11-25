// Copyright 2026 kirzo

#include "Encounter/KzEncounterConditions.h"
#include "Encounter/KzEncounterComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

namespace
{
	bool Compare(double A, EScriptableComparisonOp Operation, double B)
	{
		switch (Operation)
		{
		case EScriptableComparisonOp::Equal: return FMath::IsNearlyEqual(A, B);
		case EScriptableComparisonOp::NotEqual: return !FMath::IsNearlyEqual(A, B);
		case EScriptableComparisonOp::Less: return A < B;
		case EScriptableComparisonOp::LessOrEqual: return A <= B;
		case EScriptableComparisonOp::Greater: return A > B;
		case EScriptableComparisonOp::GreaterOrEqual: return A >= B;
		}
		return false;
	}
}

const UKzEncounterComponent* UKzEncounterCondition::GetEncounter() const
{
	return GetOwner<UKzEncounterComponent>();
}

bool UKzEncounterCondition_Defeated::Evaluate_Implementation() const
{
	const UKzEncounterComponent* Encounter = GetEncounter();
	return Encounter && Encounter->GetDefeatedCount(Since == EKzEncounterSince::LastWave, Tags) >= Count;
}

bool UKzEncounterCondition_Time::Evaluate_Implementation() const
{
	const UKzEncounterComponent* Encounter = GetEncounter();
	return Encounter && Encounter->GetState() != EKzEncounterState::Waiting && Encounter->GetTimeSince(Since == EKzEncounterSince::LastWave) >= Seconds;
}

bool UKzEncounterCondition_Waves::Evaluate_Implementation() const
{
	const UKzEncounterComponent* Encounter = GetEncounter();
	return Encounter && Encounter->GetWavesCleared() >= Count;
}

bool UKzEncounterCondition_Alive::Evaluate_Implementation() const
{
	const UKzEncounterComponent* Encounter = GetEncounter();
	return Encounter && Encounter->GetAliveCount(Of == EKzEncounterEnemies::LastWave, Tags) <= Count;
}

bool UKzEncounterCondition_Attribute::Evaluate_Implementation() const
{
	const UKzEncounterComponent* Encounter = GetEncounter();
	if (!Encounter || !Attribute.IsValid())
	{
		return false;
	}

	TArray<AActor*> Enemies;
	Encounter->GetEnemies(Tags, Enemies);

	double Sum = 0.0;
	double Lowest = TNumericLimits<double>::Max();
	int32 Count = 0;
	for (AActor* Enemy : Enemies)
	{
		const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Enemy);
		if (!AbilitySystem || !AbilitySystem->HasAttributeSetForAttribute(Attribute))
		{
			continue;
		}

		double Current = AbilitySystem->GetNumericAttribute(Attribute);
		if (MaxAttribute.IsValid())
		{
			const double Max = AbilitySystem->GetNumericAttribute(MaxAttribute);
			Current = Max > 0.0 ? Current / Max : 0.0;
		}

		Sum += Current;
		Lowest = FMath::Min(Lowest, Current);
		++Count;
	}

	return Count > 0 && Compare(Mode == EKzEncounterAttributeMode::Group ? Sum / Count : Lowest, Operation, Value);
}