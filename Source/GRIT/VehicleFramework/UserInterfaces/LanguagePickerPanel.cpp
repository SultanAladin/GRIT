#include "LanguagePickerPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"

//------------------------------------------------------------------------------
//                                  LIFECYCLE
//------------------------------------------------------------------------------

void ULanguagePickerPanel::NativeConstruct()
{
    Super::NativeConstruct();

    // Reason: Configure container appearance
    if (ContainerBorder)
    {
        ContainerBorder->SetBrushColor(BackgroundColor);
    } // End if (ContainerBorder exists)
}

void ULanguagePickerPanel::NativeDestruct()
{
    // Reason: Clear references
    LanguageEntries.Empty();
    Super::NativeDestruct();
}

//------------------------------------------------------------------------------
//                               PUBLIC API
//------------------------------------------------------------------------------

void ULanguagePickerPanel::AddLanguageOption(const FString& DisplayName, const FString& LanguageCode)
{
    // Reason: Create language entry widget
    if (!LanguageEntryClass || !LanguageList)
    {
        UE_LOG(LogTemp, Error, TEXT("[LanguagePickerPanel] Cannot add option - LanguageEntryClass or LanguageList is null"));
        return;
    } // End if (null check)

    ULanguagePickerEntry* NewEntry = CreateWidget<ULanguagePickerEntry>(this, LanguageEntryClass);
    
    if (NewEntry) // Reason: Configure and add entry
    {
        NewEntry->SetLanguageData(DisplayName, LanguageCode);
        NewEntry->OnLanguageSelected.AddDynamic(this, &ULanguagePickerPanel::ProcessLanguageSelected);
        
        // Reason: Add to vertical box
        UVerticalBoxSlot* BoxSlot = LanguageList->AddChildToVerticalBox(NewEntry);
        if (BoxSlot)
        {
            BoxSlot->SetHorizontalAlignment(HAlign_Fill);
            BoxSlot->SetVerticalAlignment(VAlign_Top);
            BoxSlot->SetPadding(FMargin(0.0f));
        } // End if (BoxSlot created)
        
        LanguageEntries.Add(NewEntry);
        UE_LOG(LogTemp, Log, TEXT("[LanguagePickerPanel] Added language: %s (%s)"), *DisplayName, *LanguageCode);
    } // End if (NewEntry created)
}

void ULanguagePickerPanel::SetSelectedLanguage(const FString& LanguageCode)
{
    CurrentLanguageCode = LanguageCode;
    
    // Reason: Update visual state for all entries
    for (ULanguagePickerEntry* Entry : LanguageEntries)
    {
        if (Entry)
        {
            Entry->SetSelected(false);
        }
    } // End for (LanguageEntries)
    
    // Reason: Mark selected entry
    for (ULanguagePickerEntry* Entry : LanguageEntries)
    {
        if (Entry)
        {
            // TODO: Compare entry's code with LanguageCode (need to expose getter)
            // For now, this is a placeholder
        }
    } // End for (LanguageEntries)
}

void ULanguagePickerPanel::PositionRelativeToButton(UWidget* SpawnButton)
{
    // Reason: Calculate smart position avoiding screen clipping
    if (!SpawnButton)
    {
        UE_LOG(LogTemp, Error, TEXT("[LanguagePickerPanel] Cannot position - SpawnButton is null"));
        return;
    } // End if (SpawnButton null check)

    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(this->Slot);
    
    if (!CanvasSlot) // Reason: Widget must be in canvas panel
    {
        UE_LOG(LogTemp, Error, TEXT("[LanguagePickerPanel] Widget must be child of CanvasPanel for positioning"));
        return;
    } // End if (CanvasSlot null check)

    // Reason: Get button geometry
    FGeometry ButtonGeometry = SpawnButton->GetCachedGeometry();
    FVector2D ButtonPos = ButtonGeometry.GetAbsolutePosition();  // [px] - Screen space position
    FVector2D ButtonSize = ButtonGeometry.GetAbsoluteSize();  // [px] - Button dimensions

    // Reason: Get container size
    FVector2D ContainerSize = GetDesiredSize();  // [px] - Container dimensions
    
    // Reason: Calculate base position (bottom-right of button, going up)
    FVector2D BasePosition;  // [px]
    BasePosition.X = ButtonPos.X + ButtonSize.X - ContainerSize.X;  // Align right edge with button right
    BasePosition.Y = ButtonPos.Y - ContainerSize.Y - SpacingFromButton.Y;  // Above button with spacing
    
    // Reason: Apply smart positioning with bounds checking
    FVector2D OptimalPosition = CalculateOptimalPosition(ButtonPos, ButtonSize, ContainerSize);  // [px]
    FVector2D ClampedPosition = ClampToViewport(OptimalPosition, ContainerSize);  // [px]
    
    // Reason: Convert to local canvas coordinates
    float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);  // [ratio] - Uniform DPI scale
    FVector2D LocalPosition = ClampedPosition / ViewportScale;  // [px] - Local canvas space
    
    CanvasSlot->SetPosition(LocalPosition);
    
    UE_LOG(LogTemp, Log, TEXT("[LanguagePickerPanel] Positioned at (%.1f, %.1f) - Size: (%.1f, %.1f)"), LocalPosition.X, LocalPosition.Y, ContainerSize.X, ContainerSize.Y);
}

//------------------------------------------------------------------------------
//                            POSITIONING LOGIC
//------------------------------------------------------------------------------

FVector2D ULanguagePickerPanel::CalculateOptimalPosition(const FVector2D& ButtonPos, const FVector2D& ButtonSize, const FVector2D& ContainerSize) const
{
    FVector2D OptimalPos;  // [px]
    
    // Reason: Align right edge with button, go upward with spacing
    OptimalPos.X = ButtonPos.X + ButtonSize.X - ContainerSize.X;
    OptimalPos.Y = ButtonPos.Y - ContainerSize.Y - SpacingFromButton.Y;
    
    return OptimalPos;
}

FVector2D ULanguagePickerPanel::ClampToViewport(const FVector2D& Position, const FVector2D& ContainerSize) const
{
    FVector2D ClampedPos = Position;  // [px]
    
    // Reason: Get viewport dimensions
    FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);  // [px]
    
    // Reason: Clamp X to avoid right edge clipping
    float MaxX = ViewportSize.X - ContainerSize.X - EdgeMargin;  // [px]
    ClampedPos.X = FMath::Min(ClampedPos.X, MaxX);
    ClampedPos.X = FMath::Max(ClampedPos.X, EdgeMargin);
    
    // Reason: Clamp Y to avoid top/bottom clipping
    float MaxY = ViewportSize.Y - ContainerSize.Y - EdgeMargin;  // [px]
    ClampedPos.Y = FMath::Min(ClampedPos.Y, MaxY);
    ClampedPos.Y = FMath::Max(ClampedPos.Y, EdgeMargin);
    
    return ClampedPos;
}

//------------------------------------------------------------------------------
//                             EVENT HANDLERS
//------------------------------------------------------------------------------

void ULanguagePickerPanel::ProcessLanguageSelected(const FString& LanguageCode)
{
    // Reason: Update selection and broadcast event
    SetSelectedLanguage(LanguageCode);
    OnLanguageChanged.Broadcast(LanguageCode);
    
    UE_LOG(LogTemp, Log, TEXT("[LanguagePickerPanel] Language selected: %s"), *LanguageCode);
}
