//DropdownItemBase.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DropdownItemBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSelected, UDropdownItemBase*, Item);

/*====================================================================================================================================
                                                         DROPDOWN ITEM BASE
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UDropdownItemBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UDropdownItemBase(const FObjectInitializer& ObjectInitializer);

    /** Initialize item with data payload */
    UFUNCTION(BlueprintCallable, Category = "Item") void InitItem(const FString& ItemData, int32 ItemIndex);
    
    /** Mark as selected/deselected */
    UFUNCTION(BlueprintCallable, Category = "Item") void SetSelected(bool bNewSelected);
    
    /** Get item data payload */
    UFUNCTION(BlueprintPure, Category = "Item") FString GetItemData() const { return DataPayload; }
    
    /** Get item index in list */
    UFUNCTION(BlueprintPure, Category = "Item") int32 GetItemIndex() const { return IndexInList; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnItemSelected OnItemSelected;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnItemSelectedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnItemDataSet(const FString& Data);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor NormalColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);  // [RGBA] - Default state

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor HoverColor = FLinearColor(1.0f, 0.194658f, 0.041635f, 0.2f);  // [RGBA] - Mouse over

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor SelectedColor = FLinearColor(1.0f, 0.194658f, 0.041635f, 0.5f);  // [RGBA] - Active selection

private:
    FString DataPayload;  // Item data
    int32 IndexInList;  // Position in dropdown
    bool bIsSelected;  // Selection state
    bool bIsHovered;  // Hover state
    bool bIsPressed;  // Press state
};