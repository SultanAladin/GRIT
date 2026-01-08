// ColorItemWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "ColorDataObject.h"
#include "ColorItemWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorItemSelected, int32, ItemIndex);

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UColorItemWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    UColorItemWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // IUserObjectListEntry interface
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    // Widget Components
    UPROPERTY(meta = (BindWidget))
    class UButton* ItemButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ColorNameText;

    UPROPERTY(meta = (BindWidget))
    class UImage* ColorPreviewImage;

    // Animation Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float HoverScale = 1.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float AnimationDuration = 0.15f;

public:
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnColorItemSelected OnColorItemSelected;

    // Public Functions
    UFUNCTION(BlueprintCallable, Category = "Color Item")
    void SetColorData(const FString& ColorName, const FLinearColor& ColorValue, int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Color Item")
    void SetSelected(bool bNewSelected);

    UFUNCTION(BlueprintCallable, Category = "Color Item")
    bool GetIsSelected() const { return bIsSelected; }

    UFUNCTION(BlueprintCallable, Category = "Color Item")
    void SetStyling(const FLinearColor& NormalColor, const FLinearColor& HoveredColor, 
                   const FLinearColor& SelectedColor, const FLinearColor& SelectedBorder, float BorderRadius);

    // Get the data object
    UFUNCTION(BlueprintCallable, Category = "Color Item")
    UColorDataObject* GetColorDataObject() const { return ColorDataObject; }

    // Get the current color value
    UFUNCTION(BlueprintCallable, Category = "Color Item")
    FLinearColor GetCurrentColorValue() const { return CurrentColorValue; }

private:
    bool bIsSelected = false;
    bool bIsHovered = false;
    int32 ItemIndex = -1;
    
    // Reference to the data object
    UPROPERTY()
    UColorDataObject* ColorDataObject = nullptr;
    
    // Store the current color value for easy access
    FLinearColor CurrentColorValue = FLinearColor::White;
    
    // Animation variables
    float CurrentAnimationTime = 0.0f;
    FVector2D StartScale;
    FVector2D TargetScale;
    FVector2D OriginalScale;

    // Styling
    FLinearColor ItemNormalColor;
    FLinearColor ItemHoveredColor;
    FLinearColor ItemSelectedColor;
    FLinearColor SelectedBorderColor;
    float ItemBorderRadius;

    UFUNCTION()
    void OnButtonClicked();

    UFUNCTION()
    void OnButtonHovered();

    UFUNCTION()
    void OnButtonUnhovered();

    void ApplyButtonStyling();
    void StartScaleAnimation(const FVector2D& NewTargetScale);
    void UpdateColorPreviewSelection();
};