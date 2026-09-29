#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LoginMethodTypes.h"
#include "LanguageTypes.h"
#include "../Components/ThemeConfig.h"
#include "SettingsMenu.generated.h"

//------------------------------------------------------------------------------
//                          BUTTON CALLBACK HELPER
//------------------------------------------------------------------------------

UCLASS()
class USettingsButtonCallback : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    class USettingsMenu* OwnerMenu;

    int32 ButtonIndex = -1;
    bool bIsLoginMethod = false;

    UFUNCTION()
    void OnClicked();

    UFUNCTION()
    void OnHovered();

    UFUNCTION()
    void OnUnhovered();
};

//------------------------------------------------------------------------------
//                              SETTINGS MENU
//------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class GRIT_API USettingsMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    USettingsMenu(const FObjectInitializer& ObjectInitializer);

    /** Set current language and refresh UI */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetCurrentLanguage(ELanguage Language);

    /** Set current login method and refresh UI */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetCurrentLoginMethod(ELoginMethod LoginMethod);

    /** Get current language */
    UFUNCTION(BlueprintPure, Category = "Settings")
    ELanguage GetCurrentLanguage() const { return CurrentLanguage; }

    /** Get current login method */
    UFUNCTION(BlueprintPure, Category = "Settings")
    ELoginMethod GetCurrentLoginMethod() const { return CurrentLoginMethod; }

    /** Apply button styling (public for callback helper) */
    void ApplyButtonStyle(UButton* Button, bool bIsSelected, bool bIsHovered = false);

    /** Process button clicks (public for callback helper) */
    void ProcessLoginMethodClicked(int32 MethodIndex);
    void ProcessLanguageClicked(int32 LanguageIndex);

    UPROPERTY()
    TArray<UButton*> LoginMethodButtons;
    
    UPROPERTY()
    TArray<UButton*> LanguageButtons;
    
    TArray<ELoginMethod> ActiveLoginMethods;
    TArray<ELanguage> ActiveLanguages;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* RootBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* LoginMethodBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LoginMethodHeader;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* LoginMethodList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* LanguageBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LanguageHeader;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* LanguageList;

    //------------------------------------------------------------------------------
    // Color configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor DefaultButtonColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);  // [RGBA] - Black

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor HoverButtonColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);  // [RGBA] - Grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor SelectedButtonColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);  // [RGBA] - White

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor DefaultBorderColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);  // [RGBA] - Dark grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor HoverBorderColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);  // [RGBA] - Light grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor SelectedBorderColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);  // [RGBA] - White

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor BackgroundColor = FLinearColor(0.036458f, 0.036458f, 0.036458f, 1.0f);  // [RGBA] - Dark grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor HeaderColor = FLinearColor(0.026042f, 0.026042f, 0.026042f, 1.0f);  // [RGBA] - Darker grey

    //------------------------------------------------------------------------------
    // Border and spacing configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Borders")
    float DefaultBorderThickness = 1.0f;  // [px] - Border width normal state

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Borders")
    float HoverBorderThickness = 2.0f;  // [px] - Border width hover state

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Borders")
    float SelectedBorderThickness = 3.0f;  // [px] - Border width selected state

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Borders")
    ECornerRadius ButtonCornerRadius = ECornerRadius::Tight;  // [enum] - Entry button corner radius

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Borders")
    ECornerRadius SectionCornerRadius = ECornerRadius::Snug;  // [enum] - Section border corner radius

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    float EntrySpacing = 8.0f;  // [px] - Vertical gap between option buttons

    //------------------------------------------------------------------------------
    // Content configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TArray<ELoginMethod> IncludedLoginMethods;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TArray<ELanguage> IncludedLanguages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    bool bSyncWithGlobalManagers = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    bool bUpdateGlobalManagers = true;

private:
    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------

    UPROPERTY()
    ULoginMethodManager* LoginMethodManager;

    UPROPERTY()
    ULanguageManager* LanguageManager;

    UPROPERTY()
    TArray<USettingsButtonCallback*> ButtonCallbacks;

    ELoginMethod CurrentLoginMethod = ELoginMethod::AccountPortal;
    ELanguage CurrentLanguage = ELanguage::English;

    //------------------------------------------------------------------------------
    // Population pipeline
    //------------------------------------------------------------------------------

    void PopulateLoginMethods();
    void PopulateLanguages();
    UButton* CreateOptionButton(const FText& Label, int32 Index);

    //------------------------------------------------------------------------------
    // Visual refresh pipeline
    //------------------------------------------------------------------------------

    void RefreshLoginMethodButtons();
    void RefreshLanguageButtons();

    //------------------------------------------------------------------------------
    // Event handlers
    //------------------------------------------------------------------------------

    UFUNCTION()
    void ProcessGlobalLoginMethodChanged(ELoginMethod NewLoginMethod);

    UFUNCTION()
    void ProcessGlobalLanguageChanged(ELanguage NewLanguage);

    //------------------------------------------------------------------------------
    // Helper management
    //------------------------------------------------------------------------------

    USettingsButtonCallback* CreateButtonCallback(int32 Index, bool bIsLoginMethod);

    /** Convert ECornerRadius enum to pixel value using theme border spec */
    float GetRadiusFromEnum(ECornerRadius Radius) const;
};
