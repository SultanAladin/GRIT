#include "ThemeUtil.h"
#include "SessionAdapter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

FThemeConfig UThemeUtil::FetchTheme(const UObject* WorldContext)
{
    if (!WorldContext) // Reason: Null context check
    {
        return FThemeConfig();
    } // End if (WorldContext check)

    USessionAdapter* Session = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(WorldContext));
    if (Session) // Reason: Valid session check
    {
        return Session->GetTheme();
    } // End if (Session check)

    return FThemeConfig();
}

FPalette UThemeUtil::FetchPalette(const UObject* WorldContext)
{
    if (!WorldContext) // Reason: Null context check
    {
        return FPalette();
    } // End if (WorldContext check)

    USessionAdapter* Session = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(WorldContext));
    if (Session) // Reason: Valid session check
    {
        return Session->GetPalette();
    } // End if (Session check)

    return FPalette();
}

FTypeScale UThemeUtil::FetchTypeScale(const UObject* WorldContext)
{
    if (!WorldContext) // Reason: Null context check
    {
        return FTypeScale();
    } // End if (WorldContext check)

    USessionAdapter* Session = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(WorldContext));
    if (Session) // Reason: Valid session check
    {
        return Session->GetTypeScale();
    } // End if (Session check)

    return FTypeScale();
}

FSpaceGrid UThemeUtil::FetchSpaceGrid(const UObject* WorldContext)
{
    if (!WorldContext) // Reason: Null context check
    {
        return FSpaceGrid();
    } // End if (WorldContext check)

    USessionAdapter* Session = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(WorldContext));
    if (Session) // Reason: Valid session check
    {
        return Session->GetSpaceGrid();
    } // End if (Session check)

    return FSpaceGrid();
}

FBorderSpec UThemeUtil::FetchBorderSpec(const UObject* WorldContext)
{
    if (!WorldContext) // Reason: Null context check
    {
        return FBorderSpec();
    } // End if (WorldContext check)

    USessionAdapter* Session = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(WorldContext));
    if (Session) // Reason: Valid session check
    {
        return Session->GetBorderSpec();
    } // End if (Session check)

    return FBorderSpec();
}

FMotionTiming UThemeUtil::FetchMotionTiming(const UObject* WorldContext)
{
    if (!WorldContext) // Reason: Null context check
    {
        return FMotionTiming();
    } // End if (WorldContext check)

    USessionAdapter* Session = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(WorldContext));
    if (Session) // Reason: Valid session check
    {
        return Session->GetMotionTiming();
    } // End if (Session check)

    return FMotionTiming();
}

void UThemeUtil::ApplyTypeSpec(UTextBlock* TextBlock, const FTypeSpec& Spec)
{
    if (!TextBlock) // Reason: Null widget check
    {
        return;
    } // End if (TextBlock check)

    TextBlock->SetColorAndOpacity(FSlateColor(Spec.Color));

    FSlateFontInfo FontInfo = TextBlock->GetFont();
    FontInfo.Size = Spec.Size;
    
    if (Spec.FontAsset) // Reason: Custom font specified
    {
        FontInfo.FontObject = Spec.FontAsset;
    } // End if (FontAsset check)

    switch (Spec.Weight) // Reason: Apply font weight
    {
        case EFontWeight::Thin:    FontInfo.TypefaceFontName = FName("Thin");    break;
        case EFontWeight::Light:   FontInfo.TypefaceFontName = FName("Light");   break;
        case EFontWeight::Regular: FontInfo.TypefaceFontName = FName("Regular"); break;
        case EFontWeight::Medium:  FontInfo.TypefaceFontName = FName("Medium");  break;
        case EFontWeight::Bold:    FontInfo.TypefaceFontName = FName("Bold");    break;
        case EFontWeight::Black:   FontInfo.TypefaceFontName = FName("Black");   break;
    } // End switch (Weight)

    TextBlock->SetFont(FontInfo);
    TextBlock->SetLineHeightPercentage(Spec.LineHeight);
}

void UThemeUtil::ApplyBorderStyling(UBorder* Border, FLinearColor Color, float Radius, float Thickness)
{
    if (!Border) // Reason: Null widget check
    {
        return;
    } // End if (Border check)

    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
    Brush.TintColor = FSlateColor(Color);
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
    
    if (Thickness > 0.0f) // Reason: Apply border outline
    {
        Brush.OutlineSettings.Width = Thickness;
        Brush.OutlineSettings.Color = FSlateColor(Color);
    } // End if (Thickness check)
    
    Border->SetBrush(Brush);
}

FLinearColor UThemeUtil::BlendOverlay(FLinearColor Base, FLinearColor Overlay)
{
    float Alpha = Overlay.A;
    return FLinearColor(
        FMath::Lerp(Base.R, Overlay.R, Alpha),
        FMath::Lerp(Base.G, Overlay.G, Alpha),
        FMath::Lerp(Base.B, Overlay.B, Alpha),
        Base.A
    );
}
