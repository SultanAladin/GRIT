// GenericEntry.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GenericButton.h"
#include "GenericEntry.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntryToggled, bool, bExpanded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntryConfirmed);

//------------------------------------------------------------------------------
// Collapsible entry widget with animated dropdown
// Header row with icon, text, dropdown toggle
// Expandable metadata area with optional confirm button
//------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UGenericEntry : public UUserWidget
{
    GENERATED_BODY()

public:
    UGenericEntry(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    /** Outer container border - color transitions on hover */
    UPROPERTY(meta = (BindWidget))
    class UBorder* RootBorder;

    /** Main vertical layout */
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* MainVbox;

    /** Header row */
    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* EntryHeader;

    /** Header text label */
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* HeaderText;

    /** Dropdown toggle button */
    UPROPERTY(meta = (BindWidget))
    class UButton* DropDownButton;

    /** Entry icon image */
    UPROPERTY(meta = (BindWidget))
    class UImage* EntryIcon;

    /** Expandable metadata container - animated height */
    UPROPERTY(meta = (BindWidget))
    class USizeBox* EntryMetadata;

    /** Metadata content border - color transitions */
    UPROPERTY(meta = (BindWidget))
    class UBorder* MetadataBorder;

    /** Confirm button */
    UPROPERTY(meta = (BindWidget))
    class UButton* ConfirmButton;

    //------------------------------------------------------------------------------
    // Size configuration
    //------------------------------------------------------------------------------

    /** Height when expanded [px] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Size")
    float ExpandedHeight = 80.0f;

    /** Height when collapsed [px] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Size")
    float CollapsedHeight = 0.0f;

    /** Start in expanded state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Size")
    bool bStartExpanded = false;

    //------------------------------------------------------------------------------
    // Animation settings
    //------------------------------------------------------------------------------

    /** Expand/collapse animation duration [s] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Animation")
    float AnimDuration = 0.25f;

    /** Color transition duration [s] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Animation")
    float ColorTransitionDuration = 0.15f;

    //------------------------------------------------------------------------------
    // Theme - Root border
    //------------------------------------------------------------------------------

    /** Root border color - normal [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Root")
    FLinearColor RootColorNormal = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

    /** Root border color - hovered [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Root")
    FLinearColor RootColorHovered = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

    /** Root border color - selected/expanded [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Root")
    FLinearColor RootColorSelected = FLinearColor(0.231f, 0.510f, 0.965f, 0.3f);

    /** Root outline color - normal [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Root")
    FLinearColor RootOutlineNormal = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

    /** Root outline color - hovered [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Root")
    FLinearColor RootOutlineHovered = FLinearColor(0.231f, 0.510f, 0.965f, 1.0f);

    /** Root outline color - selected [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Root")
    FLinearColor RootOutlineSelected = FLinearColor(0.231f, 0.510f, 0.965f, 1.0f);

    /** Root outline width [px] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Root")
    float RootOutlineWidth = 2.0f;

    //------------------------------------------------------------------------------
    // Theme - Metadata border
    //------------------------------------------------------------------------------

    /** Metadata border color - normal [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Metadata")
    FLinearColor MetadataColorNormal = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);

    /** Metadata border color - hovered [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Metadata")
    FLinearColor MetadataColorHovered = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

    /** Metadata border color - pressed [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Metadata")
    FLinearColor MetadataColorPressed = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

    /** Metadata outline color - normal [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Metadata")
    FLinearColor MetadataOutlineNormal = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

    /** Metadata outline color - hovered [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Metadata")
    FLinearColor MetadataOutlineHovered = FLinearColor(0.231f, 0.510f, 0.965f, 1.0f);

    /** Metadata outline color - pressed [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Metadata")
    FLinearColor MetadataOutlinePressed = FLinearColor(0.15f, 0.35f, 0.75f, 1.0f);

    /** Metadata border outline width [px] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme|Metadata")
    float MetadataOutlineWidth = 2.0f;

    //------------------------------------------------------------------------------
    // Theme - General
    //------------------------------------------------------------------------------

    /** Corner rounding style */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Theme")
    ECornerStyle CornerStyle = ECornerStyle::Medium;

    //------------------------------------------------------------------------------
    // Content
    //------------------------------------------------------------------------------

    /** Header label text */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Content")
    FText LabelText;

    /** Entry icon texture */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Content")
    UTexture2D* IconTexture;

    /** Show confirm button */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry|Content")
    bool bShowConfirmButton = true;

private:
    //------------------------------------------------------------------------------
    // State
    //------------------------------------------------------------------------------

    bool bIsExpanded = false;
    bool bIsHovered = false;
    bool bIsMetadataHovered = false;
    bool bIsMetadataPressed = false;

    //------------------------------------------------------------------------------
    // Height animation state
    //------------------------------------------------------------------------------

    float CurrentHeight = 0.0f;
    float StartHeight = 0.0f;
    float TargetHeight = 0.0f;
    float HeightAnimTime = 0.0f;
    bool bIsHeightAnimating = false;

    //------------------------------------------------------------------------------
    // Root border color state
    //------------------------------------------------------------------------------

    FLinearColor CurrentRootColor;
    FLinearColor StartRootColor;
    FLinearColor TargetRootColor;
    FLinearColor CurrentRootOutline;
    FLinearColor StartRootOutline;
    FLinearColor TargetRootOutline;
    float RootColorAnimTime = 0.0f;

    //------------------------------------------------------------------------------
    // Metadata border color state
    //------------------------------------------------------------------------------

    FLinearColor CurrentMetadataColor;
    FLinearColor StartMetadataColor;
    FLinearColor TargetMetadataColor;
    FLinearColor CurrentMetadataOutline;
    FLinearColor StartMetadataOutline;
    FLinearColor TargetMetadataOutline;
    float MetadataColorAnimTime = 0.0f;

    //------------------------------------------------------------------------------
    // Event handlers
    //------------------------------------------------------------------------------

    UFUNCTION()
    void OnDropDownClicked();

    UFUNCTION()
    void OnConfirmClicked();

    UFUNCTION()
    void OnConfirmPressed();

    UFUNCTION()
    void OnConfirmReleased();

    UFUNCTION()
    void OnConfirmHovered();

    UFUNCTION()
    void OnConfirmUnhovered();

    //------------------------------------------------------------------------------
    // Internal functions
    //------------------------------------------------------------------------------

    void ApplyRootBorderStyle();
    void ApplyMetadataBorderStyle();
    void ApplyContent();
    void UpdateHeightOverride();

    void StartHeightAnimation(float NewTarget);
    void StartRootColorTransition();
    void StartMetadataColorTransition();

    void GetRootTargetColors(FLinearColor& OutColor, FLinearColor& OutOutline) const;
    void GetMetadataTargetColors(FLinearColor& OutColor, FLinearColor& OutOutline) const;

    float GetCornerRadius(class UWidget* ForWidget) const;
    float ApplyEasing(float Alpha) const;

public:
    //------------------------------------------------------------------------------
    // Events
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Entry|Events")
    FOnEntryToggled OnToggled;

    UPROPERTY(BlueprintAssignable, Category = "Entry|Events")
    FOnEntryConfirmed OnConfirmed;

    UFUNCTION(BlueprintImplementableEvent, Category = "Entry|Events")
    void OnToggledBP(bool bExpanded);

    UFUNCTION(BlueprintImplementableEvent, Category = "Entry|Events")
    void OnConfirmedBP();

    //------------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------------

    /** Set expanded state with animation */
    UFUNCTION(BlueprintCallable, Category = "Entry")
    void SetExpanded(bool bExpand);

    /** Toggle expanded state */
    UFUNCTION(BlueprintCallable, Category = "Entry")
    void Toggle();

    /** Get expanded state */
    UFUNCTION(BlueprintCallable, Category = "Entry")
    bool IsExpanded() const { return bIsExpanded; }

    /** Set header text */
    UFUNCTION(BlueprintCallable, Category = "Entry")
    void SetHeaderText(const FText& NewText);

    /** Set entry icon texture */
    UFUNCTION(BlueprintCallable, Category = "Entry")
    void SetIcon(UTexture2D* NewIcon);

    /** Show/hide metadata area */
    UFUNCTION(BlueprintCallable, Category = "Entry")
    void ShowMetadata(bool bShow);
};
