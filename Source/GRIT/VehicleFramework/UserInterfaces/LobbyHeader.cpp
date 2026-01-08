#include "LobbyHeader.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Styling/SlateBrush.h"
#include "Engine/Engine.h"

ULobbyHeader::ULobbyHeader(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize components to nullptr - using exact blueprint names
    EntryBorder = nullptr;
    ContentContainer = nullptr;
    IndexContainer = nullptr;
    VerticalBox_0 = nullptr;
    TitleText = nullptr;
    EntryText = nullptr;
    TimeText = nullptr;
    SpotsText = nullptr;
    TrackText = nullptr;
    StatusText = nullptr;
    ButtonContainer = nullptr;
    PostIndexSpacer = nullptr;
    TitleBottomSpacer = nullptr;
    PostTitleSpacer = nullptr;
    PostEntrySpacer = nullptr;
    PostTimeSpacer = nullptr;
    PreSpotsSpacer = nullptr;
    PostTrackSpacer = nullptr;
    PostStatusSpacer = nullptr;
    
    // Initialize theme colors to defaults
    ThemeColors = FLobbyHeaderTheme();
}

void ULobbyHeader::NativeConstruct()
{
    Super::NativeConstruct();
    SetupLayout();
    SetupHeaderText();
}

void ULobbyHeader::SetupLayout()
{
    // Create main border with header styling
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

    // Set as root widget
    if (GetRootWidget() == nullptr)
    {
        TakeWidget(); // This will set EntryBorder as the root widget
    }

    // Create Index Section (empty container - same size as LobbyEntry)
    IndexContainer = NewObject<USizeBox>(this);
    IndexContainer->SetWidthOverride(50.0f);
    IndexContainer->SetHeightOverride(50.0f);

    // Create spacers (same sizes as LobbyEntry)
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
    TitleText->SetText(FText::FromString("Session"));
    TitleText->SetColorAndOpacity(FSlateColor(ThemeColors.HeaderText));
    FSlateFontInfo TitleFont;
    TitleFont.Size = 18.75f;
    TitleFont.TypefaceFontName = FName("Bold"); // Bolder for header
    TitleText->SetFont(TitleFont);

    // Setup title hierarchy
    VerticalBox_0->AddChild(TitleText);
    VerticalBox_0->AddChild(TitleBottomSpacer);
   

    // Create header text blocks
    EntryText = NewObject<UTextBlock>(this);
    EntryText->SetText(FText::FromString("Entry Fee"));
    EntryText->SetColorAndOpacity(FSlateColor(ThemeColors.HeaderText));
    FSlateFontInfo HeaderFont;
    HeaderFont.Size = 12.0f;
    HeaderFont.TypefaceFontName = FName("Bold");
    EntryText->SetFont(HeaderFont);
    
    TimeText = NewObject<UTextBlock>(this);
    TimeText->SetText(FText::FromString("Time"));
    TimeText->SetColorAndOpacity(FSlateColor(ThemeColors.HeaderText));
    TimeText->SetFont(HeaderFont);
    
    SpotsText = NewObject<UTextBlock>(this);
    SpotsText->SetText(FText::FromString("Players"));
    SpotsText->SetColorAndOpacity(FSlateColor(ThemeColors.HeaderText));
    SpotsText->SetFont(HeaderFont);
    
    TrackText = NewObject<UTextBlock>(this);
    TrackText->SetText(FText::FromString("Track"));
    TrackText->SetColorAndOpacity(FSlateColor(ThemeColors.HeaderText));
    TrackText->SetFont(HeaderFont);
    
    StatusText = NewObject<UTextBlock>(this);
    StatusText->SetText(FText::FromString("Status"));
    StatusText->SetColorAndOpacity(FSlateColor(ThemeColors.HeaderText));
    StatusText->SetFont(HeaderFont);

    // Create Button Section (empty container - same size as LobbyEntry)
    ButtonContainer = NewObject<USizeBox>(this);
    ButtonContainer->SetWidthOverride(150.0f);
    ButtonContainer->SetHeightOverride(45.0f);

    // Add all components to horizontal container in the same order as LobbyEntry
    UHorizontalBoxSlot* IndexHBoxSlot = ContentContainer->AddChildToHorizontalBox(IndexContainer);
    IndexHBoxSlot->SetHorizontalAlignment(HAlign_Center);
    IndexHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostIndexSpacer);
    
    UHorizontalBoxSlot* TitleHBoxSlot = ContentContainer->AddChildToHorizontalBox(VerticalBox_0);
    FSlateChildSize TitleSize;
    TitleSize.SizeRule = ESlateSizeRule::Fill;
    TitleSize.Value = 0.45f; // Same proportion as LobbyEntry
    TitleHBoxSlot->SetSize(TitleSize);
    TitleHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostTitleSpacer);
    
    UHorizontalBoxSlot* EntryHBoxSlot = ContentContainer->AddChildToHorizontalBox(EntryText);
    FSlateChildSize EntrySize;
    EntrySize.SizeRule = ESlateSizeRule::Fill;
    EntrySize.Value = 0.15f; // Same proportion as LobbyEntry
    EntryHBoxSlot->SetSize(EntrySize);
    EntryHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    EntryHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostEntrySpacer);
    
    UHorizontalBoxSlot* TimeHBoxSlot = ContentContainer->AddChildToHorizontalBox(TimeText);
    FSlateChildSize TimeSize;
    TimeSize.SizeRule = ESlateSizeRule::Fill;
    TimeSize.Value = 0.15f; // Same proportion as LobbyEntry
    TimeHBoxSlot->SetSize(TimeSize);
    TimeHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    TimeHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostTimeSpacer);
    
    UHorizontalBoxSlot* SlotsHBoxSlot = ContentContainer->AddChildToHorizontalBox(SpotsText);
    FSlateChildSize SlotsSize;
    SlotsSize.SizeRule = ESlateSizeRule::Fill;
    SlotsSize.Value = 0.2f; // Same proportion as LobbyEntry
    SlotsHBoxSlot->SetSize(SlotsSize);
    SlotsHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    SlotsHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PreSpotsSpacer);
    
    UHorizontalBoxSlot* TrackHBoxSlot = ContentContainer->AddChildToHorizontalBox(TrackText);
    FSlateChildSize TrackSize;
    TrackSize.SizeRule = ESlateSizeRule::Fill;
    TrackSize.Value = 0.35f; // Same proportion as LobbyEntry
    TrackHBoxSlot->SetSize(TrackSize);
    TrackHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    TrackHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostTrackSpacer);
    
    UHorizontalBoxSlot* StatusHBoxSlot = ContentContainer->AddChildToHorizontalBox(StatusText);
    FSlateChildSize StatusSize;
    StatusSize.SizeRule = ESlateSizeRule::Fill;
    StatusSize.Value = 0.25f; // Same proportion as LobbyEntry
    StatusHBoxSlot->SetSize(StatusSize);
    StatusHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    StatusHBoxSlot->SetVerticalAlignment(VAlign_Center);

    ContentContainer->AddChildToHorizontalBox(PostStatusSpacer);
    
    UHorizontalBoxSlot* ActionHBoxSlot = ContentContainer->AddChildToHorizontalBox(ButtonContainer);
    ActionHBoxSlot->SetHorizontalAlignment(HAlign_Left);
    ActionHBoxSlot->SetVerticalAlignment(VAlign_Center);
}

void ULobbyHeader::SetupHeaderText()
{
    // This function can be used to customize header text if needed
    // All header text is already set in SetupLayout(), but this provides
    // a convenient place to modify headers if needed in the future
    
    // You could add logic here to change header text based on context
    // or add localization support
}