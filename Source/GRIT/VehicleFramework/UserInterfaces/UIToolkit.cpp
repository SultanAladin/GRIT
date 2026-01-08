//UIToolkit.cpp
#include "UIToolkit.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Components/OverlaySlot.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../GameContext/SessionAdapter.h"
#include "Kismet/GameplayStatics.h"

//------------------------------------------------------------------------------
// Theme utilities
//------------------------------------------------------------------------------

FThemeConfig UUIToolkit::GetTheme(const UObject* WorldContextObject)
{
    if (!WorldContextObject) { return FThemeConfig(); }

    USessionAdapter* SessionAdapter = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(WorldContextObject));
    if (SessionAdapter)
    {
        return SessionAdapter->GetTheme();
    }

    return FThemeConfig();
}

void UUIToolkit::ApplyThemeToText(UTextBlock* TextBlock, const UObject* WorldContextObject, bool bUseHeadingStyle)
{
    if (!TextBlock || !WorldContextObject) { return; }

    FThemeConfig Theme = GetTheme(WorldContextObject);

    // Reason: Apply heading or body style based on parameter
    if (bUseHeadingStyle)
    {
        TextBlock->SetColorAndOpacity(FSlateColor(Theme.Palette.TextPrime));

        FSlateFontInfo FontInfo = TextBlock->GetFont();
        FontInfo.Size = Theme.Type.HeadingM.Size;
        TextBlock->SetFont(FontInfo);
    }
    else
    {
        TextBlock->SetColorAndOpacity(FSlateColor(Theme.Palette.TextPrime));

        FSlateFontInfo FontInfo = TextBlock->GetFont();
        FontInfo.Size = Theme.Type.BodyM.Size;
        TextBlock->SetFont(FontInfo);
    }
}

void UUIToolkit::ApplyThemeToBorder(UBorder* Border, const UObject* WorldContextObject)
{
    if (!Border || !WorldContextObject) { return; }

    FThemeConfig Theme = GetTheme(WorldContextObject);
    Border->SetBrushColor(Theme.Palette.SurfaceShift);
}

FTypeScale UUIToolkit::GetInterfaceStyle(const UObject* WorldContextObject)
{
    if (!WorldContextObject) { return FTypeScale(); }

    USessionAdapter* SessionAdapter = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(WorldContextObject));
    if (SessionAdapter)
    {
        return SessionAdapter->GetTypeScale();
    }

    return FTypeScale();
}

void UUIToolkit::ApplyTypographyStyle(UTextBlock* TextBlock, const FTypeSpec& TypeSpec)
{
    if (!TextBlock) { return; }

    // Apply color
    TextBlock->SetColorAndOpacity(FSlateColor(TypeSpec.Color));

    // Apply font and size
    FSlateFontInfo FontInfo = TextBlock->GetFont();
    if (TypeSpec.FontAsset)
    {
        FontInfo.FontObject = TypeSpec.FontAsset;
    }
    FontInfo.Size = TypeSpec.Size;
    
    // Apply font weight
    switch (TypeSpec.Weight)
    {
        case EFontWeight::Thin:    FontInfo.TypefaceFontName = FName("Thin");    break;
        case EFontWeight::Light:   FontInfo.TypefaceFontName = FName("Light");   break;
        case EFontWeight::Regular: FontInfo.TypefaceFontName = FName("Regular"); break;
        case EFontWeight::Medium:  FontInfo.TypefaceFontName = FName("Medium");  break;
        case EFontWeight::Bold:    FontInfo.TypefaceFontName = FName("Bold");    break;
        case EFontWeight::Black:   FontInfo.TypefaceFontName = FName("Black");   break;
    }
    
    TextBlock->SetFont(FontInfo);

    // Apply line height
    TextBlock->SetLineHeightPercentage(TypeSpec.LineHeight);
}

//------------------------------------------------------------------------------
// Layout utilities
//------------------------------------------------------------------------------

void UUIToolkit::SetWidgetVisibility(UWidget* Widget, bool bVisible, bool bAnimate)
{
    if (!Widget) { return; }

    // Reason: Simple visibility toggle (animation support can be added later)
    Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UUIToolkit::CenterInOverlay(UWidget* Widget)
{
    if (!Widget) { return; }

    UOverlaySlot* Slot = Cast<UOverlaySlot>(Widget->Slot);
    if (Slot)
    {
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetVerticalAlignment(VAlign_Center);
    }
}

//------------------------------------------------------------------------------
// Material utilities
//------------------------------------------------------------------------------

UMaterialInstanceDynamic* UUIToolkit::CreateDynamicMaterialForWidget(UImage* Image, UMaterialInterface* BaseMaterial)
{
    if (!Image || !BaseMaterial) { return nullptr; }

    UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, Image);
    if (DynamicMaterial)
    {
        Image->SetBrushFromMaterial(DynamicMaterial);
    }

    return DynamicMaterial;
}

