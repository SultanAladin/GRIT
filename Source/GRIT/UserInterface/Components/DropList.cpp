#include "DropList.h"
#include "ThemeUtil.h"
#include "TimerManager.h"
#include "Components/SizeBoxSlot.h"
#include "Components/OverlaySlot.h"

DEFINE_LOG_CATEGORY_STATIC(LogDropList, Log, All);

UDropList::UDropList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , AnimDuration(0.3f)
    , AnimCurve(EMotionCurve::QuadOut)
    , bAnimChevron(true)
    , ChevronAnimDuration(0.2f)
    , MaxContentHeight(200.0f)
    , ItemHeight(40.0f)
    , bHasBuilt(false)
{
}

void UDropList::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    if (!bHasBuilt && EntryConfigs.Num() > 0) // Reason: Auto-populate on first construct
    {
        UE_LOG(LogDropList, Log, TEXT("NativePreConstruct: Auto-building from %d configs"), EntryConfigs.Num());
        BuildFromConfigs();
        bHasBuilt = true;
    } // End if (First build check)
}

void UDropList::NativeConstruct()
{
    Super::NativeConstruct();

    bIsExpanded = false;
    bIsAnimating = false;
    bIsPressed = false;
    bChevronAnimating = false;
    AnimTime = 0.0f;
    ChevronAnimTime = 0.0f;

    InitTheme();
    ConfigureContentSlot();

    if (HeaderLabel) // Reason: Apply header text
    {
        HeaderLabel->SetText(HeaderText);
    } // End if (HeaderLabel check)

    if (ContentSizeBox) // Reason: Start collapsed
    {
        ContentSizeBox->SetHeightOverride(0.0f);
    } // End if (ContentSizeBox check)

    if (ChevronIcon) // Reason: Initialize chevron rotation
    {
        SetChevronAngle(0.0f);
    } // End if (ChevronIcon check)
}

FReply UDropList::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) // Reason: Left click only
    {
        if (HeaderBorder && HeaderBorder->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition())) // Reason: Header hit test
        {
            bIsPressed = true;
            return FReply::Handled();
        } // End if (Header hit)
    } // End if (Left button)

    return FReply::Unhandled();
}

FReply UDropList::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed) // Reason: Complete click
    {
        bIsPressed = false;

        if (HeaderBorder && HeaderBorder->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition())) // Reason: Header hit test
        {
            Toggle();
            return FReply::Handled();
        } // End if (Header hit)
    } // End if (Left button and pressed)

    return FReply::Unhandled();
}

void UDropList::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (HeaderBorder) // Reason: Check if hovering header
    {
        ApplyHeaderHover();
    } // End if (HeaderBorder check)
}

void UDropList::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    bIsPressed = false;
    RestoreHeaderDefault();
}

//------------------------------------------------------------------------------
// public operations
//------------------------------------------------------------------------------

void UDropList::Toggle()
{
    SetExpanded(!bIsExpanded);
}

void UDropList::SetExpanded(bool bExpanded)
{
    if (bExpanded == bIsExpanded) // Reason: Already in target state
    {
        return;
    } // End if (State check)

    bIsExpanded = bExpanded;

    if (bIsExpanded) // Reason: Expand animation
    {
        StartExpand();
    } // End if (Expand)
    else
    {
        StartCollapse();
    }
}

void UDropList::AddEntry(UEntryData* Data)
{
    if (ContentList) // Reason: Valid list check
    {
        ContentList->InjectEntry(Data);
    } // End if (ContentList check)
}

void UDropList::ClearEntries()
{
    if (ContentList) // Reason: Valid list check
    {
        ContentList->PurgeAll();
    } // End if (ContentList check)
}

int32 UDropList::GetEntryCount() const
{
    if (ContentList) // Reason: Valid list check
    {
        return ContentList->GetNumItems();
    } // End if (ContentList check)

    return 0;
}

void UDropList::BuildFromConfigs()
{
    UE_LOG(LogDropList, Log, TEXT("BuildFromConfigs: Starting with %d configs"), EntryConfigs.Num());
    
    if (EntryConfigs.Num() == 0) // Reason: No configs to process
    {
        UE_LOG(LogDropList, Warning, TEXT("BuildFromConfigs: EntryConfigs array is empty"));
        return;
    } // End if (Empty configs check)

    ClearEntries();

    //------------------------------------------------------------------------------
    // config iteration and entry construction
    //------------------------------------------------------------------------------
    
    for (int32 i = 0; i < EntryConfigs.Num(); i++) // Reason: Iterate configs
    {
        const FSimpleEntryConfig& Config = EntryConfigs[i];
        
        bool bHasText = !Config.EntryText.IsEmpty();
        bool bHasTexture = Config.EntryTexture != nullptr;
        bool bIsButton = Config.bIsButton;

        UE_LOG(LogDropList, Log, TEXT("BuildFromConfigs [%d]: Text='%s' HasText=%d HasTexture=%d IsButton=%d"), i, *Config.EntryText.ToString(), bHasText, bHasTexture, bIsButton);

        if (bIsButton) // Reason: Create button entries
        {
            if (bHasText && bHasTexture) // Reason: Both text and texture button
            {
                ULabeledImageButtonEntry* Entry = NewObject<ULabeledImageButtonEntry>(this);
                Entry->BootstrapLabeledImageButton(i, Config.EntryText, Config.EntryTexture);
                AddEntry(Entry);
            } // End if (Text and Texture button)
            else if (bHasTexture) // Reason: Texture only button
            {
                UImageButtonEntry* Entry = NewObject<UImageButtonEntry>(this);
                Entry->BootstrapImageButton(i, Config.EntryTexture);
                AddEntry(Entry);
            } // End if (Texture button)
            else if (bHasText) // Reason: Text only button
            {
                UTextButtonEntry* Entry = NewObject<UTextButtonEntry>(this);
                Entry->BootstrapTextButton(i, Config.EntryText);
                AddEntry(Entry);
            } // End if (Text button)
        } // End if (Button entry)
        else // Reason: Create static entries
        {
            if (bHasText && bHasTexture) // Reason: Both text and texture
            {
                ULabeledImageEntry* Entry = NewObject<ULabeledImageEntry>(this);
                Entry->BootstrapLabeledImage(i, Config.EntryText, Config.EntryTexture);
                AddEntry(Entry);
            } // End if (Text and Texture)
            else if (bHasTexture) // Reason: Texture only
            {
                UImageEntry* Entry = NewObject<UImageEntry>(this);
                Entry->BootstrapImage(i, Config.EntryTexture);
                AddEntry(Entry);
            } // End if (Texture only)
            else if (bHasText) // Reason: Text only
            {
                UTextEntry* Entry = NewObject<UTextEntry>(this);
                Entry->BootstrapText(i, Config.EntryText);
                AddEntry(Entry);
            } // End if (Text only)
        } // End else (Static entry)
    } // End for (EntryConfigs loop)
    
    UE_LOG(LogDropList, Log, TEXT("BuildFromConfigs: Complete. Final item count: %d"), GetEntryCount());
}

//------------------------------------------------------------------------------
// theme initialization
//------------------------------------------------------------------------------

void UDropList::InitTheme()
{
    FPalette Palette = UThemeUtil::FetchPalette(this);
    FBorderSpec Border = UThemeUtil::FetchBorderSpec(this);
    FTypeScale Type = UThemeUtil::FetchTypeScale(this);

    BaseHeaderColor = Palette.SurfaceShift;
    HoverHeaderColor = UThemeUtil::BlendOverlay(BaseHeaderColor, Palette.StateHover);

    if (HeaderBorder) // Reason: Apply header styling
    {
        UThemeUtil::ApplyBorderStyling(HeaderBorder, BaseHeaderColor, Border.RadiusLoose, Border.ThicknessBase);
    } // End if (HeaderBorder check)

    if (HeaderLabel) // Reason: Apply text styling
    {
        UThemeUtil::ApplyTypeSpec(HeaderLabel, Type.BodyL);
    } // End if (HeaderLabel check)

    if (ContentBorder) // Reason: Apply content styling
    {
        UThemeUtil::ApplyBorderStyling(ContentBorder, Palette.SurfaceRaised, Border.RadiusSnug, 0.0f);
    } // End if (ContentBorder check)
}

void UDropList::ConfigureContentSlot()
{
    if (!ContentSizeBox || !ContentOverlay) // Reason: Null check
    {
        return;
    } // End if (Widget check)

    UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ContentSizeBox->Slot);
    if (OverlaySlot) // Reason: Configure slot for width matching
    {
        OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
        OverlaySlot->SetVerticalAlignment(VAlign_Top);
    } // End if (OverlaySlot check)
}

void UDropList::ApplyHeaderHover()
{
    if (HeaderBorder) // Reason: Apply hover color
    {
        HeaderBorder->SetBrushColor(HoverHeaderColor);
    } // End if (HeaderBorder check)
}

void UDropList::RestoreHeaderDefault()
{
    if (HeaderBorder) // Reason: Restore base color
    {
        HeaderBorder->SetBrushColor(BaseHeaderColor);
    } // End if (HeaderBorder check)
}

//------------------------------------------------------------------------------
// animation control
//------------------------------------------------------------------------------

void UDropList::StartExpand()
{
    if (!ContentSizeBox) // Reason: Null check
    {
        return;
    } // End if (ContentSizeBox check)

    TargetHeight = ComputeTargetHeight();
    StartHeight = ContentSizeBox->GetHeightOverride();
    AnimTime = 0.0f;
    bIsAnimating = true;

    UE_LOG(LogDropList, Log, TEXT("StartExpand: From %.1f to %.1f"), StartHeight, TargetHeight);

    GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UDropList::TickAnim, 0.016f, true); // [16ms] - ~60fps update rate

    if (bAnimChevron) // Reason: Animate chevron down
    {
        AnimateChevronTo(180.0f);
    } // End if (Chevron animation)
}

void UDropList::StartCollapse()
{
    if (!ContentSizeBox) // Reason: Null check
    {
        return;
    } // End if (ContentSizeBox check)

    StartHeight = ContentSizeBox->GetHeightOverride();
    TargetHeight = 0.0f;
    AnimTime = 0.0f;
    bIsAnimating = true;

    UE_LOG(LogDropList, Log, TEXT("StartCollapse: From %.1f to %.1f"), StartHeight, TargetHeight);

    GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UDropList::TickAnim, 0.016f, true); // [16ms] - ~60fps update rate

    if (bAnimChevron) // Reason: Animate chevron up
    {
        AnimateChevronTo(0.0f);
    } // End if (Chevron animation)
}

void UDropList::TickAnim()
{
    if (!bIsAnimating || !ContentSizeBox) // Reason: Animation active check
    {
        return;
    } // End if (Animation check)

    AnimTime += 0.016f; // [s] - 16ms frame time
    float Progress = FMath::Clamp(AnimTime / AnimDuration, 0.0f, 1.0f);

    float CurrentHeight = UAnimUtil::LerpCurved(StartHeight, TargetHeight, Progress, AnimCurve);
    ContentSizeBox->SetHeightOverride(CurrentHeight);

    if (Progress >= 1.0f) // Reason: Animation complete
    {
        bIsAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(AnimTimer);
        UE_LOG(LogDropList, Log, TEXT("Animation complete at height %.1f"), CurrentHeight);
    } // End if (Complete check)
}

void UDropList::TickChevronAnim()
{
    if (!bChevronAnimating || !ChevronIcon) // Reason: Animation active check
    {
        return;
    } // End if (Animation check)

    ChevronAnimTime += 0.016f; // [s] - 16ms frame time
    float Progress = FMath::Clamp(ChevronAnimTime / ChevronAnimDuration, 0.0f, 1.0f);

    float CurrentAngle = UAnimUtil::LerpCurved(StartChevronAngle, TargetChevronAngle, Progress, EMotionCurve::QuadOut);
    SetChevronAngle(CurrentAngle);

    if (Progress >= 1.0f) // Reason: Animation complete
    {
        bChevronAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(ChevronAnimTimer);
    } // End if (Complete check)
}

float UDropList::ComputeTargetHeight() const
{
    if (!ContentList) // Reason: Null check
    {
        UE_LOG(LogDropList, Warning, TEXT("ComputeTargetHeight: ContentList is null"));
        return MaxContentHeight;
    } // End if (ContentList check)

    int32 EntryCount = ContentList->GetNumItems();
    
    if (EntryCount == 0) // Reason: No items to measure
    {
        UE_LOG(LogDropList, Warning, TEXT("ComputeTargetHeight: No items in list"));
        return 0.0f;
    } // End if (Empty list check)

    // Calculate height: EntryCount * ItemHeight
    float TotalHeight = static_cast<float>(EntryCount) * ItemHeight;
    float ClampedHeight = FMath::Min(TotalHeight, MaxContentHeight);

    UE_LOG(LogDropList, Log, TEXT("ComputeTargetHeight: Entries=%d, ItemHeight=%.1f, Total=%.1f, Clamped=%.1f"), EntryCount, ItemHeight, TotalHeight, ClampedHeight);

    return ClampedHeight;
}

void UDropList::AnimateChevronTo(float TargetAngle)
{
    if (!ChevronIcon) // Reason: Null check
    {
        return;
    } // End if (ChevronIcon check)

    StartChevronAngle = ChevronIcon->GetRenderTransform().Angle;
    TargetChevronAngle = TargetAngle;
    ChevronAnimTime = 0.0f;
    bChevronAnimating = true;

    GetWorld()->GetTimerManager().SetTimer(ChevronAnimTimer, this, &UDropList::TickChevronAnim, 0.016f, true); // [16ms] - ~60fps update rate
}

void UDropList::SetChevronAngle(float Angle)
{
    if (!ChevronIcon) // Reason: Null check
    {
        return;
    } // End if (ChevronIcon check)

    FWidgetTransform Transform = ChevronIcon->GetRenderTransform();
    Transform.Angle = Angle; // [deg] - Rotation angle
    ChevronIcon->SetRenderTransform(Transform);
}
