// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class SWidget;
enum class EKzHitDirectionMode : uint8;
struct FKzHitDirection;

/**
 * A hit direction as one dropdown: the project's Hit Directions from Project Settings, then Hitbox Motion and Custom. Under it, a preset's vector
 * greyed as Project Settings has it, or Custom's own to edit. Picking a preset copies its vector, so Custom starts from the last one picked.
 */
class FKzHitDirectionCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FKzHitDirectionCustomization>(); }

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& Utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& Utils) override;

private:
	/** The direction edited, or null while the selection mixes different ones. */
	const FKzHitDirection* GetSingle() const;

	bool IsMode(EKzHitDirectionMode Mode) const;
	FText GetSelectedText() const;
	TSharedRef<SWidget> MakeMenu();
	void Select(EKzHitDirectionMode Mode, FGameplayTag Tag);

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyHandle> ModeHandle;
	TSharedPtr<IPropertyHandle> PresetTagHandle;
	TSharedPtr<IPropertyHandle> VectorHandle;
};