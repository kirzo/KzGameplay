// Copyright 2026 kirzo

#include "Characters/KzCharacter.h"

#include "Abilities/KzAbilitySystemComponent.h"
#include "Capabilities/KzCapabilityComponent.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "InputTriggers.h"

AKzCharacter::AKzCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UKzAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	CapabilityComponent = CreateDefaultSubobject<UKzCapabilityComponent>(TEXT("CapabilityComponent"));
	InputHandlerComponent = CreateDefaultSubobject<UKzInputHandlerComponent>(TEXT("InputHandlerComponent"));
}

UAbilitySystemComponent* AKzCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AKzCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.Reset();
	AbilitySystemComponent->GetOwnedGameplayTags(TagContainer);
}

void AKzCharacter::BeginPlay()
{
	// Before Super: what BeginPlay pushes is not taken as the base, and its tags are heard
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	for (const TPair<FGameplayTag, FKzWalkSpeed>& Entry : WalkSpeedByTag)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleWalkSpeedTagChanged);
	}

	InputHandlerComponent->OnInputAxis.AddDynamic(this, &ThisClass::HandleInputAxis);

	Super::BeginPlay();
}

void AKzCharacter::PossessedBy(AController* NewController)
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	Super::PossessedBy(NewController);
}

void AKzCharacter::PushMaxWalkSpeed(FName SourceID, const FKzWalkSpeed& WalkSpeed)
{
	WalkSpeedStack.Push(WalkSpeed, SourceID, WalkSpeed.Priority);
	UpdateMaxWalkSpeed();
}

void AKzCharacter::RemoveMaxWalkSpeed(FName SourceID)
{
	if (WalkSpeedStack.Remove(SourceID) > 0)
	{
		UpdateMaxWalkSpeed();
	}
}

void AKzCharacter::UpdateMaxWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeedStack.IsEmpty() ? BaseWalkSpeed : WalkSpeedStack.Top().Resolve(BaseWalkSpeed);
}

void AKzCharacter::HandleWalkSpeedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		PushMaxWalkSpeed(Tag.GetTagName(), WalkSpeedByTag.FindChecked(Tag));
	}
	else
	{
		RemoveMaxWalkSpeed(Tag.GetTagName());
	}
}

void AKzCharacter::HandleInputAxis(FGameplayTag InputTag, const FInputActionValue& Value, ETriggerEvent TriggerEvent)
{
	if (TriggerEvent != ETriggerEvent::Triggered)
	{
		return;
	}

	const FVector Input = Value.Get<FVector>();

	// Move arrives in world space: the handler's Camera Relative Movement modifier converts it
	if (InputTag.MatchesTagExact(KzTags::Input::Move))
	{
		AddMovementInput(Input);
	}
	else if (InputTag.MatchesTagExact(KzTags::Input::Look))
	{
		AddControllerYawInput(Input.X);
		AddControllerPitchInput(Input.Y);
	}
}
