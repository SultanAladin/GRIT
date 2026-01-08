// Copyright 2025. All Rights Reserved.

#include "GenericCarousel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

void UGenericCarousel::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	UE_LOG(LogTemp, Warning, TEXT("[GenericCarousel] NativePreConstruct - AnimationDuration=%.2fs, Curve=%d, Orientation=%s"), 
	       AnimationDuration, (int32)AnimationCurve, bVerticalOrientation ? TEXT("Vertical") : TEXT("Horizontal"));
}

void UGenericCarousel::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize state variables
	ActivePanelIndex = 0;                   // Start at first panel
	ViewportWidth = 1920.0f;                // [px] - Default fallback
	ViewportHeight = 1080.0f;               // [px] - Default fallback
	CurrentTranslation = 0.0f;              // [px]
	TargetTranslation = 0.0f;               // [px]
	AnimationStartPos = 0.0f;               // [px]
	AnimationElapsed = 0.0f;                // [s]
	bIsAnimating = false;                   // [-]

	UE_LOG(LogTemp, Warning, TEXT("[GenericCarousel] NativeConstruct - Using AnimationDuration=%.2fs, Orientation=%s"), 
	       AnimationDuration, bVerticalOrientation ? TEXT("Vertical") : TEXT("Horizontal"));

	// Reason: Enable clipping on both containers to show only visible panel
//	if (CarouselContainer_H) { CarouselContainer_H->SetClipping(EWidgetClipping::ClipToBounds); }
//	if (CarouselContainer_V) { CarouselContainer_V->SetClipping(EWidgetClipping::ClipToBounds); }

	BindNavBar();
	PopulateCarousel();
}

void UGenericCarousel::BindNavBar()
{
	if (NavBar) { NavBar->OnSelectionChanged.AddDynamic(this, &UGenericCarousel::OnNavSelectionChanged); } // Bind navbar selection
}

//------------------------------------------------------------------------------
// ENTRY POPULATION
//------------------------------------------------------------------------------

void UGenericCarousel::PopulateCarousel()
{
	// Reason: Get active container based on orientation
	UPanelWidget* ActiveContainer = GetActiveContainer();
	
	if (!ActiveContainer) { return; } // Exit if invalid

	// Reason: Clear existing entries
	ActiveContainer->ClearChildren();
	CarouselEntries.Empty();
	EntrySizeBoxes.Empty();

	// Reason: Use EntryClasses array if populated, otherwise use NumPanels count
	int32 PanelCount = EntryClasses.Num() > 0 ? EntryClasses.Num() : NumPanels;

	// Reason: Create and add entries to container
	for (int32 i = 0; i < PanelCount; ++i)
	{
		UUserWidget* NewEntry = nullptr;

		// Reason: Create from EntryClasses if available, otherwise skip
		if (EntryClasses.IsValidIndex(i) && EntryClasses[i])
		{
			NewEntry = CreateWidget<UUserWidget>(this, EntryClasses[i]);
		} // End if (class valid)

		if (NewEntry)
		{
			// Reason: Wrap entry in SizeBox for fixed dimensions
			USizeBox* SizeBoxWrapper = NewObject<USizeBox>(this);
			
			// Reason: Entries fill entire viewport - clipping handles overflow
			SizeBoxWrapper->SetWidthOverride(ViewportWidth); // [px]
			SizeBoxWrapper->SetHeightOverride(ViewportHeight); // [px]
			SizeBoxWrapper->AddChild(NewEntry);

			// Reason: Add to appropriate container type based on orientation
			if (bVerticalOrientation)
			{
				UVerticalBoxSlot* VBoxSlot = CarouselContainer_V->AddChildToVerticalBox(SizeBoxWrapper);
				if (VBoxSlot)
				{
					VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic)); // Use widget's desired size
				} // End if (slot created)
			}
			else
			{
				UHorizontalBoxSlot* HBoxSlot = CarouselContainer_H->AddChildToHorizontalBox(SizeBoxWrapper);
				if (HBoxSlot)
				{
					HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic)); // Use widget's desired size
				} // End if (slot created)
			} // End if (orientation check)

			CarouselEntries.Add(NewEntry);
			EntrySizeBoxes.Add(SizeBoxWrapper);
		} // End if (entry created)
	} // End for (panel creation)

	TotalPanels = CarouselEntries.Num();
	
	UE_LOG(LogTemp, Warning, TEXT("[GenericCarousel] PopulateCarousel - Created %d panels in %s container"), 
	       TotalPanels, bVerticalOrientation ? TEXT("Vertical") : TEXT("Horizontal"));
}

/*====================================================================================================================================
                                                         FRAME UPDATE
======================================================================================================================================*/

void UGenericCarousel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Reason: Get accurate viewport dimensions every frame
	RefreshViewportSize(MyGeometry);

	// Reason: Update carousel position during animation
	if (bIsAnimating)
	{
		RefreshCarouselPosition(InDeltaTime);
	}
}

//------------------------------------------------------------------------------
// VIEWPORT TRACKING
//------------------------------------------------------------------------------

void UGenericCarousel::RefreshViewportSize(const FGeometry& Geometry)
{
	// Reason: Extract viewport dimensions from widget geometry
	FVector2D LocalSize = Geometry.GetLocalSize();
	float NewWidth = LocalSize.X;           // [px]
	float NewHeight = LocalSize.Y;          // [px]

	// Reason: Only update if dimensions changed
	if (!FMath::IsNearlyEqual(ViewportWidth, NewWidth, 1.0f) || !FMath::IsNearlyEqual(ViewportHeight, NewHeight, 1.0f))
	{
		ViewportWidth = NewWidth;
		ViewportHeight = NewHeight;
		SyncEntryDimensions();
		SnapToPanel(ActivePanelIndex);      // Recalculate translation
	} // End if (viewport changed)
}

void UGenericCarousel::SyncEntryDimensions()
{
	// Reason: Update SizeBox dimensions to match viewport
	for (USizeBox* SizeBoxWrapper : EntrySizeBoxes)
	{
		if (SizeBoxWrapper)
		{
			// Reason: Entries fill entire viewport - clipping handles overflow
			SizeBoxWrapper->SetWidthOverride(ViewportWidth); // [px]
			SizeBoxWrapper->SetHeightOverride(ViewportHeight); // [px]
		} // End if (sizebox valid)
	} // End for (entry sync)
}

//------------------------------------------------------------------------------
// CAROUSEL ANIMATION
//------------------------------------------------------------------------------

void UGenericCarousel::RefreshCarouselPosition(float DeltaTime)
{
	AnimationElapsed += DeltaTime;
	
	// Calculate normalized time [0-1]
	float Alpha = FMath::Clamp(AnimationElapsed / AnimationDuration, 0.0f, 1.0f);
	
	// Apply easing curve using UIToolkit
	float EasedAlpha = UUIToolkit::EvalFlowCurve(AnimationCurve, Alpha);
	
	// Interpolate position
	CurrentTranslation = FMath::Lerp(AnimationStartPos, TargetTranslation, EasedAlpha);
	
	// Reason: Apply RenderTransform on correct axis based on orientation
	UPanelWidget* ActiveContainer = GetActiveContainer();
	if (ActiveContainer)
	{
		FWidgetTransform Transform;
		// Reason: Vertical scrolls Y-axis, horizontal scrolls X-axis
		Transform.Translation = bVerticalOrientation ? FVector2D(0.0f, CurrentTranslation) : FVector2D(CurrentTranslation, 0.0f);
		ActiveContainer->SetRenderTransform(Transform);
	}
	
	// Stop animation when complete
	if (AnimationElapsed >= AnimationDuration)
	{
		bIsAnimating = false;
		CurrentTranslation = TargetTranslation; // Ensure exact final position
		
		UE_LOG(LogTemp, Log, TEXT("[GenericCarousel] Animation complete - Final position: %.1f"), CurrentTranslation);
	}
}

/*====================================================================================================================================
                                                         PANEL NAVIGATION
======================================================================================================================================*/

void UGenericCarousel::SnapToPanel(int32 TargetIndex)
{
	// Reason: Clamp index to valid range
	ActivePanelIndex = FMath::Clamp(TargetIndex, 0, TotalPanels - 1);

	// Reason: Panel size equals full viewport dimension in scroll direction
	float PanelSize = bVerticalOrientation ? ViewportHeight : ViewportWidth; // [px]
	float NewTargetTranslation = -PanelSize * static_cast<float>(ActivePanelIndex); // [px]
	
	// Reason: Start animation if position changed
	if (!FMath::IsNearlyEqual(CurrentTranslation, NewTargetTranslation, 1.0f))
	{
		AnimationStartPos = CurrentTranslation;
		TargetTranslation = NewTargetTranslation;
		AnimationElapsed = 0.0f;
		bIsAnimating = true;
		
		UE_LOG(LogTemp, Warning, TEXT("[GenericCarousel] Starting animation: %.1f -> %.1f (Duration=%.2fs, Curve=%d, Axis=%s)"), 
		       AnimationStartPos, TargetTranslation, AnimationDuration, (int32)AnimationCurve, 
		       bVerticalOrientation ? TEXT("Y") : TEXT("X"));
	}
	else
	{
		// Already at target position
		TargetTranslation = NewTargetTranslation;
		bIsAnimating = false;
	}
}

//------------------------------------------------------------------------------
// NAVBAR CALLBACK
//------------------------------------------------------------------------------

void UGenericCarousel::OnNavSelectionChanged(int32 SelectedIndex, const FString& SelectedData)
{
	SnapToPanel(SelectedIndex);
}

//------------------------------------------------------------------------------
// HELPER FUNCTIONS
//------------------------------------------------------------------------------

UPanelWidget* UGenericCarousel::GetActiveContainer() const
{
	// Reason: Return appropriate container based on orientation
	return bVerticalOrientation ? Cast<UPanelWidget>(CarouselContainer_V) : Cast<UPanelWidget>(CarouselContainer_H);
}