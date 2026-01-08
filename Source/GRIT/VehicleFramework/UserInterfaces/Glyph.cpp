// Glyph.cpp
#include "Glyph.h"
#include "Components/BorderSlot.h"
#include "Blueprint/WidgetTree.h"

//------------------------------------------------------------------------------
//                            widget initialization
//------------------------------------------------------------------------------

void UGlyph::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize state
	bIsHovered = false;
	bIsPressed = false;
	
	//------------------------------------------------------------------------------
	// construct widget hierarchy
	//------------------------------------------------------------------------------
	
	// Reason: Root border not auto-created in C++
	if (!RootBorder)
	{
		RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
		WidgetTree->RootWidget = RootBorder;
		
		FSlateBrush BorderBrush;
		BorderBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
		RootBorder->SetBrush(BorderBrush);
		RootBorder->SetHorizontalAlignment(HAlign_Center);
		RootBorder->SetVerticalAlignment(VAlign_Center);
		RootBorder->SetPadding(FMargin(0.0f));
	} // End if (root check)
	
	// Reason: Create icon image
	if (!Icon)
	{
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		UBorderSlot* IconSlot = Cast<UBorderSlot>(RootBorder->AddChild(Icon));
		if (IconSlot)
		{
			IconSlot->SetPadding(FMargin(0.0f));
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		} // End if (slot valid)
	} // End if (icon check)
	
	// Reason: Enable interaction
	if (RootBorder)
	{
		RootBorder->SetVisibility(ESlateVisibility::Visible);
	} // End if (border valid)
}

//------------------------------------------------------------------------------
//                            mouse event handlers
//------------------------------------------------------------------------------

void UGlyph::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	// Reason: Trigger hover state
	if (!bIsHovered)
	{
		bIsHovered = true;
		OnHovered();
	} // End if (hover transition)
}

void UGlyph::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	// Reason: Clear hover and press states
	if (bIsHovered)
	{
		bIsHovered = false;
		OnUnhovered();
	} // End if (unhover transition)
	
	if (bIsPressed)
	{
		bIsPressed = false;
	} // End if (clear press)
}

FReply UGlyph::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Reason: Left click triggers press
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPressed = true;
		OnPressed();
		return FReply::Handled();
	} // End if (left click)
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGlyph::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Reason: Release completes click action
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed)
	{
		bIsPressed = false;
		OnReleased();
		return FReply::Handled();
	} // End if (release)
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

//------------------------------------------------------------------------------
//                            content modification
//------------------------------------------------------------------------------

void UGlyph::SetIcon(UMaterialInterface* IconMaterial)
{
	// Reason: Apply material to icon image
	if (Icon && IconMaterial)
	{
		FSlateBrush Brush;
		Brush.ImageType = ESlateBrushImageType::FullColor;
		Brush.SetResourceObject(IconMaterial);
		Icon->SetBrush(Brush);
	} // End if (icon valid)
}
