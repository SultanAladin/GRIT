        #include "CyberButtonWidget.h"
        #include "Components/Button.h"
        #include "Components/TextBlock.h"
        #include "Components/Border.h"
        #include "Engine/Engine.h"
        #include "Styling/SlateStyle.h"
        #include "Framework/Text/SlateTextLayout.h"

        UCyberButtonWidget::UCyberButtonWidget(const FObjectInitializer& ObjectInitializer)
            : Super(ObjectInitializer)
            , bIsHovered(false)
            , bIsPressed(false)
            , CurrentAnimationTime(0.0f)
            , GlowAnimationTime(0.0f)
            , bGlowAnimating(false)
        {
            // Initialize with default theme
            ThemeData = GetDefaultCyberTheme();
        }

        void UCyberButtonWidget::NativePreConstruct()
        {
            Super::NativePreConstruct();
            
            // Apply theme and variant styles in design time too
            ApplyThemeToComponents();
            ApplyButtonVariantStyle();
            UpdateSelectedState();
            UpdateEnabledState();
        }

        void UCyberButtonWidget::NativeConstruct()
        {
            Super::NativeConstruct();
            
            if (MainButton)
            {
                // Store original scale
                OriginalScale = MainButton->GetRenderTransform().Scale;
                StartScale = OriginalScale;
                TargetScale = OriginalScale;
                
                // Bind button events
                MainButton->OnHovered.AddDynamic(this, &UCyberButtonWidget::OnButtonHovered);
                MainButton->OnUnhovered.AddDynamic(this, &UCyberButtonWidget::OnButtonUnhovered);
                MainButton->OnClicked.AddDynamic(this, &UCyberButtonWidget::OnButtonClicked);
            }

            // Setup text component
            if (ButtonText)
            {
                ButtonText->SetText(ButtonTextContent);
            }

            // Apply initial styling
            ApplyThemeToComponents();
            ApplyButtonVariantStyle();
            UpdateSelectedState();
            UpdateEnabledState();
        }

        void UCyberButtonWidget::NativeDestruct()
        {
            // Clean up delegates
            if (MainButton)
            {
                MainButton->OnHovered.RemoveAll(this);
                MainButton->OnUnhovered.RemoveAll(this);
                MainButton->OnClicked.RemoveAll(this);
            }
            Super::NativeDestruct();
        }

        void UCyberButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
        {
            Super::NativeTick(MyGeometry, InDeltaTime);
            
            float AnimDuration = CustomAnimationDuration > 0.0f ? CustomAnimationDuration : ThemeData.AnimationSpeed;
            
            // Handle scale animation
            if (CurrentAnimationTime < AnimDuration && MainButton)
            {
                CurrentAnimationTime = FMath::Min(CurrentAnimationTime + InDeltaTime, AnimDuration);
                float Alpha = CurrentAnimationTime / AnimDuration;
                
                // Smooth cubic ease-out curve
                Alpha = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
                
                FVector2D CurrentScale = FMath::Lerp(StartScale, TargetScale, Alpha);
                
                FWidgetTransform Transform = MainButton->GetRenderTransform();
                Transform.Scale = CurrentScale;
                MainButton->SetRenderTransform(Transform);
            }

            // Handle glow animation for selected buttons
            if (bGlowAnimating && bIsSelected && bShowGlowEffect)
            {
                GlowAnimationTime += InDeltaTime;
                float GlowPulse = (FMath::Sin(GlowAnimationTime * 2.0f) + 1.0f) * 0.5f; // 0-1 sine wave
                
                // Apply glow effect to border (this would need custom material in practice)
                if (ButtonBorder)
                {
                    FLinearColor GlowColor = ThemeData.GlowColor;
                    GlowColor.A = ThemeData.GlowIntensity * (0.5f + 0.5f * GlowPulse);
                    
                    // Update border brush with glow
                    ButtonBorder->SetBrushColor(GlowColor);
                }
            }
        }

        void UCyberButtonWidget::ApplyThemeToComponents()
        {
            if (ButtonText)
            {
                // Apply font size based on button size
                float FontSize = GetFontSizeForButtonSize();
                
                FSlateFontInfo FontInfo = ButtonText->GetFont();
                FontInfo.Size = FontSize;
                ButtonText->SetFont(FontInfo);
            }

            if (ButtonBorder)
            {
                // Apply border radius and basic styling
                ButtonBorder->SetBrushColor(FLinearColor::Transparent);
                
                // Set padding based on button size
                FMargin ButtonPadding = GetPaddingForButtonSize();
                ButtonBorder->SetPadding(ButtonPadding);
            }
        }

        void UCyberButtonWidget::ApplyButtonVariantStyle()
        {
            if (!MainButton || !ButtonText) return;
            
            FButtonStyle ButtonStyle = MainButton->GetStyle();
            
            // Set all states to rounded box
            ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
            ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
            ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
            ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
            
            // Apply corner radius
            FVector4 CornerRadii(ThemeData.BorderRadius, ThemeData.BorderRadius, ThemeData.BorderRadius, ThemeData.BorderRadius);
            ButtonStyle.Normal.OutlineSettings.CornerRadii = CornerRadii;
            ButtonStyle.Hovered.OutlineSettings.CornerRadii = CornerRadii;
            ButtonStyle.Pressed.OutlineSettings.CornerRadii = CornerRadii;
            ButtonStyle.Disabled.OutlineSettings.CornerRadii = CornerRadii;

            switch (ButtonVariant)
            {
                case ECyberButtonVariant::Primary:
                    // Solid accent background
                    ButtonStyle.Normal.TintColor = ThemeData.AccentPrimary;
                    ButtonStyle.Hovered.TintColor = ThemeData.AccentSecondary;
                    ButtonStyle.Pressed.TintColor = ThemeData.AccentPrimary * 0.8f;
                    ButtonStyle.Disabled.TintColor = ThemeData.AccentPrimary * 0.3f;
                    
                    // No outline for primary
                    ButtonStyle.Normal.OutlineSettings.Width = 0.0f;
                    ButtonStyle.Hovered.OutlineSettings.Width = 0.0f;
                    ButtonStyle.Pressed.OutlineSettings.Width = 0.0f;
                    
                    // Dark text on light background
                    ButtonText->SetColorAndOpacity(ThemeData.TextOnAccent);
                    break;

                case ECyberButtonVariant::Secondary:
                    // Card background
                    ButtonStyle.Normal.TintColor = ThemeData.BackgroundCard;
                    ButtonStyle.Hovered.TintColor = ThemeData.BackgroundHover;
                    ButtonStyle.Pressed.TintColor = ThemeData.BackgroundCard * 0.8f;
                    ButtonStyle.Disabled.TintColor = ThemeData.BackgroundCard * 0.5f;
                    
                    // Subtle border
                    ButtonStyle.Normal.OutlineSettings.Width = 1.0f;
                    ButtonStyle.Normal.OutlineSettings.Color = ThemeData.BorderNormal;
                    ButtonStyle.Hovered.OutlineSettings.Width = 1.0f;
                    ButtonStyle.Hovered.OutlineSettings.Color = ThemeData.BorderHover;
                    
                    ButtonText->SetColorAndOpacity(ThemeData.TextPrimary);
                    break;

                case ECyberButtonVariant::Outline:
                    // Transparent background with accent outline
                    ButtonStyle.Normal.TintColor = FLinearColor::Transparent;
                    ButtonStyle.Hovered.TintColor = ThemeData.AccentPrimary * FLinearColor(1.0f, 1.0f, 1.0f, 0.1f);
                    ButtonStyle.Pressed.TintColor = ThemeData.AccentPrimary * FLinearColor(1.0f, 1.0f, 1.0f, 0.2f);
                    ButtonStyle.Disabled.TintColor = FLinearColor::Transparent;
                    
                    // Accent outline
                    ButtonStyle.Normal.OutlineSettings.Width = 2.0f;
                    ButtonStyle.Normal.OutlineSettings.Color = ThemeData.AccentPrimary;
                    ButtonStyle.Hovered.OutlineSettings.Width = 2.0f;
                    ButtonStyle.Hovered.OutlineSettings.Color = ThemeData.AccentPrimary;
                    ButtonStyle.Disabled.OutlineSettings.Color = ThemeData.AccentPrimary * 0.3f;
                    
                    ButtonText->SetColorAndOpacity(ThemeData.AccentPrimary);
                    break;

                case ECyberButtonVariant::Ghost:
                    // Completely transparent, text only
                    ButtonStyle.Normal.TintColor = FLinearColor::Transparent;
                    ButtonStyle.Hovered.TintColor = ThemeData.BackgroundHover;
                    ButtonStyle.Pressed.TintColor = ThemeData.BackgroundCard;
                    ButtonStyle.Disabled.TintColor = FLinearColor::Transparent;
                    
                    // No outline
                    ButtonStyle.Normal.OutlineSettings.Width = 0.0f;
                    ButtonStyle.Hovered.OutlineSettings.Width = 0.0f;
                    ButtonStyle.Pressed.OutlineSettings.Width = 0.0f;
                    
                    ButtonText->SetColorAndOpacity(ThemeData.TextPrimary);
                    break;

                case ECyberButtonVariant::Chip:
                    // Chip style like in the HTML
                    ButtonStyle.Normal.TintColor = ThemeData.BackgroundHover;
                    ButtonStyle.Hovered.TintColor = ThemeData.BackgroundCard;
                    ButtonStyle.Pressed.TintColor = ThemeData.BackgroundHover * 0.8f;
                    ButtonStyle.Disabled.TintColor = ThemeData.BackgroundHover * 0.5f;
                    
                    // Subtle border
                    ButtonStyle.Normal.OutlineSettings.Width = 1.0f;
                    ButtonStyle.Normal.OutlineSettings.Color = ThemeData.BorderNormal;
                    ButtonStyle.Hovered.OutlineSettings.Width = 1.0f;
                    ButtonStyle.Hovered.OutlineSettings.Color = ThemeData.BorderHover;
                    
                    ButtonText->SetColorAndOpacity(ThemeData.TextPrimary);
                    break;

                default: // Default variant
                    ButtonStyle.Normal.TintColor = ThemeData.BackgroundCard;
                    ButtonStyle.Hovered.TintColor = ThemeData.BackgroundHover;
                    ButtonStyle.Pressed.TintColor = ThemeData.BackgroundCard * 0.8f;
                    ButtonStyle.Disabled.TintColor = ThemeData.BackgroundCard * 0.5f;
                    
                    ButtonStyle.Normal.OutlineSettings.Width = 0.0f;
                    ButtonStyle.Hovered.OutlineSettings.Width = 0.0f;
                    ButtonStyle.Pressed.OutlineSettings.Width = 0.0f;
                    
                    ButtonText->SetColorAndOpacity(ThemeData.TextPrimary);
                    break;
            }
            
            // Apply outline settings to all states
            ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
            ButtonStyle.Normal.OutlineSettings.bUseBrushTransparency = true;
            ButtonStyle.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
            ButtonStyle.Hovered.OutlineSettings.bUseBrushTransparency = true;
            ButtonStyle.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
            ButtonStyle.Pressed.OutlineSettings.bUseBrushTransparency = true;
            ButtonStyle.Disabled.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
            ButtonStyle.Disabled.OutlineSettings.bUseBrushTransparency = true;
            
            MainButton->SetStyle(ButtonStyle);
        }

        void UCyberButtonWidget::UpdateHoverState(bool bHovered)
        {
            if (!bShowHoverEffect) return;
            
            bIsHovered = bHovered;
            
            if (MainButton)
            {
                StartScale = MainButton->GetRenderTransform().Scale;
                float HoverScale = CustomHoverScale > 0.0f ? CustomHoverScale : ThemeData.HoverScale;
                TargetScale = bHovered ? (OriginalScale * HoverScale) : OriginalScale;
                CurrentAnimationTime = 0.0f;
            }
            
            // Update text color for hover state if needed
            if (ButtonText && (ButtonVariant == ECyberButtonVariant::Outline || ButtonVariant == ECyberButtonVariant::Ghost))
            {
                FLinearColor TextColor = bHovered ? ThemeData.AccentSecondary : ThemeData.AccentPrimary;
                ButtonText->SetColorAndOpacity(TextColor);
            }
        }

        void UCyberButtonWidget::UpdateSelectedState()
        {
            if (!MainButton || !ButtonText) return;
            
            if (bIsSelected)
            {
                // Selected state: use accent colors
                FButtonStyle ButtonStyle = MainButton->GetStyle();
                
                switch (ButtonVariant)
                {
                    case ECyberButtonVariant::Chip:
                        // Chip selected: accent background, dark text
                        ButtonStyle.Normal.TintColor = ThemeData.AccentPrimary;
                        ButtonStyle.Hovered.TintColor = ThemeData.AccentSecondary;
                        ButtonStyle.Normal.OutlineSettings.Color = ThemeData.AccentPrimary;
                        ButtonStyle.Hovered.OutlineSettings.Color = ThemeData.AccentPrimary;
                        ButtonText->SetColorAndOpacity(ThemeData.TextOnAccent);
                        break;
                        
                    case ECyberButtonVariant::Outline:
                        // Outline selected: filled with accent
                        ButtonStyle.Normal.TintColor = ThemeData.AccentPrimary;
                        ButtonStyle.Hovered.TintColor = ThemeData.AccentSecondary;
                        ButtonText->SetColorAndOpacity(ThemeData.TextOnAccent);
                        break;
                        
                    default:
                        // Other variants: add glow effect and accent outline
                        ButtonStyle.Normal.OutlineSettings.Width = 2.0f;
                        ButtonStyle.Normal.OutlineSettings.Color = ThemeData.AccentPrimary;
                        ButtonStyle.Hovered.OutlineSettings.Width = 2.0f;
                        ButtonStyle.Hovered.OutlineSettings.Color = ThemeData.AccentPrimary;
                        break;
                }
                
                MainButton->SetStyle(ButtonStyle);
                
                // Start glow animation
                if (bShowGlowEffect)
                {
                    bGlowAnimating = true;
                    GlowAnimationTime = 0.0f;
                }
            }
            else
            {
                // Not selected: restore variant style
                ApplyButtonVariantStyle();
                bGlowAnimating = false;
                
                if (ButtonBorder)
                {
                    ButtonBorder->SetBrushColor(FLinearColor::Transparent);
                }
            }
        }

        void UCyberButtonWidget::UpdateEnabledState()
        {
            if (MainButton)
            {
                MainButton->SetIsEnabled(bIsCyberEnabled);
            }
            
            if (ButtonText)
            {
                FLinearColor TextColor = bIsCyberEnabled ? 
                    (ButtonVariant == ECyberButtonVariant::Primary && bIsSelected ? ThemeData.TextOnAccent : ThemeData.TextPrimary) :
                    ThemeData.TextMuted;
                ButtonText->SetColorAndOpacity(TextColor);
            }
        }

        FVector2D UCyberButtonWidget::GetSizeMultiplierForButtonSize() const
        {
            switch (ButtonSize)
            {
                case ECyberButtonSize::Small:
                    return FVector2D(0.8f, 0.8f);
                case ECyberButtonSize::Large:
                    return FVector2D(1.3f, 1.3f);
                case ECyberButtonSize::ExtraLarge:
                    return FVector2D(1.6f, 1.6f);
                case ECyberButtonSize::Medium:
                default:
                    return FVector2D(1.0f, 1.0f);
            }
        }

        FMargin UCyberButtonWidget::GetPaddingForButtonSize() const
        {
            switch (ButtonSize)
            {
                case ECyberButtonSize::Small:
                    return FMargin(8.0f, 6.0f);
                case ECyberButtonSize::Large:
                    return FMargin(20.0f, 16.0f);
                case ECyberButtonSize::ExtraLarge:
                    return FMargin(28.0f, 20.0f);
                case ECyberButtonSize::Medium:
                default:
                    return FMargin(14.0f, 10.0f);
            }
        }

        float UCyberButtonWidget::GetFontSizeForButtonSize() const
        {
            switch (ButtonSize)
            {
                case ECyberButtonSize::Small:
                    return 11.0f;
                case ECyberButtonSize::Large:
                    return 16.0f;
                case ECyberButtonSize::ExtraLarge:
                    return 20.0f;
                case ECyberButtonSize::Medium:
                default:
                    return 13.0f;
            }
        }

        void UCyberButtonWidget::OnButtonHovered()
        {
            UpdateHoverState(true);
            
            // Broadcast events
            OnCyberButtonHovered();
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(
                    -1, 0.5f, FColor::Cyan,
                    FString::Printf(TEXT("CyberButton HOVERED: %s"), *ButtonTextContent.ToString())
                );
            }
        }

        void UCyberButtonWidget::OnButtonUnhovered()
        {
            UpdateHoverState(false);
            
            // Broadcast events
            OnCyberButtonUnhovered();
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(
                    -1, 0.5f, FColor::Blue,
                    FString::Printf(TEXT("CyberButton UNHOVERED: %s"), *ButtonTextContent.ToString())
                );
            }
        }

        void UCyberButtonWidget::OnButtonClicked()
        {
            if (!bIsCyberEnabled) return;
            
            // Toggle selection for chip variant, otherwise just click
            if (ButtonVariant == ECyberButtonVariant::Chip)
            {
                SetSelected(!bIsSelected);
            }
            
            // Visual feedback
            if (bShowClickFeedback && MainButton)
            {
                bIsPressed = true;
                
                // Quick press animation
                StartScale = MainButton->GetRenderTransform().Scale;
                TargetScale = OriginalScale * 0.95f; // Slight shrink
                CurrentAnimationTime = 0.0f;
                
                // Reset to normal scale after brief press
                FTimerHandle TimerHandle;
                GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
                {
                    if (MainButton)
                    {
                        StartScale = MainButton->GetRenderTransform().Scale;
                        float HoverScale = CustomHoverScale > 0.0f ? CustomHoverScale : ThemeData.HoverScale;
                        TargetScale = bIsHovered ? (OriginalScale * HoverScale) : OriginalScale;
                        CurrentAnimationTime = 0.0f;
                        bIsPressed = false;
                    }
                }, 0.1f, false);
            }
            
            // Broadcast events
            OnCyberButtonClickedDelegate.Broadcast();
            OnCyberButtonClicked();
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(
                    -1, 1.0f, FColor::Yellow,
                    FString::Printf(TEXT("CyberButton CLICKED: %s (Selected: %s)"), 
                        *ButtonTextContent.ToString(), 
                        bIsSelected ? TEXT("Yes") : TEXT("No"))
                );
            }
        }

        // Public interface implementations
        void UCyberButtonWidget::SetButtonText(const FText& NewText)
        {
            ButtonTextContent = NewText;
            if (ButtonText)
            {
                ButtonText->SetText(ButtonTextContent);
            }
        }

        void UCyberButtonWidget::SetSelected(bool bNewSelected)
        {
            if (bIsSelected != bNewSelected)
            {
                bIsSelected = bNewSelected;
                UpdateSelectedState();
                
                // Broadcast selection change
                OnCyberButtonSelectionChangedDelegate.Broadcast(bIsSelected);
                OnCyberButtonSelectionChanged(bIsSelected);
            }
        }

        void UCyberButtonWidget::SetEnabled(bool bNewEnabled)
        {
            if (bIsCyberEnabled != bNewEnabled)
            {
                bIsCyberEnabled = bNewEnabled;
                UpdateEnabledState();
            }
        }

        void UCyberButtonWidget::SetTheme(const FCyberThemeData& NewTheme)
        {
            ThemeData = NewTheme;
            ApplyThemeToComponents();
            ApplyButtonVariantStyle();
            UpdateSelectedState();
            UpdateEnabledState();
        }

        void UCyberButtonWidget::SetVariant(ECyberButtonVariant NewVariant)
        {
            if (ButtonVariant != NewVariant)
            {
                ButtonVariant = NewVariant;
                ApplyButtonVariantStyle();
                UpdateSelectedState();
            }
        }

        void UCyberButtonWidget::SetSize(ECyberButtonSize NewSize)
        {
            if (ButtonSize != NewSize)
            {
                ButtonSize = NewSize;
                ApplyThemeToComponents();
            }
        }

        FCyberThemeData UCyberButtonWidget::GetDefaultCyberTheme()
        {
            FCyberThemeData DefaultTheme;
            
            // Colors matching the HTML design
            DefaultTheme.AccentPrimary = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White
            DefaultTheme.AccentSecondary = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f); // Light grey
            
            DefaultTheme.BackgroundDark = FLinearColor(0.067f, 0.067f, 0.067f, 1.0f); // #111111
            DefaultTheme.BackgroundCard = FLinearColor(0.102f, 0.102f, 0.102f, 1.0f); // #1a1a1a
            DefaultTheme.BackgroundHover = FLinearColor(0.145f, 0.145f, 0.145f, 1.0f); // #252525
            
            DefaultTheme.TextPrimary = FLinearColor(0.867f, 0.867f, 0.867f, 1.0f); // #dddddd
            DefaultTheme.TextMuted = FLinearColor(0.533f, 0.533f, 0.533f, 1.0f); // #888888
            DefaultTheme.TextOnAccent = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark text on light backgrounds
            
            DefaultTheme.BorderNormal = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // #333333
            DefaultTheme.BorderHover = FLinearColor(0.333f, 0.333f, 0.333f, 1.0f); // #555555
            
            DefaultTheme.BorderRadius = 12.0f;
            DefaultTheme.AnimationSpeed = 0.25f;
            DefaultTheme.HoverScale = 1.05f; // Subtle scale
            
            DefaultTheme.GlowColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.2f);
            DefaultTheme.GlowIntensity = 0.3f;
            
            return DefaultTheme;
        }