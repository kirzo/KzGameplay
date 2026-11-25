// Copyright 2026 kirzo

#include "Combat/KzCombat.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/KzCombatConfig.h"
#include "Combat/KzCombatTags.h"
#include "Components/StateTreeComponent.h"
#include "Engagement/KzEngagementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagAssetInterface.h"
#include "KzAITags.h"

//----------------------------------------------------------------------//
//  UKzCombatant
//----------------------------------------------------------------------//

UKzCombat* UKzCombatant::GetCombat() const
{
	return Cast<UKzCombat>(GetOuter());
}

bool UKzCombatant::TakeAttack(FKzAttackDefinition& OutAttack)
{
	const FKzAttackDefinition* Definition = HasAttackToTake() && AttackSet ? AttackSet->Attacks.Find(GrantedAttack) : nullptr;
	if (!Definition)
	{
		FinishAttack();
		return false;
	}

	OutAttack = *Definition;
	bTaken = true;
	return true;
}

void UKzCombatant::FinishAttack()
{
	if (!GrantedAttack.IsValid())
	{
		return;
	}

	const FKzAttackDefinition* Definition = AttackSet ? AttackSet->Attacks.Find(GrantedAttack) : nullptr;
	if (Definition)
	{
		const bool bCompleted = bStarted && !bInterrupted;
		ReadyTimes.Add(GrantedAttack, GetTime() + (bCompleted ? Definition->Cooldown : Definition->CancelledCooldown));
	}

	StopWatching();
	if (UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn.Get()))
	{
		AbilitySystem->RemoveLooseGameplayTag(KzTags::AI::State_AttackGranted);
	}

	const bool bWasStarted = bStarted;
	GrantedAttack = FGameplayTag();
	bTaken = false;
	bStarted = false;
	bInterrupted = false;

	UKzTicketPool* Pool = Tickets.GetPool();
	if (!Pool)
	{
		Tickets = FKzTicketHandle();
		return;
	}

	// Unused tickets go to someone else right away, not back to this one
	Pool->Release(Tickets, bWasStarted);
	UKzCombat* Combat = GetCombat();
	if (!bWasStarted && Definition && Combat && Combat->GetDispatcher())
	{
		TGuardValue<bool> StandDown(bStandingDown, true);
		Combat->GetDispatcher()->DispatchNow(Definition->TicketType);
	}
}

bool UKzCombatant::GrantAttack(FGameplayTag Attack, const FKzTicketHandle& InTickets)
{
	APawn* OwnerPawn = Pawn.Get();
	if (GrantedAttack.IsValid() || !OwnerPawn || !AttackSet || !AttackSet->Attacks.Contains(Attack))
	{
		return false;
	}

	GrantedAttack = Attack;
	Tickets = InTickets;
	bTaken = false;
	bStarted = false;
	bInterrupted = false;

	// Whether it gets to attack, and whether a hit cuts it short, decides how it cools down
	if (UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerPawn))
	{
		AbilitySystem->AddLooseGameplayTag(KzTags::AI::State_AttackGranted);
		AttackingHandle = AbilitySystem->RegisterGameplayTagEvent(KzTags::Combat::State_Attacking).AddUObject(this, &UKzCombatant::OnTagChanged);
		ReactingHandle = AbilitySystem->RegisterGameplayTagEvent(KzTags::Combat::State_Reacting).AddUObject(this, &UKzCombatant::OnTagChanged);
		WatchedAbilitySystem = AbilitySystem;
	}

	const AController* Controller = OwnerPawn->GetController();
	if (UStateTreeComponent* StateTree = Controller ? Controller->FindComponentByClass<UStateTreeComponent>() : nullptr)
	{
		StateTree->SendStateTreeEvent(KzTags::AI::Attack_Granted);
	}
	return true;
}

void UKzCombatant::GetAvailableAttacks(TArray<FGameplayTag>& OutAttacks) const
{
	if (!AttackSet || !IsReady())
	{
		return;
	}

	const FArea Area = GetArea();
	for (const TPair<FGameplayTag, FKzAttackDefinition>& Pair : AttackSet->Attacks)
	{
		if (CanMake(Pair.Key, Pair.Value, Area))
		{
			OutAttacks.Add(Pair.Key);
		}
	}
}

void UKzCombatant::SetBlocked(FGameplayTag Attack, bool bBlocked)
{
	if (bBlocked)
	{
		Blocked.Add(Attack);
	}
	else
	{
		Blocked.Remove(Attack);
	}
}

