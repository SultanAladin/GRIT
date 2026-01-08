//CarouselWidget.cpp
#include "CarouselWidget.h"
#include "Components/HorizontalBoxSlot.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

UCarouselWidget::UCarouselWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , RootOverlay(nullptr)
    , PageTrack(nullptr)
    , LeftButton(nullptr)
    , RightButton(nullptr)
    , CurrentPageIndex(0)
    , TargetPageIndex(0)
    , CurrentOffset(0.0f)
    , TargetOffset(0.0f)
    , bIsAnimating(false)
{
}

void UCarouselWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindButtonEvents();

    // Reason: Apply initial page if set
    if (InitialPageIndex > 0 && InitialPageIndex < PageWidgets.Num())
    {
        CurrentPageIndex = InitialPageIndex;
        TargetPageIndex = InitialPageIndex;
        CurrentOffset = -InitialPageIndex * PageWidth;
        TargetOffset = CurrentOffset;
        UpdatePageTrackPosition();
    }

    UpdateButtonVisibility();
}

void UCarouselWidget::NativeDestruct()
{
    // Reason: Unbind button delegates
    if (LeftButton)
    {
        LeftButton->OnClicked.RemoveDynamic(this, &UCarouselWidget::HandleLeftButtonClicked);
    }

    if (RightButton)
    {
        RightButton->OnClicked.RemoveDynamic(this, &UCarouselWidget::HandleRightButtonClicked);
    }

    Super::NativeDestruct();
}

void UCarouselWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Reason: Handle smooth slide animation
    if (bIsAnimating)
    {
        // Interpolate towards target offset
        float InterpSpeed = (TransitionDuration > 0.0f) ? (1.0f / TransitionDuration) : 10.0f;
        CurrentOffset = FMath::FInterpTo(CurrentOffset, TargetOffset, InDeltaTime, InterpSpeed);

        UpdatePageTrackPosition();

        // Reason: Check if animation complete
        if (FMath::IsNearlyEqual(CurrentOffset, TargetOffset, 1.0f))
        {
            CurrentOffset = TargetOffset;
            CurrentPageIndex = TargetPageIndex;
            bIsAnimating = false;

            UpdatePageTrackPosition();
            UpdateButtonVisibility();

            OnPageChanged.Broadcast(CurrentPageIndex);
            OnPageChangedBP(CurrentPageIndex);
        }
    }
}

//------------------------------------------------------------------------------
//                                    Button bindings
//------------------------------------------------------------------------------

void UCarouselWidget::BindButtonEvents()
{
    if (LeftButton)
    {
        LeftButton->OnClicked.AddDynamic(this, &UCarouselWidget::HandleLeftButtonClicked);
    }

    if (RightButton)
    {
        RightButton->OnClicked.AddDynamic(this, &UCarouselWidget::HandleRightButtonClicked);
    }
}

void UCarouselWidget::HandleLeftButtonClicked()
{
    NavigateLeft();
}

void UCarouselWidget::HandleRightButtonClicked()
{
    NavigateRight();
}

//------------------------------------------------------------------------------
//                                    Page management
//------------------------------------------------------------------------------

void UCarouselWidget::AddPage(UUserWidget* PageWidget)
{
    if (!PageWidget || !PageTrack) { return; }

    PageWidgets.Add(PageWidget);
    PageTrack->AddChild(PageWidget);

    // Reason: Configure slot to fill page width
    if (UHorizontalBoxSlot* PageSlot = Cast<UHorizontalBoxSlot>(PageWidget->Slot))
    {
        PageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        PageSlot->SetHorizontalAlignment(HAlign_Fill);
        PageSlot->SetVerticalAlignment(VAlign_Fill);
    }

    UpdateButtonVisibility();
}

void UCarouselWidget::RemovePage(int32 PageIndex)
{
    if (PageIndex < 0 || PageIndex >= PageWidgets.Num()) { return; }

    UUserWidget* PageToRemove = PageWidgets[PageIndex];
    if (PageToRemove)
    {
        PageToRemove->RemoveFromParent();
    }

    PageWidgets.RemoveAt(PageIndex);

    // Reason: Adjust current index if needed
    if (CurrentPageIndex >= PageWidgets.Num() && PageWidgets.Num() > 0)
    {
        NavigateToPage(PageWidgets.Num() - 1);
    }
    else if (PageWidgets.Num() == 0)
    {
        CurrentPageIndex = 0;
        TargetPageIndex = 0;
        CurrentOffset = 0.0f;
        TargetOffset = 0.0f;
    }

    UpdateButtonVisibility();
}

void UCarouselWidget::ClearPages()
{
    for (UUserWidget* Page : PageWidgets)
    {
        if (Page)
        {
            Page->RemoveFromParent();
        }
    }

    PageWidgets.Empty();
    CurrentPageIndex = 0;
    TargetPageIndex = 0;
    CurrentOffset = 0.0f;
    TargetOffset = 0.0f;
    bIsAnimating = false;

    UpdatePageTrackPosition();
    UpdateButtonVisibility();
}

//------------------------------------------------------------------------------
//                                    Navigation
//------------------------------------------------------------------------------

void UCarouselWidget::NavigateToPage(int32 PageIndex)
{
    // Reason: Validate index
    if (PageIndex < 0 || PageIndex >= PageWidgets.Num()) { return; }
    if (PageIndex == CurrentPageIndex && !bIsAnimating) { return; }

    TargetPageIndex = PageIndex;
    TargetOffset = -PageIndex * PageWidth;
    bIsAnimating = true;
}

void UCarouselWidget::NavigateLeft()
{
    NavigateToPage(CurrentPageIndex - 1);
}

void UCarouselWidget::NavigateRight()
{
    NavigateToPage(CurrentPageIndex + 1);
}

//------------------------------------------------------------------------------
//                                    Getters
//------------------------------------------------------------------------------

UUserWidget* UCarouselWidget::GetCurrentPage() const
{
    if (CurrentPageIndex >= 0 && CurrentPageIndex < PageWidgets.Num())
    {
        return PageWidgets[CurrentPageIndex];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
//                                    Visual updates
//------------------------------------------------------------------------------

void UCarouselWidget::UpdateButtonVisibility()
{
    // Reason: Hide left button on first page
    if (LeftButton)
    {
        ESlateVisibility LeftVis = (CurrentPageIndex > 0) ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
        LeftButton->SetVisibility(LeftVis);
    }

    // Reason: Hide right button on last page
    if (RightButton)
    {
        ESlateVisibility RightVis = (CurrentPageIndex < PageWidgets.Num() - 1) ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
        RightButton->SetVisibility(RightVis);
    }
}

void UCarouselWidget::UpdatePageTrackPosition()
{
    if (PageTrack)
    {
        PageTrack->SetRenderTranslation(FVector2D(CurrentOffset, 0.0f));
    }
}
