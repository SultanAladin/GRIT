// ColorItemWidget.cpp  
#include "ColorItemWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Engine.h"

UColorItemWidget::UColorItemWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UColorItemWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (ItemButton)
    {
        OriginalScale = ItemButton->GetRenderTransform().Scale;
        StartScale = OriginalScale;
        TargetScale = OriginalScale;
        
        ItemButton->OnClicked.AddDynamic(this, &UColorItemWidget::OnButtonClicked);
        ItemButton->OnHovered.AddDynamic(this, &UColorItemWidget::OnButtonHovered);
        ItemButton->OnUnhovered.AddDynamic(this, &UColorItemWidget::OnButtonUnhovered);
    }
    
    ApplyButtonStyling();
}

void UColorItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    // This is called when ListView assigns a data object to this widget
    ColorDataObject = Cast<UColorDataObject>(ListItemObject);
    
    if (ColorDataObject)
    {
        SetColorData(ColorDataObject->ColorName, ColorDataObject->ColorValue, ColorDataObject->ItemIndex);
        
        // Debug output to verify color data
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, 
                FString::Printf(TEXT("ColorItem: Set color %s (R=%.2f, G=%.2f, B=%.2f)"), 
                    *ColorDataObject->ColorName, 
                    ColorDataObject->ColorValue.R, 
                    ColorDataObject->ColorValue.G, 
                    ColorDataObject->ColorValue.B));
        }
    }
}

void UColorItemWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Handle scale animation
    if (CurrentAnimationTime < AnimationDuration && ItemButton)
    {
        CurrentAnimationTime = FMath::Min(CurrentAnimationTime + InDeltaTime, AnimationDuration);
        float Alpha = CurrentAnimationTime / AnimationDuration;
        
        // Smooth ease out animation
        Alpha = FMath::Sin(Alpha * PI * 0.5f);
        
        FVector2D CurrentScale = FMath::Lerp(StartScale, TargetScale, Alpha);
        
        FWidgetTransform Transform = ItemButton->GetRenderTransform();
        Transform.Scale = CurrentScale;
        ItemButton->SetRenderTransform(Transform);
    }
}

void UColorItemWidget::SetColorData(const FString& ColorName, const FLinearColor& ColorValue, int32 Index)
{
    ItemIndex = Index;
    CurrentColorValue = ColorValue; // Store the color value
    
    if (ColorNameText)
    {
        ColorNameText->SetText(FText::FromString(ColorName));
    }
    
    if (ColorPreviewImage)
    {
        // Create a solid color brush with proper settings
        FSlateBrush ColorBrush;
        ColorBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
        ColorBrush.TintColor = FSlateColor(ColorValue);
        
        // Set proper resource (use a white texture that can be tinted)
        ColorBrush.SetResourceObject(nullptr); // This will use the default white texture
        ColorBrush.ImageSize = FVector2D(64.0f, 64.0f); // Set a proper size
        
        // Set corner radius and outline
        ColorBrush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
        ColorBrush.OutlineSettings.Width = 1.0f;
        ColorBrush.OutlineSettings.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.3f); // Subtle outline
        
        ColorPreviewImage->SetBrush(ColorBrush);
        ColorPreviewImage->SetColorAndOpacity(ColorValue); // Set the color directly
    }
}

void UColorItemWidget::SetSelected(bool bNewSelected)
{
    if (bIsSelected != bNewSelected)
    {
        bIsSelected = bNewSelected;
        ApplyButtonStyling();
        
        // Update the color preview outline when selected
        UpdateColorPreviewSelection();
        
        // Debug output
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
                FString::Printf(TEXT("ColorItem %d: Selected = %s"), ItemIndex, bIsSelected ? TEXT("TRUE") : TEXT("FALSE")));
        }
    }
}

void UColorItemWidget::UpdateColorPreviewSelection()
{
    if (ColorPreviewImage && ColorDataObject)
    {
        FSlateBrush ColorBrush;
        ColorBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
        ColorBrush.TintColor = FSlateColor(CurrentColorValue);
        ColorBrush.SetResourceObject(nullptr);
        ColorBrush.ImageSize = FVector2D(64.0f, 64.0f);
        ColorBrush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
        
        if (bIsSelected)
        {
            ColorBrush.OutlineSettings.Width = 3.0f;
            ColorBrush.OutlineSettings.Color = SelectedBorderColor;
        }
        else
        {
            ColorBrush.OutlineSettings.Width = 1.0f;
            ColorBrush.OutlineSettings.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.3f);
        }
        
        ColorPreviewImage->SetBrush(ColorBrush);
        ColorPreviewImage->SetColorAndOpacity(CurrentColorValue);
    }
}

