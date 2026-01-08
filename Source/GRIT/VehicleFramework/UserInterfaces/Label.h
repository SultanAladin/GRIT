// Label.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Label.generated.h"

//------------------------------------------------------------------------------
//                                   label button
//------------------------------------------------------------------------------

UCLASS()
class ULabel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget construction */
	virtual void NativeConstruct() override;
	
	/** Mouse hover enter */
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	/** Mouse hover exit */
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
	/** Mouse button press */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	/** Mouse button release */
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Set button caption text */
	UFUNCTION(BlueprintCallable, Category = "Label") void SetCaption(const FText& NewText);

protected:
	/** Button hover callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Label") void OnHovered();
	
	/** Button unhover callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Label") void OnUnhovered();
	
	/** Button press callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Label") void OnPressed();
	
	/** Button release callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Label") void OnReleased();

private:
	//------------------------------------------------------------------------------
	// widget references
	//------------------------------------------------------------------------------
	
	UPROPERTY(meta = (BindWidget)) UBorder* RootBorder;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Caption;

	//------------------------------------------------------------------------------
	// state tracking
	//------------------------------------------------------------------------------
	
	bool bIsHovered;                // Hover state
	bool bIsPressed;                // Press state
};
