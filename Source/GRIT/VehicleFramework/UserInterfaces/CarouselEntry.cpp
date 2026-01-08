// Copyright 2025. All Rights Reserved.

#include "CarouselEntry.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/BorderSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBoxSlot.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

void UCarouselEntry::NativeConstruct()
{
	Super::NativeConstruct();

	EntryWidth = 1920.0f;                   // [px] - Default width
	EntryHeight = 1080.0f;                  // [px] - Default height
	DesiredSize = TOptional<FVector2D>();   // Not set initially
}

/*====================================================================================================================================
                                                         SIZE MANAGEMENT
======================================================================================================================================*/

void UCarouselEntry::SetEntrySize(float Width, float Height)
{
	EntryWidth = Width;                     // [px]
	EntryHeight = Height;                   // [px]
	DesiredSize = FVector2D(Width, Height); // [px, px]

	// Reason: Get root widget dynamically
	UWidget* RootWidget = GetRootWidget();
	
	if (RootWidget && RootWidget->Slot)
	{
		// Reason: Try different slot types
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RootWidget->Slot))
		{
			CanvasSlot->SetSize(FVector2D(EntryWidth, EntryHeight)); // [px, px]
		}
		else if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(RootWidget->Slot))
		{
			BorderSlot->SetPadding(FMargin(0.0f));
		}
		else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(RootWidget->Slot))
		{
			OverlaySlot->SetPadding(FMargin(0.0f));
		}
		else if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(RootWidget->Slot))
		{
			SizeBoxSlot->SetPadding(FMargin(0.0f));
		} // End if (slot type checks)
	} // End if (root widget exists)

	// Reason: Notify Blueprint of size change
	OnSizeChanged(Width, Height);
}