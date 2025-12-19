//NavEntry.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "NavEntry.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryClicked, class UNavEntry*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryHovered, class UNavEntry*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryUnhovered, class UNavEntry*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryPressed, class UNavEntry*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavEntryReleased, class UNavEntry*, Entry);

/*====================================================================================================================================
                                                         NAV ENTRY (Simplified - No Indicator)
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UNavEntry : public UUserWidget
{
    GENERATED_BODY()

public:
    UNavEntry(const FObjectInitializer& ObjectInitializer);

    /** Initialize entry with label and data */
    UFUNCTION(BlueprintCallable, Category = "NavEntry")
    void InitEntry(const FText& Label, const FString& Data, int32 Index);

    /** Force visual refresh of the entry state */
    void SyncChrome();

    /** Set selection state */
    UFUNCTION(BlueprintCallable, Category = "NavEntry")
    void ToggleSelection(bool bState);                   // [-] 1 for selected, 0 for not

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

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* EntryLabel;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

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

private:
    FString EntryData;
    int32 EntryIndex = -1;
    bool bIsSelected = false;
    bool bIsHovered = false;
    bool bIsPressed = false;
};