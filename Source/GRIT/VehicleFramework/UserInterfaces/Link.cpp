// Link.cpp
#include "Link.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

//------------------------------------------------------------------------------
//                            widget initialization
//------------------------------------------------------------------------------

void ULink::NativeConstruct()
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
	} // End if (root check)
	
	// Reason: Create horizontal box for icon + text layout
	if (!ContentBox)
	{
		ContentBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentBox"));
		RootBorder->SetContent(ContentBox);
	} // End if (content box check)
	
	// Reason: Create icon image
	if (!Icon)
	{
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(ContentBox->AddChild(Icon));
		if (IconSlot)
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		} // End if (slot valid)
	} // End if (icon check)
	
	// Reason: Create spacer between icon and text
	if (!IconSpacer)
	{
		IconSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("IconSpacer"));
		ContentBox->AddChild(IconSpacer);
	} // End if (spacer check)
	
	// Reason: Create caption text
	if (!Caption)
	{
		Caption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Caption"));
		Caption->SetText(FText::FromString(TEXT("Name")));
		
		FLinearColor TextColor = FLinearColor::Black;
		Caption->SetColorAndOpacity(FSlateColor(TextColor));
		
		UHorizontalBoxSlot* CaptionSlot = Cast<UHorizontalBoxSlot>(ContentBox->AddChild(Caption));
		if (CaptionSlot)
		{
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

void ULink::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	// Reason: Trigger hover state
	if (!bIsHovered)
	{
		bIsHovered = true;
		OnHovered();
	} // End if (hover transition)
}

void ULink::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
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

FReply ULink::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

FReply ULink::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

void ULink::SetCaption(const FText& NewText)
{
	if (Caption) { Caption->SetText(NewText); }
}

void ULink::SetIcon(UMaterialInterface* IconMaterial)
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