float UKzCombatant::GetCooldownLeft(FGameplayTag Attack) const
{
	return FMath::Max(0.0f, static_cast<float>(ReadyTimes.FindRef(Attack) - GetTime()));
}

void UKzCombatant::StartCooldowns()
{
	if (!AttackSet)
	{
		return;
	}

	const double Now = GetTime();
	for (const TPair<FGameplayTag, FKzAttackDefinition>& Pair : AttackSet->Attacks)
	{
		ReadyTimes.Add(Pair.Key, Now + Pair.Value.Cooldown);
	}
}

UWorld* UKzCombatant::GetWorld() const
{
	const UObject* Outer = GetOuter();
	return Outer && !HasAnyFlags(RF_ClassDefaultObject) ? Outer->GetWorld() : nullptr;
}

void UKzCombatant::GetTicketOptions_Implementation(FGameplayTag Type, TArray<FKzTicketOption>& Options)
{
	if (bStandingDown || GrantedAttack.IsValid() || !AttackSet || !IsReady())
	{
		return;
	}

	const FArea Area = GetArea();
	for (const TPair<FGameplayTag, FKzAttackDefinition>& Pair : AttackSet->Attacks)
	{
		if (Pair.Value.TicketType == Type && CanMake(Pair.Key, Pair.Value, Area))
		{
			FKzTicketOption& Option = Options.AddDefaulted_GetRef();
			Option.Tag = Pair.Key;
			Option.Cost = Pair.Value.Cost;
			Option.Weight = Pair.Value.Probability;
		}
	}
}

void UKzCombatant::OnTicketsGranted_Implementation(const FKzTicketOption& Option, const FKzTicketHandle& Handle)
{
	if (!GrantAttack(Option.Tag, Handle))
	{
		FKzTicketHandle Unused = Handle;
		UKzTicketPool::ReleaseTickets(Unused, false);
	}
}

void UKzCombatant::Setup(APawn* InPawn, AActor* InTarget, UKzAttackSet* InAttackSet)
{
	Pawn = InPawn;
	Target = InTarget;
	if (AttackSet == InAttackSet)
	{
		return;
	}

	AttackSet = InAttackSet;
	Blocked.Reset();
	if (AttackSet)
	{
		for (const TPair<FGameplayTag, FKzAttackDefinition>& Pair : AttackSet->Attacks)
		{
			if (Pair.Value.bBlocked)
			{
				Blocked.Add(Pair.Key);
			}
		}
	}
}

UKzCombatant::FArea UKzCombatant::GetArea() const
{
	FArea Area;
	const AActor* TargetActor = Target.Get();
	const UKzEngagementComponent* Engagement = TargetActor ? TargetActor->FindComponentByClass<UKzEngagementComponent>() : nullptr;
	const APawn* OwnerPawn = Pawn.Get();
	if (!Engagement || !OwnerPawn || Engagement->GetSettings().Areas.IsEmpty())
	{
		return Area;
	}

	const int32 Index = Engagement->GetAreaAt(OwnerPawn->GetActorLocation());
	Area.bKnown = true;
	Area.bForbidden = Index == INDEX_NONE;
	Area.bOutside = Index >= Engagement->GetSettings().Areas.Num();
	if (Engagement->GetSettings().Areas.IsValidIndex(Index))
	{
		Area.Tag = Engagement->GetSettings().Areas[Index].Tag;
	}
	return Area;
}

bool UKzCombatant::CanMake(const FGameplayTag& Attack, const FKzAttackDefinition& Definition, const FArea& Area) const
{
	if (Blocked.Contains(Attack) || GetTime() < ReadyTimes.FindRef(Attack))
	{
		return false;
	}

	// Without an engagement around the target, areas don't apply
	if (!Area.bKnown)
	{
		return true;
	}
	if (Area.bForbidden)
	{
		return false;
	}
	if (Area.bOutside)
	{
		return Definition.bFromOutside;
	}
	return Definition.Areas.IsEmpty() || Definition.Areas.HasTagExact(Area.Tag);
}

bool UKzCombatant::IsReady() const
{
	const UKzCombat* Combat = GetCombat();
	const UKzCombatConfig* Config = Combat ? Combat->GetConfig() : nullptr;
	const IGameplayTagAssetInterface* Tags = Cast<IGameplayTagAssetInterface>(Pawn.Get());
	return Pawn.IsValid() && !(Config && Tags && Tags->HasAnyMatchingGameplayTags(Config->BlockedTags));
}

