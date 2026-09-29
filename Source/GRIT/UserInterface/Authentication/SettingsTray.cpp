#include "SettingsTray.h"
#include "SettingsMenu.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

void USettingsTray::NativeConstruct()
{
    Super::NativeConstruct();

    // Reason: Validate settings menu binding
    if (!LoginSettings)
    {
        UE_LOG(LogTemp, Error, TEXT("[SettingsTray] LoginSettings widget not bound - check Blueprint hierarchy"));
    }
}
