#include "DropListStagger.h"
#include "ThemeUtil.h"
#include "TimerManager.h"
#include "Components/ListView.h"

DEFINE_LOG_CATEGORY_STATIC(LogDropListStagger, Log, All);

UDropListStagger::UDropListStagger(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bUseStaggerAnimation(true)
    , StaggerDelay(0.08f)
    , ItemSlideDistance(20.0f)
{
    // Override base class duration for more visible stagger
    AnimDuration = 0.5f;
}

void UDropListStagger::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (bUseStaggerAnimation) // Reason: Initialize stagger animation
    {
        InitItemTransforms();
    } // End if (Stagger animation check)
}

//------------------------------------------------------------------------------
// stagger animation initialization
//------------------------------------------------------------------------------

void UDropListStagger::InitItemTransforms()
{
    if (!ContentList) // Reason: Null check
    {
        return;
    } // End if (ContentList check)

    CacheEntryWidgets();
    
    if (CachedEntryWidgets.Num() > 0) // Reason: Apply initial transforms
    {
        for (UUserWidget* Widget : CachedEntryWidgets) // Reason: Set initial hidden state
        {
            if (Widget) // Reason: Valid widget check
            {
                ApplyItemTransform(Widget, -ItemSlideDistance);
            } // End if (Widget check)
        } // End for (CachedEntryWidgets loop)
        
        UE_LOG(LogDropListStagger, Log, TEXT("InitItemTransforms: Initialized %d items with slide distance %.1f"), CachedEntryWidgets.Num(), ItemSlideDistance);
    } // End if (Widgets cached)
}

void UDropListStagger::CacheEntryWidgets()
{
    CachedEntryWidgets.Empty();
    ItemStartTimes.Empty();
    
    if (!ContentList) // Reason: Null check
    {
        return;
    } // End if (ContentList check)

    TArray<UUserWidget*> DisplayedWidgets = ContentList->GetDisplayedEntryWidgets();
    
    for (int32 i = 0; i < DisplayedWidgets.Num(); i++) // Reason: Cache widgets and calculate start times
    {
        UUserWidget* Widget = DisplayedWidgets[i];
        if (Widget) // Reason: Valid widget check
        {
            CachedEntryWidgets.Add(Widget);
            float StartTime = static_cast<float>(i) * StaggerDelay; // [s] - Staggered start time
            ItemStartTimes.Add(StartTime);
            
            UE_LOG(LogDropListStagger, Log, TEXT("CacheEntryWidgets [%d]: Widget=%s, StartTime=%.3f"), i, *Widget->GetName(), StartTime);
        } // End if (Widget check)
    } // End for (DisplayedWidgets loop)
    
    UE_LOG(LogDropListStagger, Log, TEXT("CacheEntryWidgets: Cached %d widgets"), CachedEntryWidgets.Num());
}

//------------------------------------------------------------------------------
// overridden animation methods
//------------------------------------------------------------------------------

void UDropListStagger::StartExpand()
{
    if (!ContentSizeBox) // Reason: Null check
    {
        return;
    } // End if (ContentSizeBox check)

    TargetHeight = ComputeTargetHeight();
    StartHeight = ContentSizeBox->GetHeightOverride();
    AnimTime = 0.0f;
    bIsAnimating = true;

    // Clear cache - will populate after widgets generate
    CachedEntryWidgets.Empty();
    ItemStartTimes.Empty();

    UE_LOG(LogDropListStagger, Log, TEXT("StartExpand: Height %.1f→%.1f"), StartHeight, TargetHeight);

    GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UDropListStagger::TickAnim, 0.016f, true); // [16ms] - ~60fps update rate

    if (bAnimChevron) // Reason: Animate chevron down
    {
        AnimateChevronTo(180.0f);
    } // End if (Chevron animation)
}

void UDropListStagger::StartCollapse()
{
    if (!ContentSizeBox) // Reason: Null check
    {
        return;
    } // End if (ContentSizeBox check)

    StartHeight = ContentSizeBox->GetHeightOverride();
    TargetHeight = 0.0f;
    AnimTime = 0.0f;
    bIsAnimating = true;

    UE_LOG(LogDropListStagger, Log, TEXT("StartCollapse: Height %.1f→%.1f, Items=%d"), StartHeight, TargetHeight, CachedEntryWidgets.Num());

    GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UDropListStagger::TickAnim, 0.016f, true); // [16ms] - ~60fps update rate

    if (bAnimChevron) // Reason: Animate chevron up
    {
        AnimateChevronTo(0.0f);
    } // End if (Chevron animation)
}

void UDropListStagger::TickAnim()
{
    if (!bIsAnimating || !ContentSizeBox || !ContentList) // Reason: Animation active check
    {
        return;
    } // End if (Animation check)

    AnimTime += 0.016f; // [s] - 16ms frame time
    float Progress = FMath::Clamp(AnimTime / AnimDuration, 0.0f, 1.0f);

    float CurrentHeight = UAnimUtil::LerpCurved(StartHeight, TargetHeight, Progress, AnimCurve);
    ContentSizeBox->SetHeightOverride(CurrentHeight);

    if (bUseStaggerAnimation) // Reason: Apply stagger animation to each item
    {
        // Re-cache widgets every frame to catch newly generated ones
        TArray<UUserWidget*> CurrentWidgets = ContentList->GetDisplayedEntryWidgets();
        
        // Check if new widgets appeared
        if (CurrentWidgets.Num() > CachedEntryWidgets.Num()) // Reason: New widgets generated
        {
            UE_LOG(LogDropListStagger, Log, TEXT("TickAnim: New widgets detected (%d → %d)"), CachedEntryWidgets.Num(), CurrentWidgets.Num());
            
            // Add new widgets and calculate their start times
            for (int32 i = CachedEntryWidgets.Num(); i < CurrentWidgets.Num(); i++) // Reason: Process only new widgets
            {
                if (CurrentWidgets[i]) // Reason: Valid widget check
                {
                    CachedEntryWidgets.Add(CurrentWidgets[i]);
                    float StartTime = static_cast<float>(i) * StaggerDelay; // [s] - Staggered start time
                    ItemStartTimes.Add(StartTime);
                    
                    // Initialize widget with start position
                    if (bIsExpanded) // Reason: Expanding
                    {
                        ApplyItemTransform(CurrentWidgets[i], -ItemSlideDistance);
                    } // End if (Expand)
                    
                    UE_LOG(LogDropListStagger, Log, TEXT("  Added widget [%d]: %s, StartTime=%.3f"), i, *CurrentWidgets[i]->GetName(), StartTime);
                } // End if (Widget check)
            } // End for (New widgets loop)
        } // End if (New widgets)
        
        // Animate all cached widgets
        for (int32 i = 0; i < CachedEntryWidgets.Num(); i++) // Reason: Animate each item individually
        {
            if (!CachedEntryWidgets[i]) // Reason: Skip null widgets
            {
                continue;
            } // End if (Null check)
            
            float ItemProgress = GetItemProgress(i);
            float CurrentOffset = 0.0f;
            
            if (bIsExpanded) // Reason: Expand animation
            {
                CurrentOffset = UAnimUtil::LerpCurved(-ItemSlideDistance, 0.0f, ItemProgress, AnimCurve);
                ApplyItemTransform(CachedEntryWidgets[i], CurrentOffset);
                
                // Log first frame of each item's animation
                if (ItemProgress > 0.0f && ItemProgress < 0.05f) // Reason: First few frames
                {
                    UE_LOG(LogDropListStagger, Log, TEXT("  Item[%d] starting: Progress=%.3f, Offset=%.1f"), i, ItemProgress, CurrentOffset);
                } // End if (First frame)
            } // End if (Expand)
            else // Reason: Collapse animation
            {
                CurrentOffset = UAnimUtil::LerpCurved(0.0f, -ItemSlideDistance, ItemProgress, AnimCurve);
                ApplyItemTransform(CachedEntryWidgets[i], CurrentOffset);
            }
        } // End for (CachedEntryWidgets loop)
    } // End if (Stagger animation check)

    if (Progress >= 1.0f) // Reason: Animation complete
    {
        bIsAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(AnimTimer);
        
        if (!bIsExpanded) // Reason: Reset transforms after collapse
        {
            ResetItemTransforms();
        } // End if (Collapsed check)
        
        UE_LOG(LogDropListStagger, Log, TEXT("Animation complete at height %.1f, final widgets: %d"), CurrentHeight, CachedEntryWidgets.Num());
    } // End if (Complete check)
}

//------------------------------------------------------------------------------
// item animation helpers
//------------------------------------------------------------------------------

float UDropListStagger::GetItemProgress(int32 ItemIndex) const
{
    if (ItemIndex < 0 || ItemIndex >= ItemStartTimes.Num()) // Reason: Bounds check
    {
        return 0.0f;
    } // End if (Bounds check)

    float ItemStartTime = ItemStartTimes[ItemIndex];
    float ItemElapsedTime = AnimTime - ItemStartTime;
    
    if (ItemElapsedTime < 0.0f) // Reason: Item hasn't started yet
    {
        return 0.0f;
    } // End if (Not started)

    float ItemProgress = FMath::Clamp(ItemElapsedTime / AnimDuration, 0.0f, 1.0f);
    return ItemProgress;
}

void UDropListStagger::ApplyItemTransform(UUserWidget* Widget, float YOffset)
{
    if (!Widget) // Reason: Null check
    {
        return;
    } // End if (Widget check)

    FWidgetTransform Transform;
    Transform.Translation = FVector2D(0.0f, YOffset); // [px] - Vertical offset
    Transform.Scale = FVector2D(1.0f, 1.0f);
    Transform.Shear = FVector2D(0.0f, 0.0f);
    Transform.Angle = 0.0f;
    Widget->SetRenderTransform(Transform);
}

void UDropListStagger::ResetItemTransforms()
{
    for (UUserWidget* Widget : CachedEntryWidgets) // Reason: Reset all transforms
    {
        if (Widget) // Reason: Valid widget check
        {
            ApplyItemTransform(Widget, 0.0f);
        } // End if (Widget check)
    } // End for (CachedEntryWidgets loop)
    
    CachedEntryWidgets.Empty();
    ItemStartTimes.Empty();
}
