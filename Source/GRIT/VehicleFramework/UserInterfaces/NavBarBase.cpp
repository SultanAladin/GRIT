//NavBarBase.cpp
#include "NavBarBase.h"
#include "NavEntryExtended.h"
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
    , bClickInProgress(false)
    , IndicatorStartPos(0.0f)
    , IndicatorTargetPos(0.0f)
    , bIsAnimatingIndicator(false)
{
}

void UNavBarBase::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // FIXED: Apply animation settings from PreConstruct to override Blueprint defaults
    UE_LOG(LogTemp, Warning, TEXT("[NavBarBase] NativePreConstruct - IndicatorAnimDuration=%.2fs, Curve=%d"), 
           IndicatorAnimDuration, (int32)IndicatorAnimCurve);
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
    } // End if (SlidingIndicator exists)

    // Reason: Spawn entries from config array
    if (DefaultEntryClass && EntryConfigs.Num() > 0)
    {
        for (const FNavEntryConfig& Config : EntryConfigs)
        {
            AddNavEntry(DefaultEntryClass, Config);
        } // End for (all configs)
    } // End if (configs exist)

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
        if (ClickTimeoutTimer.IsValid()) { GetWorld()->GetTimerManager().ClearTimer(ClickTimeoutTimer); }
        if (CollapseDelayTimer.IsValid()) { GetWorld()->GetTimerManager().ClearTimer(CollapseDelayTimer); }
    } // End if (timer cleanup)

    NavEntries.Empty();
    Super::NativeDestruct();
}

void UNavBarBase::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseLeave(MouseEvent);
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] NativeOnMouseLeave (whole navbar) - Setting HoveredIndex=-1, SelectedIndex=%d, Collapsing ALL"), SelectedIndex);
    
    HoveredIndex = -1;
    bNavBarHovered = false;
    
    // Reason: Cancel any pending timers
    if (GetWorld())
    {
        if (CollapseDelayTimer.IsValid()) { GetWorld()->GetTimerManager().ClearTimer(CollapseDelayTimer); }
    } // End if (timer cleanup)
    
    // Reason: Immediately collapse all entries when mouse leaves navbar
    ExpandAllEntries(false);
    
    ReturnIndicatorToSelected();
}

//------------------------------------------------------------------------------
//                              INITIALIZATION PIPELINE
//------------------------------------------------------------------------------

void UNavBarBase::TryCompleteInit()
{
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] TryCompleteInit called - bInitComplete=%d, RetryCount=%d"), bInitComplete, InitRetryCount);

    // Reason: Prevent infinite retry loop
    if (bInitComplete || InitRetryCount >= 30)
    {
        if (!bInitComplete && InitRetryCount >= 30)
        {
            UE_LOG(LogTemp, Error, TEXT("[NavBar] MAX RETRIES! Geometry never ready after 30 attempts."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[NavBar] TryCompleteInit EARLY EXIT - bInitComplete=%d, RetryCount=%d"), bInitComplete, InitRetryCount);
        }
        return;
    } // End if (retry limit)

    if (!RootOverlay || !SlidingIndicator || NavEntries.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[NavBar] TryCompleteInit FAIL - RootOverlay=%d, Indicator=%d, Entries=%d"), RootOverlay != nullptr, SlidingIndicator != nullptr, NavEntries.Num());
        return;
    } // End if (validation check)

    FVector2D OverlaySize = RootOverlay->GetCachedGeometry().GetLocalSize(); // [px]
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] TryCompleteInit - OverlaySize=(%.1f, %.1f)"), OverlaySize.X, OverlaySize.Y);

    // Reason: Geometry not ready - schedule retry
    if (OverlaySize.X <= 1.0f)
    {
        InitRetryCount++;
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] Geometry not ready, scheduling retry #%d"), InitRetryCount);

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(InitRetryTimer, this, &UNavBarBase::TryCompleteInit, 0.016f, false);
        } // End if (timer setup)

        return;
    } // End if (geometry not ready)

    // Reason: Geometry ready - calculate sizes
    RecalcEntrySizes();
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] After RecalcEntrySizes - bSizesCalculated=%d, CachedItemSize=%.1f"), bSizesCalculated, CachedItemSize);

    // Reason: Verify sizes calculated successfully
    if (!bSizesCalculated)
    {
        InitRetryCount++;
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] Sizes not calculated, scheduling retry #%d"), InitRetryCount);

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(InitRetryTimer, this, &UNavBarBase::TryCompleteInit, 0.016f, false);
        } // End if (timer setup)

        return;
    } // End if (sizes not calculated)

    // Reason: Snap indicator to initial position without animation
    int32 TargetIndex = FMath::Clamp(DefaultSelectedIndex, 0, NavEntries.Num() - 1);
    float SnapPos = CalculateIndicatorPosition(TargetIndex); // [px]

    UE_LOG(LogTemp, Warning, TEXT("[NavBar] INIT COMPLETE - DefaultSelectedIndex=%d, TargetIndex=%d, SnapPos=%.1f"), DefaultSelectedIndex, TargetIndex, SnapPos);

    UpdateIndicatorTransform(SnapPos);

    // Reason: Set init complete BEFORE calling SetSelectedIndex so animation works
    bInitComplete = true;

    // Reason: Update selection state and text colors
    SetSelectedIndex(TargetIndex);

    if (SlidingIndicator)
    {
        SlidingIndicator->SetVisibility(ESlateVisibility::HitTestInvisible);
    } // End if (SlidingIndicator exists)

    UE_LOG(LogTemp, Warning, TEXT("[NavBar] === INITIALIZATION COMPLETE === SelectedIndex=%d"), SelectedIndex);
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
        CachedItemSize = ItemWidth;
        SlidingIndicator->SetDesiredSizeOverride(FVector2D(ItemWidth, IndicatorThickness));
    }
    else
    {
        float ItemHeight = OverlaySize.Y / N; // [px]
        CachedItemSize = ItemHeight;
        SlidingIndicator->SetDesiredSizeOverride(FVector2D(IndicatorThickness, ItemHeight));
    } // End if (orientation)

    bSizesCalculated = true;
}

//------------------------------------------------------------------------------
//                              ENTRY MANAGEMENT
//------------------------------------------------------------------------------

void UNavBarBase::AddNavEntry(TSubclassOf<UNavEntry> EntryClass, const FNavEntryConfig& Config)
{
    if (!EntryClass) { return; }

    UNavEntry* NewEntry = CreateWidget<UNavEntry>(this, EntryClass);
    if (!NewEntry) { return; }

    int32 EntryIndex = NavEntries.Num();

    // Reason: Initialize entry with provided config
    NewEntry->InitEntry(Config, EntryIndex);
    NewEntry->OnEntryClicked.AddDynamic(this, &UNavBarBase::OnEntryClicked);
    NewEntry->OnEntryPressed.AddDynamic(this, &UNavBarBase::OnEntryPressed);
    NewEntry->OnEntryReleased.AddDynamic(this, &UNavBarBase::OnEntryReleased);
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
    } // End if (orientation)

    NavEntries.Add(NewEntry);
}

void UNavBarBase::AddNavEntrySimple(TSubclassOf<UNavEntry> EntryClass)
{
    // Reason: Create default config with auto-generated label
    FNavEntryConfig DefaultConfig;
    int32 EntryIndex = NavEntries.Num();
    DefaultConfig.Label = FText::FromString(FString::Printf(TEXT("Entry %d"), EntryIndex));
    DefaultConfig.Data = FString::FromInt(EntryIndex);

    AddNavEntry(EntryClass, DefaultConfig);
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
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] ========== SetSelectedIndex(%d) CALLED =========="), Index);
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] BEFORE: SelectedIndex=%d, HoveredIndex=%d, bSizesCalculated=%d, bInitComplete=%d"), SelectedIndex, HoveredIndex, bSizesCalculated, bInitComplete);

    if (!NavEntries.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Error, TEXT("[NavBar] SetSelectedIndex REJECTED - Index %d not valid (Entries=%d)"), Index, NavEntries.Num());
        return;
    } // End if (validation check)

    int32 OldSelectedIndex = SelectedIndex;
    SelectedIndex = Index;

    UE_LOG(LogTemp, Warning, TEXT("[NavBar] Selection changed: %d -> %d"), OldSelectedIndex, SelectedIndex);

    // Reason: Update all entries
    for (int32 i = 0; i < NavEntries.Num(); i++)
    {
        if (NavEntries[i])
        {
            bool bShouldSelect = (i == SelectedIndex);
            UE_LOG(LogTemp, Log, TEXT("[NavBar]   Entry[%d] ToggleSelection(%d)"), i, bShouldSelect);
            NavEntries[i]->ToggleSelection(bShouldSelect);
        } // End if (entry exists)
    } // End for (all entries)

    // Reason: Only animate if geometry is ready
    if (bSizesCalculated)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] Calling AnimateIndicatorToIndex(%d) - CachedItemSize=%.1f"), SelectedIndex, CachedItemSize);
        AnimateIndicatorToIndex(SelectedIndex);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[NavBar] SKIPPING AnimateIndicatorToIndex - bSizesCalculated=FALSE!"));
    } // End if (sizes ready)

    UE_LOG(LogTemp, Warning, TEXT("[NavBar] AFTER SetSelectedIndex: SelectedIndex=%d"), SelectedIndex);
    OnSelectionChanged.Broadcast(SelectedIndex, NavEntries[SelectedIndex]->GetEntryData());
}

//------------------------------------------------------------------------------
//                              INDICATOR ANIMATION
//------------------------------------------------------------------------------

void UNavBarBase::AnimateIndicatorToIndex(int32 Index)
{
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] AnimateIndicatorToIndex(%d) called"), Index);

    if (!SlidingIndicator || Index < 0 || Index >= NavEntries.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("[NavBar] AnimateIndicatorToIndex REJECTED - Indicator=%d, Index=%d, Entries=%d"), SlidingIndicator != nullptr, Index, NavEntries.Num());
        return;
    } // End if (validation check)

    if (!bSizesCalculated || CachedItemSize <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] Sizes not ready, recalculating..."));
        RecalcEntrySizes();

        // Reason: Still not ready after recalc
        if (!bSizesCalculated || CachedItemSize <= 0.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("[NavBar] AnimateIndicatorToIndex ABORT - Still no sizes after recalc"));
            return;
        } // End if (size check)
    } // End if (sizes not ready)

    FVector2D CurrentTranslation = SlidingIndicator->GetRenderTransform().Translation;
    IndicatorStartPos = bIsHorizontal ? CurrentTranslation.X : CurrentTranslation.Y; // [px]

    IndicatorTargetPos = CalculateIndicatorPosition(Index); // [px]

    UE_LOG(LogTemp, Warning, TEXT("[NavBar] Animation: StartPos=%.1f -> TargetPos=%.1f (Index=%d, ItemSize=%.1f)"), IndicatorStartPos, IndicatorTargetPos, Index, CachedItemSize);

    // Reason: Already at target position
    if (FMath::IsNearlyEqual(IndicatorStartPos, IndicatorTargetPos, 0.1f))
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] AnimateIndicatorToIndex SKIP - Already at target position"));
        return;
    } // End if (position check)

    IndicatorAnimElapsed = 0.0f; // [s]
    bIsAnimatingIndicator = true;

    UE_LOG(LogTemp, Warning, TEXT("[NavBar] Starting animation timer..."));

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(IndicatorAnimTimer, this, &UNavBarBase::TickIndicatorAnimation, 0.016f, true);
    } // End if (timer setup)
}

void UNavBarBase::TickIndicatorAnimation()
{
    if (!bIsAnimatingIndicator) { return; }

    IndicatorAnimElapsed += 0.016f; // [s]

    float Alpha = FMath::Clamp(IndicatorAnimElapsed / IndicatorAnimDuration, 0.0f, 1.0f); // [-]
    
    // FIXED: Use UIToolkit EFlowCurve instead of hardcoded ease-in-out
    float EasedAlpha = UUIToolkit::EvalFlowCurve(IndicatorAnimCurve, Alpha);

    float CurrentPos = FMath::Lerp(IndicatorStartPos, IndicatorTargetPos, EasedAlpha); // [px]
    UpdateIndicatorTransform(CurrentPos);

    // Reason: Stop animation when complete
    if (IndicatorAnimElapsed >= IndicatorAnimDuration)
    {
        bIsAnimatingIndicator = false;
        
        if (GetWorld() && IndicatorAnimTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(IndicatorAnimTimer);
        } // End if (timer cleanup)
        
        UE_LOG(LogTemp, Log, TEXT("[NavBarBase] Indicator animation complete"));
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
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] >>>>>> OnEntryClicked <<<<<<"));

    if (!ClickedEntry)
    {
        UE_LOG(LogTemp, Error, TEXT("[NavBar] OnEntryClicked - ClickedEntry is NULL!"));
        return;
    } // End if (validation check)

    int32 ClickedIndex = ClickedEntry->GetEntryIndex();
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] OnEntryClicked - ClickedIndex=%d, Current SelectedIndex=%d, HoveredIndex=%d"), ClickedIndex, SelectedIndex, HoveredIndex);

    // Reason: Clear click timeout timer
    if (GetWorld() && ClickTimeoutTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ClickTimeoutTimer);
    } // End if (timer cleanup)
    
    // Reason: Clear return timer
    if (GetWorld() && ReturnToSelectedTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ReturnToSelectedTimer);
    } // End if (timer cleanup)

    UE_LOG(LogTemp, Warning, TEXT("[NavBar] Calling SetSelectedIndex(%d)..."), ClickedIndex);
    SetSelectedIndex(ClickedIndex);
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] OnEntryClicked COMPLETE - SelectedIndex is now %d"), SelectedIndex);
    
    // Reason: Reset click state immediately after selection
    bClickInProgress = false;
}

void UNavBarBase::OnEntryPressed(UNavEntry* PressedEntry)
{
    if (!PressedEntry) { return; }
    
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] OnEntryPressed - Index=%d, Setting bClickInProgress=true"), PressedEntry->GetEntryIndex());
    bClickInProgress = true;
    
    // Reason: Clear return timer if active
    if (GetWorld() && ReturnToSelectedTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ReturnToSelectedTimer);
    } // End if (timer cleanup)
    
    // Reason: Start timeout fallback in case MouseButtonUp never fires
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(ClickTimeoutTimer, this, &UNavBarBase::ResetClickState, 0.5f, false);
    } // End if (timeout setup)
}

void UNavBarBase::OnEntryReleased(UNavEntry* ReleasedEntry)
{
    if (!ReleasedEntry) { return; }
    
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] OnEntryReleased - Index=%d, Setting bClickInProgress=false"), ReleasedEntry->GetEntryIndex());
    
    // Reason: Clear timeout timer since click completed normally
    if (GetWorld() && ClickTimeoutTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ClickTimeoutTimer);
    } // End if (timer cleanup)
    
    bClickInProgress = false;
}

void UNavBarBase::ResetClickState()
{
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] ResetClickState - TIMEOUT! Force clearing bClickInProgress"));
    bClickInProgress = false;
    
    // Reason: Return indicator to selected after timeout
    if (HoveredIndex == -1 && NavEntries.IsValidIndex(SelectedIndex))
    {
        AnimateIndicatorToIndex(SelectedIndex);
    } // End if (return indicator)
}

void UNavBarBase::AnimateIndicatorToEntry(UNavEntry* Entry)
{
    if (Entry)
    {
        int32 EntryIndex = Entry->GetEntryIndex();
        UE_LOG(LogTemp, Log, TEXT("[NavBar] AnimateIndicatorToEntry - EntryIndex=%d (hover), SelectedIndex=%d"), EntryIndex, SelectedIndex);

        HoveredIndex = EntryIndex;
        bNavBarHovered = true;

        // Reason: Cancel any pending collapse timer
        if (GetWorld() && CollapseDelayTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(CollapseDelayTimer);
        } // End if (timer cleanup)

        // Reason: Cancel any pending return-to-selected timer
        if (GetWorld() && ReturnToSelectedTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(ReturnToSelectedTimer);
        } // End if (timer cleanup)

        // Reason: Expand ALL entries together
        ExpandAllEntries(true);

        AnimateIndicatorToIndex(HoveredIndex);
    } // End if (Entry exists)
}

