#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "AppData.generated.h"

/*====================================================================================================================================
                                                         APP CONFIGURATION
======================================================================================================================================*/

/** Application icon configuration */
USTRUCT(BlueprintType)
struct FAppConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "App")
    FText AppLabel; // [FText] - Display name

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "App")
    UTexture2D* AppIcon; // [UTexture2D*] - Icon texture

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "App")
    int32 AppID; // [int32] - Unique identifier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "App")
    TSubclassOf<class UAppPage> PageClass; // [TSubclassOf] - Page widget to spawn

    FAppConfig() : AppIcon(nullptr), AppID(0), PageClass(nullptr) {}
};
