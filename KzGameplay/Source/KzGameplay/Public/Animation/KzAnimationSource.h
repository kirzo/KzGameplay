// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzDatabase.h"
#include "KzAnimationSource.generated.h"

class AActor;
class UAnimMontage;
class UKzAnimationNotifyState;
class UKzItemDefinition;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/** Where and for whom an animation source plays. */
struct FKzAnimationContext
{
	/** The mesh the animation plays on. */
	USkeletalMeshComponent* Mesh = nullptr;

	/** Who plays it, whose database By Query sources ask. */
	const AActor* User = nullptr;

	/** The item it plays with, whose Database fragment By Query sources ask. */
	const UKzItemDefinition* Item = nullptr;
};

/** Where a routed notify state sits on an animation's timeline. */
struct FKzAnimationNotifySpan
{
	const UKzAnimationNotifyState* Notify = nullptr;
	float Start = 0.0f;
	float End = 0.0f;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FKzOnAnimationNotify, const UKzAnimationNotifyState* /*Notify*/, bool /*bBegin*/);
DECLARE_MULTICAST_DELEGATE(FKzOnAnimationBlendOut);
DECLARE_MULTICAST_DELEGATE_OneParam(FKzOnAnimationEnded, bool /*bInterrupted*/);

/**
 * One play of an animation source, alive while its owner holds it.
 * Hears only the notifies its own animation fires, so a combo's next attack never takes the windows of the one still blending out.
 */
class KZGAMEPLAY_API FKzAnimationPlay : public TSharedFromThis<FKzAnimationPlay>
{
public:
	virtual ~FKzAnimationPlay();

	virtual void Stop(float BlendOut) = 0;
	bool IsPlaying() const { return bPlaying; }

	FKzOnAnimationNotify OnNotify;
	FKzOnAnimationBlendOut OnBlendOut;
	FKzOnAnimationEnded OnEnded;

	/** Hands a routed notify state to the play on the mesh that fired it. */
	static void RouteNotify(USkeletalMeshComponent* Mesh, const FAnimNotifyEventReference& Reference, const UKzAnimationNotifyState* Notify, bool bBegin);

protected:
	virtual bool Owns(const FAnimNotifyEventReference& Reference) const = 0;

	/** Starts hearing the mesh's routed notifies. */
	void Begin(USkeletalMeshComponent& InMesh);

	/** Stops hearing them and tells the owner. */
	void End(bool bInterrupted);

private:
	void Unregister();

	TWeakObjectPtr<USkeletalMeshComponent> Mesh;
	bool bPlaying = false;
};

/** How something is animated, such as an attack: a montage, a Limbic action, anything that plays on a mesh and fires routed notifies. Data only, shared by everyone who plays it. */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KZGAMEPLAY_API UKzAnimationSource : public UObject
{
	GENERATED_BODY()

public:
	/** Starts playing in the context; null when there is nothing to play. */
	virtual TSharedPtr<FKzAnimationPlay> Play(const FKzAnimationContext& Context) const PURE_VIRTUAL(UKzAnimationSource::Play, return nullptr;);

	/** The animation's length and its routed notify states, in its own time. */
	virtual float GetTimeline(const FKzAnimationContext& Context, TArray<FKzAnimationNotifySpan>& OutNotifies) const { return 0.0f; }

#if WITH_EDITOR
	/** What it plays, for the nodes that show it. */
	virtual FText GetDescription() const { return FText::GetEmpty(); }
#endif
};

/** Plays a montage on the mesh's AnimInstance. Subclasses choose the montage. */
UCLASS(Abstract)
class KZGAMEPLAY_API UKzAnimationSource_MontageBase : public UKzAnimationSource
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;

	/** Section to start at; none starts at the beginning. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	FName StartSection;

	virtual TSharedPtr<FKzAnimationPlay> Play(const FKzAnimationContext& Context) const override;
	virtual float GetTimeline(const FKzAnimationContext& Context, TArray<FKzAnimationNotifySpan>& OutNotifies) const override;

	/** The montage played in the context. */
	virtual UAnimMontage* GetMontage(const FKzAnimationContext& Context) const PURE_VIRTUAL(UKzAnimationSource_MontageBase::GetMontage, return nullptr;);
};

/** Plays the montage set here. */
UCLASS(DisplayName = "Montage")
class KZGAMEPLAY_API UKzAnimationSource_Montage : public UKzAnimationSource_MontageBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> Montage;

	virtual UAnimMontage* GetMontage(const FKzAnimationContext& Context) const override { return Montage; }

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif
};

/** Plays the montage a query picks: from the item's Database fragment, or from the user's database when it has a more specific one. */
UCLASS(DisplayName = "Montage (By Query)")
class KZGAMEPLAY_API UKzAnimationSource_MontageByQuery : public UKzAnimationSource_MontageBase
{
	GENERATED_BODY()

public:
	/** What to ask, from tables of soft montage references. The item's ItemTags and the user's state are the context. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	FKzDatabaseQuery Query;

	virtual UAnimMontage* GetMontage(const FKzAnimationContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif
};