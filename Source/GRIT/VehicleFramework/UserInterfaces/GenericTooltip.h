// GenericTooltip.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GenericTooltip.generated.h"

//------------------------------------------------------------------------------
// Generic tooltip widget with title, body, and optional image
//------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UGenericTooltip : public UUserWidget
{
    GENERATED_BODY()

public:
    //------------------------------------------------------------------------------
    // Content setters
    //------------------------------------------------------------------------------

    /** Set title text */
    UFUNCTION(BlueprintCallable, Category = "Tooltip|Content")
    void SetTitle(const FText& InTitle);

    /** Set body text */
    UFUNCTION(BlueprintCallable, Category = "Tooltip|Content")
    void SetBody(const FText& InBody);

    /** Set icon texture */
    UFUNCTION(BlueprintCallable, Category = "Tooltip|Content")
    void SetIcon(UTexture2D* InTexture);

    /** Set all content at once */
    UFUNCTION(BlueprintCallable, Category = "Tooltip|Content")
    void SetContent(const FText& InTitle, const FText& InBody, UTexture2D* InTexture = nullptr);

protected:
    virtual void NativeConstruct() override;

    //------------------------------------------------------------------------------
    // Widget bindings (bind in Blueprint)
    //------------------------------------------------------------------------------

    /** Title text block */
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* TitleText;

    /** Body text block */
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* BodyText;

    /** Icon image */
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* IconImage;

    //------------------------------------------------------------------------------
    // Default content (set in Blueprint defaults or via code)
    //------------------------------------------------------------------------------

    /** Default title text */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip|Content")
    FText DefaultTitle;

    /** Default body text */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip|Content", meta = (MultiLine = true))
    FText DefaultBody;

    /** Default icon texture */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip|Content")
    UTexture2D* DefaultIcon;

    //------------------------------------------------------------------------------
    // Icon material configuration
    //------------------------------------------------------------------------------

    /** Base material for icon */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip|Icon Material")
    UMaterialInterface* IconBaseMaterial;

    /** Icon texture */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip|Icon Material")
    UTexture2D* IconTexture;

    /** Material parameter names for icon */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip|Icon Material")
    TArray<FName> IconParameterNames;

    //------------------------------------------------------------------------------
    // Layout
    //------------------------------------------------------------------------------

    /** Maximum width before text wraps [px] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip|Layout")
    float MaxTextWidth = 250.0f;

    /** Enable text auto-wrap */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip|Layout")
    bool bEnableTextWrap = true;

private:
    /** Dynamic material instance for icon */
    UPROPERTY()
    UMaterialInstanceDynamic* IconDynamicMaterial;

    void ApplyContent();
    void ApplyStyling();
    void ApplyTextWrap(class UTextBlock* TextBlock);
    void InitializeIconMaterial();
};
