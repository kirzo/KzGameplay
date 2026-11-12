// Copyright 2026 kirzo

#include "Validation/KzInputProfileValidators.h"
#include "Input/KzInputProfile.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#define LOCTEXT_NAMESPACE "KzInputProfileValidators"

bool UKzInputProfileValidator::CanValidate_Implementation(const UObject* Asset) const
{
	return Asset && Asset->IsA<UKzInputProfile>();
}

void UKzInputProfileValidator::Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const
{
	const UKzInputProfile* Profile = Cast<UKzInputProfile>(Asset);
	if (!Profile) { return; }

	const FName Id = GetValidatorId();
	TMap<FGameplayTag, int32> FirstOccurrence;

	for (int32 i = 0; i < Profile->InputActions.Num(); ++i)
	{
		const FKzInputAction& Action = Profile->InputActions[i];

		if (!Action.InputAction)
		{
			OutIssues.Add(FKzValidationIssue::WithContextIndex(
				EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("NullAction", "Entry {0} has no Input Action assigned."), FText::AsNumber(i + 1)),
				Id, i));
		}

		if (!Action.InputTag.IsValid())
		{
			OutIssues.Add(FKzValidationIssue::WithContextIndex(
				EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("MissingTag", "Entry {0} has no Input Tag set."), FText::AsNumber(i + 1)),
				Id, i));
			continue;
		}

		if (const int32* Earlier = FirstOccurrence.Find(Action.InputTag))
		{
			OutIssues.Add(FKzValidationIssue::WithContextIndex(
				EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("DuplicateTag", "Entry {0} reuses Input Tag '{1}' (already at entry {2})."),
					FText::AsNumber(i + 1), FText::FromString(Action.InputTag.ToString()), FText::AsNumber(*Earlier + 1)),
				Id, i));
		}
		else
		{
			FirstOccurrence.Add(Action.InputTag, i);
		}
	}

	const UInputMappingContext* Context = Profile->MappingContext;
	if (!Context) { return; }

	const FText ContextName = FText::FromString(Context->GetName());
	TSet<const UInputAction*> Declared;

	for (int32 i = 0; i < Profile->InputActions.Num(); ++i)
	{
		const UInputAction* Action = Profile->InputActions[i].InputAction;
		if (!Action) { continue; }
		Declared.Add(Action);

		const bool bHasKey = Context->GetMappings().ContainsByPredicate([Action](const FEnhancedActionKeyMapping& Mapping) { return Mapping.Action == Action && Mapping.Key.IsValid(); });
		if (!bHasKey)
		{
			OutIssues.Add(FKzValidationIssue::WithContextIndex(
				EKzValidationSeverity::Warning,
				FText::Format(LOCTEXT("NoKey", "Entry {0}: '{1}' has no key in '{2}'."), FText::AsNumber(i + 1), FText::FromString(Action->GetName()), ContextName),
				Id, i));
		}
	}

	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		const UInputAction* Action = Mapping.Action;
		if (Action && !Declared.Contains(Action))
		{
			Declared.Add(Action);
			OutIssues.Add(FKzValidationIssue(
				EKzValidationSeverity::Warning,
				FText::Format(LOCTEXT("Undeclared", "'{0}' maps '{1}', which this profile does not declare: its keys do nothing."), ContextName, FText::FromString(Action->GetName())),
				Id));
		}
	}
}

#undef LOCTEXT_NAMESPACE