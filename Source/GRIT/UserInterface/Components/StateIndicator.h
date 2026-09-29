#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "StateIndicator.generated.h"

/*====================================================================================================================================
                                                         STATE INDICATOR
======================================================================================================================================*/

/** State indicator configuration */
USTRUCT(BlueprintType)
struct GRIT_API FStateIndicatorConfig
{
    GENERATED_BODY()

    // Border styling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    FLinearColor BackgroundColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    FLinearColor BorderColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    float BorderWidth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    float CornerRadius;

    // Image styling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Image")
    FLinearColor ImageTintColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Image")
    FVector2D ImageSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Image")
    UTexture2D* ImageTexture;

    FStateIndicatorConfig()
        : BackgroundColor(FLinearColor(0.041667f, 0.041667f, 0.041667f, 1.0f))
        , BorderColor(FLinearColor(0.708333f, 0.708333f, 0.708333f, 1.0f))
        , BorderWidth(13.473791f)
        , CornerRadius(1.865950f)
        , ImageTintColor(FLinearColor(0.604334f, 1.0f, 0.0f, 1.0f))
        , ImageSize(FVector2D(25.0f, 25.0f))
        , ImageTexture(nullptr)
    {}
};

/** State indicator widget matching the UMG structure */
UCLASS(BlueprintType, Blueprintable)
class GRIT_API UStateIndicator : public UUserWidget
{
    GENERATED_BODY()

public:
    UStateIndicator(const FObjectInitializer& ObjectInitializer);

    //------------------------------------------------------------------------------
    // Widget Bindings
    //------------------------------------------------------------------------------
    
    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* Border_17;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* Image_24;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Indicator Config")
    FStateIndicatorConfig IndicatorConfig;

    //------------------------------------------------------------------------------
    // Events
    //------------------------------------------------------------------------------

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStateIndicatorClicked, UStateIndicator*, Indicator);
    UPROPERTY(BlueprintAssignable, Category = "Indicator Events")
    FOnStateIndicatorClicked OnStateIndicatorClicked;

    //------------------------------------------------------------------------------
    // Public Interface
    //------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "State Indicator")
    void SetImageTexture(UTexture2D* NewTexture);

    UFUNCTION(BlueprintCallable, Category = "State Indicator")
    void SetImageTintColor(const FLinearColor& NewColor);

    UFUNCTION(BlueprintCallable, Category = "State Indicator")
    void SetBackgroundColor(const FLinearColor& NewColor);

    UFUNCTION(BlueprintCallable, Category = "State Indicator")
    void SetBorderColor(const FLinearColor& NewColor);

    UFUNCTION(BlueprintCallable, Category = "State Indicator")
    void ApplyStateIndicatorConfig(const FStateIndicatorConfig& NewConfig);

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
    /** Update the visual appearance */
    void UpdateVisualState();

    /** Handle indicator click */
    void HandleIndicatorClick();

    bool bIsPressed;
    bool bIsHovered;
};