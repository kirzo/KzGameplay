// Copyright 2026 kirzo

#include "Abilities/KzAttributeBarWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/ProgressBar.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UKzAttributeBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &UKzAttributeBarWidget::HandlePossessedPawnChanged);
		Bind(PlayerController->GetPawn());
	}
}

void UKzAttributeBarWidget::NativeDestruct()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &UKzAttributeBarWidget::HandlePossessedPawnChanged);
	}
	Unbind();

	Super::NativeDestruct();
}

void UKzAttributeBarWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	Bind(NewPawn);
}

void UKzAttributeBarWidget::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	Refresh();
}

void UKzAttributeBarWidget::Bind(APawn* Pawn)
{
	Unbind();

	// Without a pawn, or between pawns, the bar keeps the last values it showed
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystem = AbilitySystemComponent;
	if (Attribute.IsValid())
	{
		ValueHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UKzAttributeBarWidget::HandleAttributeChanged);
		LastValue = AbilitySystemComponent->GetNumericAttribute(Attribute);
	}
	if (MaxAttribute.IsValid())
	{
		MaxHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxAttribute).AddUObject(this, &UKzAttributeBarWidget::HandleAttributeChanged);
	}
	Refresh();
}

void UKzAttributeBarWidget::Unbind()
{
	if (UAbilitySystemComponent* AbilitySystemComponent = AbilitySystem.Get())
	{
		if (Attribute.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).Remove(ValueHandle);
		}
		if (MaxAttribute.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxAttribute).Remove(MaxHandle);
		}
	}
	AbilitySystem = nullptr;
	ValueHandle.Reset();
	MaxHandle.Reset();
}

void UKzAttributeBarWidget::Refresh()
{
	const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystem.Get();
	if (!AbilitySystemComponent)
	{
		return;
	}

	const float Value = Attribute.IsValid() ? AbilitySystemComponent->GetNumericAttribute(Attribute) : 0.0f;
	const float MaxValue = MaxAttribute.IsValid() ? AbilitySystemComponent->GetNumericAttribute(MaxAttribute) : 0.0f;
	if (Bar)
	{
		Bar->SetPercent(MaxValue > 0.0f ? Value / MaxValue : 0.0f);
	}

	const float OldValue = LastValue;
	LastValue = Value;
	OnAttributeChanged(Value, MaxValue, OldValue);
}
