//DropdownItemBase.cpp
#include "DropdownItemBase.h"

UDropdownItemBase::UDropdownItemBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , IndexInList(-1)
    , bIsSelected(false)
    , bIsHovered(false)
    , bIsPressed(false)
{
}

void UDropdownItemBase::NativeConstruct()
{
    Super::NativeConstruct();
}

void UDropdownItemBase::InitItem(const FString& ItemData, int32 ItemIndex)
{
    DataPayload = ItemData;
    IndexInList = ItemIndex;
    OnItemDataSet(ItemData);
}

void UDropdownItemBase::SetSelected(bool bNewSelected)
{
    bIsSelected = bNewSelected;
}

//------------------------------------------------------------------------------
//                                    Input event pipeline
//------------------------------------------------------------------------------

void UDropdownItemBase::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
    bIsHovered = true;
}

void UDropdownItemBase::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseLeave(MouseEvent);
    bIsHovered = false;
    bIsPressed = false;
}

FReply UDropdownItemBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bIsPressed = true;
        return FReply::Handled();
    } // End if (left button check)
    
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UDropdownItemBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed)
    {
        bIsPressed = false;
        OnItemSelected.Broadcast(this);
        OnItemSelectedBP();
        return FReply::Handled();
    } // End if (click validation)
    
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}