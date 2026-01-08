#include "LobbyList.h"
#include "LobbyDescriptor.h"
#include "LobbyEntry.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Engine/Engine.h"

void ULobbyList::NativeConstruct()
{
    Super::NativeConstruct();

    PopulateTestData();
}

void ULobbyList::ClearLobbyList()
{
    if (!LobbyScrollBox)
        return;

    // Clear the scroll box
    LobbyScrollBox->ClearChildren();
    
    // Clear our widget array
    LobbyEntryWidgets.Empty();
}

void ULobbyList::PopulateTestData()
{
    if (!LobbyScrollBox || !LobbyEntryWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyScrollBox or LobbyEntryWidgetClass is null!"));
        return;
    }

    // Clear existing data
    ClearLobbyList();

    // Create test data
    TArray<FLobbyData> TestLobbies;
    
    // Entry 1
    FLobbyData Lobby1;
    Lobby1.ID = 1001;
    Lobby1.Title = TEXT("Morning Practice");
    Lobby1.Class = TEXT("Formula 1");
    Lobby1.Track = TEXT("Monza");
    Lobby1.Date = TEXT("Today");
    Lobby1.Time = TEXT("09:30");
    Lobby1.CurrentSpots = 12;
    Lobby1.MaxSpots = 20;
    Lobby1.Entry = TEXT("Free");
    Lobby1.Status = ELobbyStatus::Open;
    TestLobbies.Add(Lobby1);

    // Entry 2
    FLobbyData Lobby2;
    Lobby2.ID = 1002;
    Lobby2.Title = TEXT("Championship Race");
    Lobby2.Class = TEXT("GT3");
    Lobby2.Track = TEXT("Silverstone");
    Lobby2.Date = TEXT("Today");
    Lobby2.Time = TEXT("14:00");
    Lobby2.CurrentSpots = 24;
    Lobby2.MaxSpots = 24;
    Lobby2.Entry = TEXT("Premium");
    Lobby2.Status = ELobbyStatus::Starting;
    TestLobbies.Add(Lobby2);

    // Entry 3
    FLobbyData Lobby3;
    Lobby3.ID = 1003;
    Lobby3.Title = TEXT("Endurance Challenge");
    Lobby3.Class = TEXT("LMP1");
    Lobby3.Track = TEXT("Le Mans");
    Lobby3.Date = TEXT("Tomorrow");
    Lobby3.Time = TEXT("18:00");
    Lobby3.CurrentSpots = 16;
    Lobby3.MaxSpots = 30;
    Lobby3.Entry = TEXT("VIP");
    Lobby3.Status = ELobbyStatus::InMatch;
    TestLobbies.Add(Lobby3);

    // Entry 4
    FLobbyData Lobby4;
    Lobby4.ID = 1004;
    Lobby4.Title = TEXT("Street Circuit Sprint");
    Lobby4.Class = TEXT("Formula E");
    Lobby4.Track = TEXT("Monaco");
    Lobby4.Date = TEXT("Today");
    Lobby4.Time = TEXT("20:30");
    Lobby4.CurrentSpots = 8;
    Lobby4.MaxSpots = 16;
    Lobby4.Entry = TEXT("Free");
    Lobby4.Status = ELobbyStatus::Open;
    TestLobbies.Add(Lobby4);

    // Entry 5
    FLobbyData Lobby5;
    Lobby5.ID = 1005;
    Lobby5.Title = TEXT("Rally Cross Event");
    Lobby5.Class = TEXT("Rally");
    Lobby5.Track = TEXT("Finland");
    Lobby5.Date = TEXT("Tomorrow");
    Lobby5.Time = TEXT("16:45");
    Lobby5.CurrentSpots = 12;
    Lobby5.MaxSpots = 12;
    Lobby5.Entry = TEXT("Standard");
    Lobby5.Status = ELobbyStatus::Open;
    TestLobbies.Add(Lobby5);

    // Entry 6
    FLobbyData Lobby6;
    Lobby6.ID = 1006;
    Lobby6.Title = TEXT("Night Racing Series");
    Lobby6.Class = TEXT("NASCAR");
    Lobby6.Track = TEXT("Daytona");
    Lobby6.Date = TEXT("Today");
    Lobby6.Time = TEXT("22:00");
    Lobby6.CurrentSpots = 18;
    Lobby6.MaxSpots = 40;
    Lobby6.Entry = TEXT("Premium");
    Lobby6.Status = ELobbyStatus::Starting;
    TestLobbies.Add(Lobby6);

    // Entry 7
    FLobbyData Lobby7;
    Lobby7.ID = 1007;
    Lobby7.Title = TEXT("Rookie Tournament");
    Lobby7.Class = TEXT("Formula 3");
    Lobby7.Track = TEXT("Spa");
    Lobby7.Date = TEXT("Tomorrow");
    Lobby7.Time = TEXT("12:15");
    Lobby7.CurrentSpots = 5;
    Lobby7.MaxSpots = 20;
    Lobby7.Entry = TEXT("Free");
    Lobby7.Status = ELobbyStatus::Open;
    TestLobbies.Add(Lobby7);

    // Entry 8
    FLobbyData Lobby8;
    Lobby8.ID = 1008;
    Lobby8.Title = TEXT("Drift Competition");
    Lobby8.Class = TEXT("Drift");
    Lobby8.Track = TEXT("Tokyo");
    Lobby8.Date = TEXT("Today");
    Lobby8.Time = TEXT("19:30");
    Lobby8.CurrentSpots = 10;
    Lobby8.MaxSpots = 16;
    Lobby8.Entry = TEXT("Standard");
    Lobby8.Status = ELobbyStatus::InMatch;
    TestLobbies.Add(Lobby8);

    // Entry 9
    FLobbyData Lobby9;
    Lobby9.ID = 1009;
    Lobby9.Title = TEXT("Historic Grand Prix");
    Lobby9.Class = TEXT("Classic F1");
    Lobby9.Track = TEXT("Brands Hatch");
    Lobby9.Date = TEXT("Tomorrow");
    Lobby9.Time = TEXT("15:00");
    Lobby9.CurrentSpots = 14;
    Lobby9.MaxSpots = 22;
    Lobby9.Entry = TEXT("VIP");
    Lobby9.Status = ELobbyStatus::Open;
    TestLobbies.Add(Lobby9);

    // Entry 10
    FLobbyData Lobby10;
    Lobby10.ID = 1010;
    Lobby10.Title = TEXT("Open Wheel Challenge");
    Lobby10.Class = TEXT("IndyCar");
    Lobby10.Track = TEXT("Indianapolis");
    Lobby10.Date = TEXT("Today");
    Lobby10.Time = TEXT("17:30");
    Lobby10.CurrentSpots = 25;
    Lobby10.MaxSpots = 33;
    Lobby10.Entry = TEXT("Premium");
    Lobby10.Status = ELobbyStatus::Starting;
    TestLobbies.Add(Lobby10);

    // Create and add lobby entry widgets
    for (int32 i = 0; i < TestLobbies.Num(); i++)
    {
        // Add spacer before each entry (except the first one)
        if (i > 0 && EntrySpacing > 0.0f)
        {
            USpacer* SpacerWidget = NewObject<USpacer>(this);
            if (SpacerWidget)
            {
                SpacerWidget->SetSize(FVector2D(1.0f, EntrySpacing));
                LobbyScrollBox->AddChild(SpacerWidget);
            }
        }

        ULobbyEntry* LobbyEntryWidget = CreateWidget<ULobbyEntry>(this, LobbyEntryWidgetClass);
        if (LobbyEntryWidget)
        {
            // Set up the lobby data
            bool bPlayerWasInLobby = (i == 2); // Example: player was in lobby 3 (Endurance Challenge)
            LobbyEntryWidget->SetLobbyData(TestLobbies[i], i + 1, bPlayerWasInLobby);
            
            // Bind to the action clicked event
            LobbyEntryWidget->OnActionClicked.AddDynamic(this, &ULobbyList::HandleLobbyActionClicked);
            
            // Add to scroll box
            LobbyScrollBox->AddChild(LobbyEntryWidget);
            
            // Keep reference for later use
            LobbyEntryWidgets.Add(LobbyEntryWidget);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create LobbyEntry widget for index %d"), i);
        }
    }
}

void ULobbyList::HandleLobbyActionClicked(int32 LobbyID, ELobbyActionType ActionType)
{
    // Handle the action based on type
    switch (ActionType)
    {
        case ELobbyActionType::Play:
            UE_LOG(LogTemp, Log, TEXT("Player wants to join lobby %d"), LobbyID);
            // Add your join lobby logic here
            break;
            
        case ELobbyActionType::Rejoin:
            UE_LOG(LogTemp, Log, TEXT("Player wants to rejoin lobby %d"), LobbyID);
            // Add your rejoin lobby logic here
            break;
            
        case ELobbyActionType::Spectate:
            UE_LOG(LogTemp, Log, TEXT("Player wants to spectate lobby %d"), LobbyID);
            // Add your spectate lobby logic here
            break;
            
        case ELobbyActionType::Unavailable:
        default:
            UE_LOG(LogTemp, Warning, TEXT("Unavailable action clicked for lobby %d"), LobbyID);
            break;
    }
}