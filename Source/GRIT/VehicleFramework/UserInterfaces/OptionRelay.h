// OptionRelay.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "OptionRelay.generated.h"

//------------------------------------------------------------------------------
//                                  easing modes
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ERelayEasing : uint8
{
	Linear      UMETA(DisplayName = "Linear"),
	EaseIn      UMETA(DisplayName = "Ease In"),
	EaseOut     UMETA(DisplayName = "Ease Out"),
	EaseInOut   UMETA(DisplayName = "Ease In-Out")
};

//------------------------------------------------------------------------------
//                                  option relay
//------------------------------------------------------------------------------

UCLASS()
class UOptionRelay : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget construction */
	virtual void NativeConstruct() override;

	/** Click handler for flip-flop toggle */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Toggles between X and options states */
	UFUNCTION(BlueprintCallable, Category = "OptionRelay") void FlipState();

	//------------------------------------------------------------------------------
	// animation config
	//------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation") float MorphDuration = 0.3f;           // [s] - Morph animation duration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation") ERelayEasing EasingMode = ERelayEasing::EaseInOut;

protected:
	//------------------------------------------------------------------------------
	// widget bindings (bound to Blueprint widgets)
	//------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UBorder* RootBorder;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UOverlay* MarkContainer;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) USizeBox* Mark_0;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) USizeBox* Mark_1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) USizeBox* Mark_2;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) USizeBox* Mark_3;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UImage* Stroke_0;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UImage* Stroke_1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UImage* Stroke_2;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UImage* Stroke_3;

	//------------------------------------------------------------------------------
	// animation binding
	//------------------------------------------------------------------------------

	UPROPERTY(Transient, meta = (BindWidgetAnim)) UWidgetAnimation* MorphAnimation;

private:
	//------------------------------------------------------------------------------
	// state tracking
	//------------------------------------------------------------------------------

	bool bIsX;  // Current display state

	//------------------------------------------------------------------------------
	// internal helpers
	//------------------------------------------------------------------------------

	void ConfigureSlotProperties();
	float ComputeEasing(float T) const;
};
