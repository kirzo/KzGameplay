// Copyright 2026 kirzo

#include "Validation/KzCapabilitySetValidators.h"
#include "Capabilities/KzCapabilitySet.h"
#include "Abilities/KzGameplayAbility.h"
#include "Input/KzInputProfile.h"

#define LOCTEXT_NAMESPACE "KzCapabilitySetValidators"

bool UKzCapabilitySetValidator::CanValidate_Implementation(const UObject* Asset) const
{
	return Asset && Asset->IsA<UKzCapabilitySet>();
}

void UKzCapabilitySetValidator::Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const
{
	const UKzCapabilitySet* Set = Cast<UKzCapabilitySet>(Asset);
	if (!Set) { return; }

	const FName Id = GetValidatorId();

	TSet<const UKzCapabilitySet*> Chain = { Set };
	for (const UKzCapabilitySet* Parent = Set->ParentSet; Parent; Parent = Parent->ParentSet)
	{
		bool bLoops = false;
		Chain.Add(Parent, &bLoops);
		if (bLoops)
		{
			OutIssues.Add(FKzValidationIssue(
				EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("ParentLoop", "The parent chain loops back on '{0}'."), FText::FromString(Parent->GetName())),
				Id));
			return;
		}
	}

	// An input may come from the profile of another capability, so every profile the set resolves to counts
	TMap<FGameplayTag, FKzCapabilityGrant> Resolved;
	Set->CollectCapabilities(Resolved);

	FGameplayTagContainer DeclaredInputs;
	for (const TPair<FGameplayTag, FKzCapabilityGrant>& Pair : Resolved)
	{
		for (const UKzInputProfile* Profile : Pair.Value.InputProfiles)
		{
			if (!Profile) { continue; }
			for (const FKzInputAction& Action : Profile->InputActions)
			{
				DeclaredInputs.AddTag(Action.InputTag);
			}
		}
	}

	// A capability's place in the map is its row in the editor
	int32 Index = INDEX_NONE;
	for (const TPair<FGameplayTag, FKzCapabilityGrant>& Pair : Set->Capabilities)
	{
		++Index;

		const UClass* AbilityClass = Pair.Value.Ability.Get();
		const UKzGameplayAbility* Ability = AbilityClass ? Cast<UKzGameplayAbility>(AbilityClass->GetDefaultObject()) : nullptr;
		if (!Ability) { continue; }

		for (const FGameplayTag& Input : Ability->GetBoundInputTags())
		{
			if (!DeclaredInputs.HasTagExact(Input))
			{
				OutIssues.Add(FKzValidationIssue::WithContextIndex(
					EKzValidationSeverity::Warning,
					FText::Format(LOCTEXT("InputNotDeclared", "{0}: its ability listens to {1}, but no Input Profile in this set declares it."),
						FText::FromString(Pair.Key.ToString()), FText::FromString(Input.ToString())),
					Id, Index));
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE