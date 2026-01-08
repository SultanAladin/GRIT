//NavEntryExtended.cpp
#include "NavEntryExtended.h"
#include "Components/HorizontalBoxSlot.h"
#include "TimerManager.h"

UNavEntryExtended::UNavEntryExtended(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Icon(nullptr)
    , TextSizeBox(nullptr)
    , ExpandProgress(0.0f)
    , ExpandStartValue(0.0f)
    , ExpandTargetValue(0.0f)
    , bExpandAnimating(false)
{
}

void UNavEntryExtended::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] NativeConstruct - Index=%d, Icon=%s, TextSizeBox=%s"),
        GetEntryIndex(),
        Icon ? TEXT("VALID") : TEXT("NULL"),
        TextSizeBox ? TEXT("VALID") : TEXT("NULL"));

    // Reason: Configure slot properties for proper layout (Fill horizontally, Center vertically)
    if (Icon && Icon->Slot)
    {
        if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(Icon->Slot))
        {
            IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic)); // Auto size icon
            IconSlot->SetHorizontalAlignment(HAlign_Left);                  // Left align
            IconSlot->SetVerticalAlignment(VAlign_Center);                  // Center vertically
            IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));        // [px] Right padding for spacing
            UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] Configured Icon slot - Auto/Left/Center"));
        } // End if (IconSlot)
    } // End if (Icon)

    // Reason: TextSizeBox fills remaining space horizontally
    if (TextSizeBox && TextSizeBox->Slot)
    {
        if (UHorizontalBoxSlot* TextSlot = Cast<UHorizontalBoxSlot>(TextSizeBox->Slot))
        {
            TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));      // Fill remaining space
            TextSlot->SetHorizontalAlignment(HAlign_Fill);                  // Fill horizontally
            TextSlot->SetVerticalAlignment(VAlign_Center);                  // Center vertically
            UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] Configured TextSizeBox slot - Fill/Fill/Center"));
        } // End if (TextSlot)
    } // End if (TextSizeBox)

    // Reason: Start collapsed (dimension = 0)
    if (TextSizeBox)
    {
        if (bExpandHorizontally)
        {
            TextSizeBox->SetWidthOverride(0.0f); // [px]
            UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] Set initial width to 0 (horizontal mode)"));
        }
        else
        {
            TextSizeBox->SetHeightOverride(0.0f); // [px]
            UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] Set initial height to 0 (vertical mode)"));
        } // End if (orientation)
    } // End if (TextSizeBox)

    SyncColors();
}

void UNavEntryExtended::NativeDestruct()
{
    // Reason: Clear timer to prevent callbacks on destroyed widget
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ExpandTimer);
    } // End if (World)

    Super::NativeDestruct();
}

void UNavEntryExtended::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseEnter(MyGeometry, MouseEvent);

    UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] MouseEnter - Index=%d (NO auto-expand, NavBar controls it)"), GetEntryIndex());

    // Reason: NavBar will handle expansion through ForceExpansionState
    SyncColors();
}

void UNavEntryExtended::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseLeave(MouseEvent);

    UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] MouseLeave - Index=%d (NO auto-collapse, NavBar controls it)"), GetEntryIndex());

    // Reason: NavBar will handle collapse through ForceExpansionState
    SyncColors();
}

//------------------------------------------------------------------------------
//                              EXPANSION ANIMATION
//------------------------------------------------------------------------------

void UNavEntryExtended::PulseExpansion(bool bExpand)
{
    StartExpandAnimation(bExpand);
}

void UNavEntryExtended::ForceExpansionState(bool bExpand)
{
    UE_LOG(LogTemp, Log, TEXT("[NavEntryExtended] ForceExpansionState(%d) - Index=%d"), bExpand, GetEntryIndex());
    StartExpandAnimation(bExpand);
}

void UNavEntryExtended::StartExpandAnimation(bool bExpand)
{
    UWorld* World = GetWorld();
    if (!World) { return; }

    // Reason: Clear existing timer
    World->GetTimerManager().ClearTimer(ExpandTimer);

    ExpandStartValue = ExpandProgress;                // [0-1]
    ExpandTargetValue = bExpand ? 1.0f : 0.0f;        // [0-1]
    bExpandAnimating = true;
    float Elapsed = 0.0f;                             // [s]

    UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] StartExpandAnimation(%d) - Start=%.2f, Target=%.2f, Duration=%.2f"),
        bExpand, ExpandStartValue, ExpandTargetValue, ExpandDuration);

    // Reason: Use lambda for captured state
    World->GetTimerManager().SetTimer(
        ExpandTimer,
        FTimerDelegate::CreateLambda([this, Elapsed]() mutable
        {
            Elapsed += 0.016f; // [s] ~60fps

            float Alpha = FMath::Clamp(Elapsed / ExpandDuration, 0.0f, 1.0f); // [0-1]
            float T = CubicEaseInOut(Alpha);                                   // [0-1]

            ExpandProgress = FMath::Lerp(ExpandStartValue, ExpandTargetValue, T); // [0-1]
            ApplyExpansion(ExpandProgress);

            // Reason: Animation complete
            if (Alpha >= 1.0f)
            {
                bExpandAnimating = false;
                if (UWorld* W = GetWorld())
                {
                    W->GetTimerManager().ClearTimer(ExpandTimer);
                } // End if (World)

                UE_LOG(LogTemp, Warning, TEXT("[NavEntryExtended] Animation complete - Progress=%.2f, Width=%.0f"),
                    ExpandProgress, ExpandProgress * ExpandedWidth);
            } // End if (complete)
        }),
        0.016f, // [s] 60fps
        true    // looping
    );
}

void UNavEntryExtended::ApplyExpansion(float Ratio)
{
    // Reason: Apply dimension to TextSizeBox based on orientation
    if (TextSizeBox)
    {
        if (bExpandHorizontally)
        {
            float Width = Ratio * ExpandedWidth; // [px]
            TextSizeBox->SetWidthOverride(Width);
        }
        else
        {
            float Height = Ratio * ExpandedHeight; // [px]
            TextSizeBox->SetHeightOverride(Height);
        } // End if (orientation)
    } // End if (TextSizeBox)

    // Reason: Also fade text opacity with expansion
    if (EntryLabel)
    {
        FLinearColor LabelColor = EntryLabel->GetColorAndOpacity().GetSpecifiedColor();
        LabelColor.A = Ratio; // [0-1]
        EntryLabel->SetColorAndOpacity(FSlateColor(LabelColor));
    } // End if (EntryLabel)
}

void UNavEntryExtended::SyncColors()
{
    // Reason: Determine color based on state (Active > Hover > Idle)
    FLinearColor TargetColor;
    if (IsSelected())
    {
        TargetColor = IconActiveColor;
    }
    else if (ExpandTargetValue > 0.5f) // Expanding = hovered
    {
        TargetColor = IconHoverColor;
    }
    else
    {
        TargetColor = IconIdleColor;
    } // End if (state check)

    // Reason: Apply color to icon
    if (Icon)
    {
        Icon->SetColorAndOpacity(TargetColor);
        UE_LOG(LogTemp, Log, TEXT("[NavEntryExtended] SyncColors - Index=%d, Color=(%.2f,%.2f,%.2f)"),
            GetEntryIndex(), TargetColor.R, TargetColor.G, TargetColor.B);
    } // End if (Icon)
}

float UNavEntryExtended::CubicEaseInOut(float T)
{
    // Reason: Cubic ease-in-out for smooth animation
    return T < 0.5f
        ? 4.0f * T * T * T
        : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 3.0f) / 2.0f; // [-]
}
