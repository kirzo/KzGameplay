// Copyright 2026 kirzo

#include "Animation/KzAnimationSource_LimbicAction.h"
#include "Animation/AnimNotifyQueue.h"
#include "Animation/KzAnimationNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Items/Fragments/KzItemFragment_Database.h"
#include "LimbicPuppeteer.h"
#include "LimbicSlotLibrary.h"

#define LOCTEXT_NAMESPACE "KzAnimationSource_LimbicAction"

namespace
{
	class FKzLimbicActionPlay : public FKzAnimationPlay
	{
	public:
		virtual ~FKzLimbicActionPlay() override
		{
			ULimbicSlotLibrary::OnSlotEvent().Remove(EventHandle);
		}

		bool Start(USkeletalMeshComponent& InMesh, const UKzAnimationSource_LimbicActionBase& Source, ULimbicPuppeteer& Puppeteer)
		{
			PlayId = ULimbicSlotLibrary::PlayLimbicAction(&InMesh, Source.Slot, &Puppeteer, Source.PlayRate, Source.BlendIn, Source.BlendOut);
			if (PlayId == INDEX_NONE)
			{
				return false;
			}

			PlayedMesh = &InMesh;
			Slot = Source.Slot;
			EventHandle = ULimbicSlotLibrary::OnSlotEvent().AddSP(StaticCastSharedRef<FKzLimbicActionPlay>(AsShared()), &FKzLimbicActionPlay::OnSlotEvent);
			Begin(InMesh);
			return true;
		}

		virtual void Stop(float InBlendOut) override
		{
			if (IsPlaying())
			{
				FLimbicSlotHandle Handle;
				Handle.Meshes.Add(PlayedMesh);
				Handle.Slot = Slot;
				Handle.PlayId = PlayId;
				ULimbicSlotLibrary::StopLimbicSlotByHandle(Handle, InBlendOut);
			}
		}

	protected:
		virtual bool Owns(const FAnimNotifyEventReference& Reference) const override
		{
			const FLimbicPlayContext* Context = Reference.GetContextData<FLimbicPlayContext>();
			return Context && Context->PlayId == PlayId;
		}

	private:
		void OnSlotEvent(USkeletalMeshComponent* EventMesh, const FLimbicSlotEvent& Event)
		{
			if (EventMesh != PlayedMesh.Get() || Event.PlayId != PlayId)
			{
				return;
			}

			switch (Event.Type)
			{
			case ELimbicSlotEvent::BlendOut:
				OnBlendOut.Broadcast();
				break;
			case ELimbicSlotEvent::Completed:
				End(false);
				break;
			case ELimbicSlotEvent::Interrupted:
				End(true);
				break;
			default:
				break;
			}
		}

		TWeakObjectPtr<USkeletalMeshComponent> PlayedMesh;
		FGameplayTag Slot;
		int32 PlayId = INDEX_NONE;
		FDelegateHandle EventHandle;
	};
}

TSharedPtr<FKzAnimationPlay> UKzAnimationSource_LimbicActionBase::Play(const FKzAnimationContext& Context) const
{
	ULimbicPuppeteer* Puppeteer = GetPuppeteer(Context);
	if (!Context.Mesh || !Puppeteer)
	{
		return nullptr;
	}

	const TSharedRef<FKzLimbicActionPlay> ActionPlay = MakeShared<FKzLimbicActionPlay>();
	if (!ActionPlay->Start(*Context.Mesh, *this, *Puppeteer))
	{
		return nullptr;
	}
	return ActionPlay;
}

float UKzAnimationSource_LimbicActionBase::GetTimeline(const FKzAnimationContext& Context, TArray<FKzAnimationNotifySpan>& OutNotifies) const
{
	const ULimbicPuppeteer* Puppeteer = GetPuppeteer(Context);
	if (!Puppeteer)
	{
		return 0.0f;
	}

	for (const FLimbicNotifyState& Entry : Puppeteer->NotifyStates)
	{
		if (const UKzAnimationNotifyState* Notify = Cast<UKzAnimationNotifyState>(Entry.State))
		{
			OutNotifies.Add({ Notify, Entry.Time, Entry.Time + Entry.Length });
		}
	}
	return Puppeteer->Duration;
}

UObject* UKzAnimationSource_LimbicActionBase::GetAsset(const FKzAnimationContext& Context) const
{
	return GetPuppeteer(Context);
}

#if WITH_EDITOR
FText UKzAnimationSource_LimbicAction::GetDescription() const
{
	return Puppeteer ? FText::FromString(Puppeteer->GetName()) : LOCTEXT("NoPuppeteer", "No puppeteer");
}

bool UKzAnimationSource_LimbicAction::SetAsset(UObject& Asset)
{
	ULimbicPuppeteer* AsPuppeteer = Cast<ULimbicPuppeteer>(&Asset);
	if (AsPuppeteer)
	{
		Puppeteer = AsPuppeteer;
	}
	return AsPuppeteer != nullptr;
}
#endif

ULimbicPuppeteer* UKzAnimationSource_LimbicActionByQuery::GetPuppeteer(const FKzAnimationContext& Context) const
{
	return UKzItemFragment_Database::ResolveFor(Context.Item, Context.User, Context.MakeQuery(Query), TSoftObjectPtr<ULimbicPuppeteer>()).LoadSynchronous();
}

#if WITH_EDITOR
FText UKzAnimationSource_LimbicActionByQuery::GetDescription() const
{
	return FText::FromString(Query.RequireTags.ToStringSimple());
}
#endif

#undef LOCTEXT_NAMESPACE