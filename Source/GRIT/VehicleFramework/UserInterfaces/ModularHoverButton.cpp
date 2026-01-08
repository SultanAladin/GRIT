#include "ModularHoverButton.h"
#include "TimerManager.h"

void UModularHoverButton::NativeConstruct()
{
	Super::NativeConstruct();

	if (WrappedButton)
	{
		WrappedButton->OnHovered.AddDynamic(this, &UModularHoverButton::OnHovered);
		WrappedButton->OnUnhovered.AddDynamic(this, &UModularHoverButton::OnUnhovered);
	}
}

void UModularHoverButton::NativeDestruct()
{
	if (WrappedButton)
	{
		WrappedButton->OnHovered.RemoveDynamic(this, &UModularHoverButton::OnHovered);
		WrappedButton->OnUnhovered.RemoveDynamic(this, &UModularHoverButton::OnUnhovered);
	}

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(ScaleTimer);

	Super::NativeDestruct();
}

void UModularHoverButton::OnHovered()
{
	UpdateScale(HoverScale);
}

void UModularHoverButton::OnUnhovered()
{
	UpdateScale(1.0f);
}

void UModularHoverButton::UpdateScale(float TargetScale)
{
	UWorld* World = GetWorld();
	if (!World || !WrappedButton) return;

	World->GetTimerManager().ClearTimer(ScaleTimer);

	const float StartScale = WrappedButton->GetRenderTransform().Scale.X;
	float Elapsed = 0.0f;

	World->GetTimerManager().SetTimer(
		ScaleTimer,
		FTimerDelegate::CreateLambda([this, StartScale, TargetScale, Elapsed]() mutable
		{
			Elapsed += GetWorld()->GetDeltaSeconds();
			const float Alpha = FMath::Clamp(Elapsed / AnimDuration, 0.0f, 1.0f);
			const float CurrentScale = FMath::Lerp(StartScale, TargetScale, Alpha);

			FWidgetTransform NewXf = WrappedButton->GetRenderTransform();
			NewXf.Scale = FVector2D(CurrentScale);
			WrappedButton->SetRenderTransform(NewXf);

			if (Alpha >= 1.0f)
				GetWorld()->GetTimerManager().ClearTimer(ScaleTimer);
		}),
		0.0f, true);
}