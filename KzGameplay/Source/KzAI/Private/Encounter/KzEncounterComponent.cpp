// Copyright 2026 kirzo

#include "Encounter/KzEncounterComponent.h"
#include "Abilities/KzAbilitySystemComponent.h"
#include "Combat/KzCombat.h"
#include "Combat/KzCombatConfig.h"
#include "Combat/KzCombatSettings.h"
#include "Combat/KzCombatSubsystem.h"
#include "Combat/KzCombatTags.h"
#include "Engagement/KzEngagementComponent.h"
#include "Kismet/KzSystemLibrary.h"
#include "KzAITags.h"
#include "Misc/KzToggleable.h"
#include "ScriptableContext.h"
#include "Sensors/KzSensorTags.h"
#include "Spawning/KzSpawnComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKzEncounter, Log, All);

namespace
{
	TAutoConsoleVariable<bool> CVarDrawEncounter(TEXT("kz.Encounter.Draw"), false, TEXT("Draws each encounter's state over it, with lines to its enemies alive."));

	/** How often an active encounter checks its conditions and its queued enemies. */
	constexpr float UpdateInterval = 0.25f;

	void AddContext(FScriptableContainer& Container)
	{
		Container.AddContextProperty<AActor*>(TEXT("Encounter"));
		Container.AddContextProperty<AActor*>(TEXT("Target"));
	}
}

FKzEncounterWave::FKzEncounterWave()
{
	AddContext(StartWhen);
}

UKzEncounterComponent::UKzEncounterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	AddContext(StartAction);
	AddContext(EndWhen);
	AddContext(EndAction);
}

void UKzEncounterComponent::BeginPlay()
{
	Super::BeginPlay();

	UKzCombatSubsystem* Subsystem = UKzCombatSubsystem::Get(this);
	Combat = Subsystem ? Subsystem->CreateCombat(CombatConfig ? CombatConfig.Get() : UKzCombatSettings::Get()->DefaultCombatConfig.LoadSynchronous()) : nullptr;
	if (Combat)
	{
		Combat->OnCombatantJoined.AddUObject(this, &ThisClass::HandleCombatantJoined);
	}

	SetLimitsEnabled(false);
	Unspawned.Init(0, Waves.Num());

	for (AActor* Actor : Initial)
	{
		if (!Actor)
		{
			continue;
		}

		if (UKzSpawnComponent* Point = Actor->FindComponentByClass<UKzSpawnComponent>())
		{
			Point->OnSpawned.AddUniqueDynamic(this, &ThisClass::HandleInitialSpawned);
			InitialPoints.Add(Point);

			TArray<AActor*> Spawned;
			Point->GetSpawnedActors(Spawned);
			for (AActor* Each : Spawned)
			{
				HandleInitialSpawned(Each);
			}
		}
		else
		{
			HandleInitialSpawned(Actor);
		}
	}

	if (bStartOnSensor)
	{
		UKzSensorComponent* Sensor = UKzSystemLibrary::FindComponentByGameplayTag<UKzSensorComponent>(GetOwner(), KzTags::Sensor::Encounter);
		if (Sensor)
		{
			StartSensor = Sensor;
			Sensor->OnObjectBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleSensed);
			Sensor->Activate();
		}
		else
		{
			UE_LOG(LogKzEncounter, Warning, TEXT("%s starts on a sensor tagged Sensor.Encounter but has none."), *GetNameSafe(GetOwner()));
		}
	}
}

void UKzEncounterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	GetWorld()->GetTimerManager().ClearTimer(EndTimer);

	for (const FEnemy& Enemy : Enemies)
	{
		StopWatching(Enemy);
	}
	Enemies.Reset();
	RestoreEngagements();
	StartAction.Reset();
	EndAction.Reset();

	Super::EndPlay(EndPlayReason);
}