void UKzCombatant::OnTagChanged(const FGameplayTag Tag, int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	if (Tag == KzTags::Combat::State_Attacking)
	{
		bStarted = true;
	}
	else if (Tag == KzTags::Combat::State_Reacting && bStarted)
	{
		bInterrupted = true;
	}
}

void UKzCombatant::StopWatching()
{
	if (UAbilitySystemComponent* AbilitySystem = WatchedAbilitySystem.Get())
	{
		AbilitySystem->UnregisterGameplayTagEvent(AttackingHandle, KzTags::Combat::State_Attacking);
		AbilitySystem->UnregisterGameplayTagEvent(ReactingHandle, KzTags::Combat::State_Reacting);
	}
	WatchedAbilitySystem.Reset();
	AttackingHandle.Reset();
	ReactingHandle.Reset();
}

double UKzCombatant::GetTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

//----------------------------------------------------------------------//
//  UKzCombat
//----------------------------------------------------------------------//

void UKzCombat::Setup(UKzCombatConfig* InConfig, bool bInAutoStart)
{
	Config = InConfig;
	bAutoStart = bInAutoStart;
	Pool = UKzTicketPool::CreateTicketPool(this, GetConfig()->Pool);
	Dispatcher = UKzTicketDispatcher::CreateTicketDispatcher(this, Pool, GetConfig()->Dispatcher);
}

void UKzCombat::ApplyConfig(UKzCombatConfig* NewConfig)
{
	Config = NewConfig;
	Pool->ApplySettings(GetConfig()->Pool);
	Dispatcher->ApplySettings(GetConfig()->Dispatcher);
}

const UKzCombatConfig* UKzCombat::GetConfig() const
{
	return Config ? Config.Get() : GetDefault<UKzCombatConfig>();
}

void UKzCombat::Start()
{
	bActive = true;
	Pool->Reset();
	Dispatcher->Start();

	if (GetConfig()->bCooldownOnStart)
	{
		for (const TPair<FGameplayTag, FKzTicketTypeSettings>& Pair : GetConfig()->Pool.Types)
		{
			Pool->StartCooldown(Pair.Key);
		}
		for (UKzCombatant* Combatant : Combatants)
		{
			Combatant->StartCooldowns();
		}
	}
}

void UKzCombat::End()
{
	bActive = false;
	Dispatcher->Stop();
}

UKzCombatant* UKzCombat::Join(APawn* Pawn, AActor* Target, UKzAttackSet* Attacks)
{
	if (!Pawn)
	{
		return nullptr;
	}

	UKzCombatant* Combatant = FindCombatant(Pawn);
	const bool bJoined = !Combatant;
	if (bJoined)
	{
		Combatant = NewObject<UKzCombatant>(this);
		Combatants.Add(Combatant);
		Dispatcher->Register(Combatant);
	}
	Combatant->Setup(Pawn, Target, Attacks);

	if (bAutoStart && !bActive)
	{
		Start();
	}
	if (bJoined)
	{
		OnCombatantJoined.Broadcast(Combatant);
	}
	return Combatant;
}

void UKzCombat::Leave(APawn* Pawn)
{
	const int32 Index = Combatants.IndexOfByPredicate([Pawn](const UKzCombatant* Combatant) { return Combatant->GetPawn() == Pawn; });
	if (Index == INDEX_NONE)
	{
		return;
	}

	UKzCombatant* Combatant = Combatants[Index];
	Combatants.RemoveAt(Index);
	Dispatcher->Unregister(Combatant);
	Combatant->FinishAttack();

	if (bAutoStart && bActive && Combatants.IsEmpty())
	{
		End();
	}
}

UKzCombatant* UKzCombat::FindCombatant(const APawn* Pawn) const
{
	const TObjectPtr<UKzCombatant>* Found = Combatants.FindByPredicate([Pawn](const UKzCombatant* Combatant) { return Combatant->GetPawn() == Pawn; });
	return Found ? Found->Get() : nullptr;
}

UWorld* UKzCombat::GetWorld() const
{
	const UObject* Outer = GetOuter();
	return Outer && !HasAnyFlags(RF_ClassDefaultObject) ? Outer->GetWorld() : nullptr;
}