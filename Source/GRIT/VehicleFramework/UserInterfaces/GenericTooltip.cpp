// GenericTooltip.cpp
#include "GenericTooltip.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "SessionAdapter.h"
#include "Kismet/GameplayStatics.h"

void UGenericTooltip::NativeConstruct()
{
    Super::NativeConstruct();
    InitializeIconMaterial();
    ApplyContent();
    ApplyStyling();
}

//------------------------------------------------------------------------------
// Content setters
//------------------------------------------------------------------------------

void UGenericTooltip::SetTitle(const FText& InTitle)
{
    if (TitleText)
    {
        TitleText->SetText(InTitle);
        TitleText->SetVisibility(InTitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
}

void UGenericTooltip::SetBody(const FText& InBody)
{
    if (BodyText)
    {
        BodyText->SetText(InBody);
        BodyText->SetVisibility(InBody.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
}

void UGenericTooltip::SetIcon(UTexture2D* InTexture)
{
    if (IconImage)
    {
        if (InTexture)
        {
            IconImage->SetBrushFromTexture(InTexture);
            IconImage->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            IconImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UGenericTooltip::SetContent(const FText& InTitle, const FText& InBody, UTexture2D* InTexture)
{
    SetTitle(InTitle);
    SetBody(InBody);
    SetIcon(InTexture);
}

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

void UGenericTooltip::ApplyContent()
{
    SetTitle(DefaultTitle);
    SetBody(DefaultBody);
    SetIcon(DefaultIcon);
}

void UGenericTooltip::ApplyStyling()
{
    // Reason: Get theme from SessionAdapter
    USessionAdapter* SessionAdapter = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(this));
    if (!SessionAdapter) { return; }

    // TODO: Apply theme styling - will be set via editor

    if (TitleText)
    {
        // TODO: Set title text styling via editor
        ApplyTextWrap(TitleText);
    }

    if (BodyText)
    {
        // TODO: Set body text styling via editor
        ApplyTextWrap(BodyText);
    }
}

void UGenericTooltip::ApplyTextWrap(UTextBlock* TextBlock)
{
    if (!TextBlock || !bEnableTextWrap) { return; }

    // Enable auto-wrapping
    TextBlock->SetAutoWrapText(true);

    // Set wrap width - WrapTextAt must be > 0 for wrapping to work
    TextBlock->SetWrapTextAt(MaxTextWidth);

    // Also set min desired width to help layout
    TextBlock->SetMinDesiredWidth(MaxTextWidth);
}

void UGenericTooltip::InitializeIconMaterial()
{
    // Reason: Create dynamic material for icon if material is specified
    if (IconImage && IconBaseMaterial)
    {
        IconDynamicMaterial = UMaterialInstanceDynamic::Create(IconBaseMaterial, this);
        if (IconDynamicMaterial)
        {
            // Reason: Set all parameter names in array
            for (const FName& ParamName : IconParameterNames)
            {
                // Note: You can set parameters here if needed
                // IconDynamicMaterial->SetVectorParameterValue(ParamName, SomeColor);
            }
            IconImage->SetBrushFromMaterial(IconDynamicMaterial);
        }
    }
    else if (IconImage && IconTexture)
    {
        // Reason: Fallback to texture if no material specified
        IconImage->SetBrushFromTexture(IconTexture);
    }
}