void UKzEncounterComponent::Start(AActor* InTarget)
{
	if (State != EKzEncounterState::Waiting)
	{
		return;
	}

	State = EKzEncounterState::Active;
	Target = InTarget;
	StartTime = WaveStartTime = GetWorld()->GetTimeSeconds();
	StartDefeat = WaveDefeat = Defeats.Num();
	UE_LOG(LogKzEncounter, Log, TEXT("%s starts against %s."), *GetNameSafe(GetOwner()), *GetNameSafe(InTarget));

	if (UKzSensorComponent* Sensor = StartSensor.Get())
	{
		Sensor->OnObjectBeginOverlap.RemoveDynamic(this, &ThisClass::HandleSensed);

		// Not in the middle of its scan
		GetWorld()->GetTimerManager().SetTimerForNextTick(Sensor, &UKzSensorComponent::Deactivate);
	}

	if (Combat)
	{
		Combat->Start();
		for (UKzCombatant* Combatant : Combat->GetCombatants())
		{
			OverrideEngagement(Combatant->GetTarget());
		}
	}
	OverrideEngagement(InTarget);
	SetLimitsEnabled(true);

	for (const FEnemy& Enemy : Enemies)
	{
		SuggestTarget(Enemy.Actor.Get());
	}

	RunAction(StartAction);
	OnStarted.Broadcast(InTarget);

	GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &ThisClass::Update, UpdateInterval, true);
	Update();
}

void UKzEncounterComponent::End()
{
	if (State == EKzEncounterState::Ended)
	{
		return;
	}

	const bool bWasActive = State == EKzEncounterState::Active;
	State = EKzEncounterState::Ended;
	UE_LOG(LogKzEncounter, Log, TEXT("%s ends."), *GetNameSafe(GetOwner()));

	GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	GetWorld()->GetTimerManager().ClearTimer(EndTimer);
	Queue.Reset();

	for (const TWeakObjectPtr<UKzSpawnComponent>& Point : InitialPoints)
	{
		if (Point.IsValid())
		{
			Point->OnSpawned.RemoveDynamic(this, &ThisClass::HandleInitialSpawned);
		}
	}
	if (UKzSensorComponent* Sensor = StartSensor.Get())
	{
		Sensor->OnObjectBeginOverlap.RemoveDynamic(this, &ThisClass::HandleSensed);
	}

	if (Combat)
	{
		Combat->End();
	}
	RestoreEngagements();

	if (bWasActive)
	{
		SetLimitsEnabled(false);
		StartAction.Reset();
		RunAction(EndAction);
	}
	OnEnded.Broadcast();
}

int32 UKzEncounterComponent::GetWavesCleared() const
{
	int32 Cleared = 0;
	for (int32 Wave = 0; Wave < NextWave; ++Wave)
	{
		if (Unspawned[Wave] == 0 && !Enemies.ContainsByPredicate([Wave](const FEnemy& Enemy) { return Enemy.Wave == Wave; }))
		{
			++Cleared;
		}
	}
	return Cleared;
}

void UKzEncounterComponent::GetEnemies(const FGameplayTagContainer& Tags, TArray<AActor*>& OutEnemies) const
{
	OutEnemies.Reset();
	for (const FEnemy& Enemy : Enemies)
	{
		if (Enemy.Actor.IsValid() && (Tags.IsEmpty() || Enemy.Tags.HasAny(Tags)))
		{
			OutEnemies.Add(Enemy.Actor.Get());
		}
	}
}

int32 UKzEncounterComponent::GetAliveCount(bool bLastWave, const FGameplayTagContainer& Tags) const
{
	const int32 LastWave = NextWave - 1;
	const auto Counts = [&](int32 Wave, const FGameplayTagContainer& EnemyTags)
	{
		return (!bLastWave || Wave == LastWave) && (Tags.IsEmpty() || EnemyTags.HasAny(Tags));
	};

	int32 Count = 0;
	for (const FEnemy& Enemy : Enemies)
	{
		Count += Counts(Enemy.Wave, Enemy.Tags) ? 1 : 0;
	}
	for (const TArray<FQueued>* Coming : { &Queue, &InFlight })
	{
		for (const FQueued& Each : *Coming)
		{
			Count += Counts(Each.Wave, Waves[Each.Wave].Spawns[Each.Spawn].Tags) ? 1 : 0;
		}
	}
	return Count;
}

int32 UKzEncounterComponent::GetDefeatedCount(bool bSinceWave, const FGameplayTagContainer& Tags) const
{
	int32 Count = 0;
	for (int32 Index = bSinceWave ? WaveDefeat : StartDefeat; Index < Defeats.Num(); ++Index)
	{
		if (Tags.IsEmpty() || Defeats[Index].HasAny(Tags))
		{
			++Count;
		}
	}
	return Count;
}

double UKzEncounterComponent::GetTimeSince(bool bSinceWave) const
{
	return State == EKzEncounterState::Waiting ? 0.0 : GetWorld()->GetTimeSeconds() - (bSinceWave ? WaveStartTime : StartTime);
}