void UColorItemWidget::SetStyling(const FLinearColor& NormalColor, const FLinearColor& HoveredColor, 
                                 const FLinearColor& SelectedColor, const FLinearColor& SelectedBorder, float BorderRadius)
{
    ItemNormalColor = NormalColor;
    ItemHoveredColor = HoveredColor;
    ItemSelectedColor = SelectedColor;
    SelectedBorderColor = SelectedBorder;
    ItemBorderRadius = BorderRadius;
    
    ApplyButtonStyling();
    UpdateColorPreviewSelection(); // Update the color preview as well
}

void UColorItemWidget::OnButtonClicked()
{
    OnColorItemSelected.Broadcast(ItemIndex);
    
    // Debug output
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
            FString::Printf(TEXT("ColorItem %d clicked!"), ItemIndex));
    }
}

void UColorItemWidget::OnButtonHovered()
{
    bIsHovered = true;
    StartScaleAnimation(OriginalScale * HoverScale);
    ApplyButtonStyling();
}

void UColorItemWidget::OnButtonUnhovered()
{
    bIsHovered = false;
    StartScaleAnimation(OriginalScale);
    ApplyButtonStyling();
}

void UColorItemWidget::ApplyButtonStyling()
{
    if (!ItemButton)
        return;
    
    FButtonStyle ButtonStyle = ItemButton->GetStyle();
    
    // Set rounded corners for all states
    ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
    ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
    ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
    
    // Set corner radius for all states
    ButtonStyle.Normal.OutlineSettings.CornerRadii = FVector4(ItemBorderRadius, ItemBorderRadius, ItemBorderRadius, ItemBorderRadius);
    ButtonStyle.Hovered.OutlineSettings.CornerRadii = FVector4(ItemBorderRadius, ItemBorderRadius, ItemBorderRadius, ItemBorderRadius);
    ButtonStyle.Pressed.OutlineSettings.CornerRadii = FVector4(ItemBorderRadius, ItemBorderRadius, ItemBorderRadius, ItemBorderRadius);
    
    if (bIsSelected)
    {
        // Selected state - use selected colors for all button states
        ButtonStyle.Normal.TintColor = ItemSelectedColor;
        ButtonStyle.Hovered.TintColor = ItemSelectedColor;
        ButtonStyle.Pressed.TintColor = ItemSelectedColor;
        
        // Selected border - thicker and colored
        ButtonStyle.Normal.OutlineSettings.Width = 2.0f;
        ButtonStyle.Hovered.OutlineSettings.Width = 2.0f;
        ButtonStyle.Pressed.OutlineSettings.Width = 2.0f;
        
        ButtonStyle.Normal.OutlineSettings.Color = SelectedBorderColor;
        ButtonStyle.Hovered.OutlineSettings.Color = SelectedBorderColor;
        ButtonStyle.Pressed.OutlineSettings.Color = SelectedBorderColor;
    }
    else
    {
        // Normal state
        ButtonStyle.Normal.TintColor = ItemNormalColor;
        ButtonStyle.Hovered.TintColor = ItemHoveredColor;
        ButtonStyle.Pressed.TintColor = ItemNormalColor;
        
        // Normal border - thin and subtle
        ButtonStyle.Normal.OutlineSettings.Width = 1.0f;
        ButtonStyle.Hovered.OutlineSettings.Width = 1.0f;
        ButtonStyle.Pressed.OutlineSettings.Width = 1.0f;
        
        ButtonStyle.Normal.OutlineSettings.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
        ButtonStyle.Hovered.OutlineSettings.Color = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
        ButtonStyle.Pressed.OutlineSettings.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    ItemButton->SetStyle(ButtonStyle);
}

void UColorItemWidget::StartScaleAnimation(const FVector2D& NewTargetScale)
{
    if (ItemButton)
    {
        StartScale = ItemButton->GetRenderTransform().Scale;
        TargetScale = NewTargetScale;
        CurrentAnimationTime = 0.0f;
    }
}