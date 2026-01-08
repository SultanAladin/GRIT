#include "EntryWidget.h"
#include "EntryData.h"
#include "ThemeUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogEntryWidget, Log, All);

void UEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ApplyThemeStyling();
}

void UEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

    UE_LOG(LogEntryWidget, Log, TEXT("NativeOnListItemObjectSet: Received object %s"), ListItemObject ? *ListItemObject->GetName() : TEXT("NULL"));

    if (UEntryData* EntryData = Cast<UEntryData>(ListItemObject)) // Reason: Valid data check
    {
        UE_LOG(LogEntryWidget, Log, TEXT("NativeOnListItemObjectSet: Cast to EntryData success, Type=%s"), *EntryData->RetrieveEntryType().ToString());
        SyncDisplay(EntryData);
    } // End if (EntryData check)
    else
    {
        UE_LOG(LogEntryWidget, Warning, TEXT("NativeOnListItemObjectSet: Failed to cast to EntryData"));
    }
}

void UEntryWidget::SyncDisplay(UEntryData* Data)
{
    if (!Data) // Reason: Null safety
    {
        UE_LOG(LogEntryWidget, Warning, TEXT("SyncDisplay: Data is null"));
        return;
    }

    FName EntryType = Data->RetrieveEntryType();
    UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: Processing type=%s"), *EntryType.ToString());

    //------------------------------------------------------------------------------
    // text-only entry branch
    //------------------------------------------------------------------------------
    
    if (EntryType == FName("Text")) // Reason: Text entry type
    {
        if (UTextEntry* TextData = Cast<UTextEntry>(Data)) // Reason: Cast to text entry
        {
            UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: Text entry - '%s'"), *TextData->LabelText.ToString());
            
            if (LabelText) // Reason: Widget exists check
            {
                LabelText->SetText(TextData->LabelText);
                LabelText->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: LabelText widget updated and visible"));
            } // End if (LabelText check)
            else
            {
                UE_LOG(LogEntryWidget, Warning, TEXT("SyncDisplay: LabelText widget not bound!"));
            }

            if (IconImage) // Reason: Hide image for text-only
            {
                IconImage->SetVisibility(ESlateVisibility::Collapsed);
                UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: IconImage widget collapsed"));
            } // End if (IconImage check)
        } // End if (TextData cast)
    } // End if (Text type)
    
    //------------------------------------------------------------------------------
    // image-only entry branch
    //------------------------------------------------------------------------------
    
    else if (EntryType == FName("Image")) // Reason: Image entry type
    {
        if (UImageEntry* ImageData = Cast<UImageEntry>(Data)) // Reason: Cast to image entry
        {
            UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: Image entry - Texture=%s"), ImageData->IconTexture ? *ImageData->IconTexture->GetName() : TEXT("NULL"));
            
            if (IconImage && ImageData->IconTexture) // Reason: Widget and texture check
            {
                IconImage->SetBrushFromTexture(ImageData->IconTexture);
                IconImage->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: IconImage widget updated and visible"));
            } // End if (IconImage check)
            else
            {
                UE_LOG(LogEntryWidget, Warning, TEXT("SyncDisplay: IconImage widget not bound or texture is null!"));
            }

            if (LabelText) // Reason: Hide text for image-only
            {
                LabelText->SetVisibility(ESlateVisibility::Collapsed);
                UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: LabelText widget collapsed"));
            } // End if (LabelText check)
        } // End if (ImageData cast)
    } // End if (Image type)
    
    //------------------------------------------------------------------------------
    // labeled image entry branch
    //------------------------------------------------------------------------------
    
    else if (EntryType == FName("LabeledImage")) // Reason: Labeled image entry type
    {
        if (ULabeledImageEntry* LabeledData = Cast<ULabeledImageEntry>(Data)) // Reason: Cast to labeled image entry
        {
            UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: LabeledImage entry - Text='%s' Texture=%s"), *LabeledData->LabelText.ToString(), LabeledData->IconTexture ? *LabeledData->IconTexture->GetName() : TEXT("NULL"));
            
            if (LabelText) // Reason: Widget exists check
            {
                LabelText->SetText(LabeledData->LabelText);
                LabelText->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: LabelText widget updated and visible"));
            } // End if (LabelText check)
            else
            {
                UE_LOG(LogEntryWidget, Warning, TEXT("SyncDisplay: LabelText widget not bound!"));
            }

            if (IconImage && LabeledData->IconTexture) // Reason: Widget and texture check
            {
                IconImage->SetBrushFromTexture(LabeledData->IconTexture);
                IconImage->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogEntryWidget, Log, TEXT("SyncDisplay: IconImage widget updated and visible"));
            } // End if (IconImage check)
            else
            {
                UE_LOG(LogEntryWidget, Warning, TEXT("SyncDisplay: IconImage widget not bound or texture is null!"));
            }
        } // End if (LabeledData cast)
    } // End if (LabeledImage type)
    else
    {
        UE_LOG(LogEntryWidget, Warning, TEXT("SyncDisplay: Unknown entry type '%s'"), *EntryType.ToString());
    }
}

//------------------------------------------------------------------------------
// theme application
//------------------------------------------------------------------------------

void UEntryWidget::ApplyThemeStyling()
{
    FTypeScale Type = UThemeUtil::FetchTypeScale(this);

    if (LabelText) // Reason: Apply text styling
    {
        UThemeUtil::ApplyTypeSpec(LabelText, Type.BodyM);
    } // End if (LabelText check)
}