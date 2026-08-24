// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KzInteractionTypes.generated.h"

class UKzInteractorComponent;
class UKzInteractableComponent;
class UKzInputModifier;

/** Defines the outcome of an interaction attempt. */
UENUM(BlueprintType)
enum class EKzInteractionResult : uint8
{
	/** The interaction failed or was ignored. The interactor should cancel the action. */
	Ignored,

	/** The interaction was an instant success (e.g., picking up an item). The interactor can finish the action. */
	Completed,

	/** The interaction is ongoing (e.g., carrying a chest). It stays alive until something ends it. */
	Continuous,

	/** Asked of the server and not answered yet. Only ever returned on a client without authority. */
	Pending
};

/** Why a continuous interaction ended. A release can play an animation where a break drops the object. */
UENUM(BlueprintType)
enum class EKzInteractionEndReason : uint8
{
	/** The instigator let go on purpose. */
	Released,

	/** The interaction ran its course and succeeded. */
	Completed,

	/** Something else cut it short (a stun, another ability, gameplay logic). */
	Interrupted,

	/** The instigator moved beyond the interactable's keep-alive range. */
	OutOfRange,

	/** The interactable's keep-alive rules stopped holding. */
	ConditionFailed,

	/** The interactable was destroyed, deactivated or otherwise went away. */
	TargetLost,

	/** The interactor or its avatar went away. */
	InstigatorLost
};

/** Identifies one live interaction. Handles are never reused, so a stale one reads as inactive. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzInteractionHandle
{
	GENERATED_BODY()

	FKzInteractionHandle() = default;
	explicit FKzInteractionHandle(int32 InId) : Id(InId) {}

	bool IsValid() const { return Id != 0; }
	void Invalidate() { Id = 0; }

	bool operator==(const FKzInteractionHandle& Other) const { return Id == Other.Id; }
	bool operator!=(const FKzInteractionHandle& Other) const { return Id != Other.Id; }

	friend uint32 GetTypeHash(const FKzInteractionHandle& Handle) { return ::GetTypeHash(Handle.Id); }

	FString ToString() const { return FString::Printf(TEXT("Interaction:%d"), Id); }

private:
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	int32 Id = 0;

	friend class UKzInteractionSubsystem;
};

/**
 * Undo list for one interaction, run exactly once when it ends, whatever ended it.
 * The system guarantees when cleanup happens, never how: only the code that applied an effect can reverse it.
 */
struct KZGAMEPLAY_API FKzInteractionScope
{
	/** Undo callbacks, run in reverse order so effects unwind the way they were stacked. */
	TArray<TFunction<void()>> Cleanups;

	/** Runs every pending cleanup once and forgets them. */
	void Run();

	~FKzInteractionScope() { Run(); }
};

/**
 * One live interaction, owned by UKzInteractionSubsystem and the single source of truth about it.
 * Components ask the subsystem instead of keeping their own copy.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzInteraction
{
	GENERATED_BODY()

	/** Identity of this interaction, valid until it ends. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FKzInteractionHandle Handle;

	/** The component that started it. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TWeakObjectPtr<UKzInteractorComponent> Interactor;

	/** The component being interacted with. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TWeakObjectPtr<UKzInteractableComponent> Interactable;

	/** The interactor's owner, cached so listeners can identify it once the components are gone. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TWeakObjectPtr<AActor> Instigator;

	/** The interactable's owner, cached for the same reason. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TWeakObjectPtr<AActor> Target;

	/** World time it began, for interactions that care about duration. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	float StartTime = 0.0f;

	/** True while both ends still exist. A false here ends the interaction on the next validation pass. */
	bool HasLiveEnds() const { return Interactor.IsValid() && Interactable.IsValid() && Instigator.IsValid(); }

	/** Registers how to undo something this interaction did. Const because handlers get it by const reference. */
	void AddCleanup(TFunction<void()> Cleanup) const;

	/** Pushes an input modifier on the avatar and takes it back when the interaction ends. */
	void PushInputModifier(AActor* Avatar, FGameplayTag InputTag, UKzInputModifier* Modifier) const;

	/** Blocks one input on the avatar and unblocks it when the interaction ends. */
	void PushInputIgnore(AActor* Avatar, FGameplayTag InputTag, FName SourceID, int32 Priority = 0) const;

	/** Grants a loose gameplay tag to the avatar and removes it when the interaction ends. */
	void GrantTag(AActor* Avatar, FGameplayTag Tag) const;

	/** Shared undo list. Copies of this struct share it, so a handler can register cleanup from its own copy. */
	TSharedPtr<FKzInteractionScope> Scope;
};

/** The shared scope must survive a copy, so the struct is copied through C++ rather than by reflection. */
template<>
struct TStructOpsTypeTraits<FKzInteraction> : public TStructOpsTypeTraitsBase2<FKzInteraction>
{
	enum { WithCopy = true };
};

/** Handles compare by value, which lets Blueprint compare them and TMap key on them. */
template<>
struct TStructOpsTypeTraits<FKzInteractionHandle> : public TStructOpsTypeTraitsBase2<FKzInteractionHandle>
{
	enum { WithIdenticalViaEquality = true };
};
