// ColorDataObject.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ColorDataObject.generated.h"

UCLASS(BlueprintType)
class GRIT_API UColorDataObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Data")
    FString ColorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Data")
    FLinearColor ColorValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Data")
    int32 ItemIndex;

    UColorDataObject()
        : ColorName(TEXT("Default Color"))
        , ColorValue(FLinearColor::White)
        , ItemIndex(-1)
    {}

    void SetColorData(const FString& InColorName, const FLinearColor& InColorValue, int32 InIndex)
    {
        ColorName = InColorName;
        ColorValue = InColorValue;
        ItemIndex = InIndex;
    }
};