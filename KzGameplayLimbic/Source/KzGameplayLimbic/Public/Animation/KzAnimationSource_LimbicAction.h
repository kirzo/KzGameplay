// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/KzAnimationSource.h"
#include "GameplayTagContainer.h"
#include "KzAnimationSource_LimbicAction.generated.h"

class ULimbicPuppeteer;

/** Plays a puppeteer once in a slot of the Limbic node of the mesh, as Play Limbic Action does. The character's puppeteer needs a Slot node with the same tag. Subclasses choose the puppeteer. */
UCLASS(Abstract)
class KZGAMEPLAYLIMBIC_API UKzAnimationSource_LimbicActionBase : public UKzAnimationSource
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (Categories = "Limbic.Slot"))
	FGameplayTag Slot;

	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0", Units = "Seconds"))
	float BlendIn = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0", Units = "Seconds"))
	float BlendOut = 0.25f;

	virtual TSharedPtr<FKzAnimationPlay> Play(const FKzAnimationContext& Context) const override;
	virtual float GetTimeline(const FKzAnimationContext& Context, TArray<FKzAnimationNotifySpan>& OutNotifies) const override;

	/** The puppeteer played in the context. */
	virtual ULimbicPuppeteer* GetPuppeteer(const FKzAnimationContext& Context) const PURE_VIRTUAL(UKzAnimationSource_LimbicActionBase::GetPuppeteer, return nullptr;);
};

/** Plays the puppeteer set here. */
UCLASS(DisplayName = "Limbic Action")
class KZGAMEPLAYLIMBIC_API UKzAnimationSource_LimbicAction : public UKzAnimationSource_LimbicActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<ULimbicPuppeteer> Puppeteer;

	virtual ULimbicPuppeteer* GetPuppeteer(const FKzAnimationContext& Context) const override { return Puppeteer; }

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif
};

/** Plays the puppeteer a query picks: from the item's Database fragment, or from the user's database when it has a more specific one. */
UCLASS(DisplayName = "Limbic Action (By Query)")
class KZGAMEPLAYLIMBIC_API UKzAnimationSource_LimbicActionByQuery : public UKzAnimationSource_LimbicActionBase
{
	GENERATED_BODY()

public:
	/** What to ask, from tables of soft Limbic Puppeteer references. The item's ItemTags and the user's state are the context. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	FKzDatabaseQuery Query;

	virtual ULimbicPuppeteer* GetPuppeteer(const FKzAnimationContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif
};