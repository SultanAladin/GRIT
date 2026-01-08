#include "EOSErrorTranslator.h"

//------------------------------------------------------------------------------
//                           PUBLIC API
//------------------------------------------------------------------------------

FString UEOSErrorTranslator::TranslateErrorCode(int32 ErrorCode)
{
    static TMap<int32, FString> ErrorMap = GetErrorCodeMap();
    
    if (FString* ErrorMessage = ErrorMap.Find(ErrorCode))
    {
        return *ErrorMessage;
    }
    
    // Return generic message for unknown error codes
    return FString::Printf(TEXT("Unknown Error (%d)"), ErrorCode);
}

FString UEOSErrorTranslator::GetDetailedErrorDescription(int32 ErrorCode)
{
    static TMap<int32, FString> DetailedMap = GetDetailedDescriptionMap();
    
    if (FString* DetailedMessage = DetailedMap.Find(ErrorCode))
    {
        return *DetailedMessage;
    }
    
    // Fallback to basic translation
    return TranslateErrorCode(ErrorCode);
}

bool UEOSErrorTranslator::IsAuthenticationError(int32 ErrorCode)
{
    // Authentication errors are in the 1000-1099 range
    return (ErrorCode >= 1001 && ErrorCode <= 1099);
}

FString UEOSErrorTranslator::GetUserFriendlyMessage(int32 ErrorCode)
{
    static TMap<int32, FString> UserFriendlyMap = GetUserFriendlyMap();
    
    if (FString* UserMessage = UserFriendlyMap.Find(ErrorCode))
    {
        return *UserMessage;
    }
    
    // Fallback to technical translation
    return TranslateErrorCode(ErrorCode);
}

//------------------------------------------------------------------------------
//                           PRIVATE MAPPINGS
//------------------------------------------------------------------------------

