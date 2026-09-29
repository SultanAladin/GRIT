#include "SettingsMenu.h"
#include "Components/ButtonSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Spacer.h"
#include "../../GameContext/SessionAdapter.h"
#include "../../GameContext/UserPreferences.h"
#include "../Components/ThemeUtil.h"

//------------------------------------------------------------------------------
//                          BUTTON CALLBACK HELPER
//------------------------------------------------------------------------------

void USettingsButtonCallback::OnClicked()
{
    if (OwnerMenu)
    {
        if (bIsLoginMethod)
        {
            OwnerMenu->ProcessLoginMethodClicked(ButtonIndex);
        }
        else
        {
            OwnerMenu->ProcessLanguageClicked(ButtonIndex);
        } // End if (type check)
    } // End if (OwnerMenu exists)
}

void USettingsButtonCallback::OnHovered()
{
    if (OwnerMenu)
    {
        UButton* Button = nullptr;
        bool bIsSelected = false;

        if (bIsLoginMethod && ButtonIndex < OwnerMenu->LoginMethodButtons.Num())
        {
            Button = OwnerMenu->LoginMethodButtons[ButtonIndex];
            bIsSelected = (ButtonIndex < OwnerMenu->ActiveLoginMethods.Num() && OwnerMenu->ActiveLoginMethods[ButtonIndex] == OwnerMenu->GetCurrentLoginMethod());
        }
        else if (!bIsLoginMethod && ButtonIndex < OwnerMenu->LanguageButtons.Num())
        {
            Button = OwnerMenu->LanguageButtons[ButtonIndex];
            bIsSelected = (ButtonIndex < OwnerMenu->ActiveLanguages.Num() && OwnerMenu->ActiveLanguages[ButtonIndex] == OwnerMenu->GetCurrentLanguage());
        } // End if (type check)

        if (Button && !bIsSelected)
        {
            OwnerMenu->ApplyButtonStyle(Button, false, true);
        } // End if (button and not selected)
    } // End if (OwnerMenu exists)
}

void USettingsButtonCallback::OnUnhovered()
{
    if (OwnerMenu)
    {
        UButton* Button = nullptr;
        bool bIsSelected = false;

        if (bIsLoginMethod && ButtonIndex < OwnerMenu->LoginMethodButtons.Num())
        {
            Button = OwnerMenu->LoginMethodButtons[ButtonIndex];
            bIsSelected = (ButtonIndex < OwnerMenu->ActiveLoginMethods.Num() && OwnerMenu->ActiveLoginMethods[ButtonIndex] == OwnerMenu->GetCurrentLoginMethod());
        }
        else if (!bIsLoginMethod && ButtonIndex < OwnerMenu->LanguageButtons.Num())
        {
            Button = OwnerMenu->LanguageButtons[ButtonIndex];
            bIsSelected = (ButtonIndex < OwnerMenu->ActiveLanguages.Num() && OwnerMenu->ActiveLanguages[ButtonIndex] == OwnerMenu->GetCurrentLanguage());
        } // End if (type check)

        if (Button)
        {
            OwnerMenu->ApplyButtonStyle(Button, bIsSelected, false);
        } // End if (button exists)
    } // End if (OwnerMenu exists)
}

//------------------------------------------------------------------------------
//                                  LIFECYCLE
//------------------------------------------------------------------------------

USettingsMenu::USettingsMenu(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USettingsMenu::NativeConstruct()
{
    Super::NativeConstruct();

    LoginMethodManager = ULoginMethodManager::GetInstance();
    LanguageManager = ULanguageManager::GetInstance();

    // Reason: Apply background colors with rounded corners
    if (RootBorder)
    {
        RootBorder->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.113043f));
    } // End if (RootBorder exists)

    // Reason: Apply consistent border styling to both sections using theme-based radius
    float SectionRadius = GetRadiusFromEnum(SectionCornerRadius);
    
    if (LoginMethodBorder)
    {
        UThemeUtil::ApplyBorderStyling(LoginMethodBorder, BackgroundColor, SectionRadius);
    } // End if (LoginMethodBorder exists)

    if (LanguageBorder)
    {
        UThemeUtil::ApplyBorderStyling(LanguageBorder, BackgroundColor, SectionRadius);
    } // End if (LanguageBorder exists)

    if (LoginMethodHeader)
    {
        LoginMethodHeader->SetText(FText::FromString("Login Methods"));
        LoginMethodHeader->SetColorAndOpacity(HeaderColor);
    } // End if (LoginMethodHeader exists)

    if (LanguageHeader)
    {
        LanguageHeader->SetText(FText::FromString("Languages"));
        LanguageHeader->SetColorAndOpacity(HeaderColor);
    } // End if (LanguageHeader exists)

    // Reason: Sync with global managers if enabled
    if (bSyncWithGlobalManagers)
    {
        if (LoginMethodManager)
        {
            CurrentLoginMethod = LoginMethodManager->GetCurrentLoginMethod();
            LoginMethodManager->OnLoginMethodChanged.AddDynamic(this, &USettingsMenu::ProcessGlobalLoginMethodChanged);
        } // End if (LoginMethodManager exists)

        if (LanguageManager)
        {
            CurrentLanguage = LanguageManager->GetCurrentLanguage();
            LanguageManager->OnLanguageChanged.AddDynamic(this, &USettingsMenu::ProcessGlobalLanguageChanged);
        } // End if (LanguageManager exists)
    } // End if (sync enabled)

    PopulateLoginMethods();
    PopulateLanguages();
}

void USettingsMenu::NativeDestruct()
{
    if (LoginMethodManager)
    {
        LoginMethodManager->OnLoginMethodChanged.RemoveDynamic(this, &USettingsMenu::ProcessGlobalLoginMethodChanged);
    } // End if (LoginMethodManager exists)

    if (LanguageManager)
    {
        LanguageManager->OnLanguageChanged.RemoveDynamic(this, &USettingsMenu::ProcessGlobalLanguageChanged);
    } // End if (LanguageManager exists)

    Super::NativeDestruct();
}

//------------------------------------------------------------------------------
//                               PUBLIC API
//------------------------------------------------------------------------------

void USettingsMenu::SetCurrentLanguage(ELanguage Language)
{
    if (CurrentLanguage == Language) return;

    CurrentLanguage = Language;
    RefreshLanguageButtons();

    if (bUpdateGlobalManagers && LanguageManager)
    {
        LanguageManager->SetCurrentLanguage(Language);
    } // End if (update enabled)

    if (UWorld* World = GetWorld())
    {
        if (USessionAdapter* SessionAdapter = Cast<USessionAdapter>(World->GetGameInstance()))
        {
            if (UUserPreferencesManager* PrefsManager = SessionAdapter->GetUserPreferencesManager())
            {
                PrefsManager->SetPreferredLanguage(Language, true);
            } // End if (PrefsManager exists)
        } // End if (SessionAdapter exists)
    } // End if (World exists)
}

void USettingsMenu::SetCurrentLoginMethod(ELoginMethod LoginMethod)
{
    if (CurrentLoginMethod == LoginMethod) return;

    CurrentLoginMethod = LoginMethod;
    RefreshLoginMethodButtons();

    if (bUpdateGlobalManagers && LoginMethodManager)
    {
        LoginMethodManager->SetCurrentLoginMethod(LoginMethod);
    } // End if (update enabled)

    if (UWorld* World = GetWorld())
    {
        if (USessionAdapter* SessionAdapter = Cast<USessionAdapter>(World->GetGameInstance()))
        {
            if (UUserPreferencesManager* PrefsManager = SessionAdapter->GetUserPreferencesManager())
            {
                PrefsManager->SetPreferredLoginMethod(LoginMethod, true);
            } // End if (PrefsManager exists)
        } // End if (SessionAdapter exists)
    } // End if (World exists)
}

//------------------------------------------------------------------------------
//                          POPULATION PIPELINE
//------------------------------------------------------------------------------

void USettingsMenu::PopulateLoginMethods()
{
    if (!LoginMethodList) return;

    // Reason: Determine which methods to include
    if (IncludedLoginMethods.Num() > 0)
    {
        ActiveLoginMethods = IncludedLoginMethods;
    }
    else
    {
        ActiveLoginMethods = ULoginMethodManager::GetAllLoginMethods();
    } // End if (included methods check)

    LoginMethodButtons.Empty();

    // Reason: Create button for each login method
    for (int32 i = 0; i < ActiveLoginMethods.Num(); ++i)
    {
        ELoginMethod LoginMethod = ActiveLoginMethods[i];
        FText Label = ULoginMethodManager::GetLoginMethodDisplayName(LoginMethod);
        UButton* Button = CreateOptionButton(Label, i);

        if (Button)
        {
            LoginMethodButtons.Add(Button);
            
            USettingsButtonCallback* Callback = CreateButtonCallback(i, true);
            Button->OnClicked.AddDynamic(Callback, &USettingsButtonCallback::OnClicked);
            Button->OnHovered.AddDynamic(Callback, &USettingsButtonCallback::OnHovered);
            Button->OnUnhovered.AddDynamic(Callback, &USettingsButtonCallback::OnUnhovered);
            
            UVerticalBoxSlot* BoxSlot = LoginMethodList->AddChildToVerticalBox(Button);
            if (BoxSlot)
            {
                BoxSlot->SetHorizontalAlignment(HAlign_Fill);
                BoxSlot->SetPadding(FMargin(0.0f));
            } // End if (BoxSlot created)

            // Reason: Add spacing between entries
            if (i < ActiveLoginMethods.Num() - 1)
            {
                USpacer* Spacer = NewObject<USpacer>(this);
                if (Spacer)
                {
                    UVerticalBoxSlot* SpacerSlot = LoginMethodList->AddChildToVerticalBox(Spacer);
                    if (SpacerSlot)
                    {
                        SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                        SpacerSlot->SetPadding(FMargin(0.0f, EntrySpacing, 0.0f, 0.0f));
                    } // End if (SpacerSlot created)
                } // End if (Spacer created)
            } // End if (not last entry)
        } // End if (Button created)
    } // End for (ActiveLoginMethods)

    RefreshLoginMethodButtons();
}

void USettingsMenu::PopulateLanguages()
{
    if (!LanguageList) return;

    // Reason: Determine which languages to include
    if (IncludedLanguages.Num() > 0)
    {
        ActiveLanguages = IncludedLanguages;
    }
    else
    {
        ActiveLanguages = ULanguageManager::GetAllLanguages();
    } // End if (included languages check)

    LanguageButtons.Empty();

    // Reason: Create button for each language
    for (int32 i = 0; i < ActiveLanguages.Num(); ++i)
    {
        ELanguage Language = ActiveLanguages[i];
        FText Label = ULanguageManager::GetLanguageDisplayName(Language);
        UButton* Button = CreateOptionButton(Label, i);

        if (Button)
        {
            LanguageButtons.Add(Button);
            
            USettingsButtonCallback* Callback = CreateButtonCallback(i, false);
            Button->OnClicked.AddDynamic(Callback, &USettingsButtonCallback::OnClicked);
            Button->OnHovered.AddDynamic(Callback, &USettingsButtonCallback::OnHovered);
            Button->OnUnhovered.AddDynamic(Callback, &USettingsButtonCallback::OnUnhovered);
            
            UVerticalBoxSlot* BoxSlot = LanguageList->AddChildToVerticalBox(Button);
            if (BoxSlot)
            {
                BoxSlot->SetHorizontalAlignment(HAlign_Fill);
                BoxSlot->SetPadding(FMargin(0.0f));
            } // End if (BoxSlot created)

            // Reason: Add spacing between entries
            if (i < ActiveLanguages.Num() - 1)
            {
                USpacer* Spacer = NewObject<USpacer>(this);
                if (Spacer)
                {
                    UVerticalBoxSlot* SpacerSlot = LanguageList->AddChildToVerticalBox(Spacer);
                    if (SpacerSlot)
                    {
                        SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                        SpacerSlot->SetPadding(FMargin(0.0f, EntrySpacing, 0.0f, 0.0f));
                    } // End if (SpacerSlot created)
                } // End if (Spacer created)
            } // End if (not last entry)
        } // End if (Button created)
    } // End for (ActiveLanguages)

    RefreshLanguageButtons();
}

UButton* USettingsMenu::CreateOptionButton(const FText& Label, int32 Index)
{
    UButton* Button = NewObject<UButton>(this);
    
    if (Button)
    {
        Button->SetBackgroundColor(DefaultButtonColor);
        
        UTextBlock* TextBlock = NewObject<UTextBlock>(Button);
        if (TextBlock)
        {
            TextBlock->SetText(Label);
            TextBlock->SetColorAndOpacity(FLinearColor::White);
            Button->AddChild(TextBlock);
        } // End if (TextBlock created)
    } // End if (Button created)

    return Button;
}

//------------------------------------------------------------------------------
//                          VISUAL REFRESH PIPELINE
//------------------------------------------------------------------------------

void USettingsMenu::RefreshLoginMethodButtons()
{
    for (int32 i = 0; i < LoginMethodButtons.Num(); ++i)
    {
        if (LoginMethodButtons[i])
        {
            bool bIsSelected = (i < ActiveLoginMethods.Num() && ActiveLoginMethods[i] == CurrentLoginMethod);
            ApplyButtonStyle(LoginMethodButtons[i], bIsSelected);
        } // End if (button exists)
    } // End for (LoginMethodButtons)
}

void USettingsMenu::RefreshLanguageButtons()
{
    for (int32 i = 0; i < LanguageButtons.Num(); ++i)
    {
        if (LanguageButtons[i])
        {
            bool bIsSelected = (i < ActiveLanguages.Num() && ActiveLanguages[i] == CurrentLanguage);
            ApplyButtonStyle(LanguageButtons[i], bIsSelected);
        } // End if (button exists)
    } // End for (LanguageButtons)
}

void USettingsMenu::ApplyButtonStyle(UButton* Button, bool bIsSelected, bool bIsHovered)
{
    if (!Button) return;

    FLinearColor TargetColor = DefaultButtonColor;  // [RGBA]
    FLinearColor BorderColor = DefaultBorderColor;  // [RGBA]
    float BorderThickness = DefaultBorderThickness;  // [px]
    
    if (bIsSelected)
    {
        TargetColor = SelectedButtonColor;
        BorderColor = SelectedBorderColor;
        BorderThickness = SelectedBorderThickness;
    }
    else if (bIsHovered)
    {
        TargetColor = HoverButtonColor;
        BorderColor = HoverBorderColor;
        BorderThickness = HoverBorderThickness;
    } // End if (state check)

    Button->SetBackgroundColor(TargetColor);

    // Reason: Use theme-based corner radius for buttons
    float ButtonRadius = GetRadiusFromEnum(ButtonCornerRadius);

    FButtonStyle Style = Button->GetStyle();
    Style.Normal.OutlineSettings.Color = FSlateColor(BorderColor);
    Style.Normal.OutlineSettings.Width = BorderThickness;
    Style.Normal.OutlineSettings.CornerRadii = FVector4(ButtonRadius, ButtonRadius, ButtonRadius, ButtonRadius);
    Style.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Style.Hovered.OutlineSettings.Color = FSlateColor(BorderColor);
    Style.Hovered.OutlineSettings.Width = BorderThickness;
    Style.Hovered.OutlineSettings.CornerRadii = FVector4(ButtonRadius, ButtonRadius, ButtonRadius, ButtonRadius);
    Style.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Style.Pressed.OutlineSettings.Color = FSlateColor(BorderColor);
    Style.Pressed.OutlineSettings.Width = BorderThickness;
    Style.Pressed.OutlineSettings.CornerRadii = FVector4(ButtonRadius, ButtonRadius, ButtonRadius, ButtonRadius);
    Style.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Button->SetStyle(Style);

    if (Button->GetChildrenCount() > 0)
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(Button->GetChildAt(0)))
        {
            TextBlock->SetColorAndOpacity(bIsSelected ? FLinearColor::Black : FLinearColor::White);
        } // End if (TextBlock found)
    } // End if (has children)
}

//------------------------------------------------------------------------------
//                             EVENT HANDLERS
//------------------------------------------------------------------------------

void USettingsMenu::ProcessLoginMethodClicked(int32 MethodIndex)
{
    if (MethodIndex >= 0 && MethodIndex < ActiveLoginMethods.Num())
    {
        SetCurrentLoginMethod(ActiveLoginMethods[MethodIndex]);
    } // End if (valid index)
}

void USettingsMenu::ProcessLanguageClicked(int32 LanguageIndex)
{
    if (LanguageIndex >= 0 && LanguageIndex < ActiveLanguages.Num())
    {
        SetCurrentLanguage(ActiveLanguages[LanguageIndex]);
    } // End if (valid index)
}

void USettingsMenu::ProcessGlobalLoginMethodChanged(ELoginMethod NewLoginMethod)
{
    if (NewLoginMethod != CurrentLoginMethod)
    {
        CurrentLoginMethod = NewLoginMethod;
        RefreshLoginMethodButtons();
    } // End if (different method)
}

void USettingsMenu::ProcessGlobalLanguageChanged(ELanguage NewLanguage)
{
    if (NewLanguage != CurrentLanguage)
    {
        CurrentLanguage = NewLanguage;
        RefreshLanguageButtons();
    } // End if (different language)
}

//------------------------------------------------------------------------------
//                          HELPER MANAGEMENT
//------------------------------------------------------------------------------

USettingsButtonCallback* USettingsMenu::CreateButtonCallback(int32 Index, bool bIsLoginMethod)
{
    USettingsButtonCallback* Callback = NewObject<USettingsButtonCallback>(this);
    Callback->OwnerMenu = this;
    Callback->ButtonIndex = Index;
    Callback->bIsLoginMethod = bIsLoginMethod;
    
    ButtonCallbacks.Add(Callback);
    
    return Callback;
}

float USettingsMenu::GetRadiusFromEnum(ECornerRadius Radius) const
{
    FBorderSpec BorderSpec = UThemeUtil::FetchBorderSpec(this);
    
    switch (Radius)
    {
        case ECornerRadius::None:   return BorderSpec.RadiusNone;
        case ECornerRadius::Tight:  return BorderSpec.RadiusTight;
        case ECornerRadius::Snug:   return BorderSpec.RadiusSnug;
        case ECornerRadius::Loose:  return BorderSpec.RadiusLoose;
        case ECornerRadius::Round:  return BorderSpec.RadiusRound;
        case ECornerRadius::Full:   return BorderSpec.RadiusFull;
        default:                    return BorderSpec.RadiusSnug;
    } // End switch (Radius)
}