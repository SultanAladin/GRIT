//FilterableItem.cpp
#include "FilterableItem.h"
#include "TimerManager.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

UFilterableItem::UFilterableItem(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UFilterableItem::NativeConstruct()
{
    if (GetItemData().IsEmpty() && !DefaultItemData.IsEmpty())
    {
        InitItem(DefaultItemData, GetItemIndex());
    } // End if (initialize from default)

    Super::NativeConstruct();

    SetRenderTransform(FWidgetTransform());
    SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

    if (AddFilterButton)
    {
        AddFilterButton->OnClicked.AddDynamic(this, &UFilterableItem::OnAddFilterButtonClicked);
    } // End if (button binding)

    if (ButtonLabel)
    {
        ButtonLabel->SetText(FText::FromString(GetItemData()));
    } // End if (set button text)
}

void UFilterableItem::NativeDestruct()
{
    StopAnimation();
    Super::NativeDestruct();
}

void UFilterableItem::OnAddFilterButtonClicked()
{
    OnItemSelected.Broadcast(this);
    StartCollapseAnimation();
}

void UFilterableItem::StartCollapseAnimation()
{
    if (bIsAnimating) { StopAnimation(); }

    bIsCollapsing = true;
    bIsAnimating = true;
    ElapsedTime = 0.0f;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UFilterableItem::AnimateTick, 0.016f, true);
    } // End if (timer setup)
}

void UFilterableItem::StartExpandAnimation()
{
    if (bIsAnimating) { StopAnimation(); }

    bIsCollapsing = false;
    bIsAnimating = true;
    ElapsedTime = 0.0f;

    FWidgetTransform Transform;
    Transform.Scale = FVector2D(0.0f, 1.0f);
    SetRenderTransform(Transform);
    SetRenderOpacity(0.0f);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UFilterableItem::AnimateTick, 0.016f, true);
    } // End if (timer setup)
}

void UFilterableItem::StopAnimation()
{
    if (GetWorld() && AnimTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(AnimTimer);
    } // End if (timer cleanup)

    bIsAnimating = false;
    ElapsedTime = 0.0f;
}

void UFilterableItem::AnimateTick()
{
    if (!bIsAnimating) { return; }

    ElapsedTime += 0.016f;

    float Alpha = FMath::Clamp(ElapsedTime / AnimationDuration, 0.0f, 1.0f);
    Alpha = ApplyEasing(Alpha);

    UpdateTransform(Alpha);

    if (ElapsedTime >= AnimationDuration)
    {
        StopAnimation();

        if (bIsCollapsing)
        {
            OnCollapseComplete.Broadcast(this);
        }
        else
        {
            OnExpandComplete.Broadcast(this);
        }
    } // End if (animation complete)
}

void UFilterableItem::UpdateTransform(float Alpha)
{
    float ScaleX, Opacity;

    if (bIsCollapsing)
    {
        ScaleX = FMath::Lerp(1.0f, 0.0f, Alpha);   // [dimensionless] - 1.0 → 0.0
        Opacity = FMath::Lerp(1.0f, 0.0f, Alpha);  // [dimensionless] - 1.0 → 0.0
    }
    else
    {
        ScaleX = FMath::Lerp(0.0f, 1.0f, Alpha);   // [dimensionless] - 0.0 → 1.0
        Opacity = FMath::Lerp(0.0f, 1.0f, Alpha);  // [dimensionless] - 0.0 → 1.0
    }

    FWidgetTransform Transform;
    Transform.Scale = FVector2D(ScaleX, 1.0f);
    SetRenderTransform(Transform);
    SetRenderOpacity(Opacity);
}

//------------------------------------------------------------------------------
//                                    Easing functions
//------------------------------------------------------------------------------

float UFilterableItem::ApplyEasing(float t) const
{
    switch (EasingType)
    {
        case EDropdownEasingType::Linear:
            return t;
        case EDropdownEasingType::EaseIn:
            return EaseInQuad(t);
        case EDropdownEasingType::EaseOut:
            return EaseOutQuad(t);
        case EDropdownEasingType::EaseInOutCubic:
            return EaseInOutCubic(t);
        case EDropdownEasingType::EaseInOut:
        default:
            return EaseInOutQuad(t);
    }
}

float UFilterableItem::EaseInOutQuad(float t) const
{
    return t < 0.5f ? (2.0f * t * t) : (1.0f - FMath::Pow(-2.0f * t + 2.0f, 2.0f) / 2.0f);
}

float UFilterableItem::EaseInQuad(float t) const
{
    return t * t;
}

float UFilterableItem::EaseOutQuad(float t) const
{
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float UFilterableItem::EaseInOutCubic(float t) const
{
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - FMath::Pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}