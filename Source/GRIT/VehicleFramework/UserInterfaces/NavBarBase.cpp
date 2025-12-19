//NavBarBase.cpp
#include "NavBarBase.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "TimerManager.h"

UNavBarBase::UNavBarBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , RootOverlay(nullptr)
    , NavContainer_H(nullptr)
    , NavContainer_V(nullptr)
    , SlidingIndicator(nullptr)
    , bIsHorizontal(true)
    , SelectedIndex(-1)
    , HoveredIndex(-1)
    , IndicatorAnimElapsed(0.0f)
    , bSizesCalculated(false)
    , bInitComplete(false)
    , InitRetryCount(0)
    , CachedItemSize(0.0f)
    , IndicatorStartPos(0.0f)
    , IndicatorTargetPos(0.0f)
    , bIsAnimatingIndicator(false)
{
}

void UNavBarBase::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UNavBarBase::NativeConstruct()
{
    Super::NativeConstruct();

    DetermineOrientation();
    ConfigureSlotProperties();

    if (SlidingIndicator)
    {
        SlidingIndicator->SetColorAndOpacity(IndicatorColor);
        SlidingIndicator->SetRenderTransformPivot(FVector2D(0.0f, 0.0f));
        SlidingIndicator->SetVisibility(ESlateVisibility::Hidden);
    }

    for (const TSubclassOf<UNavEntry>& EntryClass : DefaultEntries)
    {
        if (EntryClass) { AddNavEntry(EntryClass); }
    }

    // Reason: Start retry system to wait for geometry
    if (GetWorld() && NavEntries.Num() > 0)
    {
        TryCompleteInit();
    } // End if (world valid)
}

void UNavBarBase::NativeDestruct()
{
    // Reason: Clean up animation timers
    if (GetWorld())
    {
        if (IndicatorAnimTimer.IsValid()) { GetWorld()->GetTimerManager().ClearTimer(IndicatorAnimTimer); }
        if (ReturnToSelectedTimer.IsValid()) { GetWorld()->GetTimerManager().ClearTimer(ReturnToSelectedTimer); }
        if (InitRetryTimer.IsValid()) { GetWorld()->GetTimerManager().ClearTimer(InitRetryTimer); }
    } // End if (timer cleanup)

    NavEntries.Empty();
    Super::NativeDestruct();
}

void UNavBarBase::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseLeave(MouseEvent);
    HoveredIndex = -1;
    ReturnIndicatorToSelected();
}

//------------------------------------------------------------------------------
//                              INITIALIZATION PIPELINE
//------------------------------------------------------------------------------

