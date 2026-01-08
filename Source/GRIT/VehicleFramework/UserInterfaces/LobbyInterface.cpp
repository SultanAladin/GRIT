#include "LobbyInterface.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Engine/Font.h"

ULobbyInterface::ULobbyInterface(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize default theme values (already set in struct)
}

void ULobbyInterface::NativePreConstruct()
{
    Super::NativePreConstruct();
    SetupLayout();
    ApplyTheme();
}

void ULobbyInterface::NativeConstruct()
{
    Super::NativeConstruct();
    ApplyTheme();
}

void ULobbyInterface::SetupLayout()
{
    // === Canvas Panel Layout (Root) ===
    if (Root_BackgroundCanvas)
    {
        Root_BackgroundCanvas->SetVisibility(ESlateVisibility::Visible);
        
        // Setup Canvas Panel Slots for background elements
        if (BackgroundImage)
        {
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BackgroundImage->Slot))
            {
                CanvasSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
                CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
            }
        }
        
        if (DropShadow)
        {
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DropShadow->Slot))
            {
                CanvasSlot->SetOffsets(FMargin(325.975983f, 43.993996f, 191.516510f, 20.0f));
                CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
            }
        }
        
        if (ShadowBlur)
        {
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ShadowBlur->Slot))
            {
                CanvasSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
                CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
            }
        }
        
        // FIXED: Container_MainStack setup using percentage-based anchoring
        if (Container_MainStack)
        {
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Container_MainStack->Slot))
            {
                // Use percentage-based positioning instead of calculating pixel sizes
                // This automatically handles different screen sizes and DPI scaling
                
                // Calculate anchor positions based on container percentages
                // ContainerWidthPercentage = 0.781f (78.1%)
                // ContainerHeightPercentage = 0.926f (92.6%)
                
                float LeftAnchor = (1.0f - ContainerWidthPercentage) * 0.5f;    // 10.95%
                float RightAnchor = 1.0f - LeftAnchor;                          // 89.05%
                float TopAnchor = (1.0f - ContainerHeightPercentage) * 0.5f;    // 3.7%
                float BottomAnchor = 1.0f - TopAnchor;                          // 96.3%
                
                // Set anchors to create responsive layout
                CanvasSlot->SetAnchors(FAnchors(LeftAnchor, TopAnchor, RightAnchor, BottomAnchor));
                
                // Set alignment to center (doesn't matter much with proper anchors)
                CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
                
                // Use zero offsets - let the anchors handle the positioning
                CanvasSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
                
                UE_LOG(LogTemp, Warning, TEXT("Container Anchors - Left:%.3f, Top:%.3f, Right:%.3f, Bottom:%.3f"), 
                       LeftAnchor, TopAnchor, RightAnchor, BottomAnchor);
                UE_LOG(LogTemp, Warning, TEXT("Container positioned using responsive anchors"));
                
                // Remove manual size overrides for child components
                // Let them use their default sizing behavior with proper anchors
                if (SizeBox_Header)
                {
                    // Don't override width - let it fill the container
                    SizeBox_Header->ClearWidthOverride();
                    // Keep height as percentage of container (handled by vertical layout)
                }
                
                if (SizeBox_Table)
                {
                    // Don't override width - let it fill the container  
                    SizeBox_Table->ClearWidthOverride();
                    // Keep height as percentage of container (handled by vertical layout)
                }
                
                if (SizeBox_LobbyList)
                {
                    // Don't override height - let it be handled by the scroll box
                   //  SizeBox_LobbyList->ClearHeightOverride();
                }
            }
        }
    }

    // === Header Section ===
    if (Section_Header)
    {
        if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(Section_Header->Slot))
        {
            BorderSlot->SetPadding(FMargin(25.0f, 25.0f, 25.0f, 25.0f));
        }
    }

    // Setup Title Block Vertical Box Slots
    if (Stack_TitleBlock)
    {
        // Configure vertical box slots for proper ordering and alignment
        if (Txt_Title && Txt_Title->Slot)
        {
            if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Txt_Title->Slot))
            {
                VBoxSlot->SetHorizontalAlignment(HAlign_Left);
            }
        }
        
        if (Txt_Description && Txt_Description->Slot)
        {
            if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Txt_Description->Slot))
            {
                VBoxSlot->SetHorizontalAlignment(HAlign_Left);
            }
        }
        
        if (SizeBox_CreateButton && SizeBox_CreateButton->Slot)
        {
            if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(SizeBox_CreateButton->Slot))
            {
                VBoxSlot->SetHorizontalAlignment(HAlign_Left);
            }
        }
    }

    if (SizeBox_CreateButton)
    {
        SizeBox_CreateButton->SetHeightOverride(50.0f);
    }

    // === Table Section ===
    // Updated: Set Section_LobbyTable content padding to 25.0, 2.0, 25.0, 25.0
    if (Section_LobbyTable)
    {
        if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(Section_LobbyTable->Slot))
        {
            BorderSlot->SetPadding(FMargin(25.0f, 2.0f, 25.0f, 25.0f));
        }
    }

    // === Toolbar Section ===
    if (SizeBox_7)
    {
        SizeBox_7->SetHeightOverride(132.107391f);
    }

    if (Section_Toolbar)
    {
        if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(Section_Toolbar->Slot))
        {
            BorderSlot->SetPadding(FMargin(25.0f, 25.0f, 25.0f, 25.0f));
        }
    }

    // Setup Toolbar Content Slots
    if (HorizontalBox_0)
    {
        // Search Input - Fill remaining space
        if (Input_Search && Input_Search->Slot)
        {
            if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Input_Search->Slot))
            {
                FSlateChildSize ChildSize;
                ChildSize.SizeRule = ESlateSizeRule::Fill;
                ChildSize.Value = 1.0f;
                HBoxSlot->SetSize(ChildSize);
            }
        }
    }

    if (SizeBox_8)
    {
        SizeBox_8->SetWidthOverride(100.0f);
    }

    // Setup Filter Button Slots
    if (HBox_FilterButtons)
    {
        // Configure padding for filter buttons
        if (FilterButton_01 && FilterButton_01->Slot)
        {
            if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(FilterButton_01->Slot))
            {
                HBoxSlot->SetPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f));
            }
        }
        
        if (FilterButton_02 && FilterButton_02->Slot)
        {
            if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(FilterButton_02->Slot))
            {
                HBoxSlot->SetPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f));
            }
        }
        
        if (FilterButton_03 && FilterButton_03->Slot)
        {
            if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(FilterButton_03->Slot))
            {
                HBoxSlot->SetPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f));
            }
        }
    }

    // === Lobby List Section ===
    if (SessionEntries)
    {
        SessionEntries->SetScrollBarVisibility(ESlateVisibility::Hidden);
        SessionEntries->SetAnimateWheelScrolling(true);
        SessionEntries->SetScrollAnimationInterpolationSpeed(5.0f);
        
        // Configure ScrollBox Style via WidgetStyle property
        FScrollBoxStyle ScrollStyle = SessionEntries->GetWidgetStyle();
        ScrollStyle.RightShadowBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
        SessionEntries->SetWidgetStyle(ScrollStyle);
    }

    // === Footer Section ===
    // Updated: Set HBox_TableFooter to bottom align vertically
    if (HBox_TableFooter && HBox_TableFooter->Slot)
    {
        if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(HBox_TableFooter->Slot))
        {
            FSlateChildSize ChildSize;
            ChildSize.SizeRule = ESlateSizeRule::Fill;
            ChildSize.Value = 1.0f;
            VBoxSlot->SetSize(ChildSize);
            VBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 25.0f, 0.0f));
            VBoxSlot->SetVerticalAlignment(VAlign_Bottom);
        }
    }

    // Setup Footer Slots
    if (Txt_LobbyCount && Txt_LobbyCount->Slot)
    {
        if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Txt_LobbyCount->Slot))
        {
            FSlateChildSize ChildSize;
            ChildSize.SizeRule = ESlateSizeRule::Fill;
            ChildSize.Value = 1.0f;
            HBoxSlot->SetSize(ChildSize);
            HBoxSlot->SetVerticalAlignment(VAlign_Center);
        }
    }
    
    // Updated: Set Txt_RulesDisplay to fill horizontally
    if (Txt_RulesDisplay && Txt_RulesDisplay->Slot)
    {
        if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Txt_RulesDisplay->Slot))
        {
            FSlateChildSize ChildSize;
            ChildSize.SizeRule = ESlateSizeRule::Fill;
            ChildSize.Value = 1.0f;
            HBoxSlot->SetSize(ChildSize);
            HBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            HBoxSlot->SetVerticalAlignment(VAlign_Center);
        }
    }
    
    if (SizeBox_12 && SizeBox_12->Slot)
    {
        if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(SizeBox_12->Slot))
        {
            HBoxSlot->SetVerticalAlignment(VAlign_Center);
        }
    }

    if (SizeBox_12)
    {
        SizeBox_12->SetWidthOverride(75.0f);
        SizeBox_12->SetHeightOverride(25.0f);
    }

    // Setup Button Slots Padding
    if (Button_47)
    {
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button_47->Slot))
        {
            if (Filters && Filters->Slot == ButtonSlot)
            {
                ButtonSlot->SetPadding(FMargin(10.0f, 10.0f, 10.0f, 10.0f));
            }
        }
    }

    if (Button_125)
    {
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button_125->Slot))
        {
            if (Txt_ActionButton && Txt_ActionButton->Slot == ButtonSlot)
            {
                ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
            }
        }
    }

    // === Set Text Content ===
    if (Txt_Title)
    {
        Txt_Title->SetText(FText::FromString("Lobby"));
    }

    if (Txt_Description)
    {
        Txt_Description->SetText(FText::FromString("High-octane racing leagues with F1-style championships and driver drafts."));
    }

    if (Txt_CreateLobby)
    {
        Txt_CreateLobby->SetText(FText::FromString("+ Create Lobby"));
    }

    if (FilterDescription_01)
    {
        FilterDescription_01->SetText(FText::FromString("QuickMatch"));
    }

    if (FilterDescription_02)
    {
        FilterDescription_02->SetText(FText::FromString("Solo"));
    }

    if (FilterDescription_03)
    {
        FilterDescription_03->SetText(FText::FromString("Ranked"));
    }

    if (Filters)
    {
        Filters->SetText(FText::FromString("Filters"));
    }

    if (Txt_LobbyCount)
    {
        Txt_LobbyCount->SetText(FText::FromString("Total Lobbies Found: NA"));
    }

    if (Txt_RulesDisplay)
    {
        Txt_RulesDisplay->SetText(FText::FromString("Rules: Standard"));
        Txt_RulesDisplay->SetAutoWrapText(true);
    }

    if (Txt_ActionButton)
    {
        Txt_ActionButton->SetText(FText::FromString("Action"));
    }

    // Search Input Placeholder
    if (Input_Search)
    {
        Input_Search->SetHintText(FText::FromString("Search Lobbies..."));
    }

    // === Configure Spacer sizes to match blueprint ===
    if (Spacer_TitleBlock)
    {
        Spacer_TitleBlock->SetSize(FVector2D(1.0f, 10.0f));
    }

    if (Spacer_DescriptionBlock)
    {
        Spacer_DescriptionBlock->SetSize(FVector2D(1.0f, 15.0f));
    }

    if (Spacer_HeaderFlex)
    {
        Spacer_HeaderFlex->SetSize(FVector2D(1.0f, 25.0f));
    }

    if (Spacer_HeaderContentGap)
    {
        Spacer_HeaderContentGap->SetSize(FVector2D(1.0f, 25.0f));
    }

    if (Spacer_TableContents)
    {
        Spacer_TableContents->SetSize(FVector2D(1.0f, 10.0f));
    }

    if (Spacer_TableDetails)
    {
        Spacer_TableDetails->SetSize(FVector2D(1.0f, 15.0f));
    }

    if (Spacer_558)
    {
        Spacer_558->SetSize(FVector2D(25.0f, 1.0f));
    }

    if (Spacer_599)
    {
        Spacer_599->SetSize(FVector2D(1.0f, 10.0f));
    }

    if (Spacer_FilterSeperator_01)
    {
        Spacer_FilterSeperator_01->SetSize(FVector2D(10.0f, 10.0f));
    }

    if (Spacer_FilterSeperator_02)
    {
        Spacer_FilterSeperator_02->SetSize(FVector2D(10.0f, 10.0f));
    }

    if (Spacer_Seperator_01)
    {
        Spacer_Seperator_01->SetSize(FVector2D(350.0f, 25.0f));
    }

    if (Spacer_Seperator_02)
    {
        Spacer_Seperator_02->SetSize(FVector2D(350.0f, 25.0f));
    }
}

void ULobbyInterface::ApplyTheme()
{
    // === Background Elements ===
    if (BackgroundImage)
    {
        FSlateBrush Brush = BackgroundImage->GetBrush();
        Brush.TintColor = FSlateColor(Theme.AccentColor);
        BackgroundImage->SetBrush(Brush);
    }

    if (DropShadow)
    {
        FSlateBrush Brush = DropShadow->GetBrush();
        Brush.TintColor = FSlateColor(Theme.ShadowColor);
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.OutlineSettings.CornerRadii = FVector4(25.0f, 25.0f, 25.0f, 25.0f);
        Brush.OutlineSettings.Color = FSlateColor(Theme.AccentColor);
        Brush.OutlineSettings.Width = 1.0f;
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        DropShadow->SetBrush(Brush);
    }

    if (ShadowBlur)
    {
        ShadowBlur->SetBlurStrength(8.0f);
    }

    // === Header Section ===
    if (Section_Header)
    {
        FSlateBrush HeaderBrush;
        HeaderBrush.TintColor = FSlateColor(Theme.AccentColor);
        HeaderBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
        HeaderBrush.OutlineSettings.CornerRadii = FVector4(25.0f, 25.0f, 0.0f, 0.0f);
        HeaderBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Section_Header->SetBrush(HeaderBrush);
    }

    // === Text Styling ===
    if (Txt_Title)
    {
        Txt_Title->SetColorAndOpacity(FSlateColor(Theme.PrimaryTextColor));
    }

    if (Txt_Description)
    {
        Txt_Description->SetColorAndOpacity(FSlateColor(Theme.PrimaryTextColor));
    }

    if (Txt_CreateLobby)
    {
        Txt_CreateLobby->SetColorAndOpacity(FSlateColor(Theme.PrimaryTextColor));
    }

    // === Create Lobby Button ===
    if (Btn_CreateLobby)
    {
        FButtonStyle ButtonStyle = Btn_CreateLobby->GetStyle();
        
        // Normal state
        ButtonStyle.Normal.TintColor = FSlateColor(Theme.AccentLightColor);
        ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
        
        // Hovered state  
        ButtonStyle.Hovered.TintColor = FSlateColor(Theme.PrimaryTextColor);
        ButtonStyle.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
        
        // Pressed state
        ButtonStyle.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
        
        // Text color
        ButtonStyle.NormalForeground = FSlateColor(Theme.PrimaryTextColor);
        
        Btn_CreateLobby->SetStyle(ButtonStyle);
    }

    // === Table Section ===
    if (Section_LobbyTable)
    {
        FSlateBrush TableBrush;
        TableBrush.TintColor = FSlateColor(Theme.BackgroundColor);
        TableBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
        TableBrush.OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 50.0f, 50.0f);
        TableBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Section_LobbyTable->SetBrush(TableBrush);
    }

    // === Toolbar Section ===
    if (Section_Toolbar)
    {
        FSlateBrush ToolbarBrush;
        ToolbarBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
        ToolbarBrush.OutlineSettings.CornerRadii = FVector4(25.0f, 25.0f, 25.0f, 25.0f);
        ToolbarBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ToolbarBrush.TintColor = FSlateColor(Theme.DarkBackgroundColor);
        Section_Toolbar->SetBrush(ToolbarBrush);
        Section_Toolbar->SetBrushColor(Theme.DarkBackgroundColor);
    }

    // === Search Input ===
    if (Input_Search)
    {
        Input_Search->SetForegroundColor(Theme.PrimaryTextColor);
    }

    // === Filters Button ===
    if (Button_47)
    {
        FButtonStyle ButtonStyle = Button_47->GetStyle();
        ButtonStyle.Normal.TintColor = FSlateColor(Theme.AccentLightColor);
        ButtonStyle.Normal.OutlineSettings.Color = FSlateColor(Theme.ButtonOutlineColor);
        ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
        Button_47->SetStyle(ButtonStyle);
    }

    if (Filters)
    {
        Filters->SetColorAndOpacity(FSlateColor(Theme.PrimaryTextColor));
    }

    // === Filter Buttons ===
    ApplyFilterButtonStyle(FilterButton_01, FilterDescription_01);
    ApplyFilterButtonStyle(FilterButton_02, FilterDescription_02);
    ApplyFilterButtonStyle(FilterButton_03, FilterDescription_03);

    // === Footer Text ===
    if (Txt_LobbyCount)
    {
        Txt_LobbyCount->SetColorAndOpacity(FSlateColor(Theme.PrimaryTextColor));
    }

    if (Txt_RulesDisplay)
    {
        Txt_RulesDisplay->SetColorAndOpacity(FSlateColor(Theme.PrimaryTextColor));
    }

    // === Action Button ===
    if (Button_125)
    {
        FButtonStyle ButtonStyle = Button_125->GetStyle();
        ButtonStyle.Normal.TintColor = FSlateColor(Theme.PrimaryTextColor);
        ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
        Button_125->SetStyle(ButtonStyle);
    }

    if (Txt_ActionButton)
    {
        Txt_ActionButton->SetColorAndOpacity(FSlateColor(Theme.SecondaryTextColor));
    }
}

void ULobbyInterface::ApplyFilterButtonStyle(UButton* Button, UTextBlock* TextBlock)
{
    if (Button && TextBlock)
    {
        FButtonStyle ButtonStyle = Button->GetStyle();
        
        // Normal state - matching blueprint configuration
        ButtonStyle.Normal.TintColor = FSlateColor(Theme.PrimaryTextColor);
        ButtonStyle.Normal.OutlineSettings.Color = FSlateColor(Theme.ButtonOutlineColor);
        ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
        ButtonStyle.Normal.ImageSize = FVector2D(25.0f, 25.0f);
        
        Button->SetStyle(ButtonStyle);
        
        // Text color
        TextBlock->SetColorAndOpacity(FSlateColor(Theme.SecondaryTextColor));
    }
}