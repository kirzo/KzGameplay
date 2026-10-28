// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/KzAnimationNotifyState.h"
#include "GameplayTagContainer.h"
#include "KzCombatNotifies.generated.h"

/** While it lasts, the attack's hitboxes hit, each target once. */
UCLASS(meta = (DisplayName = "Hit Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_HitWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_HitWindow();

	/** How the target reacts, which follows the swing: a slash from the right staggers to the left. None is Reaction.Hit. */
	UPROPERTY(EditAnywhere, Category = "Hit Window", meta = (Categories = "Reaction"))
	FGameplayTag Reaction;
};

/** While it lasts, a press chains the next attack. */
UCLASS(meta = (DisplayName = "Combo Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_ComboWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_ComboWindow();
};

/** While it lasts, the attack can be cut, such as by a dodge. */
UCLASS(meta = (DisplayName = "Cancel Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_CancelWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_CancelWindow();
};

/** While it lasts, the attacker closes in on its target. */
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
};

/** While it lasts, the mesh's owner has these tags, such as State.Invulnerable on a dodge. Works in any animation, with or without an animation source. */
UCLASS(meta = (DisplayName = "Tag Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_TagWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_TagWindow();

	UPROPERTY(EditAnywhere, Category = "Tag Window")
	FGameplayTagContainer Tags;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};