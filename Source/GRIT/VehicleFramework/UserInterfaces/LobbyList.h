#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "LobbyDescriptor.h"
#include "LobbyEntry.h"
#include "LobbyList.generated.h"

UCLASS()
class GRIT_API ULobbyList : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    class UScrollBox* LobbyScrollBox;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<ULobbyEntry> LobbyEntryWidgetClass;

    // Spacing between lobby entries (in pixels)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    float EntrySpacing = 8.0f;

protected:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Lobby List")
    void PopulateTestData();

    UFUNCTION(BlueprintCallable, Category = "Lobby List")
    void ClearLobbyList();

    UFUNCTION()
    void HandleLobbyActionClicked(int32 LobbyID, ELobbyActionType ActionType);

private:
    UPROPERTY()
    TArray<ULobbyEntry*> LobbyEntryWidgets;
};