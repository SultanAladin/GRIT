#include "LanguagePicker.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

//------------------------------------------------------------------------------
//                                  LIFECYCLE
//------------------------------------------------------------------------------

void ULanguagePicker::NativeConstruct()
{
    Super::NativeConstruct();

    // Reason: Bind button click event
    if (SelectorButton)
    {
        SelectorButton->OnClicked.AddDynamic(this, &ULanguagePicker::ProcessButtonClicked);
    } // End if (SelectorButton exists)

    // Reason: Set default language text
    if (CurrentLanguageText)
    {
        CurrentLanguageText->SetText(FText::FromString(DefaultLanguage));
    } // End if (CurrentLanguageText exists)
}

void ULanguagePicker::NativeDestruct()
{
    // Reason: Clean up container if still active
    DestroyContainer();
    Super::NativeDestruct();
}

//------------------------------------------------------------------------------
//                               PUBLIC API
//------------------------------------------------------------------------------

void ULanguagePicker::ToggleContainer()
{
    // Reason: Open/close container based on current state
    if (bContainerOpen)
    {
        CloseContainer();
    }
    else
    {
        SpawnContainer();
    } // End if (container state check)
}

void ULanguagePicker::CloseContainer()
{
    // Reason: Destroy active container
    if (bContainerOpen)
    {
        DestroyContainer();
        bContainerOpen = false;
        UE_LOG(LogTemp, Log, TEXT("[LanguagePicker] Container closed"));
    } // End if (container open check)
}

void ULanguagePicker::SetDisplayLanguage(const FString& LanguageName)
{
    // Reason: Update display text
    if (CurrentLanguageText)
    {
        CurrentLanguageText->SetText(FText::FromString(LanguageName));
    } // End if (CurrentLanguageText exists)
}

void ULanguagePicker::AddLanguage(const FString& DisplayName, const FString& LanguageCode)
{
    // Reason: Add language to container (must be called after container is spawned)
    if (ActiveContainer)
    {
        ActiveContainer->AddLanguageOption(DisplayName, LanguageCode);
    } // End if (ActiveContainer exists)
}

//------------------------------------------------------------------------------
//                          CONTAINER MANAGEMENT
//------------------------------------------------------------------------------

void ULanguagePicker::SpawnContainer()
{
    // Reason: Don't spawn if already open
    if (bContainerOpen || !ContainerClass)
    {
        return;
    } // End if (spawn check)

    // Reason: Find parent canvas panel
    UCanvasPanel* ParentCanvas = nullptr;
    UWidget* CurrentParent = GetParent();
    
    while (CurrentParent) // Reason: Walk up widget hierarchy to find canvas
    {
        ParentCanvas = Cast<UCanvasPanel>(CurrentParent);
        if (ParentCanvas)
        {
            break;
        }
        CurrentParent = CurrentParent->GetParent();
    } // End while (parent search)

    if (!ParentCanvas) // Reason: Canvas required for positioning
    {
        UE_LOG(LogTemp, Error, TEXT("[LanguagePicker] No CanvasPanel found in hierarchy - cannot spawn container"));
        return;
    } // End if (ParentCanvas check)

    // Reason: Create container widget
    ActiveContainer = CreateWidget<ULanguagePickerPanel>(this, ContainerClass);
    
    if (ActiveContainer)
    {
        // Reason: Add to canvas panel
        UCanvasPanelSlot* CanvasSlot = ParentCanvas->AddChildToCanvas(ActiveContainer);
        
        if (CanvasSlot) // Reason: Configure slot for absolute positioning
        {
            CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
            CanvasSlot->SetAutoSize(true);
            CanvasSlot->SetZOrder(1000);
        } // End if (CanvasSlot created)

        // Reason: Bind language change event
        ActiveContainer->OnLanguageChanged.AddDynamic(this, &ULanguagePicker::ProcessLanguageChanged);

        // Reason: Position relative to this button
        ActiveContainer->PositionRelativeToButton(this);

        bContainerOpen = true;
        UE_LOG(LogTemp, Log, TEXT("[LanguagePicker] Container spawned"));
    } // End if (ActiveContainer created)
}

void ULanguagePicker::DestroyContainer()
{
    // Reason: Remove container from viewport
    if (ActiveContainer)
    {
        ActiveContainer->RemoveFromParent();
        ActiveContainer = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[LanguagePicker] Container destroyed"));
    } // End if (ActiveContainer exists)
}

//------------------------------------------------------------------------------
//                             EVENT HANDLERS
//------------------------------------------------------------------------------

void ULanguagePicker::ProcessButtonClicked()
{
    // Reason: Toggle container on button click
    ToggleContainer();
}

void ULanguagePicker::ProcessLanguageChanged(const FString& SelectedLanguage)
{
    // Reason: Update display and close container
    SetDisplayLanguage(SelectedLanguage);
    CloseContainer();
    
    UE_LOG(LogTemp, Log, TEXT("[LanguagePicker] Language changed to: %s"), *SelectedLanguage);
}
