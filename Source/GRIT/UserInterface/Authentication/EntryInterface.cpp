//EntryInterface.cpp
#include "EntryInterface.h"
#include "SessionAdapter.h"
#include "Kismet/GameplayStatics.h"
#include "ColourCodex/Public/ThemeConfiguration.h"

UEntryInterface::UEntryInterface(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , EntryOverlay(nullptr)
    , UsernameText(nullptr)
    , DisplayText(nullptr)
    , DescriptionText(nullptr)
    , EntrySlideIn(nullptr)
    , CachedSessionAdapter(nullptr)
{
    DefaultDisplayText = FText::FromString("Welcome back");
    DefaultDescriptionText = FText::FromString("Ready to play");
    bUseHeaderFontForUsername = true;
}

void UEntryInterface::NativeConstruct()
{
    Super::NativeConstruct();

    // Apply initial theme
    ApplyThemeStyle();

    // Set default texts
    if (DisplayText)
    {
        DisplayText->SetText(DefaultDisplayText);
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(DefaultDescriptionText);
    }

    OnEntryInitialized();
}

//------------------------------------------------------------------------------
//                                    Helper functions
//------------------------------------------------------------------------------

USessionAdapter* UEntryInterface::GetSessionAdapter()
{
    if (!CachedSessionAdapter)
    {
        CachedSessionAdapter = Cast<USessionAdapter>(UGameplayStatics::GetGameInstance(this));
    }
    return CachedSessionAdapter;
}

//------------------------------------------------------------------------------
//                                    Public API
//------------------------------------------------------------------------------

void UEntryInterface::SetUsername(const FString& Username)
{
    CurrentUsername = Username;

    if (UsernameText)
    {
        UsernameText->SetText(FText::FromString(Username));
    }
}

void UEntryInterface::SetDisplayText(const FText& Text)
{
    if (DisplayText)
    {
        DisplayText->SetText(Text);
    }
}

void UEntryInterface::SetDescriptionText(const FText& Text)
{
    if (DescriptionText)
    {
        DescriptionText->SetText(Text);
    }
}

void UEntryInterface::ApplyThemeStyle()
{
    USessionAdapter* SessionAdapter = GetSessionAdapter();
    if (!SessionAdapter) { return; }

    // TODO: Apply theme styling - will be set via editor

    // Reason: Style username text
    if (UsernameText)
    {
        // TODO: Set username text styling via editor
    }

    // Reason: Style display text
    if (DisplayText)
    {
        // TODO: Set display text styling via editor
    }

    // Reason: Style description text
    if (DescriptionText)
    {
        // TODO: Set description text styling via editor
    }
}

//------------------------------------------------------------------------------
//                           Animation pipeline
//------------------------------------------------------------------------------

void UEntryInterface::TriggerEntrySequence()
{
    // Reason: Play entry animation if available
    if (EntrySlideIn)
    {
        bAnimationComplete = false;
        PlayAnimation(EntrySlideIn, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
        FWidgetAnimationDynamicEvent AnimationFinishedDelegate;
        AnimationFinishedDelegate.BindDynamic(this, &UEntryInterface::ProcessAnimationComplete);
        BindToAnimationFinished(EntrySlideIn, AnimationFinishedDelegate);
    }
    else
    {
        bAnimationComplete = true;
        OnAnimationComplete.Broadcast();
        OnEntryAnimationComplete();
    }
} // End if (EntrySlideIn check)

void UEntryInterface::ProcessAnimationComplete()
{
    bAnimationComplete = true;
    OnAnimationComplete.Broadcast();
    OnEntryAnimationComplete();
}
