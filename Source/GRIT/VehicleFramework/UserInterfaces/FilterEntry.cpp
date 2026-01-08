//FilterEntry.cpp
#include "FilterEntry.h"
#include "Components/BorderSlot.h"
#include "TimerManager.h"

UFilterEntry::UFilterEntry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ChipBorder(nullptr)
    , FilterLabel(nullptr)
    , RemoveButton(nullptr)
    , OriginalIndex(-1)
    , ElapsedTime(0.0f)
    , bIsAnimating(false)
{
}

void UFilterEntry::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind remove button
    if (RemoveButton)
    {
        RemoveButton->OnClicked.AddDynamic(this, &UFilterEntry::OnRemoveButtonClicked);
    }

    // Set initial styling
    if (ChipBorder)
    {
        ChipBorder->SetBrushColor(ChipColor);
    }

    if (FilterLabel)
    {
        FilterLabel->SetColorAndOpacity(FSlateColor(TextColor));

        FSlateFontInfo FontInfo = FilterLabel->GetFont();
        FontInfo.Size = FontSize;
        FilterLabel->SetFont(FontInfo);
    }

    // Start with collapsed state for pop-in animation
    SetRenderTransform(FWidgetTransform());
    SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}

void UFilterEntry::NativeDestruct()
{
    if (GetWorld() && AnimTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(AnimTimer);
    }

    Super::NativeDestruct();
}

void UFilterEntry::InitFilter(const FString& FilterData, int32 FilterOriginalIndex)
{
    DataPayload = FilterData;
    OriginalIndex = FilterOriginalIndex;

    if (FilterLabel)
    {
        FilterLabel->SetText(FText::FromString(DataPayload));
    }
}

void UFilterEntry::PlayPopInAnimation()
{
    bIsAnimating = true;
    ElapsedTime = 0.0f;

    // Start invisible and small
    SetRenderOpacity(0.0f);
    FWidgetTransform StartTransform;
    StartTransform.Scale = FVector2D(0.5f, 0.5f);
    SetRenderTransform(StartTransform);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            AnimTimer,
            this,
            &UFilterEntry::AnimatePopIn,
            0.016f,
            true
        );
    }
}

void UFilterEntry::AnimatePopIn()
{
    if (!bIsAnimating) { return; }

    ElapsedTime += 0.016f;

    float Alpha = FMath::Clamp(ElapsedTime / PopInDuration, 0.0f, 1.0f);

    // Ease out for bouncy feel
    Alpha = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);

    UpdatePopInTransform(Alpha);

    if (ElapsedTime >= PopInDuration)
    {
        bIsAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(AnimTimer);
    }
}

void UFilterEntry::UpdatePopInTransform(float Alpha)
{
    // Scale from 0.5 → 1.0
    float Scale = FMath::Lerp(0.5f, 1.0f, Alpha);

    // Opacity 0 → 1
    float Opacity = Alpha;

    FWidgetTransform Transform;
    Transform.Scale = FVector2D(Scale, Scale);
    SetRenderTransform(Transform);
    SetRenderOpacity(Opacity);
}

void UFilterEntry::OnRemoveButtonClicked()
{
    OnFilterRemoved.Broadcast(this);
    OnFilterRemovedBP();
}
