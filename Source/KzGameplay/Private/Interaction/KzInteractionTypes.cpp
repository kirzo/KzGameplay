// Copyright 2026 kirzo

#include "Interaction/KzInteractionTypes.h"
#include "Input/KzInputHandlerComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

void FKzInteractionScope::Run()
{
	// Moved out first: a cleanup that ends up back here must not run anything twice
	TArray<TFunction<void()>> Pending = MoveTemp(Cleanups);
	Cleanups.Reset();

	for (int32 Index = Pending.Num() - 1; Index >= 0; --Index)
	{
		if (Pending[Index])
		{
			Pending[Index]();
		}
	}
}

void FKzInteraction::AddCleanup(TFunction<void()> Cleanup) const
{
	if (Scope.IsValid() && Cleanup)
	{
		Scope->Cleanups.Add(MoveTemp(Cleanup));
	}
}

void FKzInteraction::PushInputModifier(AActor* Avatar, FGameplayTag InputTag, UKzInputModifier* Modifier) const
{
	UKzInputHandlerComponent* Handler = Avatar ? Avatar->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	if (!Handler || !Modifier)
	{
		return;
	}

	Handler->PushInputModifier(InputTag, Modifier);

	AddCleanup([WeakHandler = TWeakObjectPtr<UKzInputHandlerComponent>(Handler), WeakModifier = TWeakObjectPtr<UKzInputModifier>(Modifier), InputTag]
	{
		if (WeakHandler.IsValid() && WeakModifier.IsValid())
		{
			WeakHandler->RemoveInputModifier(InputTag, WeakModifier.Get());
		}
	});
}

void FKzInteraction::PushInputIgnore(AActor* Avatar, FGameplayTag InputTag, FName SourceID, int32 Priority) const
{
	UKzInputHandlerComponent* Handler = Avatar ? Avatar->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	if (!Handler)
	{
		return;
	}

	Handler->PushInputIgnore(InputTag, SourceID, true, Priority);

	AddCleanup([WeakHandler = TWeakObjectPtr<UKzInputHandlerComponent>(Handler), InputTag, SourceID]
	{
		if (WeakHandler.IsValid())
		{
			WeakHandler->RemoveInputIgnore(InputTag, SourceID);
		}
	});
}

void FKzInteraction::GrantTag(AActor* Avatar, FGameplayTag Tag) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar);
	if (!ASC || !Tag.IsValid())
	{
		return;
	}

	ASC->AddLooseGameplayTag(Tag);

	AddCleanup([WeakASC = TWeakObjectPtr<UAbilitySystemComponent>(ASC), Tag]
	{
		if (WeakASC.IsValid())
		{
			WeakASC->RemoveLooseGameplayTag(Tag);
		}
	});
}
