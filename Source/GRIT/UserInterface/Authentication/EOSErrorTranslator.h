#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EOSErrorTranslator.generated.h"

/**
 * Utility class for translating EOS error codes into user-friendly messages
 */
UCLASS()
class GRIT_API UEOSErrorTranslator : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Translate an EOS error code to a technical error message
     * @param ErrorCode The EOS error code to translate
     * @return Technical error message string
     */
    static FString TranslateErrorCode(int32 ErrorCode);

    /**
     * Get a detailed error description for an EOS error code
     * @param ErrorCode The EOS error code
     * @return Detailed error description
     */
    static FString GetDetailedErrorDescription(int32 ErrorCode);

    /**
     * Get a user-friendly error message for an EOS error code
     * @param ErrorCode The EOS error code
     * @return User-friendly error message
     */
    static FString GetUserFriendlyMessage(int32 ErrorCode);

    /**
     * Check if an error code is authentication-related
     * @param ErrorCode The EOS error code to check
     * @return True if the error is authentication-related
     */
    static bool IsAuthenticationError(int32 ErrorCode);

private:
    /**
     * Get the error code mapping table
     * @return Map of error codes to technical messages
     */
    static TMap<int32, FString> GetErrorCodeMap();

    /**
     * Get the detailed description mapping table
     * @return Map of error codes to detailed descriptions
     */
    static TMap<int32, FString> GetDetailedDescriptionMap();

    /**
     * Get the user-friendly message mapping table
     * @return Map of error codes to user-friendly messages
     */
    static TMap<int32, FString> GetUserFriendlyMap();
};