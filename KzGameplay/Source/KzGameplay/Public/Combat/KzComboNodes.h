// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KzGraphNode.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "Math/Geometry/Shapes/KzWedge.h"
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

/** Where a combo starts for one of its inputs, with a single output. The combo keeps one per input. */
USTRUCT(meta = (DisplayName = "Entry", Hidden))
struct KZGAMEPLAY_API FKzComboNode_Entry : public FKzComboNode
{
	GENERATED_BODY()

	/** The input that starts the combo here, set by the combo from its Inputs. */
	UPROPERTY(VisibleAnywhere, Category = "Entry", meta = (Categories = "Input"))
	FGameplayTag Input;

	virtual FName GetEntryName() const override { return Input.GetTagName(); }

#if WITH_EDITOR
	virtual void GetInputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override {}
	virtual void GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override;
	virtual FLinearColor GetColor() const override;
	virtual FSlateIcon GetIcon() const override;
	virtual bool CanUserDelete() const override { return false; }
	virtual bool CanDuplicate() const override { return false; }
#endif
};

/** What a branch chooses by, when a press chains or a combo starts. */
struct FKzComboBranchContext
{
	/** The name of the Combo Window the press came in, if it had one. */
	FGameplayTag Window;

	/** Whether the attack playing hit something. */
	bool bHit = false;

	FGameplayTagContainer AttackerTags;
};

/** Base of the nodes that play nothing and lead on through one of their outputs. */
USTRUCT(meta = (Hidden))
struct KZGAMEPLAY_API FKzComboNode_Branch : public FKzComboNode
{
	GENERATED_BODY()

	static const FName DefaultName;

	/** The output to take, and the one to take instead when that leads nowhere, or none. */
	virtual FName PickOutput(const FKzComboBranchContext& Context, FName& OutFallback) const { OutFallback = NAME_None; return NAME_None; }

#if WITH_EDITOR
	virtual FLinearColor GetColor() const override;
#endif
};

/** Leads a press by the Combo Window it came in: one output per window named here, and Default for the rest, or for a named one left unlinked. */
USTRUCT(meta = (DisplayName = "Switch on Window"))
struct KZGAMEPLAY_API FKzComboNode_SwitchOnWindow : public FKzComboNode_Branch
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Switch", meta = (Categories = "Combo.Window"))
	TArray<FGameplayTag> Windows;

	virtual FName PickOutput(const FKzComboBranchContext& Context, FName& OutFallback) const override;

#if WITH_EDITOR
	virtual void GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override;
	virtual void Validate(TArray<FKzValidationIssue>& OutIssues) const override;
#endif
};

/** Leads one way when the attack playing hit something and another when it missed. */
USTRUCT(meta = (DisplayName = "Branch on Hit", Shortcut = "B"))
struct KZGAMEPLAY_API FKzComboNode_BranchOnHit : public FKzComboNode_Branch
{
	GENERATED_BODY()

	static const FName HitName;
	static const FName MissName;

	virtual FName PickOutput(const FKzComboBranchContext& Context, FName& OutFallback) const override;

#if WITH_EDITOR
	virtual void GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override;
#endif
};

/** A way for an attacker that has some tags, such as State.Blocking to attack from a block. */
USTRUCT()
struct KZGAMEPLAY_API FKzComboTagCase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Case")
	FGameplayTagContainer RequiredTags;

	FName GetOutputName() const { return FName(RequiredTags.ToStringSimple()); }
};

/** Leads by what the attacker has: the first case whose tags it has, and Default for the rest, or for a case left unlinked. */
USTRUCT(meta = (DisplayName = "Switch on Tags"))
struct KZGAMEPLAY_API FKzComboNode_SwitchOnTags : public FKzComboNode_Branch
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Switch", meta = (TitleProperty = "RequiredTags"))
	TArray<FKzComboTagCase> Cases;

	virtual FName PickOutput(const FKzComboBranchContext& Context, FName& OutFallback) const override;

#if WITH_EDITOR
	virtual void GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override;
	virtual void Validate(TArray<FKzValidationIssue>& OutIssues) const override;
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

	/** Where it is at Alpha of the way, as its parent goes from From to To. */
	FTransform GetTransform(const FTransform& From, const FTransform& To, float Alpha) const;

	/** How many steps a sweep takes as its parent goes from From to To: enough that the parent turns little in each, so the sweep follows the arc of a swing instead of cutting across it. */
	static int32 GetSweepSteps(const FTransform& From, const FTransform& To);
};

/** One attack: how it is animated, its damage and the hitboxes its Hit Windows sweep. It has an output per input, named after it, for where a press in a Combo Window leads. */
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

	/** Turns to the target, or to the direction pressed without one, until the first Hit Window opens. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bFaceTarget = true;

	/** Looks for targets in its own shape instead of the one of the user's Sensor.Targeting sensor, whose filters still apply. */
	UPROPERTY(EditAnywhere, Category = "Targeting")
	bool bOverrideTargetShape = false;

	/** Where it looks for targets, such as farther for a lunge or all around for a spin: on the user, turned to the direction pressed. */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (EditCondition = "bOverrideTargetShape", EditConditionHides))
	FKzShapeInstance TargetShape = FKzShapeInstance::Make<FKzWedge>();

	/** How long attacker and target freeze when it hits. */
	UPROPERTY(EditAnywhere, Category = "Impact", meta = (ClampMin = "0", Units = "Seconds"))
	float HitStop = 0.0f;

	/** How far the target's mesh shakes back and forth along the hit while the hit stop holds it. */
	UPROPERTY(EditAnywhere, Category = "Impact", meta = (ClampMin = "0", Units = "Centimeters"))
	float HitShake = 0.0f;

	/** Executed on the target when it hits: particles, sound, camera shake, slow motion. */
	UPROPERTY(EditAnywhere, Category = "Impact", meta = (Categories = "GameplayCue"))
	FGameplayTag ImpactCue;

#if WITH_EDITOR
	virtual void GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const override;
	virtual FText GetSubtitle() const override;
	virtual FLinearColor GetColor() const override;
	virtual FSlateIcon GetIcon() const override;
	virtual void Validate(TArray<FKzValidationIssue>& OutIssues) const override;
#endif
};