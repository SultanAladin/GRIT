// Copyright Epic Games, Inc. All Rights Reserved.

#include "GraphicalUserInterface.h"

UGraphicalUserInterface::UGraphicalUserInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize default theme
	ThemeConfig = FUIThemeConfiguration();
}

void UGraphicalUserInterface::NativeConstruct()
{
	Super::NativeConstruct();

	// Apply theme on construction
	ApplyTheme();
}

void UGraphicalUserInterface::NativeDestruct()
{
	Super::NativeDestruct();
}

void UGraphicalUserInterface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UGraphicalUserInterface::ApplyTheme()
{
	// Theme application logic - can be extended in Blueprint
	OnThemeApplied();
}

void UGraphicalUserInterface::UpdateTheme(const FUIThemeConfiguration& NewTheme)
{
	ThemeConfig = NewTheme;
	ApplyTheme();
}

void UGraphicalUserInterface::ShowUI()
{
	if (!bIsVisible)
	{
		bIsVisible = true;
		SetVisibility(ESlateVisibility::Visible);
		OnUIShown();
	}
}

void UGraphicalUserInterface::HideUI()
{
	if (bIsVisible)
	{
		bIsVisible = false;
		SetVisibility(ESlateVisibility::Hidden);
		OnUIHidden();
	}
}

void UGraphicalUserInterface::ToggleUI()
{
	if (bIsVisible)
	{
		HideUI();
	}
	else
	{
		ShowUI();
	}
}