void UKzEncounterComponent::Update()
{
	bUpdateRequested = false;
	if (State != EKzEncounterState::Active)
	{
		return;
	}

	while (Waves.IsValidIndex(NextWave) && CanStartWave(NextWave))
	{
		StartWave(NextWave++);
	}

	const double Now = GetWorld()->GetTimeSeconds();
	TArray<FQueued> Ready;
	for (int32 Index = 0; Index < Queue.Num();)
	{
		if (Queue[Index].Time <= Now && (MaxAlive <= 0 || Enemies.Num() + InFlight.Num() + Ready.Num() < MaxAlive))
		{
			Ready.Add(Queue[Index]);
			Queue.RemoveAt(Index);
		}
		else
		{
			++Index;
		}
	}
	for (const FQueued& Queued : Ready)
	{
		SpawnQueued(Queued);
	}

	if (!EndTimer.IsValid() && !EndWhen.IsEmpty() && Evaluate(EndWhen))
	{
		UE_LOG(LogKzEncounter, Log, TEXT("%s meets End When."), *GetNameSafe(GetOwner()));
		if (EndDelay > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(EndTimer, this, &ThisClass::End, EndDelay);
		}
		else
		{
			End();
		}
		return;
	}

	if (!Waves.IsValidIndex(NextWave) && Queue.IsEmpty() && InFlight.IsEmpty() && Enemies.IsEmpty())
	{
		End();
	}
}

void UKzEncounterComponent::RequestUpdate()
{
	if (!bUpdateRequested && State == EKzEncounterState::Active)
	{
		bUpdateRequested = true;
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::Update);
	}
}

bool UKzEncounterComponent::CanStartWave(int32 Wave)
{
	// Only once the one before it is out whole, so none of its enemies waiting to spawn look gone
	if (Wave > 0 && Unspawned[Wave - 1] > 0)
	{
		return false;
	}

	if (Waves[Wave].StartWhen.IsEmpty())
	{
		return Queue.IsEmpty() && InFlight.IsEmpty() && Enemies.IsEmpty();
	}
	return Evaluate(Waves[Wave].StartWhen);
}

void UKzEncounterComponent::StartWave(int32 Wave)
{
	WaveStartTime = GetWorld()->GetTimeSeconds();
	WaveDefeat = Defeats.Num();
	UE_LOG(LogKzEncounter, Log, TEXT("%s: wave %d comes."), *GetNameSafe(GetOwner()), Wave);

	const TArray<FKzEncounterSpawn>& Spawns = Waves[Wave].Spawns;
	for (int32 Spawn = 0; Spawn < Spawns.Num(); ++Spawn)
	{
		for (int32 Each = 0; Each < Spawns[Spawn].Count; ++Each)
		{
			Queue.Add({ Wave, Spawn, WaveStartTime + Spawns[Spawn].Delay });
			++Unspawned[Wave];
		}
	}
	OnWaveStarted.Broadcast(Wave);
}

void UKzEncounterComponent::SpawnQueued(const FQueued& Queued)
{
	const FKzEncounterSpawn& Spawn = Waves[Queued.Wave].Spawns[Queued.Spawn];
	UKzSpawnComponent* Point = Spawn.SpawnPoint ? Spawn.SpawnPoint->FindComponentByClass<UKzSpawnComponent>() : nullptr;
	if (!Point || !Spawn.Class)
	{
		UE_LOG(LogKzEncounter, Error, TEXT("%s: wave %d, spawn %d has no %s."), *GetNameSafe(GetOwner()), Queued.Wave, Queued.Spawn, Point ? TEXT("class") : TEXT("spawn point with a Spawn Component"));
		--Unspawned[Queued.Wave];
		RequestUpdate();
		return;
	}

	InFlight.Add(Queued);
	Point->RequestSpawn(Spawn.Class, [WeakThis = TWeakObjectPtr<UKzEncounterComponent>(this), Wave = Queued.Wave, SpawnIndex = Queued.Spawn](AActor* Actor)
	{
		if (UKzEncounterComponent* This = WeakThis.Get())
		{
			This->HandleWaveSpawned(Actor, Wave, SpawnIndex);
		}
	});
}

