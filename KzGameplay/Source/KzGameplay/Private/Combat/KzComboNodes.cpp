// Copyright 2026 kirzo

#include "Combat/KzComboNodes.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzComboGraph.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "KzComboNodes"

namespace
{
	// What holds a hitbox turns this much at most between two steps of a sweep.
	constexpr float MaxStepAngle = UE_DOUBLE_PI / 18.0;
	constexpr int32 MaxSteps = 8;
}

USceneComponent* FKzComboHitbox::FindParent(USkeletalMeshComponent* AnimatedMesh, USceneComponent* WeaponRoot) const
{
	if (AttachTo == EKzComboHitboxAttach::Avatar)
	{
		return AnimatedMesh;
	}

	const AActor* Weapon = WeaponRoot ? WeaponRoot->GetOwner() : nullptr;
	if (Component.IsNone() || !Weapon || Weapon->GetRootComponent() != WeaponRoot)
	{
		return WeaponRoot;
	}

	TInlineComponentArray<USceneComponent*> Components(Weapon);
	USceneComponent* const* Found = Components.FindByPredicate([this](const USceneComponent* Each) { return Each->GetFName() == Component; });
	return Found ? *Found : nullptr;
}

FTransform FKzComboHitbox::GetTransform(const USceneComponent& Parent) const
{
	return Offset * Parent.GetSocketTransform(Socket);
}

FTransform FKzComboHitbox::GetTransform(const FTransform& From, const FTransform& To, float Alpha) const
{
	FTransform Parent;
	Parent.Blend(From, To, Alpha);
	return Offset * Parent;
}

int32 FKzComboHitbox::GetSweepSteps(const FTransform& From, const FTransform& To)
{
	return FMath::Clamp(FMath::CeilToInt(From.GetRotation().AngularDistance(To.GetRotation()) / MaxStepAngle), 1, MaxSteps);
}

const FName FKzComboNode_Branch::DefaultName(TEXT("Default"));
const FName FKzComboNode_BranchOnHit::HitName(TEXT("Hit"));
const FName FKzComboNode_BranchOnHit::MissName(TEXT("Miss"));

FName FKzComboNode_SwitchOnWindow::PickOutput(const FKzComboBranchContext& Context, FName& OutFallback) const
{
	OutFallback = DefaultName;
	return Context.Window.IsValid() && Windows.Contains(Context.Window) ? Context.Window.GetTagName() : DefaultName;
}

FName FKzComboNode_BranchOnHit::PickOutput(const FKzComboBranchContext& Context, FName& OutFallback) const
{
	OutFallback = NAME_None;
	return Context.bHit ? HitName : MissName;
}

FName FKzComboNode_SwitchOnTags::PickOutput(const FKzComboBranchContext& Context, FName& OutFallback) const
{
	OutFallback = DefaultName;
	const FKzComboTagCase* Case = Cases.FindByPredicate([&Context](const FKzComboTagCase& Each) { return !Each.RequiredTags.IsEmpty() && Context.AttackerTags.HasAll(Each.RequiredTags); });
	return Case ? Case->GetOutputName() : DefaultName;
}

#if WITH_EDITOR
namespace
{
	FString GetLeaf(FGameplayTag Tag)
	{
		const FString Path = Tag.ToString();
		FString Leaf = Path;
		Path.Split(TEXT("."), nullptr, &Leaf, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		return Leaf;
	}

	FSlateIcon NodeIcon(const TCHAR* Node)
	{
		return FSlateIcon(TEXT("KzGameplayEditorStyle"), *FString::Printf(TEXT("KzGameplay.Combo.Node.%s"), Node));
	}

	FText GetInputLabel(FGameplayTag Input)
	{
		return FText::FromString(GetLeaf(Input));
	}

	/** Adds a pin unless one of that name is there, as a case or window listed twice would make. */
	void AddPin(TArray<FKzGraphPinDesc>& OutPins, FKzGraphPinDesc&& Pin)
	{
		if (!OutPins.ContainsByPredicate([&Pin](const FKzGraphPinDesc& Each) { return Each.Name == Pin.Name; }))
		{
			OutPins.Add(MoveTemp(Pin));
		}
	}
}

void FKzComboNode_Entry::GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const
{
	if (!Input.IsValid())
	{
		return;
	}

	OutPins.Add({ Input.GetTagName(), GetInputLabel(Input), NAME_None, LOCTEXT("EntryOutputTooltip", "Where this input starts the combo.") });
}

FSlateIcon FKzComboNode_Entry::GetIcon() const
{
	return NodeIcon(TEXT("Entry"));
}

FLinearColor FKzComboNode_Entry::GetColor() const
{
	return FLinearColor(0.4f, 0.06f, 0.06f);
}

FLinearColor FKzComboNode_Branch::GetColor() const
{
	return FLinearColor(0.12f, 0.12f, 0.12f);
}

void FKzComboNode_SwitchOnWindow::GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const
{
	for (const FGameplayTag Window : Windows)
	{
		if (Window.IsValid())
		{
			AddPin(OutPins, { Window.GetTagName(), FText::FromString(GetLeaf(Window)), NAME_None, LOCTEXT("WindowTooltip", "Where a press in this Combo Window leads.") });
		}
	}
	OutPins.Add({ DefaultName, LOCTEXT("Default", "Default"), NAME_None, LOCTEXT("WindowDefaultTooltip", "Where a press in any other Combo Window leads, or in one above left unlinked.") });
}

void FKzComboNode_SwitchOnWindow::Validate(TArray<FKzValidationIssue>& OutIssues) const
{
	TSet<FGameplayTag> Listed;
	for (const FGameplayTag Window : Windows)
	{
		bool bListed = false;
		Listed.Add(Window, &bListed);
		if (!Window.IsValid())
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("EmptyWindow", "A window is empty."), NAME_None));
		}
		else if (bListed)
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("DuplicateWindow", "{0} is listed twice."), FText::FromName(Window.GetTagName())), NAME_None));
		}
	}
}

