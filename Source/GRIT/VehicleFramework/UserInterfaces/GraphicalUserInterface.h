// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GraphicalUserInterface.generated.h"

/**
 * Theme Configuration Structure
 */
USTRUCT(BlueprintType)
struct FUIThemeConfiguration
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colors")
	FLinearColor PrimaryColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colors")
	FLinearColor SecondaryColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colors")
	FLinearColor AccentColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colors")
	FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colors")
	FLinearColor TextColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Typography")
	float FontSize = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Layout")
	float Padding = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Layout")
	float BorderRadius = 5.0f;
};

/**
 * Graphical User Interface Widget
 */
UCLASS()
class GRIT_API UGraphicalUserInterface : public UUserWidget
{
	GENERATED_BODY()

public:
	UGraphicalUserInterface(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Theme Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Theme")
	FUIThemeConfiguration ThemeConfig;

	// Apply theme to UI elements
	UFUNCTION(BlueprintCallable, Category = "UI|Theme")
	void ApplyTheme();

	// Update theme configuration
	UFUNCTION(BlueprintCallable, Category = "UI|Theme")
	void UpdateTheme(const FUIThemeConfiguration& NewTheme);

	// Show/Hide UI
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleUI();

	// UI State
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bIsVisible = true;

protected:
	// Blueprint implementable events
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Theme")
	void OnThemeApplied();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUIShown();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUIHidden();
};
