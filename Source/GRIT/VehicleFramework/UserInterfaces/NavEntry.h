//NavEntry.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UIToolkit.h"
#include "NavEntry.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryClicked, class UNavEntry*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryHovered, class UNavEntry*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryUnhovered, class UNavEntry*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryPressed, class UNavEntry*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryReleased, class UNavEntry*, Entry);

//------------------------------------------------------------------------------
// Entry configuration structure
//------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FNavEntryConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FText Label = FText::FromString("Entry");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FString Data = "";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor TextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HoverTextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor ActiveTextColor = FLinearColor(1.0f, 0.194658f, 0.041635f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HoverBackgroundColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.05f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    int32 FontSize = 14;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    FMaterialSpec IconMaterial;

    FNavEntryConfig() {}
};

/*====================================================================================================================================
                                                         NAV ENTRY (Config-Driven)
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UNavEntry : public UUserWidget
{
    GENERATED_BODY()

public:
    UNavEntry(const FObjectInitializer& ObjectInitializer);

    /** Initialize entry with configuration struct */
    UFUNCTION(BlueprintCallable, Category = "NavEntry")
    void InitEntry(const FNavEntryConfig& Config, int32 Index);

    /** Apply material configuration to icon image */
    UFUNCTION(BlueprintCallable, Category = "NavEntry")
    void ApplyIconMaterial(const FMaterialSpec& MaterialSpec);

    /** Debug function to check image widget state */
    UFUNCTION(BlueprintCallable, Category = "NavEntry|Debug")
    void DebugImageState();

    /** Force visual refresh of the entry state */
    void SyncChrome();

    /** Set selection state */
    UFUNCTION(BlueprintCallable, Category = "NavEntry")
    void ToggleSelection(bool bState);

    /** Get if selected */
    UFUNCTION(BlueprintPure, Category = "NavEntry")
    bool IsSelected() const { return bIsSelected; }

    /** Get entry data */
    UFUNCTION(BlueprintPure, Category = "NavEntry")
    FString GetEntryData() const { return EntryData; }

    /** Get entry index */
    UFUNCTION(BlueprintPure, Category = "NavEntry")
    int32 GetEntryIndex() const { return EntryIndex; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNavEntryClicked OnEntryClicked;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNavEntryHovered OnEntryHovered;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNavEntryUnhovered OnEntryUnhovered;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNavEntryPressed OnEntryPressed;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNavEntryReleased OnEntryReleased;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnEntryClickedBP();

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* EntryRoot;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* EntryLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* EntryImage;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // Configuration properties - these should NOT override config values
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config Override", meta = (DisplayName = "Override Label (Leave Empty to Use Config)"))
    FText OverrideLabel = FText::GetEmpty();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config Override", meta = (DisplayName = "Override Text Color"))
    FLinearColor OverrideTextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config Override", meta = (DisplayName = "Override Hover Text Color"))
    FLinearColor OverrideHoverTextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config Override", meta = (DisplayName = "Override Active Text Color"))
    FLinearColor OverrideActiveTextColor = FLinearColor(1.0f, 0.194658f, 0.041635f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config Override", meta = (DisplayName = "Override Hover Background Color"))
    FLinearColor OverrideHoverBackgroundColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.05f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config Override", meta = (DisplayName = "Override Font Size"))
    int32 OverrideFontSize = 14;

private:
    FString EntryData;
    int32 EntryIndex = -1;
    bool bIsSelected = false; // [-]
    bool bIsHovered = false;  // [-]
    bool bIsPressed = false;  // [-]

    // Runtime configuration values (set by InitEntry)
    FNavEntryConfig RuntimeConfig;
    bool bConfigInitialized = false;

    // Dynamic material instance for icon
    UPROPERTY()
    UMaterialInstanceDynamic* IconDynamicMaterial;

    void ApplyVisualConfig();
    void ApplyConfiguredValues();
};