void FKzComboNode_BranchOnHit::GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const
{
	OutPins.Add({ HitName, LOCTEXT("Hit", "Hit"), NAME_None, LOCTEXT("HitTooltip", "Where it leads once the attack playing hit something.") });
	OutPins.Add({ MissName, LOCTEXT("Miss", "Miss"), NAME_None, LOCTEXT("MissTooltip", "Where it leads while the attack playing hit nothing.") });
}

void FKzComboNode_SwitchOnTags::GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const
{
	for (const FKzComboTagCase& Case : Cases)
	{
		if (Case.RequiredTags.IsEmpty())
		{
			continue;
		}

		TArray<FString> Leaves;
		for (const FGameplayTag Tag : Case.RequiredTags)
		{
			Leaves.Add(GetLeaf(Tag));
		}
		AddPin(OutPins, { Case.GetOutputName(), FText::FromString(FString::Join(Leaves, TEXT(", "))), NAME_None, FText::Format(LOCTEXT("CaseTooltip", "Where it leads while the attacker has {0}."), FText::FromString(Case.RequiredTags.ToStringSimple())) });
	}
	OutPins.Add({ DefaultName, LOCTEXT("Default", "Default"), NAME_None, LOCTEXT("TagsDefaultTooltip", "Where it leads for an attacker that matches no case above, or one left unlinked.") });
}

void FKzComboNode_SwitchOnTags::Validate(TArray<FKzValidationIssue>& OutIssues) const
{
	TSet<FName> Listed;
	for (const FKzComboTagCase& Case : Cases)
	{
		bool bListed = false;
		Listed.Add(Case.GetOutputName(), &bListed);
		if (Case.RequiredTags.IsEmpty())
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("EmptyCase", "A case requires no tags; add them or remove it."), NAME_None));
		}
		else if (bListed)
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("DuplicateCase", "Two cases require {0}."), FText::FromString(Case.RequiredTags.ToStringSimple())), NAME_None));
		}
	}
}

void FKzComboNode_Attack::GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const
{
	const UKzComboGraph* Combo = Cast<UKzComboGraph>(&Graph);
	if (!Combo)
	{
		return;
	}

	// One input needs no name on its pins.
	const bool bSeveral = Combo->Inputs.Num() > 1;
	for (const FGameplayTag Input : Combo->Inputs)
	{
		if (!Input.IsValid())
		{
			continue;
		}

		OutPins.Add({ Input.GetTagName(), bSeveral ? GetInputLabel(Input) : FText::GetEmpty(), NAME_None, LOCTEXT("NextTooltip", "Where a press of this input in a Combo Window leads.") });
	}
}

FText FKzComboNode_Attack::GetSubtitle() const
{
	return Animation ? Animation->GetDescription() : LOCTEXT("NoAnimation", "No animation");
}

FSlateIcon FKzComboNode_Attack::GetIcon() const
{
	return NodeIcon(TEXT("Attack"));
}

FLinearColor FKzComboNode_Attack::GetColor() const
{
	return FLinearColor(0.45f, 0.17f, 0.04f);
}

void FKzComboNode_Attack::Validate(TArray<FKzValidationIssue>& OutIssues) const
{
	if (!Animation)
	{
		OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("NoAnimationIssue", "Pick how the attack is animated."), NAME_None));
	}
}
#endif

#undef LOCTEXT_NAMESPACE