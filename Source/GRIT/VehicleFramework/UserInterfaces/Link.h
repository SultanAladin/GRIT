// Link.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Link.generated.h"

//------------------------------------------------------------------------------
//                                    link button
//------------------------------------------------------------------------------

UCLASS()
class ULink : public UUserWidget
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
	UFUNCTION(BlueprintCallable, Category = "Link") void SetCaption(const FText& NewText);
	
	/** Set button icon material */
	UFUNCTION(BlueprintCallable, Category = "Link") void SetIcon(UMaterialInterface* IconMaterial);

protected:
	/** Button hover callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Link") void OnHovered();
	
	/** Button unhover callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Link") void OnUnhovered();
	
	/** Button press callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Link") void OnPressed();
	
	/** Button release callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Link") void OnReleased();

private:
	//------------------------------------------------------------------------------
	// widget references
	//------------------------------------------------------------------------------
	
	UPROPERTY(meta = (BindWidget)) UBorder* RootBorder;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ContentBox;
	UPROPERTY(meta = (BindWidget)) UImage* Icon;
	UPROPERTY(meta = (BindWidget)) USpacer* IconSpacer;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Caption;

	//------------------------------------------------------------------------------
	// state tracking
	//------------------------------------------------------------------------------
	
	bool bIsHovered;                // Hover state
	bool bIsPressed;                // Press state
};