TMap<int32, FString> UEOSErrorTranslator::GetErrorCodeMap()
{
    static TMap<int32, FString> ErrorMap;
    
    if (ErrorMap.Num() == 0)
    {
        // Success
        ErrorMap.Add(0, TEXT("Success"));
        
        // General Errors (1-42)
        ErrorMap.Add(1, TEXT("No Connection"));
        ErrorMap.Add(2, TEXT("Invalid Credentials"));
        ErrorMap.Add(3, TEXT("Invalid User"));
        ErrorMap.Add(4, TEXT("Invalid Auth"));
        ErrorMap.Add(5, TEXT("Access Denied"));
        ErrorMap.Add(6, TEXT("Missing Permissions"));
        ErrorMap.Add(7, TEXT("Token Not Account"));
        ErrorMap.Add(8, TEXT("Too Many Requests"));
        ErrorMap.Add(9, TEXT("Already Pending"));
        ErrorMap.Add(10, TEXT("Invalid Parameters"));
        ErrorMap.Add(11, TEXT("Invalid Request"));
        ErrorMap.Add(12, TEXT("Unrecognized Response"));
        ErrorMap.Add(13, TEXT("Incompatible Version"));
        ErrorMap.Add(14, TEXT("Not Configured"));
        ErrorMap.Add(15, TEXT("Already Configured"));
        ErrorMap.Add(16, TEXT("Not Implemented"));
        ErrorMap.Add(17, TEXT("Canceled"));
        ErrorMap.Add(18, TEXT("Not Found"));
        ErrorMap.Add(19, TEXT("Operation Will Retry"));
        ErrorMap.Add(20, TEXT("No Change"));
        ErrorMap.Add(21, TEXT("Version Mismatch"));
        ErrorMap.Add(22, TEXT("Limit Exceeded"));
        ErrorMap.Add(23, TEXT("Disabled"));
        ErrorMap.Add(24, TEXT("Duplicate Not Allowed"));
        ErrorMap.Add(25, TEXT("Missing Parameters (Deprecated)"));
        ErrorMap.Add(26, TEXT("Invalid Sandbox ID"));
        ErrorMap.Add(27, TEXT("Timed Out"));
        ErrorMap.Add(28, TEXT("Partial Result"));
        ErrorMap.Add(29, TEXT("Missing Role"));
        ErrorMap.Add(30, TEXT("Missing Feature"));
        ErrorMap.Add(31, TEXT("Invalid Sandbox"));
        ErrorMap.Add(32, TEXT("Invalid Deployment"));
        ErrorMap.Add(33, TEXT("Invalid Product"));
        ErrorMap.Add(34, TEXT("Invalid Product User ID"));
        ErrorMap.Add(35, TEXT("Service Failure"));
        ErrorMap.Add(36, TEXT("Cache Directory Missing"));
        ErrorMap.Add(37, TEXT("Cache Directory Invalid"));
        ErrorMap.Add(38, TEXT("Invalid State"));
        ErrorMap.Add(39, TEXT("Request In Progress"));
        ErrorMap.Add(40, TEXT("Application Suspended"));
        ErrorMap.Add(41, TEXT("Network Disconnected"));
        ErrorMap.Add(42, TEXT("Insufficient Output Buffer"));
        
        // Authentication Errors (1001-1099)
        ErrorMap.Add(1001, TEXT("Account Locked"));
        ErrorMap.Add(1002, TEXT("Account Locked For Update"));
        ErrorMap.Add(1003, TEXT("Invalid Refresh Token"));
        ErrorMap.Add(1004, TEXT("Invalid Token"));
        ErrorMap.Add(1005, TEXT("Authentication Failure"));
        ErrorMap.Add(1006, TEXT("Invalid Platform Token"));
        ErrorMap.Add(1007, TEXT("Wrong Account"));
        ErrorMap.Add(1008, TEXT("Wrong Client"));
        ErrorMap.Add(1009, TEXT("Full Account Required"));
        ErrorMap.Add(1010, TEXT("Headless Account Required"));
        ErrorMap.Add(1011, TEXT("Password Reset Required"));
        ErrorMap.Add(1012, TEXT("Password Cannot Be Reused"));
        ErrorMap.Add(1013, TEXT("Authorization Expired"));
        ErrorMap.Add(1014, TEXT("Scope Consent Required"));
        ErrorMap.Add(1015, TEXT("Application Not Found"));
        ErrorMap.Add(1016, TEXT("Scope Not Found"));
        ErrorMap.Add(1017, TEXT("Account Feature Restricted"));
        ErrorMap.Add(1018, TEXT("Account Portal Load Error"));
        ErrorMap.Add(1019, TEXT("Corrective Action Required"));
        ErrorMap.Add(1020, TEXT("Pin Grant Code"));
        ErrorMap.Add(1021, TEXT("Pin Grant Expired"));
        ErrorMap.Add(1022, TEXT("Pin Grant Pending"));
        ErrorMap.Add(1030, TEXT("External Auth Not Linked"));
        ErrorMap.Add(1032, TEXT("External Auth Revoked"));
        ErrorMap.Add(1033, TEXT("External Auth Invalid"));
        ErrorMap.Add(1034, TEXT("External Auth Restricted"));
        ErrorMap.Add(1035, TEXT("External Auth Cannot Login"));
        ErrorMap.Add(1036, TEXT("External Auth Expired"));
        ErrorMap.Add(1037, TEXT("External Auth Is Last Login Type"));
        ErrorMap.Add(1040, TEXT("Exchange Code Not Found"));
        ErrorMap.Add(1041, TEXT("Originating Exchange Code Session Expired"));
        ErrorMap.Add(1050, TEXT("Account Not Active"));
        ErrorMap.Add(1060, TEXT("MFA Required"));
        ErrorMap.Add(1070, TEXT("Parental Controls"));
        ErrorMap.Add(1080, TEXT("No Real ID"));
        ErrorMap.Add(1090, TEXT("User Interface Required"));
        
        // Add more error categories as needed...
        // For brevity, I'm including the most common ones
        // The full list can be extended based on requirements
        
        // Connect Errors (7000-7008)
        ErrorMap.Add(7000, TEXT("External Token Validation Failed"));
        ErrorMap.Add(7001, TEXT("User Already Exists"));
        ErrorMap.Add(7002, TEXT("Auth Expired"));
        ErrorMap.Add(7003, TEXT("Invalid Token"));
        ErrorMap.Add(7004, TEXT("Unsupported Token Type"));
        ErrorMap.Add(7005, TEXT("Link Account Failed"));
        ErrorMap.Add(7006, TEXT("External Service Unavailable"));
        ErrorMap.Add(7007, TEXT("External Service Configuration Failure"));
        
        // Unexpected Error
        ErrorMap.Add(0x7FFFFFFF, TEXT("Unexpected Error"));
    }
    
    return ErrorMap;
}

