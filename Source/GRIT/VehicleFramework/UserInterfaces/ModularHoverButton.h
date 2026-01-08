#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ModularHoverButton.generated.h"

UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Interactive Button A1"))
class GRIT_API UModularHoverButton : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The actual button we’ll wrap */
	UPROPERTY(meta = (BindWidget, OptionalWidget))
	TObjectPtr<UButton> WrappedButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
	float HoverScale = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
	float AnimDuration = 0.15f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION() void OnHovered();
	UFUNCTION() void OnUnhovered();
	void UpdateScale(float TargetScale);

	FTimerHandle ScaleTimer;
};