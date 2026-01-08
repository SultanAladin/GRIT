#include "LobbyEntry.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Styling/SlateBrush.h"
#include "Engine/Engine.h"

ULobbyEntry::ULobbyEntry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize components to nullptr - using exact blueprint names
    EntryBorder = nullptr;
    ContentContainer = nullptr;
    IndexContainer = nullptr;
    IndexCircle = nullptr;
    IndexNumber = nullptr;
    VerticalBox_0 = nullptr;
    TitleText = nullptr;
    SizeBox_2 = nullptr;
    Border_55 = nullptr;
    ClassTrackText = nullptr;
    EntryText = nullptr;
    TimeText = nullptr;
    SpotsText = nullptr;
    TrackText = nullptr;
    StatusText = nullptr;
    ButtonContainer = nullptr;
    Btn_Action = nullptr;
    ActionButtonText = nullptr;
    PostIndexSpacer = nullptr;
    TitleBottomSpacer = nullptr;
    PostTitleSpacer = nullptr;
    PostEntrySpacer = nullptr;
    PostTimeSpacer = nullptr;
    PreSpotsSpacer = nullptr;
    PostTrackSpacer = nullptr;
    PostStatusSpacer = nullptr;
    
    // Initialize theme colors to defaults
    ThemeColors = FLobbyEntryTheme();
}

void ULobbyEntry::NativeConstruct()
{
    Super::NativeConstruct();
    SetupLayout();
    
    // Bind button click event
    if (Btn_Action)
    {
        Btn_Action->OnClicked.AddDynamic(this, &ULobbyEntry::OnActionButtonClicked);
    }
}

void ULobbyEntry::SetLobbyData(const FLobbyData& InLobbyData, int32 InIndex, bool bInPlayerWasInLobby)
{
    LobbyData = InLobbyData;
    LobbyIndex = InIndex;
    bPlayerWasInLobby = bInPlayerWasInLobby;
    
    UpdateLobbyDisplay();
    UpdateActionButton();
}

void ULobbyEntry::SetupLayout()
{
    // Create main border
    EntryBorder = NewObject<UBorder>(this);
    FSlateBrush MainBrush;
    MainBrush.TintColor = FSlateColor(ThemeColors.Primary);
    MainBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
    MainBrush.OutlineSettings.CornerRadii = FVector4(5.0f, 5.0f, 5.0f, 5.0f);
    MainBrush.OutlineSettings.Color = FSlateColor(FLinearColor::Black);
    MainBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    EntryBorder->SetBrush(MainBrush);
    
    // Create horizontal content container
    ContentContainer = NewObject<UHorizontalBox>(this);
    
    // Add content container to main border
    UBorderSlot* MainBorderSlot = Cast<UBorderSlot>(EntryBorder->AddChild(ContentContainer));
    MainBorderSlot->SetPadding(FMargin(5.0f));

    // Set as root widget (use GetRootWidget() and SetRootWidget() methods)
    if (GetRootWidget() == nullptr)
    {
        TakeWidget(); // This will set EntryBorder as the root widget
    }

    // Create Index Section
    IndexContainer = NewObject<USizeBox>(this);
    IndexContainer->SetWidthOverride(50.0f);
    IndexContainer->SetHeightOverride(50.0f);
    
    IndexCircle = NewObject<UBorder>(this);
    FSlateBrush IndexBrush;
    IndexBrush.TintColor = FSlateColor(FLinearColor::Black);
    IndexBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
    IndexBrush.OutlineSettings.Color = FSlateColor(FLinearColor(0.036458f, 0.036458f, 0.036458f, 1.0f));
    IndexBrush.OutlineSettings.Width = 1.0f;
    IndexCircle->SetBrush(IndexBrush);
    
    IndexNumber = NewObject<UTextBlock>(this);
    IndexNumber->SetText(FText::FromString("0"));
    IndexNumber->SetColorAndOpacity(FSlateColor(ThemeColors.Text));
    
    // Setup index hierarchy
    USizeBoxSlot* IndexSizeSlot = Cast<USizeBoxSlot>(IndexContainer->AddChild(IndexCircle));
    UBorderSlot* IndexBorderSlot = Cast<UBorderSlot>(IndexCircle->AddChild(IndexNumber));
    IndexBorderSlot->SetHorizontalAlignment(HAlign_Center);
    IndexBorderSlot->SetVerticalAlignment(VAlign_Center);
    IndexBorderSlot->SetPadding(FMargin(0.0f));

    // Create spacers
    PostIndexSpacer = NewObject<USpacer>(this);
    PostIndexSpacer->SetSize(FVector2D(75.0f, 1.0f));
    
    TitleBottomSpacer = NewObject<USpacer>(this);
    TitleBottomSpacer->SetSize(FVector2D(1.0f, 3.5f));
    
    PostTitleSpacer = NewObject<USpacer>(this);
    PostTitleSpacer->SetSize(FVector2D(25.0f, 1.0f));
    
    PostEntrySpacer = NewObject<USpacer>(this);
    PostEntrySpacer->SetSize(FVector2D(15.0f, 1.0f));
    
    PostTimeSpacer = NewObject<USpacer>(this);
    PostTimeSpacer->SetSize(FVector2D(15.0f, 1.0f));
    
    PreSpotsSpacer = NewObject<USpacer>(this);
    PreSpotsSpacer->SetSize(FVector2D(15.0f, 1.0f));
    
    PostTrackSpacer = NewObject<USpacer>(this);
    PostTrackSpacer->SetSize(FVector2D(15.0f, 1.0f));
    
    PostStatusSpacer = NewObject<USpacer>(this);
    PostStatusSpacer->SetSize(FVector2D(15.0f, 1.0f));

    // Create Title Section
    VerticalBox_0 = NewObject<UVerticalBox>(this);
    
    TitleText = NewObject<UTextBlock>(this);
    TitleText->SetText(FText::FromString("Session "));
    TitleText->SetColorAndOpacity(FSlateColor(ThemeColors.Text));
    FSlateFontInfo TitleFont;
    TitleFont.Size = 18.75f;
    TitleFont.TypefaceFontName = FName("Regular");
    TitleText->SetFont(TitleFont);
    
    SizeBox_2 = NewObject<USizeBox>(this);
    SizeBox_2->SetWidthOverride(100.0f);
    SizeBox_2->SetHeightOverride(20.0f);
    
    Border_55 = NewObject<UBorder>(this);
    FSlateBrush ClassBrush;
    ClassBrush.TintColor = FSlateColor(FLinearColor::Black);
    ClassBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
    ClassBrush.OutlineSettings.Color = FSlateColor(FLinearColor(0.093750f, 0.093750f, 0.093750f, 0.0f));
    ClassBrush.OutlineSettings.Width = 0.5f;
    Border_55->SetBrush(ClassBrush);
    
    ClassTrackText = NewObject<UTextBlock>(this);
    ClassTrackText->SetText(FText::FromString("Class"));
    ClassTrackText->SetColorAndOpacity(FSlateColor(ThemeColors.Text));
    FSlateFontInfo ClassFont;
    ClassFont.Size = 7.5f;
    ClassFont.TypefaceFontName = FName("Light");
    ClassTrackText->SetFont(ClassFont);

    // Setup title hierarchy
    VerticalBox_0->AddChild(TitleText);
    VerticalBox_0->AddChild(TitleBottomSpacer);
    VerticalBox_0->AddChild(SizeBox_2);
    
    USizeBoxSlot* ClassSizeSlot = Cast<USizeBoxSlot>(SizeBox_2->AddChild(Border_55));
    UBorderSlot* ClassBorderSlot = Cast<UBorderSlot>(Border_55->AddChild(ClassTrackText));
    ClassBorderSlot->SetHorizontalAlignment(HAlign_Center);
    ClassBorderSlot->SetVerticalAlignment(VAlign_Center);
    ClassBorderSlot->SetPadding(FMargin(0.0f));

    // Create data text blocks
    EntryText = NewObject<UTextBlock>(this);
    EntryText->SetText(FText::FromString("$ 0.00"));
    EntryText->SetColorAndOpacity(FSlateColor(ThemeColors.Text));
    FSlateFontInfo DataFont;
    DataFont.Size = 11.25f;
    EntryText->SetFont(DataFont);
    
    TimeText = NewObject<UTextBlock>(this);
    TimeText->SetText(FText::FromString("00:00"));
    TimeText->SetColorAndOpacity(FSlateColor(ThemeColors.Text));
    TimeText->SetFont(DataFont);
    
    SpotsText = NewObject<UTextBlock>(this);
    SpotsText->SetText(FText::FromString("0/0"));
    SpotsText->SetColorAndOpacity(FSlateColor(ThemeColors.Text));
    SpotsText->SetFont(DataFont);
    
    TrackText = NewObject<UTextBlock>(this);
    TrackText->SetText(FText::FromString("Kings Canyon"));
    TrackText->SetColorAndOpacity(FSlateColor(ThemeColors.Text));
    TrackText->SetFont(DataFont);
    
    StatusText = NewObject<UTextBlock>(this);
    StatusText->SetText(FText::FromString("N/A"));
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.161458f, 0.161458f, 0.161458f, 1.0f)));
    FSlateFontInfo StatusFont;
    StatusFont.Size = 11.25f;
    StatusFont.TypefaceFontName = FName("Light");
    StatusText->SetFont(StatusFont);

    // Create Button Section
    ButtonContainer = NewObject<USizeBox>(this);
    ButtonContainer->SetWidthOverride(150.0f);
    ButtonContainer->SetHeightOverride(45.0f);
    
    Btn_Action = NewObject<UButton>(this);
    FButtonStyle ButtonStyle;
    FSlateBrush ButtonBrush;
    ButtonBrush.TintColor = FSlateColor(ThemeColors.Accent);
    ButtonBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
    ButtonBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
    ButtonBrush.OutlineSettings.Color = FSlateColor(FLinearColor::Black);
    ButtonStyle.SetNormal(ButtonBrush);
    Btn_Action->SetStyle(ButtonStyle);
    
    ActionButtonText = NewObject<UTextBlock>(this);
    ActionButtonText->SetText(FText::FromString("Action"));
    ActionButtonText->SetColorAndOpacity(FSlateColor(ThemeColors.Text));
    FSlateFontInfo ActionFont;
    ActionFont.Size = 15.0f;
    ActionButtonText->SetFont(ActionFont);

    // Setup button hierarchy
    USizeBoxSlot* ButtonSizeSlot = Cast<USizeBoxSlot>(ButtonContainer->AddChild(Btn_Action));
    UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Btn_Action->AddChild(ActionButtonText));
    ButtonSlot->SetPadding(FMargin(0.0f));
    ButtonSlot->SetHorizontalAlignment(HAlign_Center);
    ButtonSlot->SetVerticalAlignment(VAlign_Center);

    // Add all components to horizontal container in order
    UHorizontalBoxSlot* IndexHBoxSlot = ContentContainer->AddChildToHorizontalBox(IndexContainer);
    IndexHBoxSlot->SetHorizontalAlignment(HAlign_Center);
    IndexHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostIndexSpacer);
    
    UHorizontalBoxSlot* TitleHBoxSlot = ContentContainer->AddChildToHorizontalBox(VerticalBox_0);
    FSlateChildSize TitleSize;
    TitleSize.SizeRule = ESlateSizeRule::Fill;
    TitleSize.Value = 0.45f;
    TitleHBoxSlot->SetSize(TitleSize);
    TitleHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostTitleSpacer);
    
    UHorizontalBoxSlot* EntryHBoxSlot = ContentContainer->AddChildToHorizontalBox(EntryText);
    FSlateChildSize EntrySize;
    EntrySize.SizeRule = ESlateSizeRule::Fill;
    EntrySize.Value = 0.15f;
    EntryHBoxSlot->SetSize(EntrySize);
    EntryHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    EntryHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostEntrySpacer);
    
    UHorizontalBoxSlot* TimeHBoxSlot = ContentContainer->AddChildToHorizontalBox(TimeText);
    FSlateChildSize TimeSize;
    TimeSize.SizeRule = ESlateSizeRule::Fill;
    TimeSize.Value = 0.15f;
    TimeHBoxSlot->SetSize(TimeSize);
    TimeHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    TimeHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostTimeSpacer);
    
    UHorizontalBoxSlot* SlotsHBoxSlot = ContentContainer->AddChildToHorizontalBox(SpotsText);
    FSlateChildSize SlotsSize;
    SlotsSize.SizeRule = ESlateSizeRule::Fill;
    SlotsSize.Value = 0.2f;
    SlotsHBoxSlot->SetSize(SlotsSize);
    SlotsHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    SlotsHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PreSpotsSpacer);
    
    UHorizontalBoxSlot* TrackHBoxSlot = ContentContainer->AddChildToHorizontalBox(TrackText);
    FSlateChildSize TrackSize;
    TrackSize.SizeRule = ESlateSizeRule::Fill;
    TrackSize.Value = 0.35f;
    TrackHBoxSlot->SetSize(TrackSize);
    TrackHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    TrackHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostTrackSpacer);
    
    UHorizontalBoxSlot* StatusHBoxSlot = ContentContainer->AddChildToHorizontalBox(StatusText);
    FSlateChildSize StatusSize;
    StatusSize.SizeRule = ESlateSizeRule::Fill;
    StatusSize.Value = 0.25f;
    StatusHBoxSlot->SetSize(StatusSize);
    StatusHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    StatusHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostStatusSpacer);
    
    UHorizontalBoxSlot* ActionHBoxSlot = ContentContainer->AddChildToHorizontalBox(ButtonContainer);
    ActionHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    ActionHBoxSlot->SetVerticalAlignment(VAlign_Center);
}

void ULobbyEntry::UpdateLobbyDisplay()
{
    if (!IndexNumber || !TitleText || !ClassTrackText || !EntryText || 
        !TimeText || !SpotsText || !TrackText || !StatusText)
        return;

    // Update index
    IndexNumber->SetText(FText::AsNumber(LobbyIndex));

    // Update title
    TitleText->SetText(FText::FromString(LobbyData.Title));

    // Update class tag
    ClassTrackText->SetText(FText::FromString(LobbyData.Class));

    // Update entry fee
    if (LobbyData.Entry == TEXT("Free"))
    {
        EntryText->SetText(FText::FromString("Free"));
    }
    else
    {
        EntryText->SetText(FText::FromString(LobbyData.Entry));
    }

    // Update time
    FString TimeString = FString::Printf(TEXT("%s %s"), *LobbyData.Date, *LobbyData.Time);
    TimeText->SetText(FText::FromString(TimeString));

    // Update slots
    FString SlotsString = FString::Printf(TEXT("%d/%d"), LobbyData.CurrentSpots, LobbyData.MaxSpots);
    SpotsText->SetText(FText::FromString(SlotsString));

    // Update track
    TrackText->SetText(FText::FromString(LobbyData.Track));

    // Update status with color
    FString StatusString;
    switch (LobbyData.Status)
    {
        case ELobbyStatus::Open:
            StatusString = TEXT("Open");
            break;
        case ELobbyStatus::Starting:
            StatusString = TEXT("Starting");
            break;
        case ELobbyStatus::InMatch:
            StatusString = TEXT("In Match");
            break;
    }
    StatusText->SetText(FText::FromString(StatusString));
    StatusText->SetColorAndOpacity(FSlateColor(GetStatusColor(LobbyData.Status)));
}

void ULobbyEntry::UpdateActionButton()
{
    if (!ActionButtonText || !Btn_Action)
        return;

    ELobbyActionType ActionType = DetermineActionType();
    FString ButtonText = GetActionButtonText(ActionType);
    ActionButtonText->SetText(FText::FromString(ButtonText));

    // Update button enabled state
    bool bShouldEnable = (ActionType != ELobbyActionType::Unavailable);
    Btn_Action->SetIsEnabled(bShouldEnable);
}

void ULobbyEntry::OnActionButtonClicked()
{
    ELobbyActionType ActionType = DetermineActionType();
    
    // Broadcast the action clicked event
    OnActionClicked.Broadcast(LobbyData.ID, ActionType);
}

ELobbyActionType ULobbyEntry::DetermineActionType() const
{
    // If player was in this lobby, they can rejoin if it's starting or in match
    if (bPlayerWasInLobby)
    {
        if (LobbyData.Status == ELobbyStatus::Starting || LobbyData.Status == ELobbyStatus::InMatch)
        {
            return ELobbyActionType::Rejoin;
        }
    }

    // Check status-based actions
    switch (LobbyData.Status)
    {
        case ELobbyStatus::Open:
            // Can join if there are available slots
            if (LobbyData.CurrentSpots < LobbyData.MaxSpots)
            {
                return ELobbyActionType::Play;
            }
            return ELobbyActionType::Unavailable;
            
        case ELobbyStatus::Starting:
            // Can spectate if lobby is starting
            return ELobbyActionType::Spectate;
            
        case ELobbyStatus::InMatch:
            // Can spectate if match is in progress
            return ELobbyActionType::Spectate;
            
        default:
            return ELobbyActionType::Unavailable;
    }
}

FString ULobbyEntry::GetActionButtonText(ELobbyActionType ActionType) const
{
    switch (ActionType)
    {
        case ELobbyActionType::Play:
            return TEXT("Play");
        case ELobbyActionType::Rejoin:
            return TEXT("Rejoin");
        case ELobbyActionType::Spectate:
            return TEXT("Spectate");
        case ELobbyActionType::Unavailable:
        default:
            return TEXT("Unavailable");
    }
}

FLinearColor ULobbyEntry::GetStatusColor(ELobbyStatus Status) const
{
    switch (Status)
    {
        case ELobbyStatus::Open:
            return FLinearColor::Green;
        case ELobbyStatus::Starting:
            return FLinearColor::Yellow;
        case ELobbyStatus::InMatch:
            return FLinearColor::Red;
        default:
            return FLinearColor::Gray;
    }
}