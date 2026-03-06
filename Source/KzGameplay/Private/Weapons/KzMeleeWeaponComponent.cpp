// Copyright 2026 kirzo

#include "Weapons/KzMeleeWeaponComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/MeshComponent.h"

#include "KismetTraceUtils.h"

UKzMeleeWeaponComponent::UKzMeleeWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsTracing = false;
	ActiveTraceRadius = 15.0f;
	ObjectTypes.Add(ObjectTypeQuery1);
	ObjectTypes.Add(ObjectTypeQuery2);
	ObjectTypes.Add(ObjectTypeQuery3);
}

void UKzMeleeWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// Automatically find the weapon mesh if not explicitly set
	if (!WeaponMesh.IsValid())
	{
		WeaponMesh = GetOwner()->FindComponentByClass<UMeshComponent>();
	}
}

void UKzMeleeWeaponComponent::StartMeleeTrace(FName BaseSocket, FName TipSocket, float Radius)
{
	if (!WeaponMesh.IsValid())
	{
		return;
	}

	// Reset state for the new strike
	HitActors.Empty();
	ActiveBaseSocket = BaseSocket;
	ActiveTipSocket = TipSocket;
	ActiveTraceRadius = Radius;
	bIsTracing = true;

	// Cache the initial center position of the weapon to start the sweep from
	FVector BaseLoc = WeaponMesh->GetSocketLocation(ActiveBaseSocket);
	FVector TipLoc = WeaponMesh->GetSocketLocation(ActiveTipSocket);
	PreviousCenter = BaseLoc + ((TipLoc - BaseLoc) * 0.5f);
}

void UKzMeleeWeaponComponent::SweepMeleeTrace(FGameplayTag HitEventTag)
{
	if (!bIsTracing || !WeaponMesh.IsValid())
	{
		return;
	}

	FVector BaseLoc = WeaponMesh->GetSocketLocation(ActiveBaseSocket);
	FVector TipLoc = WeaponMesh->GetSocketLocation(ActiveTipSocket);

	// Calculate current capsule properties
	FVector Direction = (TipLoc - BaseLoc);
	float CapsuleHalfHeight = Direction.Size() * 0.5f;
	FVector CurrentCenter = BaseLoc + (Direction * 0.5f);

	// Avoid tracing if the weapon hasn't moved
	if (CurrentCenter.Equals(PreviousCenter, 0.1f))
	{
		return;
	}

	FVector UpVector = Direction.GetSafeNormal();
	FQuat CapsuleRot = FRotationMatrix::MakeFromZ(UpVector).ToQuat();

	// Prepare ignore list (Owner of the weapon, the weapon itself, and already hit actors)
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	if (AActor* Instigator = GetOwner()->GetInstigator())
	{
		ActorsToIgnore.Add(Instigator);
	}

	for (AActor* HitActor : HitActors)
	{
		if (HitActor)
		{
			ActorsToIgnore.Add(HitActor);
		}
	}

	FHitResult HitResult;
	
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(ActiveTraceRadius, CapsuleHalfHeight);

	static const FName MeleeTraceSingleName(TEXT("MeleeTraceSingle"));
	FCollisionQueryParams Params(MeleeTraceSingleName, true);
	Params.AddIgnoredActors(ActorsToIgnore);

	// Convert the Blueprint-friendly ObjectTypes array to native C++ collision parameters
	FCollisionObjectQueryParams ObjectQueryParams;
	for (TEnumAsByte<EObjectTypeQuery> ObjType : ObjectTypes)
	{
		ObjectQueryParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjType));
	}

	const UWorld* MyWorld = GetWorld();
	const bool bHit = MyWorld ? MyWorld->SweepSingleByObjectType(HitResult, PreviousCenter, CurrentCenter, CapsuleRot, ObjectQueryParams, Capsule, Params) : false;

#if ENABLE_DRAW_DEBUG
	EDrawDebugTrace::Type DrawType = bDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	DrawDebugCapsuleTraceSingle(MyWorld, PreviousCenter, CurrentCenter, ActiveTraceRadius, CapsuleHalfHeight, CapsuleRot.Rotator(), DrawType, bHit, HitResult, FLinearColor::Red, FLinearColor::Green, DebugDuration);
#endif

	if (bHit && HitResult.GetActor())
	{
		// Register hit to prevent duplicate damage in the same swing
		HitActors.Add(HitResult.GetActor());

		// Send Gameplay Event to the instigator's ASC
		AActor* EventInstigator = GetOwner()->GetInstigator() ? Cast<AActor>(GetOwner()->GetInstigator()) : GetOwner();
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(EventInstigator))
		{
			FGameplayEventData Payload;
			Payload.EventTag = HitEventTag;
			Payload.Instigator = EventInstigator;
			Payload.Target = HitResult.GetActor();
			Payload.OptionalObject = GetOwner(); // Pass the weapon as the optional context
			Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitResult.GetActor());

			ASC->HandleGameplayEvent(HitEventTag, &Payload);
		}
	}

	// Store current center for the next tick
	PreviousCenter = CurrentCenter;
}

void UKzMeleeWeaponComponent::EndMeleeTrace()
{
	bIsTracing = false;
	HitActors.Empty();
}