void UNavBarBase::OnEntryUnhovered(UNavEntry* Entry)
{
    if (!Entry)
    {
        UE_LOG(LogTemp, Error, TEXT("[NavBar] OnEntryUnhovered - Entry is NULL!"));
        return;
    } // End if (validation check)

    int32 EntryIndex = Entry->GetEntryIndex();
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] OnEntryUnhovered - EntryIndex=%d, HoveredIndex=%d, SelectedIndex=%d"), EntryIndex, HoveredIndex, SelectedIndex);

    // Reason: Verify this is the entry we were actually tracking
    if (EntryIndex == HoveredIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] OnEntryUnhovered - Clearing HoveredIndex, scheduling delayed collapse check"));
        HoveredIndex = -1;

        // Reason: Schedule delayed collapse to allow mouse movement between entries
        ScheduleCollapseAll();

        // Reason: Short delay to catch fast mouse movements across entries
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(ReturnToSelectedTimer, this, &UNavBarBase::ReturnIndicatorToSelected, 0.05f, false);
        } // End if (timer setup)
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] OnEntryUnhovered - IGNORED (EntryIndex %d != HoveredIndex %d)"), EntryIndex, HoveredIndex);
    } // End if (Hover Check)
}

void UNavBarBase::ReturnIndicatorToSelected()
{
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] ReturnIndicatorToSelected called - HoveredIndex=%d, SelectedIndex=%d, bClickInProgress=%d"), HoveredIndex, SelectedIndex, bClickInProgress);

    // Reason: Don't return indicator during active click
    if (bClickInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] ReturnIndicatorToSelected - SKIPPED (Click in progress)"));
        return;
    } // End if (click check)

    // Reason: Target the NEW SelectedIndex confirmed by the click
    if (HoveredIndex == -1 && NavEntries.IsValidIndex(SelectedIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] ReturnIndicatorToSelected - Animating to SelectedIndex=%d"), SelectedIndex);
        AnimateIndicatorToIndex(SelectedIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavBar] ReturnIndicatorToSelected - SKIPPED (HoveredIndex=%d or SelectedIndex=%d invalid)"), HoveredIndex, SelectedIndex);
    } // End if (Return Logic)
}

//------------------------------------------------------------------------------
//                         SYNCHRONIZED ENTRY EXPANSION
//------------------------------------------------------------------------------

void UNavBarBase::ExpandAllEntries(bool bExpand)
{
    UE_LOG(LogTemp, Warning, TEXT("[NavBar] ExpandAllEntries(%d) - Commanding ALL %d entries"), bExpand, NavEntries.Num());
    
    // Reason: Cast to NavEntryExtended and force expansion state
    for (UNavEntry* Entry : NavEntries)
    {
        if (UNavEntryExtended* ExtendedEntry = Cast<UNavEntryExtended>(Entry))
        {
            ExtendedEntry->ForceExpansionState(bExpand);
        } // End if (ExtendedEntry)
    } // End for (all entries)
}

void UNavBarBase::ScheduleCollapseAll()
{
    // Reason: Delay collapse to catch fast mouse movements between entries
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            CollapseDelayTimer,
            FTimerDelegate::CreateLambda([this]()
            {
                // Reason: Only collapse if no entry is currently hovered
                if (HoveredIndex == -1 && !bNavBarHovered)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[NavBar] Delayed collapse executed - Collapsing ALL entries"));
                    ExpandAllEntries(false);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[NavBar] Delayed collapse CANCELLED - HoveredIndex=%d, bNavBarHovered=%d"), HoveredIndex, bNavBarHovered);
                } // End if (collapse check)
            }),
            0.1f, // [s]
            false
        );
    } // End if (schedule timer)
}