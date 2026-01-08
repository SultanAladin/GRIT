// Copyright 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CarouselEntry.generated.h"

/*====================================================================================================================================
                                                         CAROUSEL ENTRY BASE
======================================================================================================================================*/

UCLASS()
class GRIT_API UCarouselEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget initialization */
	virtual void NativeConstruct() override;

	/** Set entry dimensions to match viewport */
	void SetEntrySize(float Width, float Height);

	/** Get current entry width */
	UFUNCTION(BlueprintCallable, Category = "Carousel")
	float GetEntryWidth() const { return EntryWidth; }

	/** Get current entry height */
	UFUNCTION(BlueprintCallable, Category = "Carousel")
	float GetEntryHeight() const { return EntryHeight; }

protected:
	float EntryWidth;                       // [px] - Entry width
	float EntryHeight;                      // [px] - Entry height

	TOptional<FVector2D> DesiredSize;       // [px, px] - Cached desired size

	/** Called when entry size is updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "Carousel")
	void OnSizeChanged(float Width, float Height);
};