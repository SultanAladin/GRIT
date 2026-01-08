#include "ButtonEntryWidget.h"
#include "EntryData.h"
#include "ThemeUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogButtonEntryWidget, Log, All);

void UButtonEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ApplyThemeStyling();
}

void UButtonEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

    UE_LOG(LogButtonEntryWidget, Log, TEXT("NativeOnListItemObjectSet: Received object %s"), ListItemObject ? *ListItemObject->GetName() : TEXT("NULL"));

    CurrentEntry = Cast<UButtonEntryBase>(ListItemObject);

    if (UEntryData* EntryData = Cast<UEntryData>(ListItemObject)) // Reason: Valid data check
    {
        UE_LOG(LogButtonEntryWidget, Log, TEXT("NativeOnListItemObjectSet: Cast to EntryData success, Type=%s"), *EntryData->RetrieveEntryType().ToString());
        SyncDisplay(EntryData);
    } // End if (EntryData check)
    else
    {
        UE_LOG(LogButtonEntryWidget, Warning, TEXT("NativeOnListItemObjectSet: Failed to cast to EntryData"));
    }
}

FReply UButtonEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) // Reason: Left click only
    {
        bIsPressed = true;
        UE_LOG(LogButtonEntryWidget, Log, TEXT("NativeOnMouseButtonDown: EntryID=%d"), CurrentEntry ? CurrentEntry->EntryID : -1);

        if (CurrentEntry) // Reason: Valid entry check
        {
            CurrentEntry->OnPressed.Broadcast(CurrentEntry->EntryID);
        } // End if (CurrentEntry check)

        return FReply::Handled();
    } // End if (Left button check)

    return FReply::Unhandled();
}

FReply UButtonEntryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed) // Reason: Left click only
    {
        bIsPressed = false;
        UE_LOG(LogButtonEntryWidget, Log, TEXT("NativeOnMouseButtonUp: EntryID=%d"), CurrentEntry ? CurrentEntry->EntryID : -1);

        if (CurrentEntry) // Reason: Valid entry check
        {
            CurrentEntry->OnReleased.Broadcast(CurrentEntry->EntryID);

            if (IsHovered()) // Reason: Check if still hovered for click
            {
                CurrentEntry->OnClicked.Broadcast(CurrentEntry->EntryID);
                UE_LOG(LogButtonEntryWidget, Log, TEXT("OnClicked: EntryID=%d"), CurrentEntry->EntryID);
            } // End if (IsHovered check)
        } // End if (CurrentEntry check)

        return FReply::Handled();
    } // End if (Left button check)

    return FReply::Unhandled();
}

void UButtonEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    ApplyHoverState();

    UE_LOG(LogButtonEntryWidget, Log, TEXT("NativeOnMouseEnter: EntryID=%d"), CurrentEntry ? CurrentEntry->EntryID : -1);

    if (CurrentEntry) // Reason: Valid entry check
    {
        CurrentEntry->OnHovered.Broadcast(CurrentEntry->EntryID);
    } // End if (CurrentEntry check)
}

void UButtonEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    bIsPressed = false;

    RestoreDefaultState();

    UE_LOG(LogButtonEntryWidget, Log, TEXT("NativeOnMouseLeave: EntryID=%d"), CurrentEntry ? CurrentEntry->EntryID : -1);

    if (CurrentEntry) // Reason: Valid entry check
    {
        CurrentEntry->OnUnhovered.Broadcast(CurrentEntry->EntryID);
    } // End if (CurrentEntry check)
}