void UUIToolkit::SetMaterialParameters(UMaterialInstanceDynamic* Material, const TArray<FName>& ParameterNames, FLinearColor Color)
{
    if (!Material) { return; }

    // Reason: Set all parameter names in array
    for (const FName& ParamName : ParameterNames)
    {
        Material->SetVectorParameterValue(ParamName, Color);
    }
}

//------------------------------------------------------------------------------
// Animation utilities
//------------------------------------------------------------------------------

float UUIToolkit::EvalFlowCurve(EFlowCurve Curve, float NormalizedTime)
{
    // Reason: Clamp input to valid range [0, 1]
    float T = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);

    switch (Curve)
    {
        case EFlowCurve::Linear:        return T;
        case EFlowCurve::QuadIn:        return EaseQuadIn(T);
        case EFlowCurve::QuadOut:       return EaseQuadOut(T);
        case EFlowCurve::QuadInOut:     return EaseQuadInOut(T);
        case EFlowCurve::CubicIn:       return EaseCubicIn(T);
        case EFlowCurve::CubicOut:      return EaseCubicOut(T);
        case EFlowCurve::CubicInOut:    return EaseCubicInOut(T);
        case EFlowCurve::QuartIn:       return EaseQuartIn(T);
        case EFlowCurve::QuartOut:      return EaseQuartOut(T);
        case EFlowCurve::QuartInOut:    return EaseQuartInOut(T);
        case EFlowCurve::QuintIn:       return EaseQuintIn(T);
        case EFlowCurve::QuintOut:      return EaseQuintOut(T);
        case EFlowCurve::QuintInOut:    return EaseQuintInOut(T);
        case EFlowCurve::ExpoIn:        return EaseExpoIn(T);
        case EFlowCurve::ExpoOut:       return EaseExpoOut(T);
        case EFlowCurve::ExpoInOut:     return EaseExpoInOut(T);
        case EFlowCurve::CircIn:        return EaseCircIn(T);
        case EFlowCurve::CircOut:       return EaseCircOut(T);
        case EFlowCurve::CircInOut:     return EaseCircInOut(T);
        case EFlowCurve::BackIn:        return EaseBackIn(T);
        case EFlowCurve::BackOut:       return EaseBackOut(T);
        case EFlowCurve::BackInOut:     return EaseBackInOut(T);
        case EFlowCurve::BounceIn:      return EaseBounceIn(T);
        case EFlowCurve::BounceOut:     return EaseBounceOut(T);
        case EFlowCurve::BounceInOut:   return EaseBounceInOut(T);
        case EFlowCurve::ElasticIn:     return EaseElasticIn(T);
        case EFlowCurve::ElasticOut:    return EaseElasticOut(T);
        case EFlowCurve::ElasticInOut:  return EaseElasticInOut(T);
        case EFlowCurve::SineIn:        return EaseSineIn(T);
        case EFlowCurve::SineOut:       return EaseSineOut(T);
        case EFlowCurve::SineInOut:     return EaseSineInOut(T);
        default:                        return T;
    } // End switch (curve type)
}

//------------------------------------------------------------------------------
// Easing implementation (Complex curves)
//------------------------------------------------------------------------------

float UUIToolkit::EaseBounceOut(float T)
{
    const float N1 = 7.5625f;
    const float D1 = 2.75f;

    if (T < 1.0f / D1)
    {
        return N1 * T * T;
    }
    else if (T < 2.0f / D1)
    {
        T -= 1.5f / D1;
        return N1 * T * T + 0.75f;
    }
    else if (T < 2.5f / D1)
    {
        T -= 2.25f / D1;
        return N1 * T * T + 0.9375f;
    }
    else
    {
        T -= 2.625f / D1;
        return N1 * T * T + 0.984375f;
    } // End if (bounce phase)
}

float UUIToolkit::EaseElasticOut(float T)
{
    if (T == 0.0f) { return 0.0f; }
    if (T == 1.0f) { return 1.0f; }

    const float C4 = (2.0f * PI) / 3.0f; // [rad]
    return FMath::Pow(2.0f, -10.0f * T) * FMath::Sin((T * 10.0f - 0.75f) * C4) + 1.0f;
}
