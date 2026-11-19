// Copyright 2026 kirzo

#include "Combat/KzCombatSubsystem.h"
#include "Combat/KzCombat.h"
#include "Combat/KzCombatConfig.h"
#include "Combat/KzCombatSettings.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<bool> CVarDrawTickets(TEXT("kz.Combat.DrawTickets"), false, TEXT("Shows each combat's tickets on screen and, over each attacker, the attack it was given and its attacks' cooldowns."));

	FAutoConsoleCommandWithWorldAndArgs GrantAttackCommand(
		TEXT("kz.Combat.GrantAttack"),
		TEXT("Gives an attacker one of the attacks it could make, without tickets: one at random, or every attacker with 'all'."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			const UKzCombatSubsystem* Subsystem = UKzCombatSubsystem::Get(World);
			if (!Subsystem)
			{
				return;
			}

			TArray<UKzCombat*> Combats;
			Subsystem->GetCombats(Combats);
			TArray<UKzCombatant*> Free;
			for (const UKzCombat* Combat : Combats)
			{
				for (UKzCombatant* Combatant : Combat->GetCombatants())
				{
					if (!Combatant->GetGrantedAttack().IsValid())
					{
						Free.Add(Combatant);
					}
				}
			}

			const bool bAll = !Args.IsEmpty() && Args[0] == TEXT("all");
			if (!bAll && !Free.IsEmpty())
			{
				Free = { Free[FMath::RandHelper(Free.Num())] };
			}
			for (UKzCombatant* Combatant : Free)
			{
				TArray<FGameplayTag> Attacks;
				Combatant->GetAvailableAttacks(Attacks);
				if (!Attacks.IsEmpty())
				{
					Combatant->GrantAttack(Attacks[FMath::RandHelper(Attacks.Num())], FKzTicketHandle());
				}
			}
		}));
}

UKzCombatSubsystem* UKzCombatSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UKzCombatSubsystem>() : nullptr;
}

UKzCombat* UKzCombatSubsystem::GetDefaultCombat()
{
	if (!DefaultCombat)
	{
		UKzCombatConfig* Config = UKzCombatSettings::Get()->DefaultCombatConfig.LoadSynchronous();
		if (!Config)
		{
			UE_LOG(LogTemp, Warning, TEXT("No Default Combat Config under Project Settings -> Plugins -> Kz Combat: attackers outside an encounter get no attacks."));
		}

		DefaultCombat = NewObject<UKzCombat>(this);
		DefaultCombat->Setup(Config, true);
	}
	return DefaultCombat;
}

UKzCombat* UKzCombatSubsystem::CreateCombat(UKzCombatConfig* Config)
{
	UKzCombat* Combat = NewObject<UKzCombat>(this);
	Combat->Setup(Config, false);
	Combats.Add(Combat);
	return Combat;
}

void UKzCombatSubsystem::AssignCombat(APawn* Pawn, UKzCombat* Combat)
{
	if (!Pawn)
	{
		return;
	}

	if (Combat)
	{
		Assigned.Add(Pawn, Combat);
	}
	else
	{
		Assigned.Remove(Pawn);
	}
}

UKzCombat* UKzCombatSubsystem::GetCombatFor(APawn* Pawn)
{
	const TWeakObjectPtr<UKzCombat>* Found = Pawn ? Assigned.Find(Pawn) : nullptr;
	return Found && Found->IsValid() ? Found->Get() : GetDefaultCombat();
}

UKzCombatant* UKzCombatSubsystem::FindCombatant(const APawn* Pawn) const
{
	TArray<UKzCombat*> All;
	GetCombats(All);
	for (const UKzCombat* Combat : All)
	{
		if (UKzCombatant* Combatant = Combat->FindCombatant(Pawn))
		{
			return Combatant;
		}
	}
	return nullptr;
}

void UKzCombatSubsystem::BlockAttack(APawn* Pawn, FGameplayTag Attack)
{
	if (UKzCombatant* Combatant = FindCombatant(Pawn))
	{
		Combatant->SetBlocked(Attack, true);
	}
}

void UKzCombatSubsystem::UnblockAttack(APawn* Pawn, FGameplayTag Attack)
{
	if (UKzCombatant* Combatant = FindCombatant(Pawn))
	{
		Combatant->SetBlocked(Attack, false);
	}
}

void UKzCombatSubsystem::GetCombats(TArray<UKzCombat*>& OutCombats) const
{
	if (DefaultCombat)
	{
		OutCombats.Add(DefaultCombat);
	}
	for (UKzCombat* Combat : Combats)
	{
		OutCombats.Add(Combat);
	}
}

void UKzCombatSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* World = GetWorld();
	TArray<UKzCombat*> All;
	GetCombats(All);
	for (const UKzCombat* Combat : All)
	{
		const FColor Color = Combat->IsActive() ? FColor::White : FColor::Silver;
		UKzTicketPool* Pool = Combat->GetPool();
		for (const TPair<FGameplayTag, FKzTicketTypeSettings>& Pair : Combat->GetConfig()->Pool.Types)
		{
			FString Line = FString::Printf(TEXT("%s %s: %d free, %d held, %d cooling"), *Combat->GetName(), *Pair.Key.ToString(),
				Pool->GetCount(Pair.Key, EKzTicketState::Free), Pool->GetCount(Pair.Key, EKzTicketState::Held), Pool->GetCount(Pair.Key, EKzTicketState::Cooling));
			for (const float Left : Pool->GetCooldownsLeft(Pair.Key))
			{
				Line += FString::Printf(TEXT(" %.1fs"), Left);
			}
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.0f, Color, Line);
		}

		for (const UKzCombatant* Combatant : Combat->GetCombatants())
		{
			const APawn* Pawn = Combatant->GetPawn();
			const UKzAttackSet* AttackSet = Combatant->GetAttackSet();
			if (!Pawn || !AttackSet)
			{
				continue;
			}

			FString Text = Combatant->GetGrantedAttack().IsValid() ? Combatant->GetGrantedAttack().ToString() + (Combatant->HasAttackToTake() ? TEXT(" (given)") : TEXT(" (under way)")) : FString();
			for (const TPair<FGameplayTag, FKzAttackDefinition>& Pair : AttackSet->Attacks)
			{
				const float Left = Combatant->GetCooldownLeft(Pair.Key);
				if (Left > 0.0f || Combatant->IsBlocked(Pair.Key))
				{
					Text += FString::Printf(TEXT("\n%s %s"), *Pair.Key.ToString(), Combatant->IsBlocked(Pair.Key) ? TEXT("blocked") : *FString::Printf(TEXT("%.1fs"), Left));
				}
			}

			if (!Text.IsEmpty())
			{
				// 0 lasts one frame
				DrawDebugString(World, Pawn->GetActorLocation() + FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight() + 40.0), Text, nullptr, Color, 0.0f, true);
			}
		}
	}
}

bool UKzCombatSubsystem::IsTickable() const
{
	return CVarDrawTickets.GetValueOnGameThread() && (DefaultCombat || !Combats.IsEmpty());
}

bool UKzCombatSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}