void UButtonEntryWidget::SyncDisplay(UEntryData* Data)
{
    if (!Data) // Reason: Null safety
    {
        UE_LOG(LogButtonEntryWidget, Warning, TEXT("SyncDisplay: Data is null"));
        return;
    }

    FName EntryType = Data->RetrieveEntryType();
    UE_LOG(LogButtonEntryWidget, Log, TEXT("SyncDisplay: Processing type=%s"), *EntryType.ToString());

    //------------------------------------------------------------------------------
    // text button branch
    //------------------------------------------------------------------------------
    
    if (EntryType == FName("TextButton")) // Reason: Text button type
    {
        if (UTextButtonEntry* TextData = Cast<UTextButtonEntry>(Data)) // Reason: Cast to text button
        {
            UE_LOG(LogButtonEntryWidget, Log, TEXT("SyncDisplay: TextButton - '%s'"), *TextData->LabelText.ToString());
            
            if (LabelText) // Reason: Widget exists check
            {
                LabelText->SetText(TextData->LabelText);
                LabelText->SetVisibility(ESlateVisibility::Visible);
            } // End if (LabelText check)

            if (IconImage) // Reason: Hide image for text-only
            {
                IconImage->SetVisibility(ESlateVisibility::Collapsed);
            } // End if (IconImage check)
        } // End if (TextData cast)
    } // End if (TextButton type)
    
    //------------------------------------------------------------------------------
    // image button branch
    //------------------------------------------------------------------------------
    
    else if (EntryType == FName("ImageButton")) // Reason: Image button type
    {
        if (UImageButtonEntry* ImageData = Cast<UImageButtonEntry>(Data)) // Reason: Cast to image button
        {
            UE_LOG(LogButtonEntryWidget, Log, TEXT("SyncDisplay: ImageButton - Texture=%s"), ImageData->IconTexture ? *ImageData->IconTexture->GetName() : TEXT("NULL"));
            
            if (IconImage && ImageData->IconTexture) // Reason: Widget and texture check
            {
                IconImage->SetBrushFromTexture(ImageData->IconTexture);
                IconImage->SetVisibility(ESlateVisibility::Visible);
            } // End if (IconImage check)

            if (LabelText) // Reason: Hide text for image-only
            {
                LabelText->SetVisibility(ESlateVisibility::Collapsed);
            } // End if (LabelText check)
        } // End if (ImageData cast)
    } // End if (ImageButton type)
    
    //------------------------------------------------------------------------------
    // labeled image button branch
    //------------------------------------------------------------------------------
    
    else if (EntryType == FName("LabeledImageButton")) // Reason: Labeled image button type
    {
        if (ULabeledImageButtonEntry* LabeledData = Cast<ULabeledImageButtonEntry>(Data)) // Reason: Cast to labeled button
        {
            UE_LOG(LogButtonEntryWidget, Log, TEXT("SyncDisplay: LabeledImageButton - Text='%s' Texture=%s"), *LabeledData->LabelText.ToString(), LabeledData->IconTexture ? *LabeledData->IconTexture->GetName() : TEXT("NULL"));
            
            if (LabelText) // Reason: Widget exists check
            {
                LabelText->SetText(LabeledData->LabelText);
                LabelText->SetVisibility(ESlateVisibility::Visible);
            } // End if (LabelText check)

            if (IconImage && LabeledData->IconTexture) // Reason: Widget and texture check
            {
                IconImage->SetBrushFromTexture(LabeledData->IconTexture);
                IconImage->SetVisibility(ESlateVisibility::Visible);
            } // End if (IconImage check)
        } // End if (LabeledData cast)
    } // End if (LabeledImageButton type)
}

//------------------------------------------------------------------------------
// theme application
//------------------------------------------------------------------------------

void UButtonEntryWidget::ApplyThemeStyling()
{
    FPalette Palette = UThemeUtil::FetchPalette(this);
    FBorderSpec Border = UThemeUtil::FetchBorderSpec(this);
    FTypeScale Type = UThemeUtil::FetchTypeScale(this);

    BaseColor = Palette.SurfaceShift;
    HoverColor = UThemeUtil::BlendOverlay(BaseColor, Palette.StateHover);

    if (ButtonBorder) // Reason: Setup brush structure and apply initial color
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(FLinearColor::White);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.CornerRadii = FVector4(Border.RadiusSnug, Border.RadiusSnug, Border.RadiusSnug, Border.RadiusSnug);
        if (Border.ThicknessBase > 0.0f) { Brush.OutlineSettings.Width = Border.ThicknessBase; Brush.OutlineSettings.Color = FSlateColor(BaseColor); }
        ButtonBorder->SetBrush(Brush);
        ButtonBorder->SetBrushColor(BaseColor);
    } // End if (ButtonBorder check)

    if (LabelText) // Reason: Apply text styling
    {
        UThemeUtil::ApplyTypeSpec(LabelText, Type.BodyM);
    } // End if (LabelText check)
}

void UButtonEntryWidget::ApplyHoverState()
{
    if (ButtonBorder) // Reason: Apply hover color
    {
        ButtonBorder->SetBrushColor(HoverColor);
    } // End if (ButtonBorder check)
}

void UButtonEntryWidget::RestoreDefaultState()
{
    if (ButtonBorder) // Reason: Restore base color
    {
        ButtonBorder->SetBrushColor(BaseColor);
    } // End if (ButtonBorder check)
}