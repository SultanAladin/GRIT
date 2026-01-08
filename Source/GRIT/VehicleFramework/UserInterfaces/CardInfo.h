#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "CardInfo.generated.h"

USTRUCT(BlueprintType)
struct GRIT_API FCardInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CardImage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText CardTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText CardDescription;

    FCardInfo()
    {
        CardImage = nullptr;
        CardTitle = FText::GetEmpty();
        CardDescription = FText::GetEmpty();
    }
};