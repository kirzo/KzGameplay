// Copyright 2026 kirzo

#include "Customizations/KzFactionSettingsCustomization.h"
#include "Factions/KzFactionSettings.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "GameplayTagsManager.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KzFactionSettingsCustomization"

namespace
{
	using FWeakSettings = TWeakObjectPtr<UKzFactionSettings>;

	constexpr float CellWidth = 110.0f;
	constexpr float CellHeight = 30.0f;

	FSlateFontInfo CellFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", 10);
	}

	FSlateFontInfo LabelFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", 10);
	}

	bool IsPair(const FKzFactionRelation& Relation, const FGameplayTag& A, const FGameplayTag& B)
	{
		return (Relation.A == A && Relation.B == B) || (Relation.A == B && Relation.B == A);
	}

	int32 CountPair(const UKzFactionSettings& Settings, const FGameplayTag& A, const FGameplayTag& B)
	{
		int32 Count = 0;
		for (const FKzFactionRelation& Relation : Settings.Relations)
		{
			Count += IsPair(Relation, A, B) ? 1 : 0;
		}
		return Count;
	}

	FText FactionName(const FGameplayTag& Tag)
	{
		FString Name = Tag.ToString();
		Name.RemoveFromStart(TEXT("Faction."));
		return FText::FromString(Name);
	}

	FText AttitudeName(EKzAttitude Attitude)
	{
		return StaticEnum<EKzAttitude>()->GetDisplayValueAsText(Attitude);
	}

	FLinearColor AttitudeColor(EKzAttitude Attitude)
	{
		switch (Attitude)
		{
		case EKzAttitude::Friendly: return FLinearColor(0.12f, 0.45f, 0.16f);
		case EKzAttitude::Hostile: return FLinearColor(0.6f, 0.1f, 0.1f);
		default: return FLinearColor(0.2f, 0.2f, 0.2f);
		}
	}

	/** Neutral, Hostile, Friendly and round again. */
	EKzAttitude NextAttitude(EKzAttitude Attitude)
	{
		switch (Attitude)
		{
		case EKzAttitude::Neutral: return EKzAttitude::Hostile;
		case EKzAttitude::Hostile: return EKzAttitude::Friendly;
		default: return EKzAttitude::Neutral;
		}
	}

	/** Every Faction.* tag, plus any a relation names outside it, sorted by name. */
	TArray<FGameplayTag> CollectFactions(const UKzFactionSettings& Settings)
	{
		FGameplayTagContainer Tags;
		const FGameplayTag Root = FGameplayTag::RequestGameplayTag(TEXT("Faction"), false);
		if (Root.IsValid())
		{
			Tags = UGameplayTagsManager::Get().RequestGameplayTagChildren(Root);
		}

		for (const FKzFactionRelation& Relation : Settings.Relations)
		{
			if (Relation.A.IsValid())
			{
				Tags.AddTag(Relation.A);
			}
			if (Relation.B.IsValid())
			{
				Tags.AddTag(Relation.B);
			}
		}

		TArray<FGameplayTag> Factions;
		Tags.GetGameplayTagArray(Factions);
		Factions.Sort([](const FGameplayTag& Left, const FGameplayTag& Right) { return Left.ToString() < Right.ToString(); });
		return Factions;
	}

	/** One line per pair listed more than once, empty when there is none. */
	FText DescribeDuplicates(const UKzFactionSettings* Settings)
	{
		if (!Settings)
		{
			return FText::GetEmpty();
		}

		const TArray<FKzFactionRelation>& Relations = Settings->Relations;
		TArray<FString> Lines;
		for (int32 Index = 0; Index < Relations.Num(); ++Index)
		{
			const FKzFactionRelation& Relation = Relations[Index];
			bool bListedBefore = false;
			for (int32 Earlier = 0; Earlier < Index && !bListedBefore; ++Earlier)
			{
				bListedBefore = IsPair(Relations[Earlier], Relation.A, Relation.B);
			}

			const int32 Count = CountPair(*Settings, Relation.A, Relation.B);
			if (!bListedBefore && Count > 1)
			{
				Lines.Add(FText::Format(LOCTEXT("DuplicateLine", "{0} - {1} is listed {2} times: the first one counts."), FactionName(Relation.A), FactionName(Relation.B), Count).ToString());
			}
		}
		return FText::FromString(FString::Join(Lines, TEXT("\n")));
	}

	/** Edits the relations through their property, so the change can be undone and the settings save to their config. */
	void EditRelations(const FWeakSettings& WeakSettings, const TSharedPtr<IPropertyHandle>& Handle, const FText& Description, TFunctionRef<void(TArray<FKzFactionRelation>&)> Edit)
	{
		UKzFactionSettings* Settings = WeakSettings.Get();
		if (!Settings || !Handle.IsValid())
		{
			return;
		}

		const FScopedTransaction Transaction(Description);
		Handle->NotifyPreChange();
		Edit(Settings->Relations);
		Handle->NotifyPostChange(EPropertyChangeType::ValueSet);
		Handle->NotifyFinishedChangingProperties();
	}

	void SetAttitude(const FWeakSettings& WeakSettings, const TSharedPtr<IPropertyHandle>& Handle, const FGameplayTag& A, const FGameplayTag& B, EKzAttitude Attitude)
	{
		EditRelations(WeakSettings, Handle, LOCTEXT("SetRelation", "Set Faction Relation"), [&A, &B, Attitude](TArray<FKzFactionRelation>& Relations)
		{
			Relations.RemoveAll([&A, &B](const FKzFactionRelation& Relation) { return IsPair(Relation, A, B); });

			// A pair left out is neutral already
			if (Attitude != EKzAttitude::Neutral)
			{
				FKzFactionRelation& Relation = Relations.AddDefaulted_GetRef();
				Relation.A = A;
				Relation.B = B;
				Relation.Attitude = Attitude;
			}
		});
	}

	void RemoveDuplicates(const FWeakSettings& WeakSettings, const TSharedPtr<IPropertyHandle>& Handle)
	{
		EditRelations(WeakSettings, Handle, LOCTEXT("RemoveDuplicates", "Remove Duplicate Faction Relations"), [](TArray<FKzFactionRelation>& Relations)
		{
			// The first entry of a pair is the one that counts, so the later ones go
			for (int32 Index = Relations.Num() - 1; Index > 0; --Index)
			{
				for (int32 Earlier = 0; Earlier < Index; ++Earlier)
				{
					if (IsPair(Relations[Earlier], Relations[Index].A, Relations[Index].B))
					{
						Relations.RemoveAt(Index);
						break;
					}
				}
			}
		});
	}

	TSharedRef<SWidget> MakeCell(const FWeakSettings& WeakSettings, const TSharedPtr<IPropertyHandle>& Handle, const FGameplayTag& A, const FGameplayTag& B)
	{
		if (A == B)
		{
			return SNew(SBox)
				.WidthOverride(CellWidth)
				.HeightOverride(CellHeight)
				.ToolTipText(LOCTEXT("SameFaction", "A faction is friendly to itself."))
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(AttitudeColor(EKzAttitude::Friendly).CopyWithNewOpacity(0.4f))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(CellFont())
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Text(AttitudeName(EKzAttitude::Friendly))
					]
				];
		}

		auto Attitude = [WeakSettings, A, B]()
		{
			const UKzFactionSettings* Settings = WeakSettings.Get();
			return Settings ? Settings->GetAttitude(A, B) : EKzAttitude::Neutral;
		};
		auto Count = [WeakSettings, A, B]()
		{
			const UKzFactionSettings* Settings = WeakSettings.Get();
			return Settings ? CountPair(*Settings, A, B) : 0;
		};

		return SNew(SBox)
			.WidthOverride(CellWidth)
			.HeightOverride(CellHeight)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(0.0f)
				.OnClicked_Lambda([WeakSettings, Handle, A, B, Attitude]()
				{
					SetAttitude(WeakSettings, Handle, A, B, NextAttitude(Attitude()));
					return FReply::Handled();
				})
				.ToolTipText_Lambda([A, B, Attitude, Count]()
				{
					const FText Tip = FText::Format(LOCTEXT("CellTip", "{0} - {1}: {2}. Click to change."), FactionName(A), FactionName(B), AttitudeName(Attitude()));
					const int32 Listed = Count();
					return Listed > 1 ? FText::Format(LOCTEXT("CellTipDuplicate", "{0}\nListed {1} times: the first one counts."), Tip, Listed) : Tip;
				})
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor_Lambda([Attitude]() { return FSlateColor(AttitudeColor(Attitude())); })
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(CellFont())
						.Text_Lambda([Attitude, Count]()
						{
							return Count() > 1 ? FText::Format(LOCTEXT("CellDuplicate", "{0} (!)"), AttitudeName(Attitude())) : AttitudeName(Attitude());
						})
					]
				]
			];
	}

	TSharedRef<SWidget> BuildMatrix(const FWeakSettings& WeakSettings, const TSharedPtr<IPropertyHandle>& Handle)
	{
		const TArray<FGameplayTag> Factions = CollectFactions(*WeakSettings.Get());
		if (Factions.IsEmpty())
		{
			return SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("NoFactions", "Add Faction.* gameplay tags to set how factions see each other."));
		}

		// The lower half: each pair once, the diagonal being a faction and itself
		TSharedRef<SGridPanel> Grid = SNew(SGridPanel);
		for (int32 Column = 0; Column < Factions.Num(); ++Column)
		{
			Grid->AddSlot(Column + 1, 0)
				.Padding(FMargin(2.0f, 0.0f, 2.0f, 6.0f))
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock).Font(LabelFont()).Text(FactionName(Factions[Column]))
				];
		}

		for (int32 Row = 0; Row < Factions.Num(); ++Row)
		{
			Grid->AddSlot(0, Row + 1)
				.Padding(FMargin(2.0f, 2.0f, 12.0f, 2.0f))
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(LabelFont()).Text(FactionName(Factions[Row]))
				];

			for (int32 Column = 0; Column <= Row; ++Column)
			{
				Grid->AddSlot(Column + 1, Row + 1)
					.Padding(2.0f)
					[
						MakeCell(WeakSettings, Handle, Factions[Row], Factions[Column])
					];
			}
		}

		return SNew(SBox)
			.Padding(FMargin(0.0f, 12.0f))
			[
				Grid
			];
	}
}

void FKzFactionSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	const FWeakSettings WeakSettings = Objects.Num() == 1 ? Cast<UKzFactionSettings>(Objects[0].Get()) : nullptr;
	if (!WeakSettings.IsValid())
	{
		return;
	}

	const TSharedPtr<IPropertyHandle> Handle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UKzFactionSettings, Relations));
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Factions"));

	Category.AddCustomRow(LOCTEXT("DuplicatesRow", "Duplicates"))
		.Visibility(TAttribute<EVisibility>::CreateLambda([WeakSettings]()
		{
			return DescribeDuplicates(WeakSettings.Get()).IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
		}))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Top)
			.Padding(FMargin(0.0f, 2.0f, 6.0f, 2.0f))
			[
				SNew(SImage).Image(FAppStyle::GetBrush("Icons.WarningWithColor"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.AutoWrapText(true)
				.Text_Lambda([WeakSettings]() { return DescribeDuplicates(WeakSettings.Get()); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(6.0f, 0.0f, 0.0f, 0.0f))
			[
				SNew(SButton)
				.Text(LOCTEXT("RemoveDuplicatesButton", "Remove Duplicates"))
				.ToolTipText(LOCTEXT("RemoveDuplicatesTip", "Keeps the first entry of each pair, the one that counts."))
				.OnClicked_Lambda([WeakSettings, Handle]()
				{
					RemoveDuplicates(WeakSettings, Handle);
					return FReply::Handled();
				})
			]
		];

	Category.AddCustomRow(LOCTEXT("MatrixRow", "Matrix"))
		.WholeRowContent()
		.HAlign(HAlign_Center)
		[
			BuildMatrix(WeakSettings, Handle)
		];

	// The matrix edits it, so the list stays out of sight
	DetailBuilder.HideProperty(Handle);
}

#undef LOCTEXT_NAMESPACE
