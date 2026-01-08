// Glyph.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Glyph.generated.h"

//------------------------------------------------------------------------------
//                                   glyph button
//------------------------------------------------------------------------------

UCLASS()
class UGlyph : public UUserWidget
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

	/** Set button icon material */
	UFUNCTION(BlueprintCallable, Category = "Glyph") void SetIcon(UMaterialInterface* IconMaterial);

protected:
	/** Button hover callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glyph") void OnHovered();
	
	/** Button unhover callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glyph") void OnUnhovered();
	
	/** Button press callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glyph") void OnPressed();
	
	/** Button release callback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glyph") void OnReleased();

private:
	//------------------------------------------------------------------------------
	// widget references
	//------------------------------------------------------------------------------
	
	UPROPERTY(meta = (BindWidget)) UBorder* RootBorder;
	UPROPERTY(meta = (BindWidget)) UImage* Icon;

	//------------------------------------------------------------------------------
	// state tracking
	//------------------------------------------------------------------------------
	
	bool bIsHovered;                // Hover state
	bool bIsPressed;                // Press state
};
