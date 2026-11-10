// Copyright 2026 kirzo

#include "Characters/KzCharacter.h"

#include "Abilities/KzAbilitySystemComponent.h"
#include "Capabilities/KzCapabilityComponent.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "InputActionValue.h"
#include "InputTriggers.h"

AKzCharacter::AKzCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UKzAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	CapabilityComponent = CreateDefaultSubobject<UKzCapabilityComponent>(TEXT("CapabilityComponent"));
	InputHandlerComponent = CreateDefaultSubobject<UKzInputHandlerComponent>(TEXT("InputHandlerComponent"));

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->PerchRadiusThreshold = 30.0f;
	Movement->PerchAdditionalHeight = 0.0f;

	for (FScriptableAction* Action : { &OnPossessedAction, &OnPossessedByAIAction, &OnPossessedByPlayerAction })
	{
		Action->AddContextProperty<AActor*>(TEXT("Character"));
		Action->AddContextProperty<AController*>(TEXT("Controller"));
	}
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
	BaseRotationRate = GetCharacterMovement()->RotationRate.Yaw;
	bBaseOrientRotationToMovement = GetCharacterMovement()->bOrientRotationToMovement;
	bBaseUseControllerDesiredRotation = GetCharacterMovement()->bUseControllerDesiredRotation;

	for (const TPair<FGameplayTag, FKzWalkSpeed>& Entry : WalkSpeedByTag)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleWalkSpeedTagChanged);
	}

	for (const FGameplayTag& Tag : FaceControlRotationTags)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleFaceControlRotationTagChanged);
	}

	InputHandlerComponent->OnInputAxis.AddDynamic(this, &ThisClass::HandleInputAxis);

	Super::BeginPlay();

	RunPossessionActions();
}

void AKzCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetPossessionActions();

	Super::EndPlay(EndPlayReason);
}

void AKzCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSlopeMultiplier(DeltaSeconds);
	UpdateRotationRate();
}

void AKzCharacter::PossessedBy(AController* NewController)
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	Super::PossessedBy(NewController);

	// A character placed or spawned is possessed before BeginPlay, which runs them once it is ready for their tasks
	if (HasActorBegunPlay())
	{
		RunPossessionActions();
	}
}

void AKzCharacter::UnPossessed()
{
	ResetPossessionActions();

	Super::UnPossessed();
}

void AKzCharacter::RunPossessionActions()
{
	AController* CurrentController = GetController();
	if (!CurrentController || RunningPossessionAction)
	{
		return;
	}

	RunningPossessionAction = CurrentController->IsPlayerController() ? &OnPossessedByPlayerAction : &OnPossessedByAIAction;
	for (FScriptableAction* Action : { &OnPossessedAction, RunningPossessionAction })
	{
		Action->SetContextProperty(TEXT("Character"), static_cast<AActor*>(this));
		Action->SetContextProperty(TEXT("Controller"), CurrentController);
		Action->Run(this);
	}
}

void AKzCharacter::ResetPossessionActions()
{
	if (!RunningPossessionAction)
	{
		return;
	}

	RunningPossessionAction->Reset();
	OnPossessedAction.Reset();
	RunningPossessionAction = nullptr;
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
	const float Speed = WalkSpeedStack.IsEmpty() ? BaseWalkSpeed : WalkSpeedStack.Top().Resolve(BaseWalkSpeed);
	GetCharacterMovement()->MaxWalkSpeed = Speed * SlopeMultiplier;
}

void AKzCharacter::UpdateSlopeMultiplier(float DeltaSeconds)
{
	const FRichCurve* Curve = SlopeSpeedCurve.GetRichCurveConst();
	if (!Curve || Curve->GetNumKeys() == 0)
	{
		return;
	}

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const FVector Direction = Movement->Velocity.GetSafeNormal2D();
	float Target = 1.0f;
	if (Movement->IsMovingOnGround() && Movement->CurrentFloor.bBlockingHit && !Direction.IsNearlyZero())
	{
		// The ground rises along the movement where its normal leans against it
		const double Slope = FMath::RadiansToDegrees(FMath::Asin(-FVector::DotProduct(Movement->CurrentFloor.HitResult.ImpactNormal, Direction)));
		Target = Curve->Eval(static_cast<float>(Slope));
	}

	const float Multiplier = FMath::FInterpTo(SlopeMultiplier, Target, DeltaSeconds, SlopeInterpSpeed);
	if (Multiplier != SlopeMultiplier)
	{
		SlopeMultiplier = Multiplier;
		UpdateMaxWalkSpeed();
	}
}

void AKzCharacter::UpdateRotationRate()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const FRichCurve* Curve = RotationRateBySpeed.GetRichCurveConst();
	float Rate = Curve && Curve->GetNumKeys() > 0 ? Curve->Eval(static_cast<float>(Movement->Velocity.Size2D())) : BaseRotationRate;

	for (const TPair<FGameplayTag, float>& Entry : RotationRateByTag)
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(Entry.Key))
		{
			Rate = FMath::Min(Rate, Entry.Value);
		}
	}
	Movement->RotationRate.Yaw = Rate;
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

void AKzCharacter::HandleFaceControlRotationTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bFaceControlRotation = AbilitySystemComponent->HasAnyMatchingGameplayTags(FaceControlRotationTags);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = !bFaceControlRotation && bBaseOrientRotationToMovement;
	Movement->bUseControllerDesiredRotation = bFaceControlRotation || bBaseUseControllerDesiredRotation;
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