void UKzEncounterComponent::HandleWaveSpawned(AActor* Actor, int32 Wave, int32 Spawn)
{
	if (State != EKzEncounterState::Active)
	{
		return;
	}

	const int32 FlightIndex = InFlight.IndexOfByPredicate([Wave, Spawn](const FQueued& Each) { return Each.Wave == Wave && Each.Spawn == Spawn; });
	if (FlightIndex != INDEX_NONE)
	{
		InFlight.RemoveAt(FlightIndex);
	}
	--Unspawned[Wave];

	if (Actor)
	{
		// One of its Initial points may have taken it already as one of its own
		const FGameplayTagContainer& Tags = Waves[Wave].Spawns[Spawn].Tags;
		if (FEnemy* Existing = FindEnemy(Actor))
		{
			Existing->Wave = Wave;
			Existing->Tags = Tags;
		}
		else
		{
			AddEnemy(Actor, Tags, Wave);
		}
		SuggestTarget(Actor);
	}
	RequestUpdate();
}

void UKzEncounterComponent::AddEnemy(AActor* Actor, const FGameplayTagContainer& Tags, int32 Wave)
{
	FEnemy& Enemy = Enemies.AddDefaulted_GetRef();
	Enemy.Actor = Actor;
	Enemy.Tags = Tags;
	Enemy.Wave = Wave;

	APawn* Pawn = Cast<APawn>(Actor);
	UKzCombatSubsystem* Subsystem = UKzCombatSubsystem::Get(this);
	if (Pawn && Subsystem && Combat)
	{
		Subsystem->AssignCombat(Pawn, Combat);
	}

	if (UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
	{
		Enemy.AbilitySystem = AbilitySystem;
		Enemy.DeadHandle = AbilitySystem->RegisterGameplayTagEvent(KzTags::Combat::State_Dead).AddUObject(this, &ThisClass::HandleDeadTag, TWeakObjectPtr<AActor>(Actor));
		if (UKzAbilitySystemComponent* KzAbilitySystem = Cast<UKzAbilitySystemComponent>(AbilitySystem))
		{
			KzAbilitySystem->OnGameplayEvent.AddUniqueDynamic(this, &ThisClass::HandleEnemyEvent);
		}
	}
	Actor->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleEnemyDestroyed);
}

UKzEncounterComponent::FEnemy* UKzEncounterComponent::FindEnemy(const AActor* Actor)
{
	return Enemies.FindByPredicate([Actor](const FEnemy& Enemy) { return Enemy.Actor.Get() == Actor; });
}

void UKzEncounterComponent::Defeat(AActor* Actor)
{
	const int32 Index = Enemies.IndexOfByPredicate([Actor](const FEnemy& Enemy) { return Enemy.Actor.Get() == Actor; });
	if (Index == INDEX_NONE)
	{
		return;
	}

	// Defeated before it could engage, such as in one blow from behind
	if (State == EKzEncounterState::Waiting && bStartOnEngage)
	{
		Start(LastAttacker.Get());
	}

	const FEnemy Enemy = Enemies[Index];
	Enemies.RemoveAt(Index);
	StopWatching(Enemy);
	Defeats.Add(Enemy.Tags);

	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (Combat)
		{
			Combat->Leave(Pawn);
		}
		if (UKzCombatSubsystem* Subsystem = UKzCombatSubsystem::Get(this))
		{
			Subsystem->AssignCombat(Pawn, nullptr);
		}
	}
	RequestUpdate();
}

void UKzEncounterComponent::StopWatching(const FEnemy& Enemy)
{
	if (UAbilitySystemComponent* AbilitySystem = Enemy.AbilitySystem.Get())
	{
		AbilitySystem->RegisterGameplayTagEvent(KzTags::Combat::State_Dead).Remove(Enemy.DeadHandle);
		if (UKzAbilitySystemComponent* KzAbilitySystem = Cast<UKzAbilitySystemComponent>(AbilitySystem))
		{
			KzAbilitySystem->OnGameplayEvent.RemoveDynamic(this, &ThisClass::HandleEnemyEvent);
		}
	}
	if (AActor* Actor = Enemy.Actor.Get())
	{
		Actor->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleEnemyDestroyed);
	}
}

void UKzEncounterComponent::SuggestTarget(AActor* Enemy) const
{
	if (Enemy && Target.IsValid())
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetOwner();
		Payload.Target = Target.Get();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Enemy, KzTags::AI::Target_Suggested, Payload);
	}
}

void UKzEncounterComponent::SetLimitsEnabled(bool bEnabled)
{
	for (AActor* Limit : Limits)
	{
		if (!Limit)
		{
			continue;
		}

		if (Limit->Implements<UKzToggleable>())
		{
			IKzToggleable::Execute_SetEnabled(Limit, bEnabled);
		}
		else
		{
			Limit->SetActorHiddenInGame(!bEnabled);
			Limit->SetActorEnableCollision(bEnabled);
		}
	}
}

void UKzEncounterComponent::OverrideEngagement(AActor* EngagedTarget)
{
	UKzEngagementComponent* Engagement = EngagementConfig && EngagedTarget ? EngagedTarget->FindComponentByClass<UKzEngagementComponent>() : nullptr;
	if (Engagement && Engagement->GetOverrideConfig() != EngagementConfig)
	{
		Engagement->SetOverrideConfig(EngagementConfig);
		Overridden.AddUnique(Engagement);
	}
}

void UKzEncounterComponent::RestoreEngagements()
{
	for (const TWeakObjectPtr<UKzEngagementComponent>& Engagement : Overridden)
	{
		// Another encounter may have put its own in since
		if (Engagement.IsValid() && Engagement->GetOverrideConfig() == EngagementConfig)
		{
			Engagement->SetOverrideConfig(nullptr);
		}
	}
	Overridden.Reset();
}

bool UKzEncounterComponent::Evaluate(const FScriptableRequirement& Requirement)
{
	FScriptableContext Context;
	Context.SetProperty(TEXT("Encounter"), GetOwner());
	Context.SetProperty(TEXT("Target"), Target.Get());
	return FScriptableRequirement::EvaluateRequirement(this, Requirement, Context);
}

void UKzEncounterComponent::RunAction(FScriptableAction& Action)
{
	if (!Action.Tasks.IsEmpty())
	{
		FScriptableContext Context;
		Context.SetProperty(TEXT("Encounter"), GetOwner());
		Context.SetProperty(TEXT("Target"), Target.Get());
		Action.Run(this, Context);
	}
}

void UKzEncounterComponent::HandleCombatantJoined(UKzCombatant* Combatant)
{
	if (State == EKzEncounterState::Waiting && bStartOnEngage)
	{
		Start(Combatant->GetTarget());
	}
	else if (State == EKzEncounterState::Active)
	{
		OverrideEngagement(Combatant->GetTarget());
	}
}

void UKzEncounterComponent::HandleDeadTag(const FGameplayTag Tag, int32 Count, TWeakObjectPtr<AActor> Actor)
{
	if (Count > 0)
	{
		Defeat(Actor.Get());
	}
}

void UKzEncounterComponent::HandleSensed(const FKzOverlapResult& OverlapInfo)
{
	Start(OverlapInfo.Actor.Get());
}

void UKzEncounterComponent::HandleInitialSpawned(AActor* Actor)
{
	if (!Actor || State == EKzEncounterState::Ended || FindEnemy(Actor))
	{
		return;
	}

	AddEnemy(Actor, FGameplayTagContainer(), INDEX_NONE);
	if (State == EKzEncounterState::Active)
	{
		SuggestTarget(Actor);
	}
}

void UKzEncounterComponent::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	Defeat(DestroyedActor);
}

void UKzEncounterComponent::HandleEnemyEvent(const FGameplayTag& Tag, FGameplayEventData EventData)
{
	if (EventData.Instigator && (Tag.MatchesTag(KzTags::Combat::Combat_Targeted) || Tag.MatchesTag(KzTags::Combat::Reaction)))
	{
		LastAttacker = const_cast<AActor*>(EventData.Instigator.Get());
	}
}

void UKzEncounterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CVarDrawEncounter.GetValueOnGameThread())
	{
		return;
	}

	static const TCHAR* StateNames[] = { TEXT("Waiting"), TEXT("Active"), TEXT("Ended") };
	const FString Text = FString::Printf(TEXT("%s: %s\nWaves %d/%d, cleared %d\nAlive %d, coming %d, defeated %d"), *GetNameSafe(GetOwner()), StateNames[static_cast<int32>(State)],
		NextWave, Waves.Num(), GetWavesCleared(), Enemies.Num(), Queue.Num() + InFlight.Num(), Defeats.Num());

	const FColor Color(245, 165, 36);
	const FVector Location = GetComponentLocation();
	DrawDebugString(GetWorld(), Location + FVector(0.0f, 0.0f, 150.0f), Text, nullptr, Color, 0.0f, true);
	for (const FEnemy& Enemy : Enemies)
	{
		if (const AActor* Actor = Enemy.Actor.Get())
		{
			DrawDebugLine(GetWorld(), Location, Actor->GetActorLocation(), Color, false, 0.0f, 0, 2.0f);
		}
	}
}