TMap<int32, FString> UEOSErrorTranslator::GetDetailedDescriptionMap()
{
    static TMap<int32, FString> DetailedMap;
    
    if (DetailedMap.Num() == 0)
    {
        // Common authentication errors with detailed descriptions
        DetailedMap.Add(1001, TEXT("Account Locked: Your account has been temporarily locked due to multiple failed login attempts. Please wait and try again later."));
        DetailedMap.Add(1002, TEXT("Account Locked For Update: Your account is temporarily locked while an update is being processed. Please try again in a few minutes."));
        DetailedMap.Add(1003, TEXT("Invalid Refresh Token: Your login session has expired. Please sign in again."));
        DetailedMap.Add(1004, TEXT("Invalid Token: Your authentication token is invalid. Please sign in again."));
        DetailedMap.Add(1005, TEXT("Authentication Failure: Login failed due to invalid credentials. Please check your username and password."));
        DetailedMap.Add(1006, TEXT("Invalid Platform Token: Platform authentication failed. Please restart the application and try again."));
        DetailedMap.Add(1007, TEXT("Wrong Account: The authentication credentials don't match this account. Please verify your login information."));
        DetailedMap.Add(1008, TEXT("Wrong Client: Authentication failed due to client mismatch. Please restart the application."));
        DetailedMap.Add(1009, TEXT("Full Account Required: A full Epic Games account is required for this operation. Please upgrade your account."));
        DetailedMap.Add(1010, TEXT("Headless Account Required: This operation requires a headless account type."));
        DetailedMap.Add(1011, TEXT("Password Reset Required: Your password needs to be reset before you can continue. Please check your email for reset instructions."));
        DetailedMap.Add(1013, TEXT("Authorization Expired: Your authorization has expired. Please sign in again."));
        DetailedMap.Add(1014, TEXT("Scope Consent Required: Additional permissions are required. Please complete the authorization process."));
        DetailedMap.Add(1018, TEXT("Account Portal Load Error: Failed to load the login page. Please check your internet connection and try again."));
        DetailedMap.Add(1019, TEXT("Corrective Action Required: Your account requires attention. Please visit the Epic Games website to resolve any issues."));
        DetailedMap.Add(1020, TEXT("Pin Grant Code: A PIN code has been generated for device authentication. Please check your email or the Epic Games website."));
        DetailedMap.Add(1021, TEXT("Pin Grant Expired: The PIN code has expired. Please request a new one."));
        DetailedMap.Add(1022, TEXT("Pin Grant Pending: Waiting for PIN code verification. Please complete the authentication process."));
        DetailedMap.Add(1060, TEXT("MFA Required: Multi-factor authentication is required. Please complete the additional security verification."));
        DetailedMap.Add(1070, TEXT("Parental Controls: Access is restricted by parental controls. Please contact your parent or guardian."));
        DetailedMap.Add(1090, TEXT("User Interface Required: Interactive login is required. Please use the account portal login method."));
        
        // Network and connection errors
        DetailedMap.Add(1, TEXT("No Connection: Unable to connect to Epic Games services. Please check your internet connection."));
        DetailedMap.Add(27, TEXT("Timed Out: The request timed out. Please check your internet connection and try again."));
        DetailedMap.Add(41, TEXT("Network Disconnected: Network connection lost. Please check your internet connection."));
        
        // General errors
        DetailedMap.Add(2, TEXT("Invalid Credentials: The username or password is incorrect. Please verify your login information."));
        DetailedMap.Add(5, TEXT("Access Denied: You don't have permission to access this resource."));
        DetailedMap.Add(8, TEXT("Too Many Requests: Too many requests sent. Please wait a moment before trying again."));
        DetailedMap.Add(17, TEXT("Canceled: The operation was canceled by the user."));
        DetailedMap.Add(35, TEXT("Service Failure: Epic Games services are currently experiencing issues. Please try again later."));
    }
    
    return DetailedMap;
}

