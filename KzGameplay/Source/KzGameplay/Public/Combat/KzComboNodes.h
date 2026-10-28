// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KzGraphNode.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "KzComboNodes.generated.h"

class UKzAnimationSource;
class USceneComponent;
class USkeletalMeshComponent;

/** Base of combo nodes. */
USTRUCT()
struct KZGAMEPLAY_API FKzComboNode : public FKzGraphNode
{
	GENERATED_BODY()
};

/** A way to start a combo only in a state, such as attacking while blocking. */
USTRUCT()
struct KZGAMEPLAY_API FKzComboOpener
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Opener", meta = (Categories = "Input"))
	FGameplayTag Input;

	/** What the attacker must have, such as State.Blocking. */
	UPROPERTY(EditAnywhere, Category = "Opener")
	FGameplayTagContainer RequiredTags;

	FName GetOutputName() const;
};

/** Where a combo starts: one output per input, followed by the input that started it, and one per opener. */
USTRUCT(meta = (DisplayName = "Entry", Hidden))
struct KZGAMEPLAY_API FKzComboNode_Entry : public FKzComboNode
{
	GENERATED_BODY()

	static const FName EntryName;

	/** Outputs taken instead of an input's own while the attacker has their tags; the first that matches wins. */
	UPROPERTY(EditAnywhere, Category = "Entry", meta = (TitleProperty = "Input"))
	TArray<FKzComboOpener> Openers;

	virtual FName GetEntryName() const override { return EntryName; }

	/** The output an input starts through, given the attacker's tags. */
	FName GetOutputName(FGameplayTag Input, const FGameplayTagContainer& AttackerTags) const;

#if WITH_EDITOR
	virtual void GetInputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override {}
	virtual void GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override;
	virtual FLinearColor GetColor() const override;
	virtual FSlateIcon GetIcon() const override;
	virtual bool CanUserDelete() const override { return false; }
	virtual bool CanDuplicate() const override { return false; }
#endif
};

/** What a hitbox hangs from. */
UENUM()
enum class EKzComboHitboxAttach : uint8
{
	/** The mesh the attack animates. */
	Avatar,

	/** The weapon's actor. */
	Weapon
};

/** A shape an attack hits with while its Hit Windows last, carried along by what it hangs from. */
USTRUCT()
struct KZGAMEPLAY_API FKzComboHitbox
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	FKzShapeInstance Shape;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	EKzComboHitboxAttach AttachTo = EKzComboHitboxAttach::Weapon;

	/** Component of the weapon's actor it hangs from; none takes the actor's root. */
	UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (EditCondition = "AttachTo == EKzComboHitboxAttach::Weapon", EditConditionHides))
	FName Component;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	FName Socket;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	FTransform Offset;

	/** What it hangs from: the animated mesh, or the weapon's root, or the component named Component when the weapon is an actor. */
	USceneComponent* FindParent(USkeletalMeshComponent* AnimatedMesh, USceneComponent* WeaponRoot) const;

	/** Where it is, hanging from Parent. */
	FTransform GetTransform(const USceneComponent& Parent) const;
};

/** One attack: how it is animated, its damage and the hitboxes its Hit Windows sweep. Its outputs say where each input leads once it hit or missed. */
USTRUCT(meta = (DisplayName = "Attack", Shortcut = "A"))
struct KZGAMEPLAY_API FKzComboNode_Attack : public FKzComboNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Instanced, Category = "Attack")
	TObjectPtr<UKzAnimationSource> Animation;

	/** Times the weapon's base damage. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.0f;

	/** Sent with each hit, for defenses and reactions: Attack.Unblockable, Attack.Undodgeable, Attack.GuardBreak. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (Categories = "Attack"))
	FGameplayTagContainer AttackTags;

	UPROPERTY(EditAnywhere, Category = "Attack")
	TArray<FKzComboHitbox> Hitboxes;

	/** Whether each input leads one way when the attack hit and another when it missed. Off, each input has a single output. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bBranchOnHit = true;

	/** Turns to the target, or to the direction pressed without one, until the first Hit Window opens. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bFaceTarget = true;

	/** How long attacker and target freeze when it hits. */
	UPROPERTY(EditAnywhere, Category = "Impact", meta = (ClampMin = "0", Units = "Seconds"))
	float HitStop = 0.0f;

	/** Executed on the target when it hits: particles, sound, camera shake, slow motion. */
	UPROPERTY(EditAnywhere, Category = "Impact", meta = (Categories = "GameplayCue"))
	FGameplayTag ImpactCue;

	/** The output an input leads through once the attack hit or missed. */
	FName GetOutputName(FGameplayTag Input, bool bHit) const;

#if WITH_EDITOR
	virtual void GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override;
	virtual FText GetSubtitle() const override;
	virtual FLinearColor GetColor() const override;
	virtual FSlateIcon GetIcon() const override;
	virtual void Validate(TArray<FKzValidationIssue>& OutIssues) const override;
#endif
};