void UNavBarBase::TryCompleteInit()
{
    // Reason: Prevent infinite retry loop
    if (bInitComplete || InitRetryCount >= 10) { return; }

    if (!RootOverlay || !SlidingIndicator || NavEntries.Num() == 0) { return; }

    FVector2D OverlaySize = RootOverlay->GetCachedGeometry().GetLocalSize(); // [px]

    // Reason: Geometry not ready - schedule retry
    if (OverlaySize.X <= 1.0f)
    {
        InitRetryCount++;
        
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(InitRetryTimer, this, &UNavBarBase::TryCompleteInit, 0.033f, false);
        }
        
        return;
    } // End if (geometry not ready)

    // Reason: Geometry ready - calculate sizes
    RecalcEntrySizes();
    
    // Reason: Verify sizes calculated successfully
    if (!bSizesCalculated)
    {
        InitRetryCount++;
        
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(InitRetryTimer, this, &UNavBarBase::TryCompleteInit, 0.033f, false);
        }
        
        return;
    } // End if (sizes not calculated)
    
    // Reason: Snap indicator to initial position without animation
    int32 TargetIndex = FMath::Clamp(DefaultSelectedIndex, 0, NavEntries.Num() - 1);
    float SnapPos = CalculateIndicatorPosition(TargetIndex); // [px]
    
    UpdateIndicatorTransform(SnapPos);
    
    // Reason: Set init complete BEFORE calling SetSelectedIndex so animation works
    bInitComplete = true;
    
    // Reason: Update selection state and text colors
    SetSelectedIndex(TargetIndex);

    if (SlidingIndicator)
    {
        SlidingIndicator->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

//------------------------------------------------------------------------------
//                              ORIENTATION DETECTION
//------------------------------------------------------------------------------

void UNavBarBase::DetermineOrientation()
{
    bool bHasHorizontal = (NavContainer_H != nullptr);
    bool bHasVertical = (NavContainer_V != nullptr);

    // Reason: Conflict resolution - prefer horizontal
    if (bHasHorizontal && bHasVertical)
    {
        UE_LOG(LogTemp, Error, TEXT("NavBarBase: BOTH containers bound! Using Horizontal."));
        bIsHorizontal = true;
    }
    else if (!bHasHorizontal && !bHasVertical)
    {
        UE_LOG(LogTemp, Error, TEXT("NavBarBase: NO containers bound!"));
        bIsHorizontal = true;
    }
    else
    {
        bIsHorizontal = bHasHorizontal;
        UE_LOG(LogTemp, Warning, TEXT("NavBarBase: Using %s layout"), bIsHorizontal ? TEXT("HORIZONTAL") : TEXT("VERTICAL"));
    } // End if (orientation check)
}

void UNavBarBase::ConfigureSlotProperties()
{
    if (!SlidingIndicator) { return; }

    UOverlaySlot* IndicatorSlot = Cast<UOverlaySlot>(SlidingIndicator->Slot);
    
    // Reason: Position indicator based on orientation
    if (IndicatorSlot)
    {
        if (bIsHorizontal)
        {
            IndicatorSlot->SetHorizontalAlignment(HAlign_Left);
            IndicatorSlot->SetVerticalAlignment(VAlign_Bottom);
        }
        else
        {
            IndicatorSlot->SetHorizontalAlignment(HAlign_Left);
            IndicatorSlot->SetVerticalAlignment(VAlign_Top);
        } // End if (orientation)
    } // End if (IndicatorSlot exists)
}

//------------------------------------------------------------------------------
//                              ENTRY SIZING
//------------------------------------------------------------------------------

void UNavBarBase::RecalcEntrySizes()
{
    if (!RootOverlay || !SlidingIndicator || NavEntries.Num() == 0) { return; }

    FVector2D OverlaySize = RootOverlay->GetCachedGeometry().GetLocalSize(); // [px]

    // Reason: Geometry not ready yet
    if (OverlaySize.X <= 1.0f) { return; }

    float N = (float)NavEntries.Num(); // [-]

    if (bIsHorizontal)
    {
        float ItemWidth = OverlaySize.X / N; // [px]
        CachedItemSize = ItemWidth; // Cache for position calculations
        SlidingIndicator->SetDesiredSizeOverride(FVector2D(ItemWidth, IndicatorThickness));
    }
    else
    {
        float ItemHeight = OverlaySize.Y / N; // [px]
        CachedItemSize = ItemHeight; // Cache for position calculations
        SlidingIndicator->SetDesiredSizeOverride(FVector2D(IndicatorThickness, ItemHeight));
    }

    bSizesCalculated = true;
}

//------------------------------------------------------------------------------
//                              ENTRY MANAGEMENT
//------------------------------------------------------------------------------

void UNavBarBase::AddNavEntry(TSubclassOf<UNavEntry> EntryClass)
{
    if (!EntryClass) { return; }

    UNavEntry* NewEntry = CreateWidget<UNavEntry>(this, EntryClass);
    if (!NewEntry) { return; }

    int32 EntryIndex = NavEntries.Num();

    NewEntry->InitEntry(FText::AsNumber(EntryIndex + 1), FString::FromInt(EntryIndex), EntryIndex);
    NewEntry->OnEntryClicked.AddDynamic(this, &UNavBarBase::OnEntryClicked);
    NewEntry->OnEntryHovered.AddDynamic(this, &UNavBarBase::AnimateIndicatorToEntry);
    NewEntry->OnEntryUnhovered.AddDynamic(this, &UNavBarBase::OnEntryUnhovered);

    if (bIsHorizontal && NavContainer_H)
    {
        UHorizontalBoxSlot* EntrySlot = NavContainer_H->AddChildToHorizontalBox(NewEntry);
        EntrySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        EntrySlot->SetHorizontalAlignment(HAlign_Fill);
        EntrySlot->SetVerticalAlignment(VAlign_Fill);
        EntrySlot->SetPadding(EntryPadding);
    }
    else if (!bIsHorizontal && NavContainer_V)
    {
        UVerticalBoxSlot* EntrySlot = NavContainer_V->AddChildToVerticalBox(NewEntry);
        EntrySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        EntrySlot->SetHorizontalAlignment(HAlign_Fill);
        EntrySlot->SetVerticalAlignment(VAlign_Fill);
        EntrySlot->SetPadding(EntryPadding);
    }

    NavEntries.Add(NewEntry);
}

void UNavBarBase::ClearEntries()
{
    if (NavContainer_H) { NavContainer_H->ClearChildren(); }
    if (NavContainer_V) { NavContainer_V->ClearChildren(); }

    NavEntries.Empty();
    SelectedIndex = -1;
    HoveredIndex = -1;
    bInitComplete = false;
    bSizesCalculated = false;
    InitRetryCount = 0;
    CachedItemSize = 0.0f;
}

void UNavBarBase::SetSelectedIndex(int32 Index)
{
    if (!NavEntries.IsValidIndex(Index)) { return; }

    SelectedIndex = Index;

    // Reason: Update all entries; 1-line logic for selection state
    for (int32 i = 0; i < NavEntries.Num(); i++) { if (NavEntries[i]) NavEntries[i]->ToggleSelection(i == SelectedIndex); }

    // Reason: Only animate if geometry is ready
    if (bSizesCalculated) { AnimateIndicatorToIndex(SelectedIndex); }

    OnSelectionChanged.Broadcast(SelectedIndex, NavEntries[SelectedIndex]->GetEntryData());
} // End if (SetSelectedIndex)

//------------------------------------------------------------------------------
//                              INDICATOR ANIMATION
//------------------------------------------------------------------------------

void UNavBarBase::AnimateIndicatorToIndex(int32 Index)
{
    if (!SlidingIndicator || Index < 0 || Index >= NavEntries.Num()) { return; }

    if (!bSizesCalculated || CachedItemSize <= 0.0f)
    {
        RecalcEntrySizes();
        
        // Reason: Still not ready after recalc
        if (!bSizesCalculated || CachedItemSize <= 0.0f) { return; }
    } // End if (sizes not ready)

    FVector2D CurrentTranslation = SlidingIndicator->GetRenderTransform().Translation;
    IndicatorStartPos = bIsHorizontal ? CurrentTranslation.X : CurrentTranslation.Y; // [px]

    IndicatorTargetPos = CalculateIndicatorPosition(Index); // [px]

    // Reason: Already at target position
    if (FMath::IsNearlyEqual(IndicatorStartPos, IndicatorTargetPos, 0.1f)) { return; }

    IndicatorAnimElapsed = 0.0f; // [s]
    bIsAnimatingIndicator = true;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(IndicatorAnimTimer, this, &UNavBarBase::TickIndicatorAnimation, 0.016f, true);
    }
}

