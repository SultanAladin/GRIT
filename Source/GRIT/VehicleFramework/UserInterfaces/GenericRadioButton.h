#pragma once

#include "CoreMinimal.h"
#include "GenericButton.h"
#include "GenericRadioButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadioToggled, bool, bSelected);

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UGenericRadioButton : public UGenericButton
{
    GENERATED_BODY()

public:
    UGenericRadioButton(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Widget bindings (additional to GenericButton)
    UPROPERTY(meta = (BindWidget))
    class UBorder* RingBorder;

    UPROPERTY(meta = (BindWidget))
    class UImage* IndicatorImage;

    // Radio button settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio")
    bool bStartSelected = false;

    //------------------------------------------------------------------------------
    // Content Configuration (inherited from GenericButton but exposed for Blueprint)
    //------------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Content", meta = (AllowPrivateAccess = "true"))
    FText RadioLabelText = FText::FromString("Radio Option");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Content", meta = (AllowPrivateAccess = "true"))
    UTexture2D* RadioIconTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Ring")
    FLinearColor RingColorIdle = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Ring")
    FLinearColor RingColorHovered = FLinearColor(0.231f, 0.510f, 0.965f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Ring")
    FLinearColor RingColorSelected = FLinearColor(0.231f, 0.510f, 0.965f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Ring")
    float RingBorderWidth = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Indicator")
    FLinearColor IndicatorColor = FLinearColor(0.231f, 0.510f, 0.965f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Indicator")
    float IndicatorScaleIdle = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Indicator")
    float IndicatorScaleHovered = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Indicator")
    float IndicatorScaleSelected = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio|Animation")
    float IndicatorAnimDuration = 0.3f;

private:
    bool bIsSelected = false;
    bool bRadioHovered = false;

    float CurrentIndicatorScale = 0.0f;
    float StartIndicatorScale = 0.0f;
    float TargetIndicatorScale = 0.0f;
    float IndicatorAnimTime = 0.0f;

    FLinearColor CurrentRingColor;
    FLinearColor StartRingColor;
    FLinearColor TargetRingColor;
    float RingAnimTime = 0.0f;

    UFUNCTION()
    void OnRadioClicked();

    void UpdateRadioVisuals();
    void ApplyRingStyle();
    void ApplyIndicatorStyle();
    void ApplyRadioContent();
    float GetTargetIndicatorScale() const;
    FLinearColor GetTargetRingColor() const;

public:
    UPROPERTY(BlueprintAssignable, Category = "Radio Events")
    FOnRadioToggled OnRadioToggled;

    UFUNCTION(BlueprintImplementableEvent, Category = "Radio Events")
    void OnRadioToggledBP(bool bNewSelected);

    UFUNCTION(BlueprintCallable, Category = "Radio")
    void SetSelected(bool bNewSelected);

    UFUNCTION(BlueprintCallable, Category = "Radio")
    bool IsSelected() const { return bIsSelected; }

    UFUNCTION(BlueprintCallable, Category = "Radio")
    void Toggle();

    //------------------------------------------------------------------------------
    // Content API (Blueprint-accessible)
    //------------------------------------------------------------------------------
    
    /** Set radio button label text at runtime */
    UFUNCTION(BlueprintCallable, Category = "Radio|Content")
    void SetRadioLabelText(const FText& NewText);

    /** Set radio button icon at runtime */
    UFUNCTION(BlueprintCallable, Category = "Radio|Content")
    void SetRadioIcon(UTexture2D* NewTexture);

    /** Get current radio button label text */
    UFUNCTION(BlueprintPure, Category = "Radio|Content")
    FText GetRadioLabelText() const { return RadioLabelText; }

    /** Get current radio button icon texture */
    UFUNCTION(BlueprintPure, Category = "Radio|Content")
    UTexture2D* GetRadioIcon() const { return RadioIconTexture; }
};