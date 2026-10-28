// Copyright 2026 kirzo

#include "Combat/KzComboGraph.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzComboNodes.h"
#include "UObject/UObjectHash.h"

#define LOCTEXT_NAMESPACE "KzComboGraph"

#if WITH_EDITOR
bool UKzComboGraph::MakeNodeForAsset(UObject& Asset, TInstancedStruct<FKzGraphNode>& OutNode) const
{
	TArray<UClass*> Sources;
	GetDerivedClasses(UKzAnimationSource::StaticClass(), Sources);
	for (UClass* Class : Sources)
	{
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}

		// The schema gives the new node its own copy of the source.
		UKzAnimationSource* Source = NewObject<UKzAnimationSource>(GetTransientPackage(), Class);
		if (Source->SetAsset(Asset))
		{
			FKzComboNode_Attack Attack;
			Attack.Animation = Source;
			OutNode.InitializeAs<FKzComboNode_Attack>(Attack);
			return true;
		}
	}
	return false;
}

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

	const int32 Entry = InData.FindEntry(FKzComboNode_Entry::EntryName);
	if (Entry == INDEX_NONE)
	{
		OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("NoEntry", "The combo has no Entry."), NAME_None));
		return;
	}

	const FKzComboNode_Entry* EntryNode = InData.GetNode<FKzComboNode_Entry>(Entry);
	if (!EntryNode)
	{
		return;
	}

	TSet<FName> Openers;
	for (const FKzComboOpener& Opener : EntryNode->Openers)
	{
		bool bListed = false;
		Openers.Add(Opener.GetOutputName(), &bListed);
		if (!Inputs.Contains(Opener.Input))
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("OpenerInput", "An opener uses {0}, which is not one of the combo's inputs."), FText::FromName(Opener.Input.GetTagName())), NAME_None));
		}
		else if (Opener.RequiredTags.IsEmpty())
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Warning, FText::Format(LOCTEXT("OpenerNoTags", "An opener of {0} requires no tags, so it always wins over the input's own output."), FText::FromName(Opener.Input.GetTagName())), NAME_None));
		}
		else if (bListed)
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("DuplicateOpener", "Two openers of {0} require the same tags."), FText::FromName(Opener.Input.GetTagName())), NAME_None));
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE