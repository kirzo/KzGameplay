// Copyright 2026 kirzo

#include "Animation/KzAnimationSource.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifyQueue.h"
#include "Animation/KzAnimationNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Items/Fragments/KzItemFragment_Database.h"

#define LOCTEXT_NAMESPACE "KzAnimationSource"

namespace
{
	/** The plays hearing each mesh's routed notifies. */
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TArray<TWeakPtr<FKzAnimationPlay>>> Registry;

	class FKzMontagePlay : public FKzAnimationPlay
	{
	public:
		bool Start(USkeletalMeshComponent& InMesh, UAnimInstance& InAnimInstance, UAnimMontage& Montage, float PlayRate, FName StartSection)
		{
			if (InAnimInstance.Montage_Play(&Montage, PlayRate) <= 0.0f)
			{
				return false;
			}

			FAnimMontageInstance* Instance = InAnimInstance.GetActiveInstanceForMontage(&Montage);
			if (!Instance)
			{
				return false;
			}

			if (!StartSection.IsNone())
			{
				InAnimInstance.Montage_JumpToSection(StartSection, &Montage);
			}

			AnimInstance = &InAnimInstance;
			InstanceID = Instance->GetInstanceID();
			const TSharedRef<FKzMontagePlay> Self = StaticCastSharedRef<FKzMontagePlay>(AsShared());
			Instance->OnMontageBlendingOutStarted.BindSP(Self, &FKzMontagePlay::OnMontageBlendingOut);
			Instance->OnMontageEnded.BindSP(Self, &FKzMontagePlay::OnMontageEnded);
			Begin(InMesh);
			return true;
		}

		virtual void Stop(float BlendOut) override
		{
			FAnimMontageInstance* Instance = IsPlaying() && AnimInstance.IsValid() ? AnimInstance->GetMontageInstanceForID(InstanceID) : nullptr;
			if (Instance)
			{
				Instance->Stop(FAlphaBlend(BlendOut));
			}
		}

	protected:
		virtual bool Owns(const FAnimNotifyEventReference& Reference) const override
		{
			const UE::Anim::FAnimNotifyMontageInstanceContext* Context = Reference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
			return Context && Context->MontageInstanceID == InstanceID;
		}

	private:
		void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
		{
			OnBlendOut.Broadcast();
		}

		void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
		{
			End(bInterrupted);
		}

		TWeakObjectPtr<UAnimInstance> AnimInstance;
		int32 InstanceID = INDEX_NONE;
	};
}

FKzAnimationPlay::~FKzAnimationPlay()
{
	Unregister();
}

void FKzAnimationPlay::RouteNotify(USkeletalMeshComponent* InMesh, const FAnimNotifyEventReference& Reference, const UKzAnimationNotifyState* Notify, bool bBegin, float Duration)
{
	const TArray<TWeakPtr<FKzAnimationPlay>>* Plays = Registry.Find(InMesh);
	if (!Plays)
	{
		return;
	}

	// A copy, since whoever hears it may start or end plays.
	for (const TWeakPtr<FKzAnimationPlay>& Weak : TArray<TWeakPtr<FKzAnimationPlay>>(*Plays))
	{
		const TSharedPtr<FKzAnimationPlay> Play = Weak.Pin();
		if (Play && Play->bPlaying && Play->Owns(Reference))
		{
			Play->OnNotify.Broadcast(Notify, bBegin, Duration);
			return;
		}
	}
}

void FKzAnimationPlay::Begin(USkeletalMeshComponent& InMesh)
{
	Mesh = &InMesh;
	bPlaying = true;
	Registry.FindOrAdd(Mesh).Add(AsShared());
}

void FKzAnimationPlay::End(bool bInterrupted)
{
	if (!bPlaying)
	{
		return;
	}

	bPlaying = false;
	Unregister();
	OnEnded.Broadcast(bInterrupted);
}

void FKzAnimationPlay::Unregister()
{
	TArray<TWeakPtr<FKzAnimationPlay>>* Plays = Registry.Find(Mesh);
	if (!Plays)
	{
		return;
	}

	// Pins of this play no longer resolve once it is being destroyed, so expired entries go too.
	Plays->RemoveAll([this](const TWeakPtr<FKzAnimationPlay>& Weak)
	{
		const TSharedPtr<FKzAnimationPlay> Play = Weak.Pin();
		return !Play || Play.Get() == this;
	});

	if (Plays->IsEmpty())
	{
		Registry.Remove(Mesh);
	}
}

TSharedPtr<FKzAnimationPlay> UKzAnimationSource_MontageBase::Play(const FKzAnimationContext& Context) const
{
	UAnimInstance* AnimInstance = Context.Mesh ? Context.Mesh->GetAnimInstance() : nullptr;
	UAnimMontage* Montage = GetMontage(Context);
	if (!AnimInstance || !Montage)
	{
		return nullptr;
	}

	const TSharedRef<FKzMontagePlay> MontagePlay = MakeShared<FKzMontagePlay>();
	if (!MontagePlay->Start(*Context.Mesh, *AnimInstance, *Montage, PlayRate, StartSection))
	{
		return nullptr;
	}
	return MontagePlay;
}

float UKzAnimationSource_MontageBase::GetTimeline(const FKzAnimationContext& Context, TArray<FKzAnimationNotifySpan>& OutNotifies) const
{
	const UAnimMontage* Montage = GetMontage(Context);
	if (!Montage)
	{
		return 0.0f;
	}

	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (const UKzAnimationNotifyState* Notify = Cast<UKzAnimationNotifyState>(Event.NotifyStateClass))
		{
			OutNotifies.Add({ Notify, Event.GetTriggerTime(), Event.GetEndTriggerTime() });
		}
	}
	return Montage->GetPlayLength();
}

UObject* UKzAnimationSource_MontageBase::GetAsset(const FKzAnimationContext& Context) const
{
	return GetMontage(Context);
}

#if WITH_EDITOR
FText UKzAnimationSource_Montage::GetDescription() const
{
	return Montage ? FText::FromString(Montage->GetName()) : LOCTEXT("NoMontage", "No montage");
}

bool UKzAnimationSource_Montage::SetAsset(UObject& Asset)
{
	UAnimMontage* AsMontage = Cast<UAnimMontage>(&Asset);
	if (AsMontage)
	{
		Montage = AsMontage;
	}
	return AsMontage != nullptr;
}
#endif

UAnimMontage* UKzAnimationSource_MontageByQuery::GetMontage(const FKzAnimationContext& Context) const
{
	return UKzItemFragment_Database::ResolveFor(Context.Item, Context.User, Context.MakeQuery(Query), TSoftObjectPtr<UAnimMontage>()).LoadSynchronous();
}

#if WITH_EDITOR
FText UKzAnimationSource_MontageByQuery::GetDescription() const
{
	return FText::FromString(Query.RequireTags.ToStringSimple());
}
#endif

#undef LOCTEXT_NAMESPACE