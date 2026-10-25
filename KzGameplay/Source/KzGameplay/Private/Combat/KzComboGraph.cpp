// Copyright 2026 kirzo

#include "Combat/KzComboGraph.h"
#include "Combat/KzComboNodes.h"

#define LOCTEXT_NAMESPACE "KzComboGraph"

#if WITH_EDITOR
const UScriptStruct* UKzComboGraph::GetNodeStruct() const
{
	return FKzComboNode::StaticStruct();
}

void UKzComboGraph::GetDefaultNodes(TArray<const UScriptStruct*>& OutNodes) const
{
	OutNodes.Add(FKzComboNode_Entry::StaticStruct());
}

void UKzComboGraph::ValidateGraph(const FKzGraphData& InData, TArray<FKzValidationIssue>& OutIssues) const
{
	if (Inputs.IsEmpty())
	{
		OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("NoInputs", "Add the inputs that start and chain attacks."), NAME_None));
	}

	TSet<FGameplayTag> Listed;
	for (const FGameplayTag Input : Inputs)
	{
		bool bListed = false;
		Listed.Add(Input, &bListed);
		if (!Input.IsValid())
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("EmptyInput", "An input is empty."), NAME_None));
		}
		else if (bListed)
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("DuplicateInput", "{0} is listed twice."), FText::FromName(Input.GetTagName())), NAME_None));
		}
	}

	if (InData.FindEntry(FKzComboNode_Entry::EntryName) == INDEX_NONE)
	{
		OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("NoEntry", "The combo has no Entry."), NAME_None));
	}
}
#endif

#undef LOCTEXT_NAMESPACE