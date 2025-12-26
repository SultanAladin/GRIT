//NavEntry.cpp
#include "NavEntry.h"
#include "Materials/MaterialInstanceDynamic.h"

UNavEntry::UNavEntry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , EntryRoot(nullptr)
    , EntryLabel(nullptr)
    , EntryIndex(-1)
    , bIsSelected(false)
    , bIsHovered(false)
    , bIsPressed(false)
    , bConfigInitialized(false)
    , IconDynamicMaterial(nullptr)
{
}

void UNavEntry::NativePreConstruct()
{
    Super::NativePreConstruct();

    // Apply material configuration if available and image exists
    if (bConfigInitialized && EntryImage && RuntimeConfig.IconMaterial.BaseMaterial)
    {
        ApplyIconMaterial(RuntimeConfig.IconMaterial);
    }

    // Apply configured values if available, otherwise use override values for designer preview
    if (bConfigInitialized)
    {
        ApplyConfiguredValues();
    }
    else
    {
        // Designer preview - use override values if set
        if (EntryLabel && !OverrideLabel.IsEmpty()) 
        { 
            EntryLabel->SetText(OverrideLabel); 
        }
        ApplyVisualConfig();
    }
}

void UNavEntry::NativeConstruct()
{
    Super::NativeConstruct();

    // Reason: Construct only handles runtime event binding
    SyncChrome();
}

void UNavEntry::InitEntry(const FNavEntryConfig& Config, int32 Index)
{
    EntryData = Config.Data;
    EntryIndex = Index;
    RuntimeConfig = Config;
    bConfigInitialized = true;

    UE_LOG(LogTemp, Warning, TEXT("[NavEntry] InitEntry Index=%d, Label='%s', Data='%s'"), 
           Index, *Config.Label.ToString(), *Config.Data);

    // Apply the configuration immediately
    ApplyConfiguredValues();

    // Apply material if image exists and material is specified
    if (EntryImage && Config.IconMaterial.BaseMaterial)
    {
        ApplyIconMaterial(Config.IconMaterial);
    }
}

void UNavEntry::ApplyConfiguredValues()
{
    if (!bConfigInitialized) return;

    // ALWAYS apply the config label text - this is the main fix for your issue
    if (EntryLabel)
    {
        EntryLabel->SetText(RuntimeConfig.Label);
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] ApplyConfiguredValues - Setting text to '%s' for Index=%d"), 
               *RuntimeConfig.Label.ToString(), EntryIndex);
    }

    // Apply material configuration if available and image exists
    if (EntryImage && RuntimeConfig.IconMaterial.BaseMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] ApplyConfiguredValues - Applying icon material for Index=%d"), EntryIndex);
        ApplyIconMaterial(RuntimeConfig.IconMaterial);
    }
    else if (EntryImage)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] ApplyConfiguredValues - EntryImage exists but no material specified for Index=%d"), EntryIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] ApplyConfiguredValues - No EntryImage widget found for Index=%d"), EntryIndex);
    }

    ApplyVisualConfig();
}

void UNavEntry::ApplyIconMaterial(const FMaterialSpec& MaterialSpec)
{
    if (!EntryImage || !MaterialSpec.BaseMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] ApplyIconMaterial FAILED - EntryImage=%s, BaseMaterial=%s"), 
               EntryImage ? TEXT("Valid") : TEXT("NULL"), 
               MaterialSpec.BaseMaterial ? TEXT("Valid") : TEXT("NULL"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[NavEntry] ApplyIconMaterial - Creating dynamic material for Index=%d"), EntryIndex);

    // Create dynamic material instance
    IconDynamicMaterial = UMaterialInstanceDynamic::Create(MaterialSpec.BaseMaterial, this);
    if (!IconDynamicMaterial)
    {
        UE_LOG(LogTemp, Error, TEXT("[NavEntry] Failed to create dynamic material instance"));
        return;
    }

    // Apply texture to "Image" parameter and common alternatives
    if (MaterialSpec.Texture)
    {
        // Primary parameter names to try
        IconDynamicMaterial->SetTextureParameterValue(TEXT("Image"), MaterialSpec.Texture);
        IconDynamicMaterial->SetTextureParameterValue(TEXT("Texture"), MaterialSpec.Texture);
        IconDynamicMaterial->SetTextureParameterValue(TEXT("MainTexture"), MaterialSpec.Texture);
        IconDynamicMaterial->SetTextureParameterValue(TEXT("IconTexture"), MaterialSpec.Texture);
        IconDynamicMaterial->SetTextureParameterValue(TEXT("BaseTexture"), MaterialSpec.Texture);
        
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] Set texture '%s' to common parameter names"), 
               *MaterialSpec.Texture->GetName());
        
        // Also try any additional parameter names specified
        for (const FName& ParamName : MaterialSpec.ParameterNames)
        {
            IconDynamicMaterial->SetTextureParameterValue(ParamName, MaterialSpec.Texture);
            UE_LOG(LogTemp, Log, TEXT("[NavEntry] Set custom texture parameter '%s'"), *ParamName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] No texture specified in MaterialSpec"));
    }

    // Apply color to common color parameter names
    IconDynamicMaterial->SetVectorParameterValue(TEXT("FillColor"), MaterialSpec.Color);
    IconDynamicMaterial->SetVectorParameterValue(TEXT("FillColour"), MaterialSpec.Color);  // British spelling
    IconDynamicMaterial->SetVectorParameterValue(TEXT("Color"), MaterialSpec.Color);
    IconDynamicMaterial->SetVectorParameterValue(TEXT("Colour"), MaterialSpec.Color);      // British spelling
    IconDynamicMaterial->SetVectorParameterValue(TEXT("Tint"), MaterialSpec.Color);
    IconDynamicMaterial->SetVectorParameterValue(TEXT("TintColor"), MaterialSpec.Color);
    
    UE_LOG(LogTemp, Warning, TEXT("[NavEntry] Set color parameters to (%.2f, %.2f, %.2f, %.2f)"), 
           MaterialSpec.Color.R, MaterialSpec.Color.G, MaterialSpec.Color.B, MaterialSpec.Color.A);

    // Apply border color parameters if enabled
    if (MaterialSpec.bUseBorderColor)
    {
        IconDynamicMaterial->SetVectorParameterValue(TEXT("BorderColor"), MaterialSpec.BorderColor);
        IconDynamicMaterial->SetVectorParameterValue(TEXT("BorderColour"), MaterialSpec.BorderColor);  // British spelling
        IconDynamicMaterial->SetVectorParameterValue(TEXT("OutlineColor"), MaterialSpec.BorderColor);
        IconDynamicMaterial->SetVectorParameterValue(TEXT("OutlineColour"), MaterialSpec.BorderColor); // British spelling
        IconDynamicMaterial->SetVectorParameterValue(TEXT("StrokeColor"), MaterialSpec.BorderColor);
        IconDynamicMaterial->SetVectorParameterValue(TEXT("EdgeColor"), MaterialSpec.BorderColor);
        
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] Set border color parameters to (%.2f, %.2f, %.2f, %.2f)"), 
               MaterialSpec.BorderColor.R, MaterialSpec.BorderColor.G, MaterialSpec.BorderColor.B, MaterialSpec.BorderColor.A);
    }

    // Set the material to the image
    EntryImage->SetBrushFromMaterial(IconDynamicMaterial);
    
    // Make sure the image is visible
    EntryImage->SetVisibility(ESlateVisibility::Visible);
    
    UE_LOG(LogTemp, Warning, TEXT("[NavEntry] Applied material to icon for Index=%d - Image visibility set to Visible"), EntryIndex);
}

