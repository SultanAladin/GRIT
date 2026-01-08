// Copyright 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/SizeBox.h"
#include "NavBarBase.h"
#include "UIToolkit.h"
#include "GenericCarousel.generated.h"

/*====================================================================================================================================
                                                         GENERIC CAROUSEL SYSTEM
======================================================================================================================================*/

UCLASS()
class GRIT_API UGenericCarousel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget initialization */
	virtual void NativeConstruct() override;

	/** Frame update with viewport tracking */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Snap to target panel index */
	void SnapToPanel(int32 TargetIndex);

	/** Populate carousel with entry instances */
	void PopulateCarousel();

protected:
	virtual void NativePreConstruct() override;

	//------------------------------------------------------------------------------
	// Widget References
	//------------------------------------------------------------------------------
	
	UPROPERTY(meta = (BindWidgetOptional))
	UHorizontalBox* CarouselContainer_H;    // Horizontal carousel container

	UPROPERTY(meta = (BindWidgetOptional))
	UVerticalBox* CarouselContainer_V;      // Vertical carousel container

	UPROPERTY(meta = (BindWidget))
	UNavBarBase* NavBar;                    // Navigation bar for panel selection

	//------------------------------------------------------------------------------
	// Entry Management
	//------------------------------------------------------------------------------
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carousel")
	TArray<TSubclassOf<UUserWidget>> EntryClasses; // Array of widget classes for each panel

	TArray<UUserWidget*> CarouselEntries;   // All carousel entry instances
	TArray<USizeBox*> EntrySizeBoxes;       // SizeBox wrappers for fixed dimensions

	//------------------------------------------------------------------------------
	// State Variables
	//------------------------------------------------------------------------------
	
	int32 ActivePanelIndex;                 // Current panel index
	int32 TotalPanels;                      // Total number of panels
	
	float ViewportWidth;                    // [px] - Current viewport width
	float ViewportHeight;                   // [px] - Current viewport height
	
	float CurrentTranslation;               // [px] - Current translation (X or Y based on orientation)
	float TargetTranslation;                // [px] - Target translation (X or Y based on orientation)

	// Animation state
	float AnimationStartPos;                // [px] - Animation start position
	float AnimationElapsed;                 // [s] - Animation elapsed time
	bool bIsAnimating;                      // [-] - Animation in progress

	//------------------------------------------------------------------------------
	// Configuration
	//------------------------------------------------------------------------------
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carousel|Layout")
	bool bVerticalOrientation = false;      // [-] - Use vertical layout instead of horizontal

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carousel|Animation")
	float AnimationDuration = 0.3f;         // [s] - Carousel slide duration

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carousel|Animation")
	EFlowCurve AnimationCurve = EFlowCurve::QuadOut; // Animation easing curve

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carousel")
	int32 NumPanels = 3;                    // Total panels in carousel

	//------------------------------------------------------------------------------
	// Callbacks
	//------------------------------------------------------------------------------
	
	/** NavBar selection changed handler */
	UFUNCTION()
	void OnNavSelectionChanged(int32 SelectedIndex, const FString& SelectedData);

	//------------------------------------------------------------------------------
	// Internal Logic
	//------------------------------------------------------------------------------
	
	/** Update viewport dimensions from geometry */
	void RefreshViewportSize(const FGeometry& Geometry);

	/** Update carousel RenderTransform based on interpolation */
	void RefreshCarouselPosition(float DeltaTime);

	/** Propagate viewport size to all entries */
	void SyncEntryDimensions();

	/** Bind navbar selection delegate */
	void BindNavBar();

	/** Get active container based on orientation */
	UPanelWidget* GetActiveContainer() const;
};