// Copyright 2026 kirzo

#include "Combat/KzComboNodes.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzComboGraph.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "KzComboNodes"

const FName FKzComboNode_Entry::EntryName(TEXT("Entry"));

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

FName FKzComboOpener::GetOutputName() const
{
	return FName(FString::Printf(TEXT("%s(%s)"), *Input.ToString(), *RequiredTags.ToStringSimple()));
}

FName FKzComboNode_Entry::GetOutputName(FGameplayTag Input, const FGameplayTagContainer& AttackerTags) const
{
	const FKzComboOpener* Opener = Openers.FindByPredicate([&](const FKzComboOpener& Each) { return Each.Input == Input && AttackerTags.HasAll(Each.RequiredTags); });
	return Opener ? Opener->GetOutputName() : Input.GetTagName();
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
}

void FKzComboNode_Entry::GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const
{
	const UKzComboGraph* Combo = Cast<UKzComboGraph>(&Graph);
	if (!Combo)
	{
		return;
	}

	for (const FGameplayTag Input : Combo->Inputs)
	{
		if (Input.IsValid())
		{
			OutPins.Add({ Input.GetTagName(), GetInputLabel(Input), NAME_None, LOCTEXT("EntryOutputTooltip", "The attack this input starts the combo with.") });
		}
	}

	for (const FKzComboOpener& Opener : Openers)
	{
		if (!Opener.Input.IsValid())
		{
			continue;
		}

		TArray<FString> States;
		for (const FGameplayTag Tag : Opener.RequiredTags)
		{
			States.Add(GetLeaf(Tag));
		}
		const FText Label = FText::Format(LOCTEXT("OpenerLabel", "{0} ({1})"), GetInputLabel(Opener.Input), FText::FromString(FString::Join(States, TEXT(", "))));
		OutPins.Add({ Opener.GetOutputName(), Label, NAME_None, FText::Format(LOCTEXT("OpenerTooltip", "The attack this input starts the combo with while the attacker has {0}."), FText::FromString(Opener.RequiredTags.ToStringSimple())) });
	}
}

FSlateIcon FKzComboNode_Entry::GetIcon() const
{
	return NodeIcon(TEXT("Entry"));
}

FLinearColor FKzComboNode_Entry::GetColor() const
{
	return FLinearColor(0.4f, 0.06f, 0.06f);
}
#endif

FName FKzComboNode_Attack::GetOutputName(FGameplayTag Input, bool bHit) const
{
	if (!bBranchOnHit)
	{
		return Input.GetTagName();
	}
	return FName(FString::Printf(TEXT("%s.%s"), *Input.ToString(), bHit ? TEXT("Hit") : TEXT("Miss")));
}

#if WITH_EDITOR
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

		const FText Label = GetInputLabel(Input);
		if (!bBranchOnHit)
		{
			OutPins.Add({ GetOutputName(Input, true), bSeveral ? Label : FText::GetEmpty(), NAME_None, LOCTEXT("NextTooltip", "The attack this input chains to.") });
			continue;
		}

		OutPins.Add({ GetOutputName(Input, true), bSeveral ? FText::Format(LOCTEXT("InputHit", "{0} Hit"), Label) : LOCTEXT("Hit", "Hit"), NAME_None, LOCTEXT("HitTooltip", "The attack this input chains to once this one hit something.") });
		OutPins.Add({ GetOutputName(Input, false), bSeveral ? FText::Format(LOCTEXT("InputMiss", "{0} Miss"), Label) : LOCTEXT("Miss", "Miss"), NAME_None, LOCTEXT("MissTooltip", "The attack this input chains to while this one hit nothing.") });
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