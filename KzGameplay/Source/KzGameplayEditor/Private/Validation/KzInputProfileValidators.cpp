// Copyright 2026 kirzo

#include "Validation/KzInputProfileValidators.h"
#include "Input/KzInputProfile.h"

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
}

#undef LOCTEXT_NAMESPACE
