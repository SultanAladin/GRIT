#pragma once

#include "CoreMinimal.h"
#include "GenericTray.h"
#include "SettingsTray.generated.h"

class USettingsMenu;

/*====================================================================================================================================
                                                         SETTINGS TRAY
======================================================================================================================================*/

UCLASS()
class GRIT_API USettingsTray : public UGenericTray
{
    GENERATED_BODY()

public:
    /** Retrieve settings menu reference */
    USettingsMenu* RetrieveSettingsMenu() const { return LoginSettings; }

protected:
    virtual void NativeConstruct() override;

private:
    //------------------------------------------------------------------------------
    // Bound Widgets
    //------------------------------------------------------------------------------
    UPROPERTY(meta = (BindWidget))
    USettingsMenu* LoginSettings;  // Settings configuration panel
};
