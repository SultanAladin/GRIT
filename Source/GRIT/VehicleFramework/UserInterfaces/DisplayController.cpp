// Fill out your copyright notice in the Description page of Project Settings.


#include "DisplayController.h"
#include "Blueprint/UserWidget.h"

void ADisplayController::BeginPlay()
{
	Super::BeginPlay();

	// Check if the WidgetClass is set
	if (WidgetClass)
	{
		// Create the widget and add it to the viewport
		WidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
		if (WidgetInstance)
		{
			WidgetInstance->AddToViewport();
		}
	}
}