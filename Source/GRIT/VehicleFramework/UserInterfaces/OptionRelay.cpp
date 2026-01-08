// OptionRelay.cpp
#include "OptionRelay.h"
#include "Animation/UMGSequencePlayer.h"
#include "Components/BorderSlot.h"
#include "Components/OverlaySlot.h"

//------------------------------------------------------------------------------
//                            widget initialization
//------------------------------------------------------------------------------

void UOptionRelay::NativeConstruct()
{
	Super::NativeConstruct();

	bIsX = false;  // Initialize state

	ConfigureSlotProperties();
}

//------------------------------------------------------------------------------
//                            slot configuration
//------------------------------------------------------------------------------

void UOptionRelay::ConfigureSlotProperties()
{
	// Reason: Set MarkContainer slot (child of RootBorder)
	if (MarkContainer)
	{
		if (UBorderSlot* ContainerSlot = Cast<UBorderSlot>(MarkContainer->Slot))
		{
			ContainerSlot->SetPadding(FMargin(50.0f));           // [px] - 50 padding all sides
			ContainerSlot->SetHorizontalAlignment(HAlign_Fill);
			ContainerSlot->SetVerticalAlignment(VAlign_Fill);
		} // End if (ContainerSlot)
	} // End if (MarkContainer)

	// Reason: Set Mark slots (children of MarkContainer overlay)
	TArray<USizeBox*> Marks = {Mark_0, Mark_1, Mark_2, Mark_3};
	for (USizeBox* Mark : Marks)
	{
		if (Mark)
		{
			if (UOverlaySlot* MarkSlot = Cast<UOverlaySlot>(Mark->Slot))
			{
				MarkSlot->SetPadding(FMargin(100.0f));           // [px] - 100 padding all sides
				MarkSlot->SetHorizontalAlignment(HAlign_Center);
				MarkSlot->SetVerticalAlignment(VAlign_Center);
			} // End if (MarkSlot)
		} // End if (Mark)
	} // End for (Marks)
}

//------------------------------------------------------------------------------
//                              state flip logic
//------------------------------------------------------------------------------

void UOptionRelay::FlipState()
{
	bIsX = !bIsX;  // Toggle state

	// Reason: Play animation with configured easing
	if (MorphAnimation)
	{
		float PlaybackSpeed = 1.0f / FMath::Max(MorphDuration, 0.01f);  // [Hz] - Convert duration to speed

		// Reason: Forward plays to X, reverse plays to options
		if (bIsX)
		{
			PlayAnimation(MorphAnimation, 0.0f, 1, EUMGSequencePlayMode::Forward, PlaybackSpeed);
		}
		else
		{
			PlayAnimation(MorphAnimation, 0.0f, 1, EUMGSequencePlayMode::Reverse, PlaybackSpeed);
		} // End if (direction branch)
	} // End if (animation check)
}

//------------------------------------------------------------------------------
//                              easing computation
//------------------------------------------------------------------------------

float UOptionRelay::ComputeEasing(float T) const
{
	T = FMath::Clamp(T, 0.0f, 1.0f);  // [0-1] - Normalized time

	// Reason: Apply easing curve based on selected mode
	switch (EasingMode)
	{
		case ERelayEasing::Linear:
			return T;

		case ERelayEasing::EaseIn:
			return T * T * T;  // Cubic ease in

		case ERelayEasing::EaseOut:
			return 1.0f - FMath::Pow(1.0f - T, 3.0f);  // Cubic ease out

		case ERelayEasing::EaseInOut:
			return (T < 0.5f) ? 4.0f * T * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 3.0f) / 2.0f;  // Cubic ease in-out

		default:
			return T;
	} // End switch (easing mode)
}

//------------------------------------------------------------------------------
//                              click handling
//------------------------------------------------------------------------------

FReply UOptionRelay::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Reason: Left click triggers flip-flop toggle
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		FlipState();
		return FReply::Handled();
	} // End if (left click)

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
