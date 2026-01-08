//NavEntryExtended.h
#pragma once

#include "CoreMinimal.h"
#include "NavEntry.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "NavEntryExtended.generated.h"

//------------------------------------------------------------------------------
//                    EXTENDED NAV ENTRY (Icon + Expanding Text)
//------------------------------------------------------------------------------
// Blueprint structure:
//   EntryRoot (Border) ← from NavEntry
//   └── HorizontalBox
//       ├── Icon (Image) ← color syncs with state
//       └── TextSizeBox (SizeBox) ← width animates 0↔ExpandedWidth
//           └── EntryLabel (TextBlock) ← from NavEntry
//------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UNavEntryExtended : public UNavEntry
{
    GENERATED_BODY()

public:
    UNavEntryExtended(const FObjectInitializer& ObjectInitializer);

    /** Trigger expand/collapse animation */
    UFUNCTION(BlueprintCallable, Category = "NavEntryExtended")
    void PulseExpansion(bool bExpand);

    /** Force expansion state (called by NavBar for synchronized expansion) */
    UFUNCTION(BlueprintCallable, Category = "NavEntryExtended")
    void ForceExpansionState(bool bExpand);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;

    //------------------------------------------------------------------------------
    // Bound Widgets
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Icon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    USizeBox* TextSizeBox;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Expansion")
    bool bExpandHorizontally = true;                  // [true=width, false=height]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Expansion")
    float ExpandedWidth = 500.0f;                     // [px] - Target width when expanded (horizontal mode)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Expansion")
    float ExpandedHeight = 40.0f;                     // [px] - Target height when expanded (vertical mode)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Expansion")
    float ExpandDuration = 0.25f;                     // [s] - Animation duration

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FLinearColor IconIdleColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FLinearColor IconHoverColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FLinearColor IconActiveColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

private:
    FTimerHandle ExpandTimer;                         // [handle]
    float ExpandProgress = 0.0f;                      // [0-1]
    float ExpandStartValue = 0.0f;                    // [0-1]
    float ExpandTargetValue = 0.0f;                   // [0-1]
    bool bExpandAnimating = false;                    // [-]

    void StartExpandAnimation(bool bExpand);
    void ApplyExpansion(float Ratio);
    void SyncColors();
    float CubicEaseInOut(float T);
};
