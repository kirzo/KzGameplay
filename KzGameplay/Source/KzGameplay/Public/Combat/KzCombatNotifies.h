// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/KzAnimationNotifyState.h"
#include "Combat/KzCombatTypes.h"
#include "GameplayTagContainer.h"
#include "KzCombatNotifies.generated.h"

/** While it lasts, the attack's hitboxes hit, each target once. */
UCLASS(meta = (DisplayName = "Hit Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_HitWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_HitWindow();

	/** How the target reacts, such as a stronger stagger for a heavy blow. None is Reaction.Hit. */
	UPROPERTY(EditAnywhere, Category = "Hit Window", meta = (Categories = "Reaction"))
	FGameplayTag Reaction;

	/** Which way the blow travels, which the target's reaction follows: the side it staggers from, how it turns, and where it is pushed. */
	UPROPERTY(EditAnywhere, Category = "Hit Window")
	FKzHitDirection Direction;

	/** How hard it pushes, as a multiplier on the Knockback Window of the target's reaction, such as 2 for a heavy blow. */
	UPROPERTY(EditAnywhere, Category = "Hit Window", meta = (ClampMin = "0"))
	float Knockback = 1.0f;

	/** This hit's share of the attack, as a multiplier on top of the attack's own, such as 0.3 for each of two quick cuts and 1 for the slash that ends them. */
	UPROPERTY(EditAnywhere, Category = "Hit Window", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.0f;

	/** Sent with this hit along with the attack's, for defenses and reactions: Attack.Unblockable, Attack.Undodgeable, Attack.GuardBreak. */
	UPROPERTY(EditAnywhere, Category = "Hit Window", meta = (Categories = "Attack"))
	FGameplayTagContainer AttackTags;

	virtual FString GetNotifyName_Implementation() const override;
};

/** While it lasts, a press chains the next attack. */
UCLASS(meta = (DisplayName = "Combo Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_ComboWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_ComboWindow();

	/** Names the window, for a Switch on Window to lead a press in it its own way, such as a late, well-timed press to a stronger attack. */
	UPROPERTY(EditAnywhere, Category = "Combo Window", meta = (Categories = "Combo.Window"))
	FGameplayTag Window;

	virtual FString GetNotifyName_Implementation() const override;
};

/** While it lasts, the attack can be cut, such as by a dodge. */
UCLASS(meta = (DisplayName = "Cancel Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_CancelWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_CancelWindow();
};

/** While it lasts, the attacker closes in on its target, or steps forward without one, along an arc if it has a Height. */
UCLASS(meta = (DisplayName = "Warp Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_WarpWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_WarpWindow();

	/** The gap left between attacker and target. A target already closer is not approached. */
	UPROPERTY(EditAnywhere, Category = "Warp Window", meta = (ClampMin = "0", Units = "Centimeters"))
	float StopDistance = 50.0f;

	/** The farthest the window moves the attacker. */
	UPROPERTY(EditAnywhere, Category = "Warp Window", meta = (ClampMin = "0", Units = "Centimeters"))
	float MaxDistance = 400.0f;

	/** How far the window moves the attacker forward when there is no target. */
	UPROPERTY(EditAnywhere, Category = "Warp Window", meta = (ClampMin = "0", Units = "Centimeters"))
	float FallbackDistance = 100.0f;

	/** The peak of a leap: the attacker rises to it halfway through the window and comes back down by its end. Zero stays on the ground. */
	UPROPERTY(EditAnywhere, Category = "Warp Window", meta = (ClampMin = "0", Units = "Centimeters"))
	float Height = 0.0f;
};

/**
 * While it lasts, the mesh's owner has these tags, such as State.Invulnerable on a dodge, or with Remove, does not. Works in any animation, with or without an
 * animation source.
 */
UCLASS(meta = (DisplayName = "Tag Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_TagWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_TagWindow();

	UPROPERTY(EditAnywhere, Category = "Tag Window")
	FGameplayTagContainer Tags;

	/**
	 * Takes the tags away while it lasts instead, wherever they came from, and gives back what it took once the last such window on the owner closes, such as
	 * State.SuperArmor off during an animation of a character that has it otherwise.
	 */
	UPROPERTY(EditAnywhere, Category = "Tag Window")
	bool bRemove = false;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};

/**
 * While it lasts, the mesh's owner is pushed along the ground the way the blow it reacts to travels, Distance times that hit's Knockback, spread over the
 * window. Outside a reaction it is pushed straight back. Walls stop it.
 */
UCLASS(meta = (DisplayName = "Knockback Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_KnockbackWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_KnockbackWindow();

	/** How far a hit with Knockback 1 pushes over the whole window. */
	UPROPERTY(EditAnywhere, Category = "Knockback Window", meta = (ClampMin = "0", Units = "Centimeters"))
	float Distance = 50.0f;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};

/** While it lasts, the mesh's owner moves Distance forward, spread over the window, so any montage can step without root motion. Walls stop it. */
UCLASS(meta = (DisplayName = "Move Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_MoveWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_MoveWindow();

	/** How far it moves over the whole window. Negative steps back. */
	UPROPERTY(EditAnywhere, Category = "Move Window", meta = (Units = "Centimeters"))
	float Distance = 100.0f;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};

/**
 * While it lasts, the mesh's owner turns towards where its move input points, even while Input.Move is blocked, such as to steer a dodge near its
 * end. The action's own facing comes before it; this only lets the animation hand some turning back. Works in any animation.
 */
UCLASS(meta = (DisplayName = "Rotation Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_RotationWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_RotationWindow();

	UPROPERTY(EditAnywhere, Category = "Rotation Window", meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float TurnRate = 180.0f;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};

/** While it lasts, the mesh's owner ignores these inputs, such as Input.Move so an attack neither steers nor turns. Facing a target and warping still move it. */
UCLASS(meta = (DisplayName = "Input Block Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_InputBlockWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_InputBlockWindow();

	UPROPERTY(EditAnywhere, Category = "Input Block Window", meta = (Categories = "Input"))
	FGameplayTagContainer Inputs;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	/** Unique per window, so one ending does not lift another's block, as the next attack blends in. */
	FName GetSourceID() const;
};