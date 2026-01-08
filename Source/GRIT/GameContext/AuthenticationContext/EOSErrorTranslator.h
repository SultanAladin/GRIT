#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EOSErrorTranslator.generated.h"

/**
 * Utility class to translate EOS error codes to human-readable messages
 * Based on EOS SDK v1.18.1.2 error definitions
 */
UCLASS(BlueprintType)
class GRIT_API UEOSErrorTranslator : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Convert EOS error code to human-readable message
     * @param ErrorCode The EOS error code (int32)
     * @return Human-readable error message
     */
    UFUNCTION(BlueprintCallable, Category = "EOS Error Translation")
    static FString TranslateErrorCode(int32 ErrorCode);

    /**
     * Get detailed error description for specific error codes
     * @param ErrorCode The EOS error code (int32)
     * @return Detailed error description with potential solutions
     */
    UFUNCTION(BlueprintCallable, Category = "EOS Error Translation")
    static FString GetDetailedErrorDescription(int32 ErrorCode);

    /**
     * Check if error code is authentication-related
     * @param ErrorCode The EOS error code (int32)
     * @return True if error is authentication-related
     */
    UFUNCTION(BlueprintCallable, Category = "EOS Error Translation")
    static bool IsAuthenticationError(int32 ErrorCode);

    /**
     * Get user-friendly error message suitable for display
     * @param ErrorCode The EOS error code (int32)
     * @return User-friendly error message
     */
    UFUNCTION(BlueprintCallable, Category = "EOS Error Translation")
    static FString GetUserFriendlyMessage(int32 ErrorCode);

private:
    /** Internal mapping of error codes to messages */
    static TMap<int32, FString> GetErrorCodeMap();
    
    /** Internal mapping of error codes to detailed descriptions */
    static TMap<int32, FString> GetDetailedDescriptionMap();
    
    /** Internal mapping of error codes to user-friendly messages */
    static TMap<int32, FString> GetUserFriendlyMap();
};