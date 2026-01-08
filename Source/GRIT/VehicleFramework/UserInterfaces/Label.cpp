// Label.cpp
#include "Label.h"
#include "Components/BorderSlot.h"
#include "Blueprint/WidgetTree.h"

//------------------------------------------------------------------------------
//                            widget initialization
//------------------------------------------------------------------------------

void ULabel::NativeConstruct()
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
	
	// Reason: Create caption text
	if (!Caption)
	{
		Caption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Caption"));
		Caption->SetText(FText::FromString(TEXT("Text Block")));
		
		UBorderSlot* CaptionSlot = Cast<UBorderSlot>(RootBorder->AddChild(Caption));
		if (CaptionSlot)
		{
			CaptionSlot->SetPadding(FMargin(0.0f));
			CaptionSlot->SetHorizontalAlignment(HAlign_Center);
			CaptionSlot->SetVerticalAlignment(VAlign_Center);
		} // End if (slot valid)
	} // End if (caption check)
	
	// Reason: Enable interaction
	if (RootBorder)
	{
		RootBorder->SetVisibility(ESlateVisibility::Visible);
	} // End if (border valid)
}

//------------------------------------------------------------------------------
//                            mouse event handlers
//------------------------------------------------------------------------------

void ULabel::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	// Reason: Trigger hover state
	if (!bIsHovered)
	{
		bIsHovered = true;
		OnHovered();
	} // End if (hover transition)
}

void ULabel::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
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

FReply ULabel::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

FReply ULabel::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

void ULabel::SetCaption(const FText& NewText)
{
	if (Caption) { Caption->SetText(NewText); }
}