void UNavEntry::DebugImageState()
{
    UE_LOG(LogTemp, Warning, TEXT("=== NavEntry Debug Info (Index=%d) ==="), EntryIndex);
    UE_LOG(LogTemp, Warning, TEXT("EntryImage widget: %s"), EntryImage ? TEXT("EXISTS") : TEXT("NULL"));
    
    if (EntryImage)
    {
        UE_LOG(LogTemp, Warning, TEXT("Image Visibility: %d"), (int32)EntryImage->GetVisibility());
        UE_LOG(LogTemp, Warning, TEXT("Image Size: %.1f x %.1f"), 
               EntryImage->GetDesiredSize().X, EntryImage->GetDesiredSize().Y);
        
        const FSlateBrush* Brush = &EntryImage->GetBrush();
        UE_LOG(LogTemp, Warning, TEXT("Brush Resource: %s"), 
               Brush->GetResourceObject() ? *Brush->GetResourceObject()->GetName() : TEXT("NULL"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("IconDynamicMaterial: %s"), IconDynamicMaterial ? TEXT("EXISTS") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("bConfigInitialized: %s"), bConfigInitialized ? TEXT("TRUE") : TEXT("FALSE"));
    
    if (bConfigInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("Config BaseMaterial: %s"), 
               RuntimeConfig.IconMaterial.BaseMaterial ? *RuntimeConfig.IconMaterial.BaseMaterial->GetName() : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("Config Texture: %s"), 
               RuntimeConfig.IconMaterial.Texture ? *RuntimeConfig.IconMaterial.Texture->GetName() : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("Config ParameterNames: %d"), RuntimeConfig.IconMaterial.ParameterNames.Num());
    }
    UE_LOG(LogTemp, Warning, TEXT("=== End Debug Info ==="));
}

void UNavEntry::ApplyVisualConfig()
{
    if (!EntryLabel) { return; }

    // Use configured values if available, otherwise use override values
    FLinearColor CurrentTextColor = bConfigInitialized ? RuntimeConfig.TextColor : OverrideTextColor;
    int32 CurrentFontSize = bConfigInitialized ? RuntimeConfig.FontSize : OverrideFontSize;

    // Set text color
    EntryLabel->SetColorAndOpacity(FSlateColor(CurrentTextColor));

    // Set font size
    FSlateFontInfo FontInfo = EntryLabel->GetFont();
    FontInfo.Size = CurrentFontSize;
    EntryLabel->SetFont(FontInfo);
}

void UNavEntry::ToggleSelection(bool bState)
{
    UE_LOG(LogTemp, Log, TEXT("[NavEntry] ToggleSelection(%d) on Index=%d - WAS bIsSelected=%d"), bState, EntryIndex, bIsSelected);
    bIsSelected = bState;
    SyncChrome();
    UE_LOG(LogTemp, Log, TEXT("[NavEntry] ToggleSelection DONE - Index=%d, bIsSelected=%d"), EntryIndex, bIsSelected);
}

void UNavEntry::SyncChrome()
{
    if (!EntryLabel)
    {
        UE_LOG(LogTemp, Error, TEXT("[NavEntry] SyncChrome - EntryLabel is NULL! Index=%d"), EntryIndex);
        return;
    }

    // Use configured colors if available, otherwise use override colors
    FLinearColor CurrentTextColor = bConfigInitialized ? RuntimeConfig.TextColor : OverrideTextColor;
    FLinearColor CurrentHoverTextColor = bConfigInitialized ? RuntimeConfig.HoverTextColor : OverrideHoverTextColor;
    FLinearColor CurrentActiveTextColor = bConfigInitialized ? RuntimeConfig.ActiveTextColor : OverrideActiveTextColor;
    FLinearColor CurrentHoverBackgroundColor = bConfigInitialized ? RuntimeConfig.HoverBackgroundColor : OverrideHoverBackgroundColor;

    // Selection (Orange) takes precedence over Hover (Grey)
    FLinearColor TargetColor = bIsSelected ? CurrentActiveTextColor : (bIsHovered ? CurrentHoverTextColor : CurrentTextColor);

    UE_LOG(LogTemp, Log, TEXT("[NavEntry] SyncChrome Index=%d - bIsSelected=%d, bIsHovered=%d -> Color=(%.2f,%.2f,%.2f)"), 
           EntryIndex, bIsSelected, bIsHovered, TargetColor.R, TargetColor.G, TargetColor.B);

    EntryLabel->SetColorAndOpacity(FSlateColor(TargetColor));

    if (EntryRoot)
    {
        float Alpha = (bIsSelected || bIsHovered) ? 0.05f : 0.0f;
        FLinearColor BgColor = CurrentHoverBackgroundColor;
        BgColor.A = Alpha;
        EntryRoot->SetBrushColor(BgColor);
    }
}

//------------------------------------------------------------------------------
//                                    INPUT HANDLING
//------------------------------------------------------------------------------

void UNavEntry::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
    UE_LOG(LogTemp, Log, TEXT("[NavEntry] NativeOnMouseEnter - Index=%d"), EntryIndex);
    bIsHovered = true;
    SyncChrome();

    OnEntryHovered.Broadcast(this);
}

void UNavEntry::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseLeave(MouseEvent);
    UE_LOG(LogTemp, Log, TEXT("[NavEntry] NativeOnMouseLeave - Index=%d, bIsSelected=%d"), EntryIndex, bIsSelected);
    bIsHovered = false;
    // Reason: Don't clear bIsPressed - let MouseButtonUp handle it so clicks register
    SyncChrome();

    OnEntryUnhovered.Broadcast(this);
}

FReply UNavEntry::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] NativeOnMouseButtonDown - Index=%d, CLICK!"), EntryIndex);
        bIsPressed = true;
        
        // Reason: Broadcast pressed event so navbar can track click state
        OnEntryPressed.Broadcast(this);
        
        // Reason: Fire click immediately on press (MouseButtonUp may not fire if cursor leaves)
        OnEntryClicked.Broadcast(this);
        OnEntryClickedBP();
        
        // Reason: Capture mouse for proper button release handling
        TSharedPtr<SWidget> WidgetPtr = TakeWidget();
        if (WidgetPtr.IsValid())
        {
            return FReply::Handled().CaptureMouse(WidgetPtr.ToSharedRef());
        } // End if (widget capture)
        
        return FReply::Handled();
    } // End if (left mouse button)

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UNavEntry::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Reason: Always broadcast released event
        OnEntryReleased.Broadcast(this);
        
        if (bIsPressed)
        {
            UE_LOG(LogTemp, Warning, TEXT("[NavEntry] NativeOnMouseButtonUp - CLICK! Index=%d, Broadcasting OnEntryClicked"), EntryIndex);
            bIsPressed = false;
            OnEntryClicked.Broadcast(this);
            OnEntryClickedBP();
            
            // Reason: Release mouse capture AFTER broadcasting
            return FReply::Handled().ReleaseMouseCapture();
        } // End if (pressed check)
        
        // Reason: Clear pressed state and release capture even if click didn't register
        bIsPressed = false;
        return FReply::Handled().ReleaseMouseCapture();
    } // End if (left mouse button)

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}