void UNavBarBase::TickIndicatorAnimation()
{
    if (!bIsAnimatingIndicator) { return; }

    IndicatorAnimElapsed += 0.016f; // [s]

    float Alpha = FMath::Clamp(IndicatorAnimElapsed / IndicatorAnimDuration, 0.0f, 1.0f); // [-]
    
    // Reason: Apply ease in-out curve
    Alpha = Alpha < 0.5f ? (2.0f * Alpha * Alpha) : (1.0f - FMath::Pow(-2.0f * Alpha + 2.0f, 2.0f) / 2.0f); // [-]

    float CurrentPos = FMath::Lerp(IndicatorStartPos, IndicatorTargetPos, Alpha); // [px]
    UpdateIndicatorTransform(CurrentPos);

    // Reason: Stop animation when complete
    if (IndicatorAnimElapsed >= IndicatorAnimDuration)
    {
        bIsAnimatingIndicator = false;
        
        if (GetWorld() && IndicatorAnimTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(IndicatorAnimTimer);
        } // End if (timer cleanup)
    } // End if (animation complete)
}

float UNavBarBase::CalculateIndicatorPosition(int32 Index)
{
    if (NavEntries.Num() == 0 || CachedItemSize <= 0.0f) { return 0.0f; }

    // Reason: Use cached size to avoid geometry recalculation issues
    return Index * CachedItemSize; // [px]
}

void UNavBarBase::UpdateIndicatorSizeAndPosition()
{
    if (!SlidingIndicator || NavEntries.Num() == 0) { return; }

    if (bIsHorizontal && NavContainer_H)
    {
        float ContainerWidth = NavContainer_H->GetCachedGeometry().GetLocalSize().X; // [px]
        float ItemWidth = ContainerWidth / NavEntries.Num(); // [px]
        
        SlidingIndicator->SetDesiredSizeOverride(FVector2D(ItemWidth, IndicatorThickness));
    }
    else if (!bIsHorizontal && NavContainer_V)
    {
        float ContainerHeight = NavContainer_V->GetCachedGeometry().GetLocalSize().Y; // [px]
        float ItemHeight = ContainerHeight / NavEntries.Num(); // [px]
        
        SlidingIndicator->SetDesiredSizeOverride(FVector2D(IndicatorThickness, ItemHeight));
    } // End if (orientation)
}

void UNavBarBase::UpdateIndicatorTransform(float Position)
{
    if (!SlidingIndicator) { return; }

    FWidgetTransform Transform;
    
    if (bIsHorizontal)
    {
        Transform.Translation = FVector2D(Position, 0.0f); // [px]
    }
    else
    {
        Transform.Translation = FVector2D(0.0f, Position); // [px]
    } // End if (orientation)

    SlidingIndicator->SetRenderTransform(Transform);
}

//------------------------------------------------------------------------------
//                              EVENT HANDLERS
//------------------------------------------------------------------------------

void UNavBarBase::OnEntryClicked(UNavEntry* ClickedEntry)
{
    if (!ClickedEntry) { return; }

    // Reason: DON'T reset HoveredIndex here; the mouse is still over the button!
    // Just clear the return timer if it was pending from a weird jitter
    if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(ReturnToSelectedTimer); }

    SetSelectedIndex(ClickedEntry->GetEntryIndex());
} // End if (OnEntryClicked)

void UNavBarBase::AnimateIndicatorToEntry(UNavEntry* Entry)
{
    if (Entry)
    {
        HoveredIndex = Entry->GetEntryIndex();
        
        // Reason: Cancel any pending return-to-selected timer
        if (GetWorld() && ReturnToSelectedTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(ReturnToSelectedTimer);
        } // End if (timer cleanup)
        
        AnimateIndicatorToIndex(HoveredIndex);
    }
}

void UNavBarBase::OnEntryUnhovered(UNavEntry* Entry)
{
    // Reason: Verify this is the entry we were actually tracking
    if (Entry && Entry->GetEntryIndex() == HoveredIndex)
    {
        HoveredIndex = -1;

        // Reason: Short delay to catch fast mouse movements across entries [s]
        if (GetWorld()) { GetWorld()->GetTimerManager().SetTimer(ReturnToSelectedTimer, this, &UNavBarBase::ReturnIndicatorToSelected, 0.05f, false); }
    } // End if (Hover Check)
} // End if (OnEntryUnhovered)

void UNavBarBase::ReturnIndicatorToSelected()
{
    // Reason: Target the NEW SelectedIndex confirmed by the click
    if (HoveredIndex == -1 && NavEntries.IsValidIndex(SelectedIndex))
    {
        AnimateIndicatorToIndex(SelectedIndex);
    } // End if (Return Logic)
}