TMap<int32, FString> UEOSErrorTranslator::GetUserFriendlyMap()
{
    static TMap<int32, FString> UserFriendlyMap;
    
    if (UserFriendlyMap.Num() == 0)
    {
        // User-friendly messages suitable for display to end users
        UserFriendlyMap.Add(1001, TEXT("Account temporarily locked. Please try again later."));
        UserFriendlyMap.Add(1002, TEXT("Account updating. Please wait a few minutes."));
        UserFriendlyMap.Add(1003, TEXT("Session expired. Please sign in again."));
        UserFriendlyMap.Add(1004, TEXT("Session expired. Please sign in again."));
        UserFriendlyMap.Add(1005, TEXT("Login failed. Please check your credentials."));
        UserFriendlyMap.Add(1006, TEXT("Platform error. Please restart the application."));
        UserFriendlyMap.Add(1007, TEXT("Account mismatch. Please verify your login."));
        UserFriendlyMap.Add(1008, TEXT("Client error. Please restart the application."));
        UserFriendlyMap.Add(1009, TEXT("Full Epic Games account required."));
        UserFriendlyMap.Add(1011, TEXT("Password reset required. Check your email."));
        UserFriendlyMap.Add(1013, TEXT("Authorization expired. Please sign in again."));
        UserFriendlyMap.Add(1014, TEXT("Additional permissions required."));
        UserFriendlyMap.Add(1018, TEXT("Login page failed to load. Check your connection."));
        UserFriendlyMap.Add(1019, TEXT("Account needs attention. Visit Epic Games website."));
        UserFriendlyMap.Add(1020, TEXT("PIN code sent. Check your email."));
        UserFriendlyMap.Add(1021, TEXT("PIN code expired. Request a new one."));
        UserFriendlyMap.Add(1022, TEXT("Waiting for PIN verification."));
        UserFriendlyMap.Add(1060, TEXT("Multi-factor authentication required."));
        UserFriendlyMap.Add(1070, TEXT("Access restricted by parental controls."));
        UserFriendlyMap.Add(1090, TEXT("Interactive login required."));
        
        // Network errors
        UserFriendlyMap.Add(1, TEXT("No internet connection. Please check your network."));
        UserFriendlyMap.Add(27, TEXT("Connection timed out. Please try again."));
        UserFriendlyMap.Add(41, TEXT("Network disconnected. Check your connection."));
        
        // General errors
        UserFriendlyMap.Add(2, TEXT("Invalid username or password."));
        UserFriendlyMap.Add(5, TEXT("Access denied."));
        UserFriendlyMap.Add(8, TEXT("Too many requests. Please wait and try again."));
        UserFriendlyMap.Add(17, TEXT("Operation canceled."));
        UserFriendlyMap.Add(35, TEXT("Service temporarily unavailable. Try again later."));
    }
    
    return UserFriendlyMap;
}