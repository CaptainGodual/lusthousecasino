#include "SugarLustRootWidget.h"
#include "MegaLustPotWidget.h"
#include "SocialCasinoPrototype.h"
#include "CasinoCabinetDisplay.h"
#include "LustHouseLounge.h"
#include "GameFramework/PlayerController.h"
#include "SugarLustSlotMachine.h"
#include "SugarLustGameInstance.h"
#include "LustVendWidget.h"
#include "ImageUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/AudioComponent.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundWave.h"
#include "HttpModule.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

namespace SugarLustStyle
{
    const FLinearColor Deep(0.035f, 0.008f, 0.055f, 0.97f);
    const FLinearColor Panel(0.10f, 0.018f, 0.13f, 0.96f);
    const FLinearColor Pink(1.0f, 0.04f, 0.47f, 1.0f);
    const FLinearColor Gold(1.0f, 0.66f, 0.08f, 1.0f);
    const FLinearColor White(1.0f, 0.91f, 0.98f, 1.0f);
}

USugarLustRootWidget::USugarLustRootWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USugarLustRootWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
    UScaleBox* BackgroundScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("WideBackgroundScale"));
    BackgroundScale->SetStretch(EStretch::ScaleToFill);
    BackgroundScale->SetClipping(EWidgetClipping::ClipToBounds);
    WideBackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("WideBackgroundImage"));
    BackgroundScale->AddChild(WideBackgroundImage);
    if (UOverlaySlot* BackgroundSlot = RootOverlay->AddChildToOverlay(BackgroundScale))
    {
        BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
        BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
        BackgroundSlot->SetPadding(FMargin(0.0f));
    }

    // Keep the authored 1920x1080 layout coordinates, then stretch the entire
    // canvas to the actual viewport size in NativeTick (including wide phones).
    DesignSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DesignSize"));
    DesignSizeBox->SetWidthOverride(1920.0f);
    DesignSizeBox->SetHeightOverride(1080.0f);
    DesignCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DesignCanvas"));
    DesignSizeBox->AddChild(DesignCanvas);
    if (UOverlaySlot* DesignSlot = RootOverlay->AddChildToOverlay(DesignSizeBox))
    {
        DesignSlot->SetHorizontalAlignment(HAlign_Left);
        DesignSlot->SetVerticalAlignment(VAlign_Top);
    }
    DesignSizeBox->SetRenderTransformPivot(FVector2D::ZeroVector);
    WidgetTree->RootWidget = RootOverlay;
    SetWideBackground(TEXT("/Game/Art/FuturisticNeonLobby.FuturisticNeonLobby"));
}

void USugarLustRootWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!SlotGame) SlotGame = new FSugarLustSlotMachine();
    bShowingSlot = false;
    BuildDisclaimer();
}

void USugarLustRootWidget::NativeDestruct()
{
    if (SocialRoom) { SocialRoom->Destroy(); SocialRoom = nullptr; }
    StopNeonWelcome();
    NeonWelcomeSound = nullptr;
    if (SpinMusicComponent)
    {
        SpinMusicComponent->Stop();
        SpinMusicComponent = nullptr;
    }
    if (PayoutWinTuneComponent)
    {
        PayoutWinTuneComponent->Stop();
        PayoutWinTuneComponent = nullptr;
    }
    PayoutWinTuneSound = nullptr;
    SpinMusicSound = nullptr;
    bSpinMusicStartedForCurrentSpin = false;
    delete SlotGame;
    SlotGame = nullptr;
    Super::NativeDestruct();
}

void USugarLustRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (CabinetControls && DesignCanvas)
    {
        const bool bModal = IsProgressiveJackpotVisible() || (BonusCanvas && BonusCanvas->GetVisibility() == ESlateVisibility::Visible) ||
            (BigWinCanvas && BigWinCanvas->GetVisibility() == ESlateVisibility::Visible);
        DesignCanvas->SetRenderScale(bModal ? FVector2D(1,1) : FVector2D(1.35,1.38));
        DesignCanvas->SetRenderTranslation(bModal ? FVector2D::ZeroVector : FVector2D(-337.5,-179.4));
    }
    if (SocialRoom && bShowingSlot)
    {
        int32 W=0,H=0; if(auto* PC=GetOwningPlayer()) PC->GetViewportSize(W,H);
        SetRenderTranslation(FVector2D(W*.37f,H*(SocialRoom && SocialRoom->IsGroupBonusView() ? .36f : .18f)) / FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this),.01f));
    }
    if (LobbyHostGreetingSeconds > 0.0f)
    {
        LobbyHostGreetingSeconds = FMath::Max(0.0f, LobbyHostGreetingSeconds - InDeltaTime);
        if (LobbyHostGreetingSeconds <= 0.0f && LobbyHostGreetingPanel)
            LobbyHostGreetingPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    const FVector2D ViewportSize = MyGeometry.GetLocalSize();
    if (DesignSizeBox && ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
    {
        DesignSizeBox->SetRenderScale(FVector2D(ViewportSize.X / 1920.0f, ViewportSize.Y / 1080.0f));
    }
    if (bPlayFabProfileReady && !bJackpotRequestInFlight && !bPlayFabRequestInFlight &&
        !bBonusPickRequestInFlight && !bDailyBonusRequestInFlight)
    {
        JackpotRefreshSeconds -= InDeltaTime;
        if (JackpotRefreshSeconds <= 0.0f) RequestPlayFabJackpots();
    }
    if (!bShowingSlot && LobbyNoticeSeconds > 0.0f)
    {
        LobbyNoticeSeconds = FMath::Max(0.0f, LobbyNoticeSeconds - InDeltaTime);
        if (LobbyNoticeSeconds <= 0.0f && LobbyNoticePanel)
            LobbyNoticePanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    // Ignore the editor's Play click until its press/release has passed.
    if (bShowingDisclaimer && DisclaimerContinueButton && !DisclaimerContinueButton->GetIsEnabled() && GetWorld() &&
        GetWorld()->GetTimeSeconds() - DisclaimerShownAt >= 1.0f)
    {
        DisclaimerContinueButton->SetIsEnabled(true);
    }
    if (bShowingSlot && SlotGame)
    {
        GrapeWinAnimationSeconds += InDeltaTime;
        if(bSpinButtonHeld && !bAutoSpinStartedFromHold)
        {
            SpinButtonHoldSeconds+=InDeltaTime;
            if(SpinButtonHoldSeconds>=0.45f && SugarFreeSpins.Remaining==0 && AutoSpinsRemaining==0 && bPlayFabProfileReady &&
                !bPlayFabRequestInFlight && SlotGame->CanSpin())
            {
                bAutoSpinStartedFromHold=true;
                HandleAutoSpin();
            }
        }
        const bool bWasSpinning = SlotGame->IsSpinning();
        SlotGame->Update(InDeltaTime);
        const bool bIsSpinning = SlotGame->IsSpinning();
        // PlayFab starts the authoritative spin asynchronously between widget ticks.
        // Track whether music has been started for this spin instead of relying on
        // observing the exact false-to-true transition.
        if (bIsSpinning && !bSpinMusicStartedForCurrentSpin)
        {
            StartSpinMusic();
            bSpinMusicStartedForCurrentSpin = true;
        }
        else if (!bIsSpinning && bSpinMusicStartedForCurrentSpin)
        {
            ScheduleSpinMusicFadeOut();
            bSpinMusicStartedForCurrentSpin = false;
        }

        if (bSpinMusicFadeOutPending && !bIsSpinning)
        {
            SpinMusicStopDelaySeconds = FMath::Max(0.0f, SpinMusicStopDelaySeconds - InDeltaTime);
            if (SpinMusicStopDelaySeconds <= 0.0f)
            {
                bSpinMusicFadeOutPending = false;
                FadeOutSpinMusic();
            }
        }
        if (BonusTransitionSeconds > 0.0f)
            BonusTransitionSeconds = FMath::Max(0.0f, BonusTransitionSeconds - InDeltaTime);
        if (BonusJarAnimatingCard != INDEX_NONE)
        {
            BonusJarAnimationSeconds += InDeltaTime;
            // Keep the final open frame visible until the authoritative pick reply
            // arrives; a failed request resets the card in its failure callback.
            if (BonusJarAnimationSeconds >= 1.45f && !bBonusPickRequestInFlight)
            {
                BonusJarAnimatingCard = INDEX_NONE;
                BonusJarAnimationSeconds = 0.0f;
            }
        }
        if (bWasSpinning && !SlotGame->IsSpinning() &&
            !SlotGame->IsBonusRoundActive() && !SlotGame->IsBonusResultReady())
        {
            if (SlotGame->GetLastWin() > 0)
            {
                PlayPayoutWinTune(SlotGame->GetLastWin());
                bSpinMusicFadeOutPending = false;
                SpinMusicStopDelaySeconds = 0.0f;
                if (SpinMusicComponent && SpinMusicComponent->IsPlaying())
                    SpinMusicComponent->FadeOut(0.3f, 0.0f);
            }
            SugarFreeSpins.DelaySeconds = 1.0f;
            if (PendingSugarJackpotPrize > 0)
            {
                ShowProgressiveJackpotWin(PendingSugarJackpotTier, PendingSugarJackpotPrize);
                PendingSugarJackpotPrize = 0;
            }
            else ShowBigWinPresentation();
        }
        UpdateBigWinUI(InDeltaTime);
        if (SlotNoticeSeconds > 0.0f)
        {
            SlotNoticeSeconds = FMath::Max(0.0f, SlotNoticeSeconds - InDeltaTime);
            if (SlotNoticeSeconds <= 0.0f && SlotNoticeText)
                SlotNoticeText->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (bAutoSpinWaitingForAnimation && !bPlayFabRequestInFlight && !SlotGame->IsSpinning())
        {
            bAutoSpinWaitingForAnimation = false;
            if (SlotGame->IsBonusRoundActive() || SlotGame->IsBonusResultReady()) AutoSpinsRemaining = 0;
        }
        if (AutoSpinsRemaining > 0 && (!BigWinCanvas || BigWinCanvas->GetVisibility() != ESlateVisibility::Visible) &&
            !bAutoSpinWaitingForAnimation && !bPlayFabRequestInFlight && !SlotGame->IsSpinning())
        {
            if (SlotGame->CanSpin())
            {
                --AutoSpinsRemaining;
                bAutoSpinWaitingForAnimation = true;
                RequestPlayFabSpin();
            }
            else
            {
                AutoSpinsRemaining = 0;
            }
        }
        if (SugarFreeSpins.Remaining > 0 && !SugarFreeSpins.bPaused && bPlayFabProfileReady && !bPlayFabRequestInFlight &&
            SlotGame->CanSpin() && !IsProgressiveJackpotVisible() && (!BigWinCanvas || BigWinCanvas->GetVisibility() != ESlateVisibility::Visible))
        {
            SugarFreeSpins.DelaySeconds = FMath::Max(0.0f, SugarFreeSpins.DelaySeconds - InDeltaTime);
            if (SugarFreeSpins.DelaySeconds <= 0.0f) RequestPlayFabSpin();
        }
        UpdateSlotUI();
    }
    else if (bShowingNeonNightsPreview)
    {
        UpdateNeonNights(InDeltaTime);
    }
    else if (DailyBonusCanvas && DailyBonusCanvas->GetVisibility() == ESlateVisibility::Visible)
    {
        UpdateDailyBonusUI();
    }
}

UWidget* USugarLustRootWidget::MakeText(const FString& Text, float Size, FLinearColor Color, bool bBold, EHorizontalAlignment HAlign)
{
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Text));
    Label->SetColorAndOpacity(FSlateColor(Color));
    Label->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size, bBold ? FName("Bold") : FName("Regular")));
    Label->SetJustification(HAlign == HAlign_Left ? ETextJustify::Left : HAlign == HAlign_Right ? ETextJustify::Right : ETextJustify::Center);
    Label->SetAutoWrapText(true);
    Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
    Label->SetShadowOffset(FVector2D(1.0f, 2.0f));
    return Label;
}

UWidget* USugarLustRootWidget::MakePanel(const FString& Text, FLinearColor BorderColor, FLinearColor FillColor, float FontSize)
{
    UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Box->SetBrushColor(BorderColor);
    Box->SetPadding(FMargin(3.0f));
    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Inner->SetBrushColor(FillColor);
    Inner->SetPadding(FMargin(8.0f));
    Inner->SetContent(MakeText(Text, FontSize, SugarLustStyle::White, true));
    Box->SetContent(Inner);
    return Box;
}

UButton* USugarLustRootWidget::MakeButton(const FString& Text, FLinearColor Accent, float FontSize)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    RemoveButtonChrome(Button);
    UBorder* Face = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Face->SetBrushColor(Accent);
    Face->SetPadding(FMargin(3.0f));
    UBorder* Fill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Fill->SetBrushColor(FLinearColor(Accent.R * 0.16f, Accent.G * 0.045f, Accent.B * 0.18f, 0.98f));
    Fill->SetPadding(FMargin(10.0f));
    Fill->SetContent(MakeText(Text, FontSize, SugarLustStyle::White, true));
    Face->SetContent(Fill);
    Button->SetContent(Face);
    return Button;
}

UButton* USugarLustRootWidget::MakeArtButton(const TCHAR* AssetPath, const FString& FallbackText)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    RemoveButtonChrome(Button);
    if (UTexture2D* Texture = LoadArt(AssetPath))
    {
        UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        Image->SetBrushFromTexture(Texture, true);
        UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
        Scale->SetStretch(EStretch::ScaleToFit);
        Scale->AddChild(Image);
        Button->SetContent(Scale);
    }
    else
    {
        Button->SetContent(MakeText(FallbackText, 22, SugarLustStyle::Gold, true));
    }
    return Button;
}

void USugarLustRootWidget::RemoveButtonChrome(UButton* Button)
{
    if (!Button) return;
    FButtonStyle Style = Button->GetStyle();
    Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
    Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
    Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
    Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
    Style.NormalForeground = FSlateColor(FLinearColor::White);
    Style.HoveredForeground = FSlateColor(FLinearColor(1.0f, 0.82f, 0.35f, 1.0f));
    Style.PressedForeground = FSlateColor(FLinearColor(1.0f, 0.24f, 0.68f, 1.0f));
    Style.DisabledForeground = FSlateColor(FLinearColor(0.88f, 0.78f, 0.9f, 1.0f));
    Style.NormalPadding = FMargin(0.0f);
    Style.PressedPadding = FMargin(2.0f);
    Button->SetStyle(Style);
}

void USugarLustRootWidget::Place(UWidget* Widget, float X, float Y, float Width, float Height, int32 Z)
{
    if (!Widget || !DesignCanvas) return;
    UCanvasPanelSlot* CanvasSlot = DesignCanvas->AddChildToCanvas(Widget);
    CanvasSlot->SetPosition(FVector2D(X, Y));
    CanvasSlot->SetSize(FVector2D(Width, Height));
    CanvasSlot->SetZOrder(Z);
}

UTexture2D* USugarLustRootWidget::LoadArt(const TCHAR* AssetPath)
{
    return LoadObject<UTexture2D>(nullptr, AssetPath);
}

void USugarLustRootWidget::SetCandyButtonImage(UImage* Image, UTexture2D* Texture)
{
    if (Image && Texture)
    {
        Image->SetBrushFromTexture(Texture, true);
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void USugarLustRootWidget::SetWideBackground(const TCHAR* AssetPath, FLinearColor Tint)
{
    if (WideBackgroundImage)
    {
        if (UTexture2D* Texture = LoadArt(AssetPath))
        {
            WideBackgroundImage->SetBrushFromTexture(Texture, true);
            WideBackgroundImage->SetColorAndOpacity(Tint);
        }
    }
}

void USugarLustRootWidget::BuildDisclaimer()
{
    SetWideBackground(TEXT("/Game/Art/FuturisticNeonLobby.FuturisticNeonLobby"), FLinearColor(0.42f, 0.36f, 0.48f, 1.0f));
    bShowingSlot = false;
    bShowingDisclaimer = true;
    DisclaimerShownAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    DisclaimerContinueButton = nullptr;
    ProgressiveJackpotCanvas = nullptr;
    ClearScreenWidgets();

    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Backdrop->SetBrushColor(FLinearColor(0.025f, 0.004f, 0.035f, 1.0f));
    Place(Backdrop, 0, 0, 1920, 1080, 0);

    if (UTexture2D* Background = LoadArt(TEXT("/Game/Art/FuturisticNeonLobby.FuturisticNeonLobby")))
    {
        UImage* BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        BackgroundImage->SetBrushFromTexture(Background, true);
        BackgroundImage->SetColorAndOpacity(FLinearColor(0.42f, 0.36f, 0.48f, 1.0f));
        Place(BackgroundImage, 0, 0, 1920, 1080, 1);
    }

    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Dim->SetBrushColor(FLinearColor(0.015f, 0.002f, 0.025f, 0.62f));
    Place(Dim, 0, 0, 1920, 1080, 2);

    UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Frame->SetBrushColor(SugarLustStyle::Gold);
    Frame->SetPadding(FMargin(5.0f));
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(FLinearColor(0.055f, 0.008f, 0.075f, 0.97f));
    Frame->SetContent(Panel);
    Place(Frame, 340, 170, 1240, 740, 3);

    if (UTexture2D* Logo = LoadArt(TEXT("/Game/Art/logo.logo")))
    {
        UImage* LogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        LogoImage->SetBrushFromTexture(Logo, true);
        UScaleBox* LogoScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
        LogoScale->SetStretch(EStretch::ScaleToFit);
        LogoScale->AddChild(LogoImage);
        Place(LogoScale, 720, 215, 480, 190, 4);
    }

    Place(MakeText(TEXT("IMPORTANT GAME NOTICE"), 38, SugarLustStyle::Gold, true), 450, 425, 1020, 64, 4);

    const FString Notice = TEXT("All of the characters appearing within the game are 18 years or over.\n"
        "No characters under 18 years old appear within the game.\n\n"
        "The stories in the game are all fiction. The characters, organization, settings,\n"
        "and names in the game are fictitious and bear no relationship to actual ones.");
    UTextBlock* NoticeText = Cast<UTextBlock>(MakeText(Notice, 22, SugarLustStyle::White));
    if (NoticeText)
    {
        NoticeText->SetAutoWrapText(true);
        Place(NoticeText, 455, 505, 1010, 245, 4);
    }

    DisclaimerContinueButton = MakeButton(TEXT("CONTINUE TO LOBBY"), FLinearColor(0.08f, 0.78f, 0.16f), 28);
    DisclaimerContinueButton->SetIsEnabled(false);
    DisclaimerContinueButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleContinueToLobby);
    Place(DisclaimerContinueButton, 690, 790, 540, 78, 4);
}

UWidget* USugarLustRootWidget::MakeCard(const FString& Title, const FString& Type, FLinearColor Accent, bool bFeatured, int32 X, int32 Y, int32 W, int32 H)
{
    UButton* CardButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    RemoveButtonChrome(CardButton);
    UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Frame->SetBrushColor(Accent);
    Frame->SetPadding(FMargin(4.0f));
    UOverlay* Content = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Inner->SetBrushColor(SugarLustStyle::Deep);
    Content->AddChild(Inner);
    const TCHAR* ArtPath = nullptr;
    if (Title == TEXT("SUGAR LUST")) ArtPath = TEXT("/Game/Art/VelvetRoom.VelvetRoom");
    else if (Title == TEXT("NEON NIGHTS")) ArtPath = TEXT("/Game/Art/NeonNights/neon-nights-game-card.neon-nights-game-card");
    else if (Title == TEXT("MIDNIGHT\nMASQUERADE")) ArtPath = TEXT("/Game/Art/scatter.scatter");
    else if (Title == TEXT("DIAMOND HOLD")) ArtPath = TEXT("/Game/Art/chocolate.chocolate");
    else if (Title == TEXT("SINFUL RICHES")) ArtPath = TEXT("/Game/Art/chocolate.chocolate");
    else if (Title == TEXT("KITTY'S CLUB")) ArtPath = TEXT("/Game/Art/wild.wild");
    if (ArtPath)
    {
        if (UTexture2D* Texture = LoadArt(ArtPath))
        {
            UImage* Art = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
            Art->SetBrushFromTexture(Texture, true);
            Art->SetColorAndOpacity(FLinearColor(1, 1, 1, bFeatured ? 0.96f : 0.82f));
            Content->AddChild(Art);
        }
        UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Shade->SetBrushColor(FLinearColor(0.025f, 0.005f, 0.035f, 0.40f));
        Content->AddChild(Shade);
    }
    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UTextBlock* Name = Cast<UTextBlock>(MakeText(Title, 23.0f, SugarLustStyle::White, true));
    Name->SetWrapTextAt(220.0f);
    Stack->AddChildToVerticalBox(Name);
    UBorder* TypeBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    TypeBadge->SetBrushColor(FLinearColor(0.025f, 0.008f, 0.04f, 0.92f));
    TypeBadge->SetPadding(FMargin(8, 5));
    TypeBadge->SetContent(MakeText(Type, 20.0f, SugarLustStyle::White, true));
    Stack->AddChildToVerticalBox(TypeBadge);
    UBorder* InfoPlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    InfoPlate->SetBrushColor(FLinearColor(0.025f, 0.004f, 0.035f, 0.94f));
    InfoPlate->SetPadding(FMargin(8.0f, 5.0f));
    InfoPlate->SetContent(Stack);
    UOverlaySlot* InfoSlot = Content->AddChildToOverlay(InfoPlate);
    InfoSlot->SetHorizontalAlignment(HAlign_Fill);
    InfoSlot->SetVerticalAlignment(VAlign_Bottom);
    if (!bFeatured)
    {
        UBorder* ComingSoonPlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        ComingSoonPlate->SetBrushColor(FLinearColor(0.035f, 0.008f, 0.055f, 0.92f));
        ComingSoonPlate->SetPadding(FMargin(12.0f, 8.0f));
        ComingSoonPlate->SetContent(MakeText(TEXT("COMING SOON"), 16.0f, SugarLustStyle::Gold, true));
        ComingSoonPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
        UOverlaySlot* ComingSoonSlot = Content->AddChildToOverlay(ComingSoonPlate);
        ComingSoonSlot->SetHorizontalAlignment(HAlign_Center);
        ComingSoonSlot->SetVerticalAlignment(VAlign_Center);
    }
    Frame->SetContent(Content);
    CardButton->SetContent(Frame);
    if (Title == TEXT("SUGAR LUST")) CardButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleOpenSlot);
    else if (Title == TEXT("NEON NIGHTS")) CardButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleOpenNeonNights);
    else CardButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleComingSoonCard);
    Place(CardButton, (float)X, (float)Y, (float)W, (float)H, 10);
    return CardButton;
}

void USugarLustRootWidget::BuildLobby()
{
    WideBackgroundImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (Lounge3D) { Lounge3D->SetActorHiddenInGame(true); Lounge3D->SetActorTickEnabled(false); }
    SetWideBackground(TEXT("/Game/Art/MainMenu/T_MainMenu.T_MainMenu"));
    bShowingDisclaimer = false;
    DisclaimerContinueButton = nullptr;
    AutoSpinsRemaining = 0;
    bAutoSpinWaitingForAnimation = false;
    // These raw widget references belonged to the slot view, which is about to be removed.
    SlotXpText = nullptr;
    SpinButton = nullptr;
    AutoSpinButton = nullptr;
    LinesBadgeButton = nullptr;
    SlotNoticeText = nullptr;
    SlotNoticeSeconds = 0.0f;
    MaxBetButton = nullptr;
    AutoSpinStatusText = nullptr;
    DailyBonusCanvas = nullptr;
    DailyBonusButton = nullptr;
    DailyBonusClaimButton = nullptr;
    DailyBonusCloseButton = nullptr;
    DailyBonusCardFrames.Reset();
    DailyBonusCardLabels.Reset();
    DailyBonusStatusText = nullptr;
    DailyBonusMessageText = nullptr;
    DailyBonusClaimLabel = nullptr;
    LobbyCoinsText = nullptr;
    LobbyGemsText = nullptr;
    LobbyNoticePanel = nullptr;
    LobbyNoticeText = nullptr;
    LobbyNoticeSeconds = 0.0f;
    LobbyHostGreetingPanel = nullptr;
    LobbyHostGreetingText = nullptr;
    LobbyHostGreetingSeconds = 0.0f;
    MissionsCanvas = nullptr;
    GemEarningProgressText = nullptr;
    SettingsCanvas = nullptr;
    SettingsMusicButton = nullptr;
    SettingsSfxButton = nullptr;
    SettingsMusicLabel = nullptr;
    SettingsSfxLabel = nullptr;
    MissionsCloseButton = nullptr;
    MissionsStatusText = nullptr;
    MissionClaimButtons.Reset();
    MissionLabels.Reset();
    MissionBorders.Reset();
    PlayerLevelText = nullptr;
    PlayerBadgeLevelText = nullptr;
    JackpotMiniText = nullptr;
    JackpotMinorText = nullptr;
    JackpotMajorText = nullptr;
    JackpotGrandText = nullptr;
    ProgressiveJackpotCanvas = nullptr;
    ClearScreenWidgets();

    // Artwork and hit targets share the authored canvas, including wide phones.
    SetWideBackground(TEXT("/Game/Art/MainMenu/T_MainMenu.T_MainMenu"));
    if (UTexture2D* Art = LoadArt(TEXT("/Game/Art/MainMenu/T_MainMenu.T_MainMenu")))
    {
        UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainMenuArtwork"));
        Image->SetBrushFromTexture(Art, true);
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Image, 0, 0, 1920, 1080, 0);
    }
    auto Hit = [this](const TCHAR* Name, const TCHAR* Description, float X, float Y, float W, float H)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
        RemoveButtonChrome(Button);
        // Transparent in every state; keyboard focus and tooltips remain available.
        FButtonStyle Style = Button->GetStyle();
        Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
        Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
        Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
        Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
        Button->SetStyle(Style);
        Button->SetToolTipText(FText::FromString(Description));
        Place(Button, X * 1920.f / 1672.f, Y * 1080.f / 941.f, W * 1920.f / 1672.f, H * 1080.f / 941.f, 5);
        return Button;
    };
    Hit(TEXT("MenuEvent"), TEXT("Spin Duel - Enchanted Grove practice"), 500, 195, 1010, 315)->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleMenuEvent);
    Hit(TEXT("MenuInbox"), TEXT("Inbox"), 550, 538, 235, 180)->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleInboxButton);
    Hit(TEXT("MenuDailyRewards"), TEXT("Collect daily rewards"), 800, 538, 238, 180)->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleOpenDailyBonus);
    Hit(TEXT("MenuFriends"), TEXT("Friends"), 1052, 538, 231, 180)->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleMenuFriends);
    Hit(TEXT("MenuVip"), TEXT("VIP rewards"), 1298, 538, 247, 180)->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleVipButton);
    Hit(TEXT("MenuEnterCasino"), TEXT("Enter casino floor"), 655, 758, 760, 143)->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleEnterSocialRoom);

    LobbyNoticePanel = WidgetTree->ConstructWidget<UBorder>();
    LobbyNoticePanel->SetBrushColor(FLinearColor(.035f,.008f,.055f,.96f));
    LobbyNoticePanel->SetPadding(FMargin(12,5));
    LobbyNoticeText = Cast<UTextBlock>(MakeText(TEXT(""),18,SugarLustStyle::White,true));
    LobbyNoticePanel->SetContent(LobbyNoticeText);
    LobbyNoticePanel->SetVisibility(ESlateVisibility::Collapsed);
    Place(LobbyNoticePanel,640,1040,1110,38,20);
    BuildDailyBonusModal();
    BuildSettingsModal();
    BuildMissionsModal();
    UpdateDailyBonusUI();
}

void USugarLustRootWidget::HandleMenuFriends()
{
    ShowSlotNotice(TEXT("Friends are coming soon. Multiplayer friends are not connected yet."));
}
void USugarLustRootWidget::HandleMenuEvent()
{
    HandleEnterSocialRoom();
    if(SocialRoom) SocialRoom->QueueSpinDuelEntry();
}

void USugarLustRootWidget::ShowLobbyHostGreeting()
{
    if (bHasShownLobbyHostGreeting || !DesignCanvas || !WidgetTree) return;
    bHasShownLobbyHostGreeting = true;
    LobbyHostGreetingSeconds = 8.0f;
    LobbyHostGreetingLineIndex = FMath::RandRange(0, 2);

    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Outer->SetBrushColor(SugarLustStyle::Gold);
    Outer->SetPadding(FMargin(3.0f));
    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Inner->SetBrushColor(FLinearColor(0.055f, 0.008f, 0.075f, 0.95f));
    Inner->SetPadding(FMargin(20.0f, 12.0f));

    UVerticalBox* GreetingContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UTextBlock* HostLabel = Cast<UTextBlock>(MakeText(TEXT("YOUR HOUSE HOST"), 16.0f, SugarLustStyle::Gold, true));
    GreetingContent->AddChildToVerticalBox(HostLabel);
    LobbyHostGreetingText = Cast<UTextBlock>(MakeText(TEXT(""), 23.0f, SugarLustStyle::White, true));
    LobbyHostGreetingText->SetAutoWrapText(true);
    GreetingContent->AddChildToVerticalBox(LobbyHostGreetingText);
    Inner->SetContent(GreetingContent);
    Outer->SetContent(Inner);
    LobbyHostGreetingPanel = Outer;
    LobbyHostGreetingPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
    Place(LobbyHostGreetingPanel, 620.0f, 405.0f, 680.0f, 126.0f, 25);
    UpdateLobbyHostGreeting();
}

void USugarLustRootWidget::UpdateLobbyHostGreeting()
{
    if (!LobbyHostGreetingText || LobbyHostGreetingSeconds <= 0.0f) return;

    FString Name = PlayerDisplayName;
    Name.TrimStartAndEndInline();
    if (Name.IsEmpty() || Name.Equals(TEXT("Guest"), ESearchCase::IgnoreCase) ||
        Name.StartsWith(TEXT("Guest-"), ESearchCase::IgnoreCase))
        Name.Reset();
    if (Name.Len() > 20)
    {
        Name.LeftInline(19);
        Name += TEXT("…");
    }

    const int32 Hour = FDateTime::Now().GetHour();
    const TCHAR* Greeting = Hour < 12
        ? TEXT("Good morning")
        : Hour < 18
            ? TEXT("Good afternoon")
            : TEXT("Good evening");
    const TCHAR* WelcomeLines[] =
    {
        TEXT("Welcome to Lust House. The lights are on and your seat is waiting."),
        TEXT("Welcome in. Ready to make this a sweet visit?"),
        TEXT("It's lovely to see you at Lust House. Let's see what tonight brings.")
    };
    const int32 LineIndex = FMath::Clamp(LobbyHostGreetingLineIndex, 0, UE_ARRAY_COUNT(WelcomeLines) - 1);
    const FString Welcome = WelcomeLines[LineIndex];
    const FString Message = Name.IsEmpty()
        ? FString::Printf(TEXT("%s! %s"), Greeting, *Welcome)
        : FString::Printf(TEXT("%s, %s! %s"), *Name, Greeting, *Welcome);
    LobbyHostGreetingText->SetText(FText::FromString(Message));
}

void USugarLustRootWidget::ShowSlot()
{
    StopNeonWelcome();
    StopPayoutWinTune();
    if (USugarLustGameInstance* GameInstance = GetGameInstance<USugarLustGameInstance>())
    {
        GameInstance->ShowLoadingScreen();
    }

    bShowingSlot = true;
    bShowingNeonNightsPreview = false;
    BuildSlot();

    if (USugarLustGameInstance* GameInstance = GetGameInstance<USugarLustGameInstance>())
    {
        GameInstance->StopLoadingScreen();
    }
}

void USugarLustRootWidget::BuildNeonNightsPreview()
{
    if (!DesignCanvas || !WidgetTree) return;

    // Drop all lobby-only widget pointers before replacing the canvas tree.
    PlayerLevelText = nullptr; PlayerBadgeLevelText = nullptr; PlayerXpText = nullptr; PlayerXpBar = nullptr;
    JackpotMiniText = nullptr; JackpotMinorText = nullptr; JackpotMajorText = nullptr; JackpotGrandText = nullptr;
    LobbyCoinsText = nullptr; LobbyGemsText = nullptr; LobbyNoticePanel = nullptr; LobbyNoticeText = nullptr;
    LobbyHostGreetingPanel = nullptr; LobbyHostGreetingText = nullptr; LobbyHostGreetingSeconds = 0.0f;
    DailyBonusCanvas = nullptr; DailyBonusButton = nullptr; DailyBonusClaimButton = nullptr; DailyBonusCloseButton = nullptr;
    DailyBonusCardFrames.Reset(); DailyBonusCardLabels.Reset(); DailyBonusStatusText = nullptr; DailyBonusMessageText = nullptr; DailyBonusClaimLabel = nullptr;
    MissionsCanvas = nullptr; SettingsCanvas = nullptr; MissionsCloseButton = nullptr; MissionsStatusText = nullptr;
    MissionClaimButtons.Reset(); MissionLabels.Reset(); MissionBorders.Reset();
    SettingsMusicButton = nullptr; SettingsSfxButton = nullptr; SettingsMusicLabel = nullptr; SettingsSfxLabel = nullptr;

    NeonCoinsText = nullptr; NeonXpText = nullptr; NeonBetText = nullptr; NeonTotalBetText = nullptr; NeonWinText = nullptr; NeonStatusText = nullptr;
    NeonSpinButton = nullptr; NeonBetDownButton = nullptr; NeonBetUpButton = nullptr; NeonMaxBetButton = nullptr; NeonAutoSpinButton = nullptr;
    NeonJackpotTexts[0] = NeonJackpotTexts[1] = NeonJackpotTexts[2] = NeonJackpotTexts[3] = nullptr;
    NeonSymbolImages.Reset(); NeonSymbolTextures.Reset(); NeonHeatPrizeLabels.Reset();
    NeonWinFrames.Reset(); NeonWinSegments.Reset(); NeonJackpotSymbolLabels.Reset();
    NeonWinningGroups.Reset(); NeonWinningScatterCells.Reset(); NeonWinningFreeSpinCells.Reset(); NeonWinningJackpotCells.Reset(); NeonWinPresentationSeconds = 0.0f;
    NeonVisibleGrid.Reset(); NeonTargetGrid.Reset();
    bNeonSpinAnimating = false; bNeonResultReady = false; bNeonPayoutAnimating = false;
    NeonSpinSeconds = 0.0f; NeonPayoutSeconds = 0.0f; NeonDisplayedWin = 0; NeonAutoSpinsRemaining = 0;
    ProgressiveJackpotCanvas = nullptr;
    ClearScreenWidgets();

    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Backdrop->SetBrushColor(FLinearColor(0.008f, 0.004f, 0.028f, 1.0f));
    Place(Backdrop, 0, 0, 1920, 1080, 0);

    auto AddArt = [this](const TCHAR* Path, float X, float Y, float W, float H, int32 Z) -> UImage*
    {
        UTexture2D* Texture = LoadArt(Path);
        if (!Texture) return nullptr;
        UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        Image->SetBrushFromTexture(Texture, true);
        Place(Image, X, Y, W, H, Z);
        return Image;
    };
    auto AddCrop = [this](UTexture2D* Texture, FVector2f Min, FVector2f Max, float X, float Y, float W, float H, int32 Z) -> UImage*
    {
        if (!Texture) return nullptr;
        UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        FSlateBrush Brush;
        Brush.SetResourceObject(Texture);
        Brush.SetUVRegion(FBox2f(Min, Max));
        Image->SetBrush(Brush);
        Place(Image, X, Y, W, H, Z);
        return Image;
    };
    auto AddHitButton = [this](float X, float Y, float W, float H) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        RemoveButtonChrome(Button);
        Place(Button, X, Y, W, H, 10);
        return Button;
    };

    // Neon Nights gets its own top HUD, jackpot meter, 5x4 reel board and controls.
    UTexture2D* Top = LoadArt(TEXT("/Game/Art/NeonNights/neon-nights-top-bar.neon-nights-top-bar"));
    UTexture2D* Meter = LoadArt(TEXT("/Game/Art/NeonNights/neon-nights-jackpot-bar.neon-nights-jackpot-bar"));
    UTexture2D* Frame = LoadArt(TEXT("/Game/Art/NeonNights/neon-nights-reel-frame.neon-nights-reel-frame"));
    UTexture2D* Brand = LoadArt(TEXT("/Game/Art/NeonNights/neon-nights-brand.neon-nights-brand"));
    if (Top) { UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()); Image->SetBrushFromTexture(Top, true); Place(Image, 0, 0, 1920, 200, 1); }
    if (Meter) { UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()); Image->SetBrushFromTexture(Meter, true); Place(Image, 350, 200, 1220, 118, 1); }
    if (Frame) { UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()); Image->SetBrushFromTexture(Frame, true); Place(Image, 330, 312, 1260, 590, 1); }
    if (Brand) { UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()); Image->SetBrushFromTexture(Brand, true); Place(Image, 18, 210, 280, 130, 2); }
    Place(MakeText(TEXT("1,024\nWAYS"), 24, FLinearColor(0.45f, 0.85f, 1.0f), true), 220, 520, 100, 92, 3);
    Place(MakeText(TEXT("1,024\nWAYS"), 24, FLinearColor(0.45f, 0.85f, 1.0f), true), 1600, 520, 100, 92, 3);

    // Center the live values vertically in the top-bar fields; keep the gem
    // count clear of its diamond icon and plus button.
    NeonCoinsText = Cast<UTextBlock>(MakeText(TEXT("0"), 24, FLinearColor::White, true)); Place(NeonCoinsText, 285, 82, 265, 36, 5);
    Place(MakeText(TEXT("25"), 23, FLinearColor::White, true), 786, 82, 132, 36, 5);
    NeonXpText = Cast<UTextBlock>(MakeText(TEXT("LVL 1"), 16, FLinearColor::White, true)); Place(NeonXpText, 1118, 82, 150, 36, 5);

    // The supplied art orders the shared progressive pools Grand, Major, Minor, Mini.
    // Centers mapped from the four number windows in the updated jackpot strip.
    const float JackpotCenters[4] = { 544.0f, 845.0f, 1135.0f, 1416.0f };
    const int64 JackpotValues[4] = { JackpotGrand, JackpotMajor, JackpotMinor, JackpotMini };
    for (int32 I = 0; I < 4; ++I)
    {
        NeonJackpotTexts[I] = Cast<UTextBlock>(MakeText(FText::AsNumber(JackpotValues[I]).ToString(), 17, FLinearColor::White, true));
        Place(NeonJackpotTexts[I], JackpotCenters[I] - 116.0f, 253.0f, 232.0f, 32.0f, 5);
    }

    static const TCHAR* SymbolAssetNames[13] = {
        TEXT("ace"), TEXT("king"), TEXT("queen"), TEXT("jack"), TEXT("ten"), TEXT("cherries"),
        TEXT("lips"), TEXT("cocktail"), TEXT("cat"), TEXT("perfume"), TEXT("shoe"), TEXT("wild"), TEXT("scatter")
    };
    for (const TCHAR* SymbolName : SymbolAssetNames)
    {
        const FString Asset = FString::Printf(TEXT("/Game/Art/NeonNights/neon-symbol-%s.neon-symbol-%s"), SymbolName, SymbolName);
        NeonSymbolTextures.Add(LoadArt(*Asset));
    }

    // The supplied Free Spins banner is symbol ID 13, separate from Scatter.
    NeonSymbolTextures.Add(LoadArt(TEXT("/Game/Art/NeonNights/neon-nights-promo-free-spins.neon-nights-promo-free-spins")));
    const TCHAR* JackpotArtNames[] = {TEXT("mini"), TEXT("minor"), TEXT("major"), TEXT("grand")};
    for (const TCHAR* Tier : JackpotArtNames)
    {
        const FString Path = FString::Printf(TEXT("/Game/Art/NeonNights/neon-symbol-jackpot-%s.neon-symbol-jackpot-%s"), Tier, Tier);
        NeonSymbolTextures.Add(LoadArt(*Path));
    }
    NeonSymbolTextures.Add(LoadArt(TEXT("/Game/Art/NeonNights/neon-symbol-muse.neon-symbol-muse")));
    NeonFreeSpinsText = Cast<UTextBlock>(MakeText(TEXT(""), 16, SugarLustStyle::Gold, true));
    Place(NeonFreeSpinsText, 350, 174, 1220, 28, 9);
    // 5 reels × 4 visible rows: 4^5 = 1,024 left-to-right ways.
    // The taller cells fill the four horizontal windows in the Neon Nights frame.
    const float GridX = 496.0f, GridY = 347.0f, CellW = 185.0f, CellH = 125.0f;
    for (int32 Row = 0; Row < 4; ++Row)
    {
        for (int32 Reel = 0; Reel < 5; ++Reel)
        {
            UImage* SymbolImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
            if (NeonSymbolTextures.IsValidIndex(0) && NeonSymbolTextures[0])
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(NeonSymbolTextures[0]);
                Brush.ImageSize = FVector2D(NeonSymbolTextures[0]->GetSizeX(), NeonSymbolTextures[0]->GetSizeY());
                SymbolImage->SetBrush(Brush);
            }
            SymbolImage->SetVisibility(ESlateVisibility::Visible);
            UOverlay* Fit = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
            UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
            Scale->SetStretch(EStretch::ScaleToFit);
            if (UScaleBoxSlot* SymbolSlot = Cast<UScaleBoxSlot>(Scale->AddChild(SymbolImage)))
            {
                SymbolSlot->SetHorizontalAlignment(HAlign_Center);
                SymbolSlot->SetVerticalAlignment(VAlign_Center);
            }
            if (UOverlaySlot* ScaleSlot = Fit->AddChildToOverlay(Scale))
            {
                ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
                ScaleSlot->SetVerticalAlignment(VAlign_Fill);
            }
            Place(Fit, GridX + Reel * CellW + 6.0f, GridY + Row * CellH + 4.0f, CellW - 12.0f, CellH - 8.0f, 4);
            NeonSymbolImages.Add(SymbolImage);
            UTextBlock* JackpotLabel = Cast<UTextBlock>(MakeText(TEXT(""), 21, SugarLustStyle::Gold, true));
            Place(JackpotLabel, GridX + Reel * CellW + 10, GridY + Row * CellH + 22, CellW - 20, CellH - 40, 5);
            JackpotLabel->SetVisibility(ESlateVisibility::Collapsed);
            NeonJackpotSymbolLabels.Add(JackpotLabel);
            UBorder* WinFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
            FSlateBrush Outline = *FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox"));
            Outline.DrawAs = ESlateBrushDrawType::Border;
            Outline.Margin = FMargin(0.2f);
            WinFrame->SetBrush(Outline);
            WinFrame->SetVisibility(ESlateVisibility::Collapsed);
            Place(WinFrame, GridX + Reel * CellW + 6.0f, GridY + Row * CellH + 4.0f, CellW - 12.0f, CellH - 8.0f, 7);
            NeonWinFrames.Add(WinFrame);
            NeonVisibleGrid.Add(FMath::RandRange(0, 13));
            UTextBlock* PrizeLabel = Cast<UTextBlock>(MakeText(TEXT(""), 13.0f, FLinearColor(1.0f, 0.84f, 0.30f), true));
            Place(PrizeLabel, GridX + Reel * CellW + 10.0f, GridY + Row * CellH + CellH - 32.0f, CellW - 20.0f, 24.0f, 6);
            PrizeLabel->SetVisibility(ESlateVisibility::Collapsed);
            NeonHeatPrizeLabels.Add(PrizeLabel);
        }
    }
    for (int32 SegmentIndex = 0; SegmentIndex < 4; ++SegmentIndex)
    {
        UBorder* Segment = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Place(Segment, 0, 0, 1, 3, 8);
        if (UCanvasPanelSlot* SegmentSlot = Cast<UCanvasPanelSlot>(Segment->Slot))
            SegmentSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        Segment->SetVisibility(ESlateVisibility::Collapsed);
        NeonWinSegments.Add(Segment);
    }
    UWidget* MusePays = MakeText(TEXT("NEON MUSE\n3 / 4 / 5 REELS\n10x / 30x / 100x BET PER WAY"), 14, SugarLustStyle::Gold, true);
    MusePays->SetVisibility(ESlateVisibility::HitTestInvisible);
    Place(MusePays, 20, 352, 265, 92, 4);
    NeonTargetGrid = NeonVisibleGrid;
    for (int32 HeldIndex = 0; HeldIndex < NeonHeatLockedIndices.Num(); ++HeldIndex)
    {
        const int32 Cell = NeonHeatLockedIndices[HeldIndex];
        if (NeonVisibleGrid.IsValidIndex(Cell))
        {
            NeonVisibleGrid[Cell] = 12;
            NeonTargetGrid[Cell] = 12;
        }
    }

    UBorder* Footer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Footer->SetBrushColor(FLinearColor(0.018f, 0.004f, 0.04f, 0.96f));
    Place(Footer, 0, 892, 1920, 188, 2);
    UTexture2D* Controls = LoadArt(TEXT("/Game/Art/NeonNights/neon-nights-controls.neon-nights-controls"));
    if (Controls)
    {
        // Keep each crop at its source aspect ratio and group the buttons into a compact row.
        AddCrop(Controls, FVector2f(0.0f, 15.0f/132.0f), FVector2f(340.0f/792.0f, 121.0f/132.0f), 22, 916, 430, 134, 3);
        AddCrop(Controls, FVector2f(344.0f/792.0f, 10.0f/132.0f), FVector2f(463.0f/792.0f, 128.0f/132.0f), 466, 930, 120, 118, 3);
        // Sample pixel centers: rows 0-1 are an atlas line, x<465 is MAX BET,
        // and x>=657 begins AUTO SPIN. Keep the full left rim and source aspect.
        constexpr float SpinWidth = 205.0f;
        constexpr float SpinHeight = SpinWidth * (131.5f - 2.5f) / (656.0f - 465.5f);
        AddCrop(Controls, FVector2f(465.5f/792.0f, 2.5f/132.0f), FVector2f(656.0f/792.0f, 131.5f/132.0f), 620, 916, SpinWidth, SpinHeight, 3);
        AddCrop(Controls, FVector2f(665.0f/792.0f, 10.0f/132.0f), FVector2f(790.0f/792.0f, 128.0f/132.0f), 850, 930, 130, 118, 3);
        NeonBetDownButton = AddHitButton(22, 916, 112, 134);
        NeonBetDownButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleNeonBetDown);
        NeonBetUpButton = AddHitButton(334, 916, 118, 134);
        NeonBetUpButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleNeonBetUp);
        NeonMaxBetButton = AddHitButton(466, 930, 120, 118);
        NeonMaxBetButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleNeonMaxBet);
        NeonSpinButton = AddHitButton(620, 916, SpinWidth, SpinHeight);
        NeonSpinButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleNeonSpin);
        NeonAutoSpinButton = AddHitButton(850, 930, 130, 118);
        NeonAutoSpinButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleNeonAutoSpin);
    }
    NeonBetText = Cast<UTextBlock>(MakeText(TEXT("100"), 19, FLinearColor::White, true)); Place(NeonBetText, 120, 984, 240, 32, 6);

    UTexture2D* InfoPanelTexture = LoadArt(TEXT("/Game/Art/NeonNights/neon-nights-info-panel.neon-nights-info-panel"));
    if (InfoPanelTexture)
    {
        // The updated panel has clean, empty number fields; no cover blocks are needed.
        AddCrop(InfoPanelTexture, FVector2f(0.0f, 0.0f), FVector2f(1.0f, 1.0f), 1035, 902, 310, 153, 3);
        NeonTotalBetText = Cast<UTextBlock>(MakeText(TEXT("500"), 20, FLinearColor::White, true)); Place(NeonTotalBetText, 1055, 950, 270, 32, 6);
        NeonWinText = Cast<UTextBlock>(MakeText(TEXT("0"), 21, FLinearColor(0.45f, 0.9f, 1.0f), true)); Place(NeonWinText, 1055, 1008, 270, 32, 6);
    }
    else
    {
        NeonTotalBetText = Cast<UTextBlock>(MakeText(TEXT("500"), 18, FLinearColor::White, true)); Place(NeonTotalBetText, 1035, 946, 366, 35, 5);
        NeonWinText = Cast<UTextBlock>(MakeText(TEXT("0"), 22, FLinearColor(0.45f, 0.9f, 1.0f), true)); Place(NeonWinText, 1035, 995, 366, 35, 5);
    }
    NeonStatusText = Cast<UTextBlock>(MakeText(TEXT(""), 16, FLinearColor(1.0f, 0.32f, 0.85f), true)); Place(NeonStatusText, 1430, 944, 450, 68, 6);

    UButton* Back = MakeButton(TEXT("‹  LOBBY"), FLinearColor(0.9f, 0.08f, 0.55f), 20.0f);
    Back->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleBack);
    Place(Back, 22, 24, 126, 56, 10);

    UpdateNeonNightsUI();
}
void USugarLustRootWidget::ShowNeonNightsPreview()
{
    StopPayoutWinTune();
    bShowingSlot = false;
    bShowingNeonNightsPreview = true;
    bSpinMusicFadeOutPending = false;
    bSpinMusicStartedForCurrentSpin = false;
    FadeOutSpinMusic();
    BuildNeonNightsPreview();
    if (SlotGame)
    {
        NeonCoins = SlotGame->GetCoins();
        NeonXp = SlotGame->GetXP();
    }
    UpdateNeonNightsUI();
    PlayNeonWelcome();
}

void USugarLustRootWidget::UpdateNeonNights(float DeltaSeconds)
{
    if (IsValid(NeonWelcomeComponent) && !NeonWelcomeComponent->IsPlaying())
    {
        StopNeonWelcome();
        if (bMusicEnabled && bNeonSpinAnimating && IsValid(SpinMusicComponent) && SpinMusicComponent->IsPlaying())
            SpinMusicComponent->AdjustVolume(0.4f, 0.65f);
    }
    if (bNeonSpinAnimating)
    {
        NeonSpinSeconds += DeltaSeconds;
        NeonGridAnimationTick -= DeltaSeconds;
        if (NeonGridAnimationTick <= 0.0f)
        {
            NeonGridAnimationTick = 0.055f;
            if (NeonSpinSeconds < 2.0f || !bNeonResultReady)
            {
                for (int32 Index = 0; Index < NeonVisibleGrid.Num(); ++Index)
                    if (!NeonHeatLockedIndices.Contains(Index)) NeonVisibleGrid[Index] = FMath::RandRange(0, 11);
            }
        }
        if (NeonSpinSeconds >= 2.0f && bNeonResultReady)
        {
            NeonVisibleGrid = NeonTargetGrid;
            bNeonSpinAnimating = false;
            bNeonResultReady = false;
            NeonSpinSeconds = 0.0f;
            NeonDisplayedWin = 0;
            NeonPayoutSeconds = 0.0f;
            bNeonPayoutAnimating = NeonWin > 0;
            NeonFreeSpins.DelaySeconds = 1.0f;
            BuildNeonWinPresentation();
            if (PendingNeonJackpotPrize > 0)
            {
                ShowProgressiveJackpotWin(PendingNeonJackpotTier, PendingNeonJackpotPrize);
                PendingNeonJackpotPrize = 0;
            }
            ScheduleSpinMusicFadeOut();
            if (bNeonPayoutAnimating)
            {
                const double Ratio = static_cast<double>(NeonWin) / FMath::Max<int64>(1, static_cast<int64>(NeonBet) * 5);
                NeonPayoutDuration = FMath::Clamp(0.9f + 0.32f * FMath::Log2(static_cast<float>(Ratio + 1.0)), 1.0f, 4.0f);
                PlayPayoutWinTune(NeonWin, NeonPayoutDuration);
                bSpinMusicFadeOutPending = false;
                if (SpinMusicComponent && SpinMusicComponent->IsPlaying()) SpinMusicComponent->FadeOut(0.25f, 0.0f);
            }
            if (bNeonHeatBonusActive)
            {
                NeonAutoSpinsRemaining = 0;
                NeonHeatBonusDelaySeconds = 0.9f;
            }
            if (!bNeonPayoutAnimating && NeonAutoSpinsRemaining > 0)
                RequestNeonNightsSpin();
        }
    }
    if (bNeonPayoutAnimating)
    {
        NeonPayoutSeconds += DeltaSeconds;
        const float Progress = FMath::Clamp(NeonPayoutSeconds / NeonPayoutDuration, 0.0f, 1.0f);
        NeonDisplayedWin = FMath::RoundToInt64(static_cast<double>(NeonWin) * Progress);
        if (Progress >= 1.0f)
        {
            bNeonPayoutAnimating = false;
            StopPayoutWinTune();
            if (bNeonHeatBonusActive) NeonHeatBonusDelaySeconds = 0.9f;
            else if (NeonAutoSpinsRemaining > 0) RequestNeonNightsSpin();
        }
    }

    if (bSpinMusicFadeOutPending && !bNeonSpinAnimating)
    {
        SpinMusicStopDelaySeconds = FMath::Max(0.0f, SpinMusicStopDelaySeconds - DeltaSeconds);
        if (SpinMusicStopDelaySeconds <= 0.0f)
        {
            bSpinMusicFadeOutPending = false;
            FadeOutSpinMusic();
        }
    }
    if (bNeonHeatBonusActive && !bNeonSpinAnimating && !bNeonPayoutAnimating && !bPlayFabRequestInFlight)
    {
        NeonHeatBonusDelaySeconds = FMath::Max(0.0f, NeonHeatBonusDelaySeconds - DeltaSeconds);
        if (NeonHeatBonusDelaySeconds <= 0.0f) RequestNeonNightsSpin();
    }
    if (NeonHeatBonusCompleteSeconds > 0.0f && !bNeonSpinAnimating && !bNeonPayoutAnimating)
    {
        NeonHeatBonusCompleteSeconds = FMath::Max(0.0f, NeonHeatBonusCompleteSeconds - DeltaSeconds);
        if (NeonHeatBonusCompleteSeconds <= 0.0f) bNeonHeatBonusComplete = false;
    }

    for (int32 Index = 0; Index < NeonSymbolImages.Num(); ++Index)
    {
        if (!NeonSymbolImages[Index] || !NeonVisibleGrid.IsValidIndex(Index)) continue;
        const int32 SymbolId = FMath::Clamp(NeonVisibleGrid[Index], 0, NeonSymbolTextures.Num() - 1);
        const bool bJackpot = SymbolId >= 14 && SymbolId <= 17;
        const bool bPlaceholder = bJackpot && (!NeonSymbolTextures.IsValidIndex(SymbolId) || !NeonSymbolTextures[SymbolId]);
        NeonSymbolImages[Index]->SetVisibility(bPlaceholder ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        if (NeonJackpotSymbolLabels.IsValidIndex(Index))
        {
            UTextBlock* Label = NeonJackpotSymbolLabels[Index];
            Label->SetVisibility(bPlaceholder ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
            if (bJackpot)
            {
                const TCHAR* Names[] = {TEXT("MINI\nJACKPOT"),TEXT("MINOR\nJACKPOT"),TEXT("MAJOR\nJACKPOT"),TEXT("GRAND\nJACKPOT")};
                Label->SetText(FText::FromString(Names[SymbolId - 14]));
                Label->SetColorAndOpacity(FSlateColor(FSugarLustSlotMachine::GetSymbolAccent(static_cast<ESugarLustSymbol>(SymbolId + 4))));
            }
        }
        if (!NeonSymbolTextures.IsValidIndex(SymbolId) || !NeonSymbolTextures[SymbolId]) continue;
        FSlateBrush Brush = NeonSymbolImages[Index]->GetBrush();
        if (Brush.GetResourceObject() != NeonSymbolTextures[SymbolId])
        {
            Brush.SetResourceObject(NeonSymbolTextures[SymbolId]);
            Brush.ImageSize = FVector2D(NeonSymbolTextures[SymbolId]->GetSizeX(), NeonSymbolTextures[SymbolId]->GetSizeY());
            NeonSymbolImages[Index]->SetBrush(Brush);
        }
    }
    if (NeonFreeSpins.Remaining > 0 && !NeonFreeSpins.bPaused && !IsProgressiveJackpotVisible() && !bNeonHeatBonusActive && !bNeonHeatBonusComplete &&
        !bNeonSpinAnimating && !bNeonPayoutAnimating && bPlayFabProfileReady && !bPlayFabRequestInFlight)
    {
        NeonFreeSpins.DelaySeconds = FMath::Max(0.0f, NeonFreeSpins.DelaySeconds - DeltaSeconds);
        if (NeonFreeSpins.DelaySeconds <= 0.0f) RequestNeonNightsSpin();
    }
    UpdateNeonWinPresentation(DeltaSeconds);
    UpdateNeonNightsUI();
}


void USugarLustRootWidget::StopPayoutWinTune()
{
    if (PayoutWinTuneComponent) PayoutWinTuneComponent->Stop();
    PayoutWinTuneComponent = nullptr;
    PayoutWinTuneSound = nullptr;
}

void USugarLustRootWidget::BuildNeonWinPresentation()
{
    NeonWinningGroups.Reset();
    NeonWinningScatterCells.Reset();
    NeonWinningFreeSpinCells.Reset(); NeonWinningJackpotCells.Reset();
    NeonWinPresentationSeconds = 0.0f;
    if (bNeonCurrentSpinIsBonus || NeonVisibleGrid.Num() != 20) return;
    if (PendingNeonJackpotPrize > 0)
    {
        const FString Tiers[] = {TEXT("Mini"),TEXT("Minor"),TEXT("Major"),TEXT("Grand")};
        for (int32 Tier = 0; Tier < 4; ++Tier)
            if (PendingNeonJackpotTier == Tiers[Tier])
                for (int32 Cell = 0; Cell < 20; ++Cell)
                    if (NeonVisibleGrid[Cell] == 14 + Tier) NeonWinningJackpotCells.Add(Cell);
    }
    if (NeonFreeSpins.LastAward > 0)
    {
        for (int32 Cell = 0; Cell < 20; ++Cell)
            if (NeonVisibleGrid[Cell] == 13) NeonWinningFreeSpinCells.Add(Cell);
        if (NeonWinningFreeSpinCells.Num() < 3) NeonWinningFreeSpinCells.Reset();
    }

    // Presentation only. Mirror the Nakama evaluator, then require its BaseWin
    // to agree before displaying anything. Never calculate or award balance here.
    constexpr int32 SymbolMultipliers[] = {1,1,1,1,1,2,2,3,4,3,3,0,0,0,0,0,0,0,5};
    constexpr int32 LengthMultipliers[] = {2,6,20};
    int64 ExpectedWin = 0;
    for (int32 Symbol = 0; Symbol < UE_ARRAY_COUNT(SymbolMultipliers); ++Symbol)
    {
        if (SymbolMultipliers[Symbol] <= 0) continue;
        FNeonWinningGroup Group;
        Group.Color = FLinearColor::MakeFromHSV8(static_cast<uint8>(Symbol * 23), 160, 255);
        for (int32 Reel = 0; Reel < 5; ++Reel)
        {
            TArray<int32> Cells;
            for (int32 Row = 0; Row < 4; ++Row)
            {
                const int32 Index = Row * 5 + Reel;
                if (NeonVisibleGrid[Index] == Symbol || NeonVisibleGrid[Index] == 11) Cells.Add(Index);
            }
            if (Cells.IsEmpty()) break;
            Group.Ways *= Cells.Num();
            Group.CellsByReel.Add(MoveTemp(Cells));
        }
        if (Group.CellsByReel.Num() < 3) continue;
        ExpectedWin += static_cast<int64>(NeonBet) * LengthMultipliers[Group.CellsByReel.Num() - 3] * SymbolMultipliers[Symbol] * Group.Ways;
        NeonWinningGroups.Add(MoveTemp(Group));
    }
    for (int32 Cell = 0; Cell < 20; ++Cell)
        if (NeonVisibleGrid[Cell] == 12) NeonWinningScatterCells.Add(Cell);
    if (NeonWinningScatterCells.Num() >= 3)
        ExpectedWin += static_cast<int64>(NeonBet) * 5 * (NeonWinningScatterCells.Num() == 3 ? 2 : NeonWinningScatterCells.Num() == 4 ? 10 : 50);
    else NeonWinningScatterCells.Reset();
    if (ExpectedWin != NeonBaseWin)
    {
        NeonWinningGroups.Reset(); NeonWinningScatterCells.Reset(); NeonWinningFreeSpinCells.Reset(); NeonWinningJackpotCells.Reset();
        UE_LOG(LogTemp, Warning, TEXT("Neon win presentation skipped: displayed grid does not match authoritative BaseWin."));
    }
}

void USugarLustRootWidget::UpdateNeonWinPresentation(float DeltaSeconds)
{
    NeonWinPresentationSeconds += DeltaSeconds;
    for (UBorder* Frame : NeonWinFrames) if (Frame) Frame->SetVisibility(ESlateVisibility::Collapsed);
    for (UBorder* Segment : NeonWinSegments) if (Segment) Segment->SetVisibility(ESlateVisibility::Collapsed);
    for (UImage* Image : NeonSymbolImages) if (Image) Image->SetColorAndOpacity(FLinearColor::White);
    if (bNeonSpinAnimating || bPlayFabRequestInFlight) return;

    const int32 Groups = NeonWinningGroups.Num() + (NeonWinningScatterCells.IsEmpty() ? 0 : 1) + (NeonWinningFreeSpinCells.IsEmpty() ? 0 : 1) + (NeonWinningJackpotCells.IsEmpty() ? 0 : 1);
    if (Groups == 0) return;
    const int32 Step = FMath::FloorToInt(NeonWinPresentationSeconds / 0.9f);
    const int32 GroupIndex = Step % Groups;
    TArray<int32> Path;
    FLinearColor Color(1.0f, 0.78f, 0.15f);
    const bool bScatterGroup = GroupIndex >= NeonWinningGroups.Num();
    if (bScatterGroup)
    {
        int32 FeatureIndex = GroupIndex - NeonWinningGroups.Num();
        if (!NeonWinningScatterCells.IsEmpty() && FeatureIndex-- == 0) Path = NeonWinningScatterCells;
        else if (!NeonWinningFreeSpinCells.IsEmpty() && FeatureIndex-- == 0) Path = NeonWinningFreeSpinCells;
        else Path = NeonWinningJackpotCells;
    }
    else
    {
        const FNeonWinningGroup& Group = NeonWinningGroups[GroupIndex];
        Color = Group.Color;
        int32 Way = (Step / Groups) % Group.Ways;
        for (const TArray<int32>& Cells : Group.CellsByReel)
        {
            Path.Add(Cells[Way % Cells.Num()]);
            Way /= Cells.Num();
        }
    }
    const float Pulse = 0.78f + 0.22f * FMath::Sin(NeonWinPresentationSeconds * 7.0f);
    for (int32 Cell = 0; Cell < NeonSymbolImages.Num(); ++Cell)
    {
        const bool bHighlight = Path.Contains(Cell);
        if (NeonSymbolImages[Cell]) NeonSymbolImages[Cell]->SetColorAndOpacity(bHighlight ? FLinearColor::White : FLinearColor(0.42f, 0.42f, 0.48f, 1.0f));
        if (bHighlight && NeonWinFrames.IsValidIndex(Cell) && NeonWinFrames[Cell])
        {
            NeonWinFrames[Cell]->SetBrushColor(Color.CopyWithNewOpacity(Pulse));
            NeonWinFrames[Cell]->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
    // Scatter and Free Spins triggers highlight anywhere; only paying ways get connecting lines.
    if (bScatterGroup) return;
    for (int32 Index = 0; Index + 1 < Path.Num() && NeonWinSegments.IsValidIndex(Index); ++Index)
    {
        const FVector2D A(496.0f + (Path[Index] % 5 + 0.5f) * 185.0f, 347.0f + (Path[Index] / 5 + 0.5f) * 125.0f);
        const FVector2D B(496.0f + (Path[Index + 1] % 5 + 0.5f) * 185.0f, 347.0f + (Path[Index + 1] / 5 + 0.5f) * 125.0f);
        UBorder* Segment = NeonWinSegments[Index];
        if (UCanvasPanelSlot* SegmentSlot = Cast<UCanvasPanelSlot>(Segment->Slot))
        {
            SegmentSlot->SetPosition((A + B) * 0.5f);
            SegmentSlot->SetSize(FVector2D((B - A).Size(), 3.0f));
        }
        Segment->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X)));
        Segment->SetBrushColor(Color.CopyWithNewOpacity(Pulse));
        Segment->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void USugarLustRootWidget::UpdateNeonNightsUI()
{
    if (!bShowingNeonNightsPreview) return;
    UpdateFreeSpinsUI();
    if (NeonCoinsText) NeonCoinsText->SetText(FText::AsNumber(NeonCoins));
    if (NeonXpText)
    {
        const int32 Level = SlotGame ? SlotGame->GetLevel() : 1;
        NeonXpText->SetText(FText::FromString(FString::Printf(TEXT("LVL %d"), Level)));
    }
    const int64 TotalBet = static_cast<int64>(NeonBet) * 5;
    if (NeonBetText) NeonBetText->SetText(FText::AsNumber(NeonBet));
    if (NeonTotalBetText) NeonTotalBetText->SetText(FText::AsNumber(TotalBet));
    if (NeonWinText) NeonWinText->SetText(FText::AsNumber(NeonDisplayedWin));
    const int64 Values[4] = { JackpotGrand, JackpotMajor, JackpotMinor, JackpotMini };
    for (int32 Index = 0; Index < 4; ++Index)
        if (NeonJackpotTexts[Index]) NeonJackpotTexts[Index]->SetText(FText::AsNumber(Values[Index]));
    for (int32 Cell = 0; Cell < NeonHeatPrizeLabels.Num(); ++Cell)
    {
        const int32 PrizeIndex = NeonHeatLockedIndices.IndexOfByKey(Cell);
        if (PrizeIndex != INDEX_NONE && NeonHeatLockedPrizes.IsValidIndex(PrizeIndex))
        {
            NeonHeatPrizeLabels[Cell]->SetText(FText::AsNumber(NeonHeatLockedPrizes[PrizeIndex]));
            NeonHeatPrizeLabels[Cell]->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else NeonHeatPrizeLabels[Cell]->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (NeonStatusText)
    {
        const FString Status = !NeonSpinError.IsEmpty() ? NeonSpinError : bSpinRequestRetrying ? TEXT("RECONNECTING TO THE CASINO...") : bPlayFabRequestInFlight ? TEXT("CONNECTING TO THE CASINO...") :
            bNeonSpinAnimating ? TEXT("REELS SPINNING...") :
            bNeonHeatBonusComplete ? FString::Printf(TEXT("NEON HEAT BONUS COMPLETE!  +%s"), *FText::AsNumber(NeonHeatBonusTotal).ToString()) :
            bNeonHeatBonusActive ? FString::Printf(TEXT("NEON HEAT BONUS\nRESPINS: %d  |  BANK: %s"), NeonHeatRespinsRemaining, *FText::AsNumber(NeonHeatBonusTotal).ToString()) :
            NeonAutoSpinsRemaining > 0 ? FString::Printf(TEXT("AUTO SPIN  •  %d LEFT"), NeonAutoSpinsRemaining) : FString();
        NeonStatusText->SetText(FText::FromString(Status));
    }
    const bool bCanChangeBet = PendingNeonSpinId.IsEmpty() && !bPlayFabRequestInFlight && !bNeonSpinAnimating && !bNeonPayoutAnimating && !bNeonHeatBonusActive && NeonFreeSpins.Remaining == 0;
    const bool bCanSpin = !bPlayFabRequestInFlight && !bNeonSpinAnimating && !bNeonPayoutAnimating && !bNeonHeatBonusActive;
    if (NeonSpinButton) NeonSpinButton->SetIsEnabled(bPlayFabProfileReady && !bNeonHeatBonusComplete &&
        ((NeonFreeSpins.Remaining > 0 && !bNeonHeatBonusActive) || NeonAutoSpinsRemaining > 0 || bCanSpin));
    if (NeonBetDownButton) NeonBetDownButton->SetIsEnabled(bCanChangeBet);
    if (NeonBetUpButton) NeonBetUpButton->SetIsEnabled(bCanChangeBet);
    if (NeonMaxBetButton) NeonMaxBetButton->SetIsEnabled(bCanChangeBet);
    if (NeonAutoSpinButton) NeonAutoSpinButton->SetIsEnabled(bPlayFabProfileReady && !bNeonHeatBonusActive && (NeonAutoSpinsRemaining > 0 || bCanChangeBet));
}

void USugarLustRootWidget::ApplyNeonHeatBonusState(const TSharedPtr<FJsonObject>& State)
{
    bNeonHeatBonusActive = false;
    bNeonHeatBonusComplete = false;
    NeonHeatRespinsRemaining = 0;
    NeonHeatBonusTotal = 0;
    NeonHeatBonusDelaySeconds = 0.0f;
    NeonHeatBonusCompleteSeconds = 0.0f;
    NeonHeatLockedIndices.Reset();
    NeonHeatLockedPrizes.Reset();

    if (!State.IsValid()) return;
    State->TryGetBoolField(TEXT("Active"), bNeonHeatBonusActive);
    State->TryGetBoolField(TEXT("Complete"), bNeonHeatBonusComplete);
    double Respins = 0.0, Total = 0.0;
    State->TryGetNumberField(TEXT("RespinsRemaining"), Respins);
    State->TryGetNumberField(TEXT("Total"), Total);
    NeonHeatRespinsRemaining = FMath::Max(0, static_cast<int32>(Respins));
    NeonHeatBonusTotal = FMath::Max<int64>(0, static_cast<int64>(Total));
    if (bNeonHeatBonusComplete) NeonHeatBonusCompleteSeconds = 4.0f;

    const TArray<TSharedPtr<FJsonValue>>* Locked = nullptr;
    if (State->TryGetArrayField(TEXT("Locked"), Locked) && Locked)
    {
        for (const TSharedPtr<FJsonValue>& Entry : *Locked)
        {
            const TSharedPtr<FJsonObject> Cell = Entry.IsValid() ? Entry->AsObject() : nullptr;
            if (!Cell.IsValid()) continue;
            double IndexValue = -1.0, PrizeValue = 0.0;
            if (!Cell->TryGetNumberField(TEXT("Index"), IndexValue) || !Cell->TryGetNumberField(TEXT("Prize"), PrizeValue)) continue;
            const int32 Index = static_cast<int32>(IndexValue);
            if (Index < 0 || Index >= 20 || NeonHeatLockedIndices.Contains(Index)) continue;
            NeonHeatLockedIndices.Add(Index);
            NeonHeatLockedPrizes.Add(FMath::Max<int64>(0, static_cast<int64>(PrizeValue)));
        }
    }
    if (bNeonHeatBonusActive) NeonAutoSpinsRemaining = 0;
}

void USugarLustRootWidget::HandleNeonBetDown()
{
    if (!PendingNeonSpinId.IsEmpty() || bPlayFabRequestInFlight || bNeonSpinAnimating || bNeonPayoutAnimating || bNeonHeatBonusActive || NeonFreeSpins.Remaining > 0) return;
    NeonBet = FMath::Max(100, NeonBet - 100);
    UpdateNeonNightsUI();
}
void USugarLustRootWidget::HandleNeonBetUp()
{
    if (!PendingNeonSpinId.IsEmpty() || bPlayFabRequestInFlight || bNeonSpinAnimating || bNeonPayoutAnimating || bNeonHeatBonusActive || NeonFreeSpins.Remaining > 0) return;
    NeonBet = FMath::Min(10000, NeonBet + 100);
    UpdateNeonNightsUI();
}
void USugarLustRootWidget::HandleNeonMaxBet()
{
    if (!PendingNeonSpinId.IsEmpty() || bPlayFabRequestInFlight || bNeonSpinAnimating || bNeonPayoutAnimating || bNeonHeatBonusActive || NeonFreeSpins.Remaining > 0) return;
    NeonBet = 10000;
    UpdateNeonNightsUI();
}
void USugarLustRootWidget::HandleNeonSpin()
{
    UE_LOG(LogTemp, Log, TEXT("Neon SPIN clicked: ready=%d request=%d free=%d paused=%d spinning=%d payout=%d heat=%d complete=%d jackpot=%d"),
        bPlayFabProfileReady, bPlayFabRequestInFlight, NeonFreeSpins.Remaining, NeonFreeSpins.bPaused, bNeonSpinAnimating, bNeonPayoutAnimating, bNeonHeatBonusActive, bNeonHeatBonusComplete, IsProgressiveJackpotVisible());
    if (NeonFreeSpins.Remaining > 0 && !bNeonHeatBonusActive && !bNeonHeatBonusComplete)
    {
        NeonFreeSpins.bPaused = !NeonFreeSpins.bPaused;
        NeonFreeSpins.DelaySeconds = 1.0f;
        UpdateNeonNightsUI();
        return;
    }
    if (NeonAutoSpinsRemaining > 0)
    {
        NeonAutoSpinsRemaining = 0;
        UpdateNeonNightsUI();
        return;
    }
    RequestNeonNightsSpin();
}
void USugarLustRootWidget::HandleNeonAutoSpin()
{
    if (bNeonHeatBonusActive || NeonFreeSpins.Remaining > 0) return;
    if (NeonAutoSpinsRemaining > 0)
    {
        NeonAutoSpinsRemaining = 0;
        UpdateNeonNightsUI();
        return;
    }
    if (bPlayFabProfileReady && !bPlayFabRequestInFlight && !bNeonSpinAnimating && !bNeonPayoutAnimating && NeonCoins >= static_cast<int64>(NeonBet) * 5)
    {
        NeonAutoSpinsRemaining = 10;
        RequestNeonNightsSpin();
    }
}
void USugarLustRootWidget::RequestNeonNightsSpin()
{
    if (IsProgressiveJackpotVisible() || !bShowingNeonNightsPreview || !bPlayFabProfileReady || bPlayFabRequestInFlight || bNeonSpinAnimating || bNeonPayoutAnimating || bNeonHeatBonusComplete) return;
    const int64 TotalBet = static_cast<int64>(NeonBet) * 5;
    if (!bNeonHeatBonusActive && NeonFreeSpins.Remaining == 0 && NeonCoins < TotalBet)
    {
        if (NeonStatusText) NeonStatusText->SetText(FText::FromString(TEXT("NOT ENOUGH COINS FOR THIS BET.")));
        NeonAutoSpinsRemaining = 0;
        return;
    }

    TSharedRef<FJsonObject> Desired = MakeShared<FJsonObject>();
    Desired->SetNumberField(TEXT("Bet"), NeonBet);
    if (!PreparePendingSpin(PendingNeonSpinId, PendingNeonSpinParameters, Desired))
    {
        NeonSpinError = TEXT("PENDING SPIN PARAMETERS ARE UNAVAILABLE. RECOVER THE ORIGINAL REQUEST BEFORE PLAYING.");
        UpdateNeonNightsUI();
        return;
    }
    NeonSpinError.Reset();
    if (!bNeonHeatBonusActive) NeonFreeSpins.bPaused = false;
    bNeonCurrentSpinIsBonus = bNeonHeatBonusActive;
    NeonWinningGroups.Reset(); NeonWinningScatterCells.Reset(); NeonWinningFreeSpinCells.Reset(); NeonWinningJackpotCells.Reset();
    NeonBaseWin = 0; NeonWinPresentationSeconds = 0.0f;
    StopPayoutWinTune();
    StartSpinMusic();
    bPlayFabRequestInFlight = true;
    if (NeonAutoSpinsRemaining > 0) --NeonAutoSpinsRemaining;
    bNeonSpinAnimating = true;
    bNeonResultReady = false;
    NeonSpinSeconds = 0.0f;
    NeonGridAnimationTick = 0.0f;
    NeonDisplayedWin = 0;
    NeonWin = 0; PendingNeonJackpotPrize = 0; PendingNeonJackpotTier.Reset();
    const FString SpinId = PendingNeonSpinId;
    const TSharedRef<FJsonObject> Parameters = PendingNeonSpinParameters.ToSharedRef();
    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    UpdateNeonNightsUI();
    ExecutePlayFabCloudScript(TEXT("NeonNightsSpin"), Parameters,
        [WeakThis, SpinId](TSharedPtr<FJsonObject> Result)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bPlayFabRequestInFlight = false;
            const TArray<TSharedPtr<FJsonValue>>* GridJson = nullptr;
            const TSharedPtr<FJsonObject>* Profile = nullptr;
            const TSharedPtr<FJsonObject>* BonusState = nullptr;
            double RulesVersion = 0.0;
            double WinValue = 0.0, BaseWinValue = 0.0, CoinsValue = 0.0, XPValue = 0.0;
            if (!Result.IsValid() || !Result->TryGetNumberField(TEXT("RulesVersion"), RulesVersion) || RulesVersion != 5.0 ||
                !Result->TryGetArrayField(TEXT("Grid"), GridJson) || !GridJson || GridJson->Num() != 20 ||
                !Result->TryGetObjectField(TEXT("Bonus"), BonusState) || !BonusState || !BonusState->IsValid() ||
                !Result->TryGetNumberField(TEXT("BaseWin"), BaseWinValue) ||
                !Result->TryGetNumberField(TEXT("Win"), WinValue) || !Result->TryGetObjectField(TEXT("Profile"), Profile) || !Profile || !Profile->IsValid() ||
                !(*Profile)->TryGetNumberField(TEXT("Coins"), CoinsValue) || !(*Profile)->TryGetNumberField(TEXT("XP"), XPValue))
            {
                WeakThis->bNeonSpinAnimating = false; WeakThis->bNeonResultReady = false; WeakThis->NeonAutoSpinsRemaining = 0;
                WeakThis->NeonFreeSpins.bPaused = true;
                WeakThis->ScheduleSpinMusicFadeOut();
                if (WeakThis->bNeonHeatBonusActive) WeakThis->NeonHeatBonusDelaySeconds = 5.0f;
                WeakThis->NeonSpinError = TEXT("SPIN RESPONSE INCOMPLETE - CHECK THAT CLIENT AND SERVER RULES MATCH.");
                WeakThis->UpdateNeonNightsUI();
                return;
            }

            WeakThis->NeonTargetGrid.Reset();
            for (const TSharedPtr<FJsonValue>& Cell : *GridJson)
                WeakThis->NeonTargetGrid.Add(Cell.IsValid() ? FMath::Clamp(static_cast<int32>(Cell->AsNumber()), 0, 18) : 0);
            WeakThis->NeonCoins = static_cast<int64>(CoinsValue);
            WeakThis->NeonXp = static_cast<int64>(XPValue);
            WeakThis->NeonWin = static_cast<int64>(WinValue);
            const TSharedPtr<FJsonObject>* Jackpot = nullptr;
            double JackpotPrize = 0;
            if (Result->TryGetObjectField(TEXT("JackpotWin"), Jackpot) && Jackpot && Jackpot->IsValid() &&
                (*Jackpot)->TryGetStringField(TEXT("Tier"), WeakThis->PendingNeonJackpotTier) &&
                (*Jackpot)->TryGetNumberField(TEXT("Prize"), JackpotPrize))
                WeakThis->PendingNeonJackpotPrize = FMath::Max<int64>(0, static_cast<int64>(JackpotPrize));
            WeakThis->NeonBaseWin = static_cast<int64>(BaseWinValue);
            WeakThis->bNeonResultReady = true;
            WeakThis->ApplyNeonHeatBonusState(*BonusState);
            WeakThis->ApplyJackpotData(*Profile);
            WeakThis->ApplyCloudMissionData(*Profile);
            WeakThis->UpdateMissionsUI();
            if (WeakThis->SlotGame)
            {
                FString ClaimDate;
                double Streak = 0.0;
                (*Profile)->TryGetStringField(TEXT("DailyBonusLastClaimDate"), ClaimDate);
                (*Profile)->TryGetNumberField(TEXT("DailyBonusStreakDay"), Streak);
                WeakThis->SlotGame->ApplyCloudProfile(static_cast<int64>(CoinsValue), static_cast<int64>(XPValue), ClaimDate, static_cast<int32>(Streak));
            }
            if (WeakThis->PendingNeonSpinId == SpinId)
            {
                WeakThis->PendingNeonSpinId.Reset();
                WeakThis->PendingNeonSpinParameters.Reset();
            }
            UE_LOG(LogTemp, Log, TEXT("Neon Nights spin accepted. Win: %lld, coins: %lld, XP: %lld."),
                static_cast<long long>(WinValue), static_cast<long long>(CoinsValue), static_cast<long long>(XPValue));
            WeakThis->UpdateNeonNightsUI();
        },
        [WeakThis](const FString& Error)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bPlayFabRequestInFlight = false;
            WeakThis->bNeonSpinAnimating = false; WeakThis->bNeonResultReady = false; WeakThis->NeonAutoSpinsRemaining = 0;
            if (WeakThis->bNeonHeatBonusActive) WeakThis->NeonHeatBonusDelaySeconds = 5.0f;
            WeakThis->NeonFreeSpins.bPaused = true;
            WeakThis->ScheduleSpinMusicFadeOut();
            UE_LOG(LogTemp, Warning, TEXT("Neon Nights spin failed: %s"), *Error);
            WeakThis->NeonSpinError = FString::Printf(TEXT("SPIN FAILED: %s"), *Error);
            WeakThis->UpdateNeonNightsUI();
        });
}

void USugarLustRootWidget::ShowLobby()
{
    StopNeonWelcome();
    StopPayoutWinTune();
    bShowingSlot = false;
    bShowingNeonNightsPreview = false;
    bSpinMusicFadeOutPending = false;
    bSpinMusicStartedForCurrentSpin = false;
    FadeOutSpinMusic();
    BuildLobby();
}
void USugarLustRootWidget::HandleOpenSlot() { WideBackgroundImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible); if(Lounge3D) { Lounge3D->SetActorHiddenInGame(true); Lounge3D->SetActorTickEnabled(false); } ShowSlot(); }
void USugarLustRootWidget::HandleOpenNeonNights() { ShowNeonNightsPreview(); }
void USugarLustRootWidget::HandleContinueToLobby()
{
    if (!bShowingDisclaimer || !DisclaimerContinueButton || !DisclaimerContinueButton->GetIsEnabled()) return;
    bShowingDisclaimer = false;
    DisclaimerContinueButton = nullptr;
    ShowLobby();
    StartPlayFabLogin();
}

void USugarLustRootWidget::StartPlayFabLogin()
{
    if (bPlayFabLoginStarted) return;
    bPlayFabLoginStarted = true;

    FString TitleId;
    GConfig->GetString(TEXT("SugarLust.PlayFab"), TEXT("TitleId"), TitleId, GGameIni);
    TitleId.TrimStartAndEndInline();
    if (TitleId.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("PlayFab login not started: TitleId is missing from DefaultGame.ini."));
        return;
    }

    const FString DeviceIdPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PlayFabDeviceId.txt"));
    FString CustomId;
    if (!FFileHelper::LoadFileToString(CustomId, *DeviceIdPath) || CustomId.TrimStartAndEnd().IsEmpty())
    {
        CustomId = FString::Printf(TEXT("SugarLust_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
        if (!FFileHelper::SaveStringToFile(CustomId, *DeviceIdPath))
        {
            UE_LOG(LogTemp, Error, TEXT("PlayFab login not started: could not save the local test account ID to %s."), *DeviceIdPath);
            return;
        }
    }
    CustomId.TrimStartAndEndInline();

    TSharedRef<FJsonObject> LoginBody = MakeShared<FJsonObject>();
    LoginBody->SetStringField(TEXT("TitleId"), TitleId);
    LoginBody->SetStringField(TEXT("CustomId"), CustomId);
    LoginBody->SetBoolField(TEXT("CreateAccount"), true);
    TSharedRef<FJsonObject> InfoRequest = MakeShared<FJsonObject>();
    InfoRequest->SetBoolField(TEXT("GetPlayerProfile"), true);
    LoginBody->SetObjectField(TEXT("InfoRequestParameters"), InfoRequest);

    FString RequestBody;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    if (!FJsonSerializer::Serialize(LoginBody, Writer))
    {
        UE_LOG(LogTemp, Error, TEXT("PlayFab login not started: failed to create the login request."));
        return;
    }

    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("https://%s.playfabapi.com/Client/LoginWithCustomID"), *TitleId));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(RequestBody);
    Request->SetTimeout(20.0f);
    Request->OnProcessRequestComplete().BindLambda(
        [WeakThis](FHttpRequestPtr CompletedRequest, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            if (!WeakThis.IsValid()) return;
            if (!bConnectedSuccessfully || !Response.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("PlayFab login failed: could not reach PlayFab. Check the connection and Output Log."));
                return;
            }

            TSharedPtr<FJsonObject> ResponseJson;
            const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (!FJsonSerializer::Deserialize(Reader, ResponseJson) || !ResponseJson.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("PlayFab login failed: PlayFab returned an unreadable response (HTTP %d)."), Response->GetResponseCode());
                return;
            }

            if (Response->GetResponseCode() < 200 || Response->GetResponseCode() >= 300)
            {
                FString ErrorMessage;
                ResponseJson->TryGetStringField(TEXT("errorMessage"), ErrorMessage);
                UE_LOG(LogTemp, Error, TEXT("PlayFab login failed (HTTP %d): %s"), Response->GetResponseCode(), *ErrorMessage);
                return;
            }

            const TSharedPtr<FJsonObject>* Data = nullptr;
            if (!ResponseJson->TryGetObjectField(TEXT("data"), Data) || !Data || !Data->IsValid() ||
                !(*Data)->TryGetStringField(TEXT("PlayFabId"), WeakThis->PlayFabPlayerId))
            {
                UE_LOG(LogTemp, Error, TEXT("PlayFab login returned success, but the player ID was missing."));
                return;
            }

            bool bNewAccount = false;
            (*Data)->TryGetBoolField(TEXT("NewlyCreated"), bNewAccount);
            (*Data)->TryGetStringField(TEXT("SessionTicket"), WeakThis->PlayFabSessionTicket);
            FString DisplayName;
            const TSharedPtr<FJsonObject>* InfoResult = nullptr;
            const TSharedPtr<FJsonObject>* PlayerProfile = nullptr;
            if ((*Data)->TryGetObjectField(TEXT("InfoResultPayload"), InfoResult) && InfoResult && InfoResult->IsValid() &&
                (*InfoResult)->TryGetObjectField(TEXT("PlayerProfile"), PlayerProfile) && PlayerProfile && PlayerProfile->IsValid())
            {
                (*PlayerProfile)->TryGetStringField(TEXT("DisplayName"), DisplayName);
            }
            DisplayName.TrimStartAndEndInline();
            if (!DisplayName.IsEmpty())
            {
                WeakThis->PlayerDisplayName = DisplayName;
            }
            else
            {
                const FString IdSuffix = WeakThis->PlayFabPlayerId.Right(4).ToUpper();
                WeakThis->PlayerDisplayName = FString::Printf(TEXT("Guest-%s"), *IdSuffix);
            }
            WeakThis->UpdatePlayerProfileUI();
            UE_LOG(LogTemp, Log, TEXT("PlayFab login successful. Player ID: %s. Account created: %s"),
                *WeakThis->PlayFabPlayerId, bNewAccount ? TEXT("yes") : TEXT("no"));
            WeakThis->RequestPlayFabProfile();
        });

    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("PlayFab login failed to start. Check the Output Log for HTTP module errors."));
    }
}

void USugarLustRootWidget::RequestPlayFabProfile()
{
    bPlayFabProfileReady = false;
    bPlayFabMissionsReady = false;
    TSharedRef<FJsonObject> Parameters = MakeShared<FJsonObject>();
    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    ExecutePlayFabCloudScript(TEXT("GetOrMigratePlayerProfile"), Parameters,
        [WeakThis](TSharedPtr<FJsonObject> Profile)
        {
            if (!WeakThis.IsValid() || !WeakThis->SlotGame || !Profile.IsValid()) return;

            double CoinsValue = 0.0, XPValue = 0.0, StreakValue = 0.0;
            FString ClaimDate;
            if (!Profile->TryGetNumberField(TEXT("Coins"), CoinsValue) ||
                !Profile->TryGetNumberField(TEXT("XP"), XPValue) ||
                !Profile->TryGetStringField(TEXT("DailyBonusLastClaimDate"), ClaimDate) ||
                !Profile->TryGetNumberField(TEXT("DailyBonusStreakDay"), StreakValue))
            {
                UE_LOG(LogTemp, Warning, TEXT("PlayFab profile response was incomplete; local progress remains active."));
                return;
            }

            WeakThis->SlotGame->ApplyCloudProfile(static_cast<int64>(CoinsValue), static_cast<int64>(XPValue), ClaimDate, static_cast<int32>(StreakValue));
            const TSharedPtr<FJsonObject>* NeonBonus = nullptr;
            WeakThis->ApplyNeonHeatBonusState(Profile->TryGetObjectField(TEXT("NeonHeatBonus"), NeonBonus) && NeonBonus ? *NeonBonus : nullptr);
            WeakThis->ApplyJackpotData(Profile);
            WeakThis->ApplyCloudMissionData(Profile);
            const TSharedPtr<FJsonObject>* Pending = nullptr;
            if (Profile->TryGetObjectField(TEXT("PendingBonus"), Pending) && Pending && Pending->IsValid())
            {
                double ScatterValue = 0.0, PicksValue = 0.0, TotalValue = 0.0;
                (*Pending)->TryGetNumberField(TEXT("ScatterCount"), ScatterValue);
                (*Pending)->TryGetNumberField(TEXT("PicksRemaining"), PicksValue);
                (*Pending)->TryGetNumberField(TEXT("Total"), TotalValue);
                TArray<int32> RevealedCards;
                TArray<int64> RevealedPrizes;
                const TArray<TSharedPtr<FJsonValue>>* Revealed = nullptr;
                if ((*Pending)->TryGetArrayField(TEXT("Revealed"), Revealed) && Revealed)
                {
                    for (const TSharedPtr<FJsonValue>& Entry : *Revealed)
                    {
                        const TSharedPtr<FJsonObject> CardData = Entry.IsValid() ? Entry->AsObject() : nullptr;
                        if (!CardData.IsValid()) continue;
                        double CardValue = -1.0, PrizeValue = 0.0;
                        if (CardData->TryGetNumberField(TEXT("Card"), CardValue) && CardData->TryGetNumberField(TEXT("Prize"), PrizeValue))
                        {
                            RevealedCards.Add(static_cast<int32>(CardValue));
                            RevealedPrizes.Add(static_cast<int64>(PrizeValue));
                        }
                    }
                }
                WeakThis->SlotGame->ResumeAuthoritativeBonus(static_cast<int32>(ScatterValue), static_cast<int32>(PicksValue),
                    static_cast<int64>(TotalValue), RevealedCards, RevealedPrizes);
            }

            WeakThis->bPlayFabProfileReady = true;
            WeakThis->RefreshLustVendBalance();
            WeakThis->JackpotRefreshSeconds = 15.0f;
            WeakThis->UpdatePlayerProfileUI();
            WeakThis->UpdateSlotUI();
            UE_LOG(LogTemp, Log, TEXT("PlayFab profile loaded and local save synchronized."));
        },
        [WeakThis](const FString& Error)
        {
            if (WeakThis.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("PlayFab profile load failed: %s. Local save remains available."), *Error);
                WeakThis->bPlayFabProfileReady = false;
                WeakThis->bPlayFabMissionsReady = false;
            }
        });
}

void USugarLustRootWidget::RequestPlayFabJackpots()
{
    if (!bPlayFabProfileReady || bJackpotRequestInFlight || bPlayFabRequestInFlight ||
        bBonusPickRequestInFlight || bDailyBonusRequestInFlight) return;

    bJackpotRequestInFlight = true;
    JackpotRefreshSeconds = 15.0f;
    TSharedRef<FJsonObject> Parameters = MakeShared<FJsonObject>();
    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    ExecutePlayFabCloudScript(TEXT("GetProgressiveJackpots"), Parameters,
        [WeakThis](TSharedPtr<FJsonObject> Result)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bJackpotRequestInFlight = false;
            WeakThis->ApplyJackpotData(Result);
        },
        [WeakThis](const FString& Error)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bJackpotRequestInFlight = false;
            UE_LOG(LogTemp, Verbose, TEXT("Progressive jackpot refresh failed: %s"), *Error);
        });
}

bool USugarLustRootWidget::ShouldRetrySpinRequest(const FString& FunctionName, const FString& SpinId,
    int32 Attempt, int32 HttpCode, const FString& CloudError, bool bConnected)
{
    // Only spin operations carry the end-to-end receipt protection required for
    // automatic retry. Never retry validation, rules or Javascript exceptions.
    return Attempt >= 0 && Attempt < 2 && !SpinId.IsEmpty() &&
        (FunctionName == TEXT("Spin") || FunctionName == TEXT("NeonNightsSpin") || FunctionName == TEXT("MegaLustPotSpin")) &&
        (!bConnected || HttpCode == 408 || HttpCode == 429 || (HttpCode >= 500 && HttpCode <= 599) ||
            CloudError == TEXT("CloudScriptHTTPRequestError"));
}

bool USugarLustRootWidget::PreparePendingSpin(FString& SpinId, TSharedPtr<FJsonObject>& Pending,
    const TSharedRef<FJsonObject>& Desired)
{
    if (SpinId.IsEmpty())
    {
        SpinId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Pending = MakeShared<FJsonObject>();
        Pending->Values = Desired->Values;
        Pending->SetStringField(TEXT("SpinId"), SpinId);
    }
    FString SavedId;
    // A manual retry must preserve the original wager and request identity.
    return Pending.IsValid() && Pending->TryGetStringField(TEXT("SpinId"), SavedId) && SavedId == SpinId;
}

void USugarLustRootWidget::ExecutePlayFabCloudScript(const FString& FunctionName, const TSharedRef<FJsonObject>& Parameters,
    TFunction<void(TSharedPtr<FJsonObject>)> OnSuccess, TFunction<void(const FString&)> OnFailure)
{
    ExecutePlayFabCloudScriptAttempt(FunctionName, Parameters, MoveTemp(OnSuccess), MoveTemp(OnFailure), 0);
}

void USugarLustRootWidget::ExecutePlayFabCloudScriptAttempt(const FString& FunctionName, const TSharedRef<FJsonObject>& Parameters,
    TFunction<void(TSharedPtr<FJsonObject>)> OnSuccess, TFunction<void(const FString&)> OnFailure, int32 Attempt)
{
    if (PlayFabSessionTicket.IsEmpty())
    {
        OnFailure(TEXT("PlayFab login session ticket is missing."));
        return;
    }

    FString TitleId;
    GConfig->GetString(TEXT("SugarLust.PlayFab"), TEXT("TitleId"), TitleId, GGameIni);
    TitleId.TrimStartAndEndInline();
    if (TitleId.IsEmpty())
    {
        OnFailure(TEXT("PlayFab TitleId is missing from DefaultGame.ini."));
        return;
    }

    TSharedRef<FJsonObject> BodyObject = MakeShared<FJsonObject>();
    BodyObject->SetStringField(TEXT("FunctionName"), FunctionName);
    BodyObject->SetObjectField(TEXT("FunctionParameter"), Parameters);
    BodyObject->SetBoolField(TEXT("GeneratePlayStreamEvent"), true);
    BodyObject->SetStringField(TEXT("RevisionSelection"), TEXT("Live"));

    FString Body;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
    if (!FJsonSerializer::Serialize(BodyObject, Writer))
    {
        OnFailure(TEXT("Could not create the CloudScript request."));
        return;
    }

    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("https://%s.playfabapi.com/Client/ExecuteCloudScript"), *TitleId));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("X-Authorization"), PlayFabSessionTicket);
    Request->SetContentAsString(Body);
    Request->SetTimeout(20.0f);
    TSharedRef<TFunction<void(TSharedPtr<FJsonObject>)>> SuccessCallback = MakeShared<TFunction<void(TSharedPtr<FJsonObject>)>>(MoveTemp(OnSuccess));
    TSharedRef<TFunction<void(const FString&)>> FailureCallback = MakeShared<TFunction<void(const FString&)>>(MoveTemp(OnFailure));
    Request->OnProcessRequestComplete().BindLambda(
        [WeakThis, SuccessCallback, FailureCallback, FunctionName, Parameters, Attempt](FHttpRequestPtr CompletedRequest, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            if (!WeakThis.IsValid()) return;
            auto Retry = [WeakThis, SuccessCallback, FailureCallback, FunctionName, Parameters, Attempt](int32 HttpCode, const FString& CloudError, bool bConnected)
            {
                FString SpinId;
                Parameters->TryGetStringField(TEXT("SpinId"), SpinId);
                if (!ShouldRetrySpinRequest(FunctionName, SpinId, Attempt, HttpCode, CloudError, bConnected) || !WeakThis->GetWorld()) return false;
                WeakThis->bSpinRequestRetrying = true;
                const float Delay = Attempt == 0 ? 0.75f : 1.5f;
                UE_LOG(LogTemp, Warning, TEXT("%s transient failure (HTTP %d, %s); retry %d/2 retains the original spin receipt."),
                    *FunctionName, HttpCode, *CloudError, Attempt + 1);
                FTimerHandle RetryTimer;
                WeakThis->GetWorld()->GetTimerManager().SetTimer(RetryTimer, FTimerDelegate::CreateLambda(
                    [WeakThis, SuccessCallback, FailureCallback, FunctionName, Parameters, Attempt]()
                    {
                        if (WeakThis.IsValid())
                            WeakThis->ExecutePlayFabCloudScriptAttempt(FunctionName, Parameters, *SuccessCallback, *FailureCallback, Attempt + 1);
                    }), Delay, false);
                return true;
            };
            if (FunctionName == TEXT("Spin") || FunctionName == TEXT("NeonNightsSpin") || FunctionName == TEXT("MegaLustPotSpin")) WeakThis->bSpinRequestRetrying = false;
            if (!bConnectedSuccessfully || !Response.IsValid())
            {
                if (Retry(0, TEXT(""), false)) return;
                (*FailureCallback)(TEXT("Could not reach PlayFab. Your pending spin can be retried."));
                return;
            }

            if (Retry(Response->GetResponseCode(), TEXT(""), true)) return;
            TSharedPtr<FJsonObject> ResponseJson;
            const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (!FJsonSerializer::Deserialize(Reader, ResponseJson) || !ResponseJson.IsValid())
            {
                (*FailureCallback)(FString::Printf(TEXT("PlayFab returned unreadable data (HTTP %d)."), Response->GetResponseCode()));
                return;
            }

            const TSharedPtr<FJsonObject>* Data = nullptr;
            if (!ResponseJson->TryGetObjectField(TEXT("data"), Data) || !Data || !Data->IsValid())
            {
                FString ErrorMessage;
                ResponseJson->TryGetStringField(TEXT("errorMessage"), ErrorMessage);
                (*FailureCallback)(ErrorMessage.IsEmpty() ? FString::Printf(TEXT("PlayFab request failed (HTTP %d)."), Response->GetResponseCode()) : ErrorMessage);
                return;
            }

            const TSharedPtr<FJsonObject>* CloudError = nullptr;
            if ((*Data)->TryGetObjectField(TEXT("Error"), CloudError) && CloudError && CloudError->IsValid())
            {
                FString ErrorName, ErrorMessage;
                (*CloudError)->TryGetStringField(TEXT("Error"), ErrorName);
                (*CloudError)->TryGetStringField(TEXT("Message"), ErrorMessage);
                if (Retry(Response->GetResponseCode(), ErrorName, true)) return;
                (*FailureCallback)(FString::Printf(TEXT("%s: %s"), *ErrorName, *ErrorMessage));
                return;
            }

            const TSharedPtr<FJsonObject>* Result = nullptr;
            if (!(*Data)->TryGetObjectField(TEXT("FunctionResult"), Result) || !Result || !Result->IsValid())
            {
                (*FailureCallback)(TEXT("CloudScript returned no result."));
                return;
            }
            (*SuccessCallback)(*Result);
        });

    if (!Request->ProcessRequest()) (*FailureCallback)(TEXT("PlayFab request could not be started."));
}

void USugarLustRootWidget::RequestPlayFabSpin()
{
    if (IsProgressiveJackpotVisible() || !SlotGame || !bPlayFabProfileReady || bPlayFabRequestInFlight || !SlotGame->CanSpin()) return;
    SugarSpinError.Reset();
    SugarFreeSpins.bPaused = false;
    PendingSugarJackpotPrize = 0; PendingSugarJackpotTier.Reset();
    TSharedRef<FJsonObject> Desired = MakeShared<FJsonObject>();
    Desired->SetNumberField(TEXT("BetPerLine"), SlotGame->GetBetPerLine());
    Desired->SetNumberField(TEXT("ActiveLines"), SlotGame->GetActiveLines());
    if (!PreparePendingSpin(PendingPlayFabSpinId, PendingSugarSpinParameters, Desired))
    {
        SugarSpinError = TEXT("PENDING SPIN PARAMETERS ARE UNAVAILABLE. RECOVER THE ORIGINAL REQUEST BEFORE PLAYING.");
        UpdateSlotUI();
        return;
    }
    bPlayFabRequestInFlight = true;
    const FString SpinId = PendingPlayFabSpinId;
    const TSharedRef<FJsonObject> Parameters = PendingSugarSpinParameters.ToSharedRef();
    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    UpdateSlotUI();
    ExecutePlayFabCloudScript(TEXT("Spin"), Parameters,
        [WeakThis, SpinId](TSharedPtr<FJsonObject> Result)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bPlayFabRequestInFlight = false;
            if (!WeakThis->SlotGame || !Result.IsValid())
            {
                WeakThis->AutoSpinsRemaining = 0;
                WeakThis->bAutoSpinWaitingForAnimation = false;
                WeakThis->UpdateSlotUI();
                return;
            }

            const TArray<TSharedPtr<FJsonValue>>* StopsJson = nullptr;
            const TSharedPtr<FJsonObject>* Profile = nullptr;
            const TSharedPtr<FJsonObject>* JackpotWin = nullptr;
            double RulesVersion = 0.0;
            double WinValue = 0.0, ScatterValue = 0.0, CoinsValue = 0.0, XPValue = 0.0, StreakValue = 0.0;
            double JackpotPrizeValue = 0.0;
            FString JackpotTier;
            FString ClaimDate;
            if (!Result->TryGetNumberField(TEXT("RulesVersion"), RulesVersion) || RulesVersion != 4.0 ||
                !Result->TryGetArrayField(TEXT("Stops"), StopsJson) || !StopsJson || StopsJson->Num() != FSugarLustSlotMachine::ReelCount ||
                !Result->TryGetNumberField(TEXT("Win"), WinValue) || !Result->TryGetNumberField(TEXT("ScatterCount"), ScatterValue) ||
                !Result->TryGetObjectField(TEXT("Profile"), Profile) || !Profile || !Profile->IsValid() ||
                !(*Profile)->TryGetNumberField(TEXT("Coins"), CoinsValue) || !(*Profile)->TryGetNumberField(TEXT("XP"), XPValue) ||
                !(*Profile)->TryGetStringField(TEXT("DailyBonusLastClaimDate"), ClaimDate) ||
                !(*Profile)->TryGetNumberField(TEXT("DailyBonusStreakDay"), StreakValue))
            {
                WeakThis->SugarFreeSpins.bPaused = true;
                WeakThis->SugarSpinError = TEXT("SPIN RESPONSE INCOMPLETE - CHECK THAT CLIENT AND SERVER RULES MATCH.");
                UE_LOG(LogTemp, Warning, TEXT("PlayFab spin response was incomplete; no reels were changed."));
                WeakThis->AutoSpinsRemaining = 0;
                WeakThis->bAutoSpinWaitingForAnimation = false;
                WeakThis->UpdateSlotUI();
                return;
            }

            TArray<int32> Stops;
            for (const TSharedPtr<FJsonValue>& Stop : *StopsJson) Stops.Add(Stop.IsValid() ? static_cast<int32>(Stop->AsNumber()) : -1);
            const int32 ScatterCount = static_cast<int32>(ScatterValue);
            WeakThis->ApplyJackpotData(*Profile);
            WeakThis->ApplyCloudMissionData(*Profile);
            WeakThis->UpdateMissionsUI();
            if (Result->TryGetObjectField(TEXT("JackpotWin"), JackpotWin) && JackpotWin && JackpotWin->IsValid() &&
                (*JackpotWin)->TryGetStringField(TEXT("Tier"), JackpotTier) &&
                (*JackpotWin)->TryGetNumberField(TEXT("Prize"), JackpotPrizeValue))
            {
                WeakThis->PendingSugarJackpotTier = JackpotTier;
                WeakThis->PendingSugarJackpotPrize = static_cast<int64>(JackpotPrizeValue);
                UE_LOG(LogTemp, Log, TEXT("PlayFab progressive jackpot hit. Tier: %s, prize: %lld."),
                    *JackpotTier, static_cast<long long>(JackpotPrizeValue));
            }
            bool bRecoveredLegacyReceipt = false;
            Result->TryGetBoolField(TEXT("RecoveredLegacyReceipt"), bRecoveredLegacyReceipt);
            if (bRecoveredLegacyReceipt)
            {
                // Old stops refer to the version-3 strips. Restore the settled
                // profile/bonus without showing those stops on version-4 strips.
                WeakThis->SlotGame->ApplyCloudProfile(static_cast<int64>(CoinsValue), static_cast<int64>(XPValue), ClaimDate, static_cast<int32>(StreakValue));
                if (WeakThis->PendingPlayFabSpinId == SpinId)
                {
                    WeakThis->PendingPlayFabSpinId.Reset();
                    WeakThis->PendingSugarSpinParameters.Reset();
                }
                WeakThis->bAutoSpinWaitingForAnimation = false;
                WeakThis->AutoSpinsRemaining = 0;
                WeakThis->RequestPlayFabProfile();
                WeakThis->ShowSlotNotice(FString::Printf(TEXT("PREVIOUS SPIN RECOVERED | WIN: %s"), *FText::AsNumber(static_cast<int64>(WinValue)).ToString()));
                if (WeakThis->PendingSugarJackpotPrize > 0)
                {
                    WeakThis->ShowProgressiveJackpotWin(WeakThis->PendingSugarJackpotTier, WeakThis->PendingSugarJackpotPrize);
                    WeakThis->PendingSugarJackpotPrize = 0;
                }
                WeakThis->UpdateSlotUI();
                return;
            }
            if (!WeakThis->SlotGame->BeginAuthoritativeSpin(Stops, static_cast<int64>(CoinsValue), static_cast<int64>(XPValue),
                static_cast<int64>(WinValue), ScatterCount, ClaimDate, static_cast<int32>(StreakValue)))
            {
                UE_LOG(LogTemp, Warning, TEXT("PlayFab returned a spin, but the reel animation could not be started."));
                WeakThis->AutoSpinsRemaining = 0;
                WeakThis->bAutoSpinWaitingForAnimation = false;
            }
            else
            {
                if (WeakThis->PendingPlayFabSpinId == SpinId)
                {
                    WeakThis->PendingPlayFabSpinId.Reset();
                    WeakThis->PendingSugarSpinParameters.Reset();
                }
                UE_LOG(LogTemp, Log, TEXT("PlayFab spin accepted. Win: %lld, coins: %lld, XP: %lld."),
                    static_cast<long long>(WinValue), static_cast<long long>(CoinsValue), static_cast<long long>(XPValue));
            }
            WeakThis->UpdateSlotUI();
        },
        [WeakThis](const FString& Error)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bPlayFabRequestInFlight = false;
            WeakThis->AutoSpinsRemaining = 0;
            WeakThis->bAutoSpinWaitingForAnimation = false;
            WeakThis->SugarFreeSpins.bPaused = true;
            WeakThis->SugarSpinError = FString::Printf(TEXT("SPIN FAILED: %s"), *Error);
            UE_LOG(LogTemp, Warning, TEXT("PlayFab spin failed: %s"), *Error);
            WeakThis->UpdateSlotUI();

        });
}


void USugarLustRootWidget::RequestPlayFabBonusPick(int32 Card)
{
    if (!SlotGame || !bPlayFabProfileReady || bBonusPickRequestInFlight || !SlotGame->IsBonusRoundActive()) return;
    bBonusPickRequestInFlight = true;
    TSharedRef<FJsonObject> Parameters = MakeShared<FJsonObject>();
    Parameters->SetNumberField(TEXT("Card"), Card);
    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    UpdateBonusUI();
    ExecutePlayFabCloudScript(TEXT("PickBonusCard"), Parameters,
        [WeakThis, Card](TSharedPtr<FJsonObject> Result)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bBonusPickRequestInFlight = false;
            const TSharedPtr<FJsonObject>* Profile = nullptr;
            double PrizeValue = 0.0, RemainingValue = 0.0, TotalValue = 0.0, CoinsValue = 0.0, XPValue = 0.0, StreakValue = 0.0;
            FString ClaimDate;
            if (!WeakThis->SlotGame || !Result.IsValid() ||
                !Result->TryGetNumberField(TEXT("Prize"), PrizeValue) || !Result->TryGetNumberField(TEXT("BonusTotal"), TotalValue) ||
                !Result->TryGetObjectField(TEXT("Profile"), Profile) || !Profile || !Profile->IsValid() ||
                !(*Profile)->TryGetNumberField(TEXT("Coins"), CoinsValue) || !(*Profile)->TryGetNumberField(TEXT("XP"), XPValue) ||
                !(*Profile)->TryGetStringField(TEXT("DailyBonusLastClaimDate"), ClaimDate) ||
                !(*Profile)->TryGetNumberField(TEXT("DailyBonusStreakDay"), StreakValue))
            {
                UE_LOG(LogTemp, Warning, TEXT("PlayFab bonus pick response was incomplete."));
                if (WeakThis->BonusJarAnimatingCard == Card)
                {
                    WeakThis->BonusJarAnimatingCard = INDEX_NONE;
                    WeakThis->BonusJarAnimationSeconds = 0.0f;
                }
                WeakThis->UpdateBonusUI();
                return;
            }
            Result->TryGetNumberField(TEXT("PicksRemaining"), RemainingValue);
            WeakThis->SlotGame->ApplyCloudProfile(static_cast<int64>(CoinsValue), static_cast<int64>(XPValue), ClaimDate, static_cast<int32>(StreakValue));
            WeakThis->ApplyCloudMissionData(*Profile);
            WeakThis->SlotGame->ApplyAuthoritativeBonusPick(Card, static_cast<int64>(PrizeValue), static_cast<int32>(RemainingValue),
                static_cast<int64>(TotalValue), static_cast<int64>(CoinsValue));
            if (static_cast<int32>(RemainingValue) == 0)
                WeakThis->PlayPayoutWinTune(WeakThis->SlotGame->GetLastWin());
            UE_LOG(LogTemp, Log, TEXT("PlayFab bonus pick accepted. Prize: %lld, picks remaining: %d."),
                static_cast<long long>(PrizeValue), static_cast<int32>(RemainingValue));
            WeakThis->UpdateBonusUI();
            WeakThis->UpdateSlotUI();
            WeakThis->UpdateMissionsUI();
        },
        [WeakThis, Card](const FString& Error)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bBonusPickRequestInFlight = false;
            if (WeakThis->BonusJarAnimatingCard == Card)
            {
                WeakThis->BonusJarAnimatingCard = INDEX_NONE;
                WeakThis->BonusJarAnimationSeconds = 0.0f;
            }
            UE_LOG(LogTemp, Warning, TEXT("PlayFab bonus pick failed: %s"), *Error);
            WeakThis->UpdateBonusUI();
        });
}

void USugarLustRootWidget::RequestPlayFabDailyBonus()
{
    if (!SlotGame || !bPlayFabProfileReady || bDailyBonusRequestInFlight || !SlotGame->CanClaimDailyBonus()) return;
    bDailyBonusRequestInFlight = true;
    TSharedRef<FJsonObject> Parameters = MakeShared<FJsonObject>();
    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    UpdateDailyBonusUI();
    ExecutePlayFabCloudScript(TEXT("ClaimDailyBonus"), Parameters,
        [WeakThis](TSharedPtr<FJsonObject> Result)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bDailyBonusRequestInFlight = false;
            const TSharedPtr<FJsonObject>* Profile = nullptr;
            double RewardValue = 0.0, CoinsValue = 0.0, XPValue = 0.0, StreakValue = 0.0;
            FString ClaimDate;
            if (!WeakThis->SlotGame || !Result.IsValid() || !Result->TryGetNumberField(TEXT("Reward"), RewardValue) ||
                !Result->TryGetObjectField(TEXT("Profile"), Profile) || !Profile || !Profile->IsValid() ||
                !(*Profile)->TryGetNumberField(TEXT("Coins"), CoinsValue) || !(*Profile)->TryGetNumberField(TEXT("XP"), XPValue) ||
                !(*Profile)->TryGetStringField(TEXT("DailyBonusLastClaimDate"), ClaimDate) ||
                !(*Profile)->TryGetNumberField(TEXT("DailyBonusStreakDay"), StreakValue))
            {
                if (WeakThis->DailyBonusMessageText) WeakThis->DailyBonusMessageText->SetText(FText::FromString(TEXT("BONUS RESPONSE WAS INCOMPLETE")));
                WeakThis->UpdateDailyBonusUI();
                return;
            }

            WeakThis->SlotGame->ApplyCloudProfile(static_cast<int64>(CoinsValue), static_cast<int64>(XPValue), ClaimDate, static_cast<int32>(StreakValue));
            UE_LOG(LogTemp, Log, TEXT("PlayFab daily bonus claimed. Reward: %lld, coins: %lld."),
                static_cast<long long>(RewardValue), static_cast<long long>(CoinsValue));
            if (WeakThis->DailyBonusMessageText)
            {
                WeakThis->DailyBonusMessageText->SetText(FText::FromString(FString::Printf(
                    TEXT("CLAIMED  +%s COINS"), *FText::AsNumber(static_cast<int64>(RewardValue)).ToString())));
            }
            WeakThis->UpdateDailyBonusUI();
            WeakThis->UpdatePlayerProfileUI();
        },
        [WeakThis](const FString& Error)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->bDailyBonusRequestInFlight = false;
            UE_LOG(LogTemp, Warning, TEXT("PlayFab daily bonus failed: %s"), *Error);
            if (WeakThis->DailyBonusMessageText) WeakThis->DailyBonusMessageText->SetText(FText::FromString(TEXT("BONUS NOT CLAIMED - PLEASE TRY AGAIN")));
            WeakThis->UpdateDailyBonusUI();
        });
}

void USugarLustRootWidget::HandleOpenDailyBonus()
{
    if (!DailyBonusCanvas) return;
    if (DailyBonusMessageText) DailyBonusMessageText->SetText(FText::GetEmpty());
    DailyBonusCanvas->SetVisibility(ESlateVisibility::Visible);
    UpdateDailyBonusUI();
}
void USugarLustRootWidget::HandleClaimDailyBonus()
{
    RequestPlayFabDailyBonus();
}
void USugarLustRootWidget::HandleCloseDailyBonus()
{
    if (DailyBonusCanvas) DailyBonusCanvas->SetVisibility(ESlateVisibility::Collapsed);
}
void USugarLustRootWidget::HandleOpenMissions()
{
    if (!MissionsCanvas) return;
    RefreshLustVendBalance();
    UpdateMissionsUI();
    MissionsCanvas->SetVisibility(ESlateVisibility::Visible);
}
void USugarLustRootWidget::HandleCloseMissions()
{
    if (MissionsCanvas) MissionsCanvas->SetVisibility(ESlateVisibility::Collapsed);
}
void USugarLustRootWidget::HandleOpenSettings()
{
    if (!SettingsCanvas) return;
    UpdateSettingsUI();
    SettingsCanvas->SetVisibility(ESlateVisibility::Visible);
}
void USugarLustRootWidget::HandleCloseSettings()
{
    if (SettingsCanvas) SettingsCanvas->SetVisibility(ESlateVisibility::Collapsed);
}
void USugarLustRootWidget::HandleToggleMusic()
{
    bMusicEnabled = !bMusicEnabled;
    if (!bMusicEnabled) FadeOutSpinMusic();
    else if ((bShowingSlot && SlotGame && SlotGame->IsSpinning()) || (bShowingNeonNightsPreview && bNeonSpinAnimating)) StartSpinMusic();
    UpdateSettingsUI();
}
void USugarLustRootWidget::HandleToggleSfx()
{
    bSfxEnabled = !bSfxEnabled;
    if (!bSfxEnabled)
    {
        StopPayoutWinTune();
        StopNeonWelcome();
        if (bMusicEnabled && bShowingNeonNightsPreview && bNeonSpinAnimating && IsValid(SpinMusicComponent) && SpinMusicComponent->IsPlaying())
            SpinMusicComponent->AdjustVolume(0.4f, 0.65f);
    }
    UpdateSettingsUI();
}
void USugarLustRootWidget::ClaimMissionAtIndex(int32 MissionIndex)
{
    if (!SlotGame) return;
    const TArray<FSugarLustSlotMachine::FMission> Missions = SlotGame->GetMissions();
    if (!Missions.IsValidIndex(MissionIndex)) return;
    if (bPlayFabProfileReady && !bPlayFabMissionsReady)
    {
        if (MissionsStatusText) MissionsStatusText->SetText(FText::FromString(TEXT("UPDATE PLAYFAB CLOUDSCRIPT TO ENABLE SERVER CLAIMS.")));
        return;
    }
    if (bPlayFabProfileReady)
    {
        if (bMissionClaimRequestInFlight) return;
        bMissionClaimRequestInFlight = true;
        if (MissionsStatusText) MissionsStatusText->SetText(FText::FromString(TEXT("VERIFYING MISSION REWARD...")));
        UpdateMissionsUI();

        TSharedRef<FJsonObject> Parameters = MakeShared<FJsonObject>();
        Parameters->SetStringField(TEXT("MissionId"), Missions[MissionIndex].Id);
        Parameters->SetStringField(TEXT("ClaimId"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
        const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
        ExecutePlayFabCloudScript(TEXT("ClaimMissionReward"), Parameters,
            [WeakThis](TSharedPtr<FJsonObject> Result)
            {
                if (!WeakThis.IsValid()) return;
                WeakThis->bMissionClaimRequestInFlight = false;
                const TSharedPtr<FJsonObject>* Profile = nullptr;
                double CoinsValue = 0.0, XPValue = 0.0, StreakValue = 0.0, RewardValue = 0.0;
                FString ClaimDate;
                bool bAlreadyClaimed = false, bMissionsReset = false;
                if (!WeakThis->SlotGame || !Result.IsValid() || !Result->TryGetObjectField(TEXT("Profile"), Profile) || !Profile || !Profile->IsValid() ||
                    !(*Profile)->TryGetNumberField(TEXT("Coins"), CoinsValue) || !(*Profile)->TryGetNumberField(TEXT("XP"), XPValue) ||
                    !(*Profile)->TryGetStringField(TEXT("DailyBonusLastClaimDate"), ClaimDate) ||
                    !(*Profile)->TryGetNumberField(TEXT("DailyBonusStreakDay"), StreakValue))
                {
                    if (WeakThis->MissionsStatusText) WeakThis->MissionsStatusText->SetText(FText::FromString(TEXT("CLAIM RESPONSE INCOMPLETE - REOPEN MISSIONS TO SYNC.")));
                    WeakThis->UpdateMissionsUI();
                    return;
                }

                Result->TryGetNumberField(TEXT("Reward"), RewardValue);
                Result->TryGetBoolField(TEXT("AlreadyClaimed"), bAlreadyClaimed);
                Result->TryGetBoolField(TEXT("MissionsReset"), bMissionsReset);
                WeakThis->SlotGame->ApplyCloudProfile(static_cast<int64>(CoinsValue), static_cast<int64>(XPValue), ClaimDate, static_cast<int32>(StreakValue));
                WeakThis->ApplyCloudMissionData(*Profile);
                if (WeakThis->MissionsStatusText)
                {
                    const FString Status = bMissionsReset
                        ? TEXT("NEW DAILY MISSIONS STARTED. PROGRESS RESET.")
                        : bAlreadyClaimed
                            ? TEXT("THIS MISSION WAS ALREADY CLAIMED. BALANCE SYNCED.")
                            : FString::Printf(TEXT("MISSION REWARD CLAIMED: +%s COINS"), *FText::AsNumber(static_cast<int64>(RewardValue)).ToString());
                    WeakThis->MissionsStatusText->SetText(FText::FromString(Status));
                }
                WeakThis->UpdatePlayerProfileUI();
                WeakThis->UpdateMissionsUI();
            },
            [WeakThis](const FString& Error)
            {
                if (!WeakThis.IsValid()) return;
                WeakThis->bMissionClaimRequestInFlight = false;
                UE_LOG(LogTemp, Warning, TEXT("PlayFab mission claim failed: %s"), *Error);
                if (WeakThis->MissionsStatusText) WeakThis->MissionsStatusText->SetText(FText::FromString(TEXT("MISSION CLAIM FAILED - PLEASE TRY AGAIN.")));
                WeakThis->UpdateMissionsUI();
            });
        return;
    }
    if (MissionsStatusText)
        MissionsStatusText->SetText(FText::FromString(TEXT("CONNECT TO PLAYFAB TO CLAIM SERVER-VERIFIED COINS.")));
    UpdateMissionsUI();
}
void USugarLustRootWidget::HandleClaimMission0() { ClaimMissionAtIndex(0); }
void USugarLustRootWidget::HandleClaimMission1() { ClaimMissionAtIndex(1); }
void USugarLustRootWidget::HandleClaimMission2() { ClaimMissionAtIndex(2); }
void USugarLustRootWidget::HandleClaimMission3() { ClaimMissionAtIndex(3); }
void USugarLustRootWidget::HandleClaimMission4() { ClaimMissionAtIndex(4); }
void USugarLustRootWidget::HandleBack()
{
    if (SocialRoom)
    {
        if(CabinetHost && CabinetHost->MegaSession && !CabinetHost->MegaSession->PrepareToLeaveCabinet())return;
        if (bPlayFabRequestInFlight || bBonusPickRequestInFlight || (SlotGame && SlotGame->IsSpinning()) ||
            (bShowingNeonNightsPreview && (bNeonSpinAnimating || bNeonPayoutAnimating || bNeonHeatBonusActive || bNeonHeatBonusComplete || IsProgressiveJackpotVisible()))) return;
        NeonAutoSpinsRemaining = 0;
        NeonFreeSpins.bPaused = true;
        StopNeonWelcome();
        AutoSpinsRemaining = 0;
        bAutoSpinWaitingForAnimation = false;
        if (CabinetHost) { auto* Host=CabinetHost.Get(); Host->UnmountSession(); Host->Destroy(); }
        bShowingSlot = false;
        bShowingNeonNightsPreview = false;
        SetVisibility(ESlateVisibility::Hidden);
        SetRenderScale(FVector2D(1,1)); SetRenderTranslation(FVector2D::ZeroVector);
        SocialRoom->ReturnFromGame();
        return;
    }
    ShowLobby();
}
void USugarLustRootWidget::ShowSlotNotice(const FString& Message)
{
    if (bShowingSlot && SlotNoticeText)
    {
        SlotNoticeText->SetText(FText::FromString(Message));
        SlotNoticeText->SetVisibility(ESlateVisibility::Visible);
        SlotNoticeSeconds = 2.5f;
        return;
    }
    if (LobbyNoticeText && LobbyNoticePanel)
    {
        LobbyNoticeText->SetText(FText::FromString(Message));
        LobbyNoticePanel->SetVisibility(ESlateVisibility::Visible);
        LobbyNoticeSeconds = 2.8f;
    }
}


bool USugarLustRootWidget::IsProgressiveJackpotVisible() const
{
    return IsValid(ProgressiveJackpotCanvas) && ProgressiveJackpotCanvas->GetVisibility() == ESlateVisibility::Visible;
}

void USugarLustRootWidget::ShowProgressiveJackpotWin(const FString& Tier, int64 Prize)
{
    if (!DesignCanvas || !WidgetTree || Prize <= 0) return;
    AutoSpinsRemaining = 0; NeonAutoSpinsRemaining = 0;
    FadeOutSpinMusic();
    if (IsValid(ProgressiveJackpotCanvas)) ProgressiveJackpotCanvas->RemoveFromParent();
    ProgressiveJackpotCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    Place(ProgressiveJackpotCanvas, 0, 0, 1920, 1080, 50);
    auto Add = [this](UWidget* Widget, float X, float Y, float W, float H)
    {
        UCanvasPanelSlot* Slot = ProgressiveJackpotCanvas->AddChildToCanvas(Widget);
        Slot->SetPosition(FVector2D(X,Y)); Slot->SetSize(FVector2D(W,H));
    };
    const FString Tiers[] = {TEXT("Mini"),TEXT("Minor"),TEXT("Major"),TEXT("Grand")};
    int32 TierIndex = 0;
    for (int32 Index = 0; Index < 4; ++Index) if (Tier == Tiers[Index]) TierIndex = Index;
    const FLinearColor Color = FSugarLustSlotMachine::GetSymbolAccent(static_cast<ESugarLustSymbol>(18 + TierIndex));
    UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Shade->SetBrushColor(FLinearColor(0,0,0,0.8f)); Add(Shade,0,0,1920,1080);
    UBorder* Edge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Edge->SetBrushColor(Color); Add(Edge,430,260,1060,570);
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(SugarLustStyle::Deep); Add(Panel,435,265,1050,560);
    Add(MakeText(TEXT("PROGRESSIVE JACKPOT"),28,SugarLustStyle::White,true),480,305,960,55);
    Add(MakeText(Tier.ToUpper()+TEXT(" JACKPOT!"),60,Color,true),480,380,960,100);
    Add(MakeText(FText::AsNumber(Prize).ToString(),62,SugarLustStyle::Gold,true),480,510,960,100);
    UButton* Continue = MakeButton(TEXT("CONTINUE"),Color,24);
    Continue->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleCloseProgressiveJackpot);
    Add(Continue,780,700,360,72);
}

void USugarLustRootWidget::HandleCloseProgressiveJackpot()
{
    if (IsValid(ProgressiveJackpotCanvas)) ProgressiveJackpotCanvas->SetVisibility(ESlateVisibility::Collapsed);
    SugarFreeSpins.DelaySeconds = 1.0f; NeonFreeSpins.DelaySeconds = 1.0f;
}

void USugarLustRootWidget::PlayNeonWelcome()
{
    StopNeonWelcome();
    if (!bSfxEnabled || !bShowingNeonNightsPreview) return;
    if (!NeonWelcomeSound)
        NeonWelcomeSound = LoadObject<USoundWave>(nullptr, TEXT("/Game/Audio/neon-nights-welcome.neon-nights-welcome"));
    if (!NeonWelcomeSound)
    {
        UE_LOG(LogTemp, Warning, TEXT("Neon Nights welcome announcement asset could not be loaded."));
        return;
    }
    NeonWelcomeSound->bLooping = false;
    NeonWelcomeComponent = UGameplayStatics::CreateSound2D(this, NeonWelcomeSound, 1.0f, 1.0f, 0.0f, nullptr, false, false);
    if (NeonWelcomeComponent) NeonWelcomeComponent->Play();
}

void USugarLustRootWidget::StopNeonWelcome()
{
    if (IsValid(NeonWelcomeComponent))
    {
        NeonWelcomeComponent->Stop();
        NeonWelcomeComponent->DestroyComponent();
    }
    NeonWelcomeComponent = nullptr;
}

void USugarLustRootWidget::StartSpinMusic()
{
    bSpinMusicFadeOutPending = false;
    SpinMusicStopDelaySeconds = 0.0f;
    if (!bMusicEnabled)
    {
        FadeOutSpinMusic();
        return;
    }

    const TCHAR* AssetPath = bShowingNeonNightsPreview
        ? TEXT("/Game/Audio/neon-nights-spin.neon-nights-spin")
        : TEXT("/Game/Audio/sugar-rush-spin.sugar-rush-spin");
    USoundWave* DesiredSound = LoadObject<USoundWave>(nullptr, AssetPath);
    if (!DesiredSound)
    {
        UE_LOG(LogTemp, Warning, TEXT("Spin music asset could not be loaded: %s"), AssetPath);
        return;
    }
    if (SpinMusicSound != DesiredSound)
    {
        if (SpinMusicComponent)
        {
            SpinMusicComponent->Stop();
            SpinMusicComponent->DestroyComponent();
            SpinMusicComponent = nullptr;
        }
        SpinMusicSound = DesiredSound;
    }
    SpinMusicSound->bLooping = true;

    if (!SpinMusicComponent)
    {
        SpinMusicComponent = UGameplayStatics::CreateSound2D(this, SpinMusicSound, 1.0f, 1.0f, 0.0f,
            nullptr, false, false);
    }

    if (!SpinMusicComponent) return;
    if (SpinMusicComponent->IsPlaying())
    {
        // Cancels a fade-out already in progress without restarting the track.
        SpinMusicComponent->AdjustVolume(0.45f, IsValid(NeonWelcomeComponent) && NeonWelcomeComponent->IsPlaying() ? 0.20f : 0.65f);
        UE_LOG(LogTemp, Log, TEXT("Slot spin music volume restored."));
    }
    else
    {
        SpinMusicComponent->FadeIn(0.65f, IsValid(NeonWelcomeComponent) && NeonWelcomeComponent->IsPlaying() ? 0.20f : 0.65f, 0.0f);
        UE_LOG(LogTemp, Log, TEXT("Slot spin music fade-in started."));
    }
}

void USugarLustRootWidget::ScheduleSpinMusicFadeOut()
{
    SpinMusicStopDelaySeconds = 2.5f;
    bSpinMusicFadeOutPending = true;
}

void USugarLustRootWidget::FadeOutSpinMusic()
{
    if (SpinMusicComponent && SpinMusicComponent->IsPlaying())
    {
        SpinMusicComponent->FadeOut(1.25f, 0.0f);
    }
}

// Procedural approximation of the supplied casino cue: short F-A-C figures,
// accented middle notes and a resolving F-major "bum". No recording is loaded.
TArray<uint8> USugarLustRootWidget::BuildPayoutWinPcm(float DurationSeconds, float Gain, int32 SampleRate)
{
    if (SampleRate <= 0) return {};
    const float Duration = FMath::Clamp(DurationSeconds, 0.4f, 12.0f);
    const int32 FrameCount = FMath::Max(1, FMath::RoundToInt(Duration * SampleRate));
    const int32 Figures = FMath::Clamp(FMath::FloorToInt((Duration - 0.36f) / 0.37f), 1, 10);
    const int32 NoteCount = Figures * 3 + 1;
    constexpr float FinishUnits = 3.3f;
    const float TotalUnits = Figures * 3.0f + FinishUnits;
    constexpr float Pitches[] = {349.23f, 440.0f, 523.25f}; // F4, A4, C5
    constexpr float Accents[] = {0.82f, 1.0f, 0.72f};
    Gain = FMath::Clamp(Gain, 0.0f, 0.4f);
    TArray<uint8> Pcm;
    Pcm.SetNumZeroed(FrameCount * sizeof(int16));
    int32 StartFrame = 0;
    auto Reed = [](float Frequency, float Time)
    {
        // A rounded reed/organ tone: upper octave, bright fourth harmonic,
        // light detuning and subtle vibrato rather than the old sine arpeggio.
        const float Phase = 2.0f * PI * Frequency * Time + 0.018f * FMath::Sin(2.0f * PI * 5.5f * Time);
        return 0.54f * FMath::Sin(Phase) + 0.20f * FMath::Sin(Phase * 2.0f) +
            0.09f * FMath::Sin(Phase * 3.0f) + 0.17f * FMath::Sin(Phase * 4.0f) +
            0.045f * FMath::Sin(Phase * 1.003f);
    };
    for (int32 Note = 0; Note < NoteCount; ++Note)
    {
        const bool bFinal = Note == NoteCount - 1;
        const int32 EndFrame = bFinal ? FrameCount : FMath::RoundToInt(FrameCount * (Note + 1.0f) / TotalUnits);
        const int32 NoteFrames = EndFrame - StartFrame;
        const int32 SoundFrames = bFinal ? NoteFrames : FMath::RoundToInt(NoteFrames * 0.82f);
        const float NoteSeconds = static_cast<float>(SoundFrames) / SampleRate;
        const float Frequency = bFinal ? Pitches[0] : Pitches[Note % 3];
        const float Accent = bFinal ? 1.08f : Accents[Note % 3];
        for (int32 Frame = 0; Frame < SoundFrames; ++Frame)
        {
            const float Time = static_cast<float>(Frame) / SampleRate;
            const float Progress = static_cast<float>(Frame) / FMath::Max(1, SoundFrames - 1);
            const float Attack = FMath::Min(1.0f, Time / 0.004f);
            const float Release = FMath::Clamp((NoteSeconds - Time - 1.0f / SampleRate) / (bFinal ? 0.10f : 0.022f), 0.0f, 1.0f);
            const float Envelope = Attack * Release * FMath::Exp(-(bFinal ? 1.6f : 1.1f) * Progress);
            float Voice = Reed(Frequency, Time);
            if (bFinal)
                Voice = (Voice + 0.42f * Reed(440.0f, Time) + 0.30f * Reed(523.25f, Time) +
                    0.30f * FMath::Sin(2.0f * PI * 174.615f * Time)) / 1.6f;
            else if (Note % 3 == 1)
                Voice += 0.16f * FMath::Sin(PI * Frequency * Time) * FMath::Exp(-5.0f * Progress);
            const float Mixed = Voice * Gain * Accent * Envelope;
            const int16 Sample = static_cast<int16>(FMath::Clamp(Mixed, -1.0f, 1.0f) * 32767.0f);
            FMemory::Memcpy(Pcm.GetData() + (StartFrame + Frame) * sizeof(int16), &Sample, sizeof(int16));
        }
        StartFrame = EndFrame;
    }
    return Pcm;
}

void USugarLustRootWidget::PlayPayoutWinTune(int64 WinAmount, float DurationSeconds)
{
    if (!bSfxEnabled) return;
    if (WinAmount <= 0 || (!bShowingNeonNightsPreview && !SlotGame)) return;

    // Big (10x wager) and higher wins use dedicated tier clips when those
    // assets are added. Keep the synthesized cue for regular wins only.
    const int64 TotalBet = bShowingNeonNightsPreview ? static_cast<int64>(NeonBet) * 5 : SlotGame->GetTotalBet();
    if (TotalBet <= 0 || (!bShowingNeonNightsPreview && WinAmount >= TotalBet * 10)) return;

    if (PayoutWinTuneComponent && PayoutWinTuneComponent->IsPlaying())
        PayoutWinTuneComponent->Stop();

    constexpr int32 SampleRate = 22050;
    const float WinRatio = static_cast<float>(WinAmount) / static_cast<float>(TotalBet);
    const float Gain = FMath::Clamp(0.22f + 0.025f * FMath::Log2(FMath::Max(1.0f, WinRatio)), 0.22f, 0.34f);
    TArray<uint8> Pcm = BuildPayoutWinPcm(DurationSeconds > 0.0f ? DurationSeconds : 1.5f, Gain, SampleRate);
    const int32 FrameCount = Pcm.Num() / sizeof(int16);

    PayoutWinTuneSound = NewObject<USoundWaveProcedural>(this);
    if (!PayoutWinTuneSound) return;
    PayoutWinTuneSound->NumChannels = 1;
    PayoutWinTuneSound->SetSampleRate(SampleRate);
    PayoutWinTuneSound->SetNumFrames(FrameCount);
    PayoutWinTuneSound->Duration = static_cast<float>(FrameCount) / SampleRate;
    PayoutWinTuneSound->bLooping = false;
    PayoutWinTuneSound->QueueAudio(Pcm.GetData(), Pcm.Num());
    PayoutWinTuneComponent = UGameplayStatics::SpawnSound2D(this, PayoutWinTuneSound, 0.8f);
}

void USugarLustRootWidget::HandleFreeCoinsButton()
{
    if (!bShowingSlot && DailyBonusCanvas)
    {
        HandleOpenDailyBonus();
        return;
    }
    ShowSlotNotice(TEXT("FREE COINS: CLAIM YOUR DAILY BONUS FROM THE LOBBY REWARDS."));
}
void USugarLustRootWidget::HandleStoreButton() { ShowSlotNotice(TEXT("THE STORE IS COMING SOON.")); }
void USugarLustRootWidget::HandleCoinPlusButton() { ShowSlotNotice(TEXT("COIN PURCHASES WILL BE AVAILABLE IN THE STORE.")); }
void USugarLustRootWidget::HandleGemsPlusButton()
{
    if (ActiveVendWidget && ActiveVendWidget->IsInViewport()) return;
    if (!bPlayFabProfileReady || PlayFabSessionTicket.IsEmpty()) { ShowSlotNotice(TEXT("PLEASE WAIT FOR YOUR CASINO LOGIN.")); return; }
    if (ULustVendWidget* Vend = CreateWidget<ULustVendWidget>(GetOwningPlayer(), ULustVendWidget::StaticClass()))
    {
        ActiveVendWidget = Vend;
        const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
        Vend->OnWalletChanged = [WeakThis](int64 Gems)
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->LobbyGemBalance = Gems;
            if (WeakThis->LobbyGemsText) WeakThis->LobbyGemsText->SetText(FText::AsNumber(Gems));
        };
        Vend->Configure(PlayFabSessionTicket, PlayFabPlayerId);
        Vend->AddToViewport(100);
    }
}
void USugarLustRootWidget::HandleOpenRewardLibrary()
{
    HandleGemsPlusButton();
    if(ActiveVendWidget && ActiveVendWidget->IsInViewport())ActiveVendWidget->ShowRewardLibrary();
}
void USugarLustRootWidget::RefreshLustVendBalance()
{
    if (PlayFabSessionTicket.IsEmpty()) return;
    const TWeakObjectPtr<USugarLustRootWidget> WeakThis(this);
    ULustVendWidget::SendAccount(PlayFabSessionTicket,TEXT("state"),nullptr,[WeakThis](TSharedPtr<FJsonObject> Json,int32 Code)
    {
        if (!WeakThis.IsValid() || Code!=200 || !Json.IsValid()) return;
        double Balance=0;
        if (Json->TryGetNumberField(TEXT("balance"), Balance))
        {
            WeakThis->GemEarningSummary=ULustVendWidget::DescribeEarnings(Json);
            if(WeakThis->GemEarningProgressText)WeakThis->GemEarningProgressText->SetText(FText::FromString(WeakThis->GemEarningSummary));
            WeakThis->LobbyGemBalance=static_cast<int64>(Balance);
            if(WeakThis->LobbyGemsText) WeakThis->LobbyGemsText->SetText(FText::AsNumber(WeakThis->LobbyGemBalance));
        }
    });
}
void USugarLustRootWidget::HandlePiggyBankButton() { ShowSlotNotice(TEXT("THE PIGGY BANK FEATURE IS COMING SOON.")); }
void USugarLustRootWidget::HandleLevelProgressButton() { ShowSlotNotice(TEXT("EARN XP FROM SPINS TO LEVEL UP.")); }
void USugarLustRootWidget::HandleSettingsButton() { HandleOpenSettings(); }
void USugarLustRootWidget::HandleMissionsButton() { HandleOpenMissions(); }
void USugarLustRootWidget::HandleLobbyHomeButton() { ShowSlotNotice(TEXT("YOU'RE ON THE LOBBY HOME SCREEN.")); }
void USugarLustRootWidget::HandleJackpotsButton() { ShowSlotNotice(TEXT("PROGRESSIVE JACKPOT METERS ARE SHOWN ABOVE THE GAME CARDS.")); }
void USugarLustRootWidget::HandleTournamentsButton() { ShowSlotNotice(TEXT("TOURNAMENTS ARE COMING SOON.")); }
void USugarLustRootWidget::HandleEventsButton() { ShowSlotNotice(TEXT("EVENTS ARE COMING SOON.")); }
void USugarLustRootWidget::HandleInboxButton() { ShowSlotNotice(TEXT("YOUR INBOX IS EMPTY FOR NOW.")); }
void USugarLustRootWidget::HandleVipButton() { ShowSlotNotice(TEXT("VIP REWARDS ARE COMING SOON.")); }
void USugarLustRootWidget::HandleComingSoonCard() { ShowSlotNotice(TEXT("THIS GAME IS COMING SOON. SUGAR LUST IS AVAILABLE TO PLAY.")); }
void USugarLustRootWidget::HandleSpin()
{
    UE_LOG(LogTemp, Log, TEXT("Sugar Lust SPIN clicked: ready=%d request=%d free=%d paused=%d canSpin=%d jackpot=%d"),
        bPlayFabProfileReady, bPlayFabRequestInFlight, SugarFreeSpins.Remaining, SugarFreeSpins.bPaused, SlotGame && SlotGame->CanSpin(), IsProgressiveJackpotVisible());
    if (SugarFreeSpins.Remaining > 0)
    {
        SugarFreeSpins.bPaused = !SugarFreeSpins.bPaused;
        SugarFreeSpins.DelaySeconds = 1.0f;
        UpdateSlotUI();
        return;
    }
    if(bAutoSpinStartedFromHold)
    {
        bAutoSpinStartedFromHold=false;
        return;
    }
    if(AutoSpinsRemaining>0)
    {
        AutoSpinsRemaining=0;
        UpdateSlotUI();
        return;
    }
    RequestPlayFabSpin();
}
void USugarLustRootWidget::HandleSpinPressed()
{
    bSpinButtonHeld=true;
    bAutoSpinStartedFromHold=false;
    SpinButtonHoldSeconds=0.0f;
}
void USugarLustRootWidget::HandleSpinReleased() { bSpinButtonHeld=false; }
void USugarLustRootWidget::HandleAutoSpin()
{
    if (SugarFreeSpins.Remaining > 0) return;
    if (AutoSpinsRemaining > 0)
    {
        AutoSpinsRemaining = 0;
    }
    else if (bPlayFabProfileReady && !bPlayFabRequestInFlight && SlotGame && SlotGame->CanSpin())
    {
        AutoSpinsRemaining = 10;
    }
    UpdateSlotUI();
}
void USugarLustRootWidget::HandleMaxBet()
{
    if (!PendingPlayFabSpinId.IsEmpty()) return;
    if (SlotGame) SlotGame->SetBetPerLine(10'000);
    UpdateSlotUI();
}
void USugarLustRootWidget::HandleBetDown()
{
    if (!SlotGame || !PendingPlayFabSpinId.IsEmpty()) return;
    SlotGame->SetBetPerLine(SlotGame->GetBetPerLine() - 1);
    ShowSlotNotice(FString::Printf(TEXT("BET PER LINE: %s  |  TOTAL BET: %s"),
        *FText::AsNumber(SlotGame->GetBetPerLine()).ToString(), *FText::AsNumber(SlotGame->GetTotalBet()).ToString()));
    UpdateSlotUI();
}
void USugarLustRootWidget::HandleBetDownPressed() { SetCandyButtonImage(BetMinusArt, CandyBetMinusPressedTexture); }
void USugarLustRootWidget::HandleBetDownReleased() { SetCandyButtonImage(BetMinusArt, CandyBetMinusNormalTexture); }
void USugarLustRootWidget::HandleBetUpPressed() { SetCandyButtonImage(BetPlusArt, CandyBetPlusPressedTexture); }
void USugarLustRootWidget::HandleBetUpReleased() { SetCandyButtonImage(BetPlusArt, CandyBetPlusNormalTexture); }
void USugarLustRootWidget::HandleBetUp()
{
    if (!SlotGame || !PendingPlayFabSpinId.IsEmpty()) return;
    SlotGame->SetBetPerLine(SlotGame->GetBetPerLine() + 1);
    ShowSlotNotice(FString::Printf(TEXT("BET PER LINE: %s  |  TOTAL BET: %s"),
        *FText::AsNumber(SlotGame->GetBetPerLine()).ToString(), *FText::AsNumber(SlotGame->GetTotalBet()).ToString()));
    UpdateSlotUI();
}
void USugarLustRootWidget::HandleLinesDown() { if (SlotGame && PendingPlayFabSpinId.IsEmpty()) SlotGame->SetActiveLines(SlotGame->GetActiveLines() - 1); UpdateSlotUI(); }
void USugarLustRootWidget::HandleLinesUp() { if (SlotGame && PendingPlayFabSpinId.IsEmpty()) SlotGame->SetActiveLines(SlotGame->GetActiveLines() + 1); UpdateSlotUI(); }
void USugarLustRootWidget::HandleCycleLines()
{
    if (!SlotGame || !PendingPlayFabSpinId.IsEmpty()) return;
    const int32 Current = SlotGame->GetActiveLines();
    const int32 Next = Current >= FSugarLustSlotMachine::PaylineCount
        ? 1
        : FMath::Min(FSugarLustSlotMachine::PaylineCount, ((Current / 5) + 1) * 5);
    SlotGame->SetActiveLines(Next);
    ShowSlotNotice(FString::Printf(TEXT("PAYLINES: %d"), Next));
    UpdateSlotUI();
}
void USugarLustRootWidget::HandleBonusCard0() { PickBonusCard(0); }
void USugarLustRootWidget::HandleBonusCard1() { PickBonusCard(1); }
void USugarLustRootWidget::HandleBonusCard2() { PickBonusCard(2); }
void USugarLustRootWidget::HandleBonusCard3() { PickBonusCard(3); }
void USugarLustRootWidget::HandleBonusCard4() { PickBonusCard(4); }
void USugarLustRootWidget::HandleBonusCard5() { PickBonusCard(5); }
void USugarLustRootWidget::HandleBonusContinue() { if (SlotGame) SlotGame->DismissBonusResult(); UpdateBonusUI(); UpdateSlotUI(); }

void USugarLustRootWidget::BuildSlot()
{
    CabinetControls = nullptr;
    SetWideBackground(TEXT("/Game/Art/sugarlust_backdrop.sugarlust_backdrop"));
    if (!SlotGame) SlotGame = new FSugarLustSlotMachine();
    SpinButton = nullptr;
    AutoSpinButton = nullptr;
    BetMinusButton = nullptr;
    BetPlusButton = nullptr;
    BetMinusArt = nullptr;
    BetPlusArt = nullptr;
    SpinButtonArt = nullptr;
    MaxBetButton = nullptr;
    AutoSpinStatusText = nullptr;
    AutoSpinsRemaining = 0;
    bAutoSpinWaitingForAnimation = false;
    bSpinButtonHeld = false;
    bAutoSpinStartedFromHold = false;
    SpinButtonHoldSeconds = 0.0f;
    BetText = nullptr;
    LinesMinusButton = nullptr;
    LinesPlusButton = nullptr;
    LinesText = nullptr;
    LinesLabel = nullptr;
    // Clear references to lobby widgets before ClearChildren lets them be collected.
    PlayerLevelText = nullptr;
    PlayerBadgeLevelText = nullptr;
    JackpotMiniText = nullptr;
    JackpotMinorText = nullptr;
    JackpotMajorText = nullptr;
    JackpotGrandText = nullptr;
    PlayerXpText = nullptr;
    PlayerXpBar = nullptr;
    DailyBonusCanvas = nullptr;
    DailyBonusButton = nullptr;
    DailyBonusClaimButton = nullptr;
    DailyBonusCloseButton = nullptr;
    DailyBonusCardFrames.Reset();
    DailyBonusCardLabels.Reset();
    DailyBonusStatusText = nullptr;
    MissionsCanvas = nullptr;
    SettingsCanvas = nullptr;
    SettingsMusicButton = nullptr;
    SettingsSfxButton = nullptr;
    SettingsMusicLabel = nullptr;
    SettingsSfxLabel = nullptr;
    MissionsCloseButton = nullptr;
    MissionsStatusText = nullptr;
    MissionClaimButtons.Reset();
    MissionLabels.Reset();
    MissionBorders.Reset();
    DailyBonusMessageText = nullptr;
    DailyBonusClaimLabel = nullptr;
    LobbyCoinsText = nullptr;
    LobbyGemsText = nullptr;
    LobbyNoticePanel = nullptr;
    LobbyNoticeText = nullptr;
    LobbyNoticeSeconds = 0.0f;
    LobbyHostGreetingPanel = nullptr;
    LobbyHostGreetingText = nullptr;
    LobbyHostGreetingSeconds = 0.0f;
    ProgressiveJackpotCanvas = nullptr;
    ClearScreenWidgets();
    ReelViewports.Reset(); ReelTileSlots.Reset(); ReelArt.Reset(); ReelMarks.Reset(); ReelSymbolCache.Reset(); WinLineSegments.Reset();
    BonusCardButtons.Reset(); BonusCardLabels.Reset(); BonusCardImages.Reset(); BonusCardPrizeLabels.Reset();
    BonusJarSpriteSheet = nullptr;
    BonusJarAnimatingCard = INDEX_NONE;
    BonusJarAnimationSeconds = 0.0f;
    BonusCanvas = nullptr;
    BigWinCanvas = nullptr;
    BigWinBackdropImage = nullptr;
    BigWinAmountCover = nullptr;
    BigWinFallbackPanel = nullptr;
    BigWinCharacterLeft = nullptr;
    BigWinCharacterRight = nullptr;
    BigWinTitleText = nullptr;
    BigWinAmountText = nullptr;
    BigWinCoinsLabel = nullptr;
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Backdrop->SetBrushColor(FLinearColor(0.025f,0.004f,0.04f,1)); Place(Backdrop,0,0,1920,1080,0);
    if (UTexture2D* Background = LoadArt(TEXT("/Game/Art/sugarlust_backdrop.sugarlust_backdrop")))
    {
        UImage* BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        BackgroundImage->SetBrushFromTexture(Background, true);
        Place(BackgroundImage, 0, 0, 1920, 1080, 1);
    }
    UBorder* BackArtShade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    BackArtShade->SetBrushColor(FLinearColor(0.025f, 0.004f, 0.04f, 0.24f));
    Place(BackArtShade, 0, 0, 1920, 1080, 2);
    if(UTexture2D* TopBarTexture=LoadArt(TEXT("/Game/Art/slot-top-bar-clean.slot-top-bar-clean")))
    {
        UImage* TopBar=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        TopBar->SetBrushFromTexture(TopBarTexture,true);
        Place(TopBar,0,0,1920,126,2);
    }
    auto MakeBarHitTarget=[this]()
    {
        UButton* Hit=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        RemoveButtonChrome(Hit);
        return Hit;
    };
    UButton* Back=MakeBarHitTarget();
    Back->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBack); Place(Back,92,6,106,105,5);
    UButton* CoinPlusHit=MakeBarHitTarget();
    CoinPlusHit->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleCoinPlusButton); Place(CoinPlusHit,638,8,70,98,5);
    UButton* StoreHit=MakeBarHitTarget();
    StoreHit->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleStoreButton); Place(StoreHit,765,4,440,108,5);
    UButton* LevelPlusHit=MakeBarHitTarget();
    LevelPlusHit->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleLevelProgressButton); Place(LevelPlusHit,1484,8,72,98,5);
    UButton* LockHit=MakeBarHitTarget();
    LockHit->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandlePiggyBankButton); Place(LockHit,1592,4,128,108,5);
    UButton* SettingsHit=MakeBarHitTarget();
    SettingsHit->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleSettingsButton); Place(SettingsHit,1740,4,124,108,5);
    // The imported sprite has inset display wells where sample values were removed.
    CoinsText=Cast<UTextBlock>(MakeText(TEXT("10,050,000"),22,SugarLustStyle::White,true)); Place(CoinsText,408,36,200,45,6);
    // The top-bar star is the player's level/XP indicator, replacing the sample count.
    SlotXpText=Cast<UTextBlock>(MakeText(TEXT("LEVEL 1\n0 / 1,000 XP"),15,SugarLustStyle::White,true));
    Place(SlotXpText,1292,24,184,72,6);
    Place(MakeText(TEXT("SUGAR LUST"),24,SugarLustStyle::Gold,true),198,128,390,30,4);
    SugarFreeSpinsText = Cast<UTextBlock>(MakeText(TEXT(""), 17, SugarLustStyle::Gold, true));
    Place(SugarFreeSpinsText, 590, 125, 1080, 32, 9);

    // The custom Mini frame has a transparent center reserved for its live value.
    if (UTexture2D* MiniFrameTexture = LoadArt(TEXT("/Game/Art/mini.mini")))
    {
        UImage* MiniFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        MiniFrame->SetBrushFromTexture(MiniFrameTexture, true);
        Place(MiniFrame, 335, 182, 260, 92, 3);
    }
    else
    {
        Place(MakePanel(TEXT("MINI"),FLinearColor(0,0.7f,1),SugarLustStyle::Panel,20),335,182,260,92,3);
    }
    JackpotMiniText = Cast<UTextBlock>(MakeText(FText::AsNumber(JackpotMini).ToString(), 20, SugarLustStyle::White, true));
    Place(JackpotMiniText, 372, 219, 186, 32, 4);
    if (UTexture2D* MinorFrameTexture = LoadArt(TEXT("/Game/Art/minor.minor")))
    {
        UImage* MinorFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        MinorFrame->SetBrushFromTexture(MinorFrameTexture, true);
        Place(MinorFrame, 615, 182, 270, 92, 3);
    }
    else
    {
        Place(MakePanel(TEXT("MINOR"),FLinearColor(0.1f,0.9f,0.2f),SugarLustStyle::Panel,20),615,182,270,92,3);
    }
    JackpotMinorText = Cast<UTextBlock>(MakeText(FText::AsNumber(JackpotMinor).ToString(), 19, SugarLustStyle::White, true));
    Place(JackpotMinorText, 652, 216, 196, 32, 4);
    if (UTexture2D* GrandFrameTexture = LoadArt(TEXT("/Game/Art/grand-frame.grand-frame")))
    {
        UImage* GrandFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        GrandFrame->SetBrushFromTexture(GrandFrameTexture, true);
        Place(GrandFrame, 905, 154, 360, 120, 3);
    }
    else
    {
        Place(MakePanel(TEXT("GRAND"),SugarLustStyle::Gold,SugarLustStyle::Panel,25),905,162,360,112,3);
    }
    JackpotGrandText = Cast<UTextBlock>(MakeText(FText::AsNumber(JackpotGrand).ToString(), 22, SugarLustStyle::White, true));
    Place(JackpotGrandText, 980, 222, 210, 32, 4);
    if (UTexture2D* MajorFrameTexture = LoadArt(TEXT("/Game/Art/major-frame.major-frame")))
    {
        UImage* MajorFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        MajorFrame->SetBrushFromTexture(MajorFrameTexture, true);
        Place(MajorFrame, 1285, 174, 300, 100, 3);
    }
    else
    {
        Place(MakePanel(TEXT("MAJOR"),SugarLustStyle::Pink,SugarLustStyle::Panel,20),1285,182,300,92,3);
    }
    JackpotMajorText = Cast<UTextBlock>(MakeText(FText::AsNumber(JackpotMajor).ToString(), 18, SugarLustStyle::White, true));
    Place(JackpotMajorText, 1341, 230, 188, 28, 4);

    // Expand the reel cabinet into the space above the bottom HUD while preserving
    // the bezel's source aspect ratio and keeping the reels inset inside its frame.
    UBorder* Cabinet=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()); Cabinet->SetBrushColor(FLinearColor::Transparent); Cabinet->SetPadding(FMargin(8));
    UCanvasPanel* ReelArea=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass()); Cabinet->SetContent(ReelArea); Place(Cabinet,330,290,1260,610,3);
    const float ReelW=224.0f, CellH=190.0f, Gap=18.0f, Top=5.0f;
    const float ReelFrameThickness=4.0f;
    const float ReelAreaWidth=1260.0f-16.0f;
    const float GridWidth=FSugarLustSlotMachine::ReelCount*ReelW+(FSugarLustSlotMachine::ReelCount-1)*Gap;
    const float Left=(ReelAreaWidth-GridWidth)*0.5f;
    UTexture2D* ReelCellFrameTexture = LoadArt(TEXT("/Game/Art/sugar-lust-reel-cell-frame.sugar-lust-reel-cell-frame"));
    const UTexture2D* Wild=LoadArt(TEXT("/Game/Art/wild.wild")); const UTexture2D* Scatter=LoadArt(TEXT("/Game/Art/scatter.scatter")); const UTexture2D* Chocolate=LoadArt(TEXT("/Game/Art/chocolate.chocolate")); const UTexture2D* Cupcake=LoadArt(TEXT("/Game/Art/cupcake.cupcake")); const UTexture2D* Cupcake2=LoadArt(TEXT("/Game/Art/cupcake2.cupcake2")); const UTexture2D* LollipopGirl=LoadArt(TEXT("/Game/Art/lollipop.lollipop")); const UTexture2D* LollipopCandy=LoadArt(TEXT("/Game/Art/lollipop_candy.lollipop_candy")); const UTexture2D* GummyBear=LoadArt(TEXT("/Game/Art/gummybear.gummybear"));
    for(int32 Reel=0;Reel<FSugarLustSlotMachine::ReelCount;++Reel)
    {
        UCanvasPanel* View=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass()); View->SetClipping(EWidgetClipping::ClipToBounds);
        UBorder* ReelFrame=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        ReelFrame->SetBrushColor(FLinearColor::Transparent);
        ReelFrame->SetPadding(FMargin(ReelFrameThickness));
        UBorder* ReelInterior=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        ReelInterior->SetBrushColor(FLinearColor(0.025f,0.004f,0.035f,1.0f));
        ReelInterior->SetContent(View);
        ReelFrame->SetContent(ReelInterior);
        UCanvasPanelSlot* FrameSlot=ReelArea->AddChildToCanvas(ReelFrame);
        FrameSlot->SetPosition(FVector2D(Left+Reel*(ReelW+Gap),Top));
        FrameSlot->SetSize(FVector2D(ReelW,CellH*3+ReelFrameThickness*2));
        ReelViewports.Add(View);
        for(int32 TileRow=0;TileRow<4;++TileRow)
        {
            UOverlay* Layers=WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
            UBorder* Fill=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()); Fill->SetBrushColor(FLinearColor(0.07f,0.012f,0.09f,1)); Layers->AddChild(Fill);
            UImage* Art=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()); Art->SetVisibility(ESlateVisibility::Collapsed);
            // Keep the square symbol art proportional inside the near-square reel cells.
            UScaleBox* ArtScale=WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
            ArtScale->SetStretch(EStretch::ScaleToFit);
            ArtScale->AddChild(Art);
            UOverlaySlot* ArtSlot=Cast<UOverlaySlot>(Layers->AddChild(ArtScale));
            if(ArtSlot) ArtSlot->SetHorizontalAlignment(HAlign_Fill);
            if(ArtSlot) ArtSlot->SetVerticalAlignment(VAlign_Fill);
            // Several of the common candy symbols still need dedicated art.
            // Keep their temporary initials restrained so they read as a fallback,
            // instead of overpowering the reel like oversized debug text.
            UTextBlock* Mark=Cast<UTextBlock>(MakeText(TEXT(""),27,SugarLustStyle::Gold,true));
            UOverlaySlot* MarkSlot=Cast<UOverlaySlot>(Layers->AddChild(Mark));
            if(MarkSlot)
            {
                MarkSlot->SetHorizontalAlignment(HAlign_Center);
                MarkSlot->SetVerticalAlignment(VAlign_Center);
            }
            UCanvasPanelSlot* TileSlot=View->AddChildToCanvas(Layers); TileSlot->SetPosition(FVector2D(0,(TileRow-1)*CellH)); TileSlot->SetSize(FVector2D(ReelW-ReelFrameThickness*2,CellH));
            ReelTileSlots.Add(TileSlot); ReelArt.Add(Art); ReelMarks.Add(Mark); ReelSymbolCache.Add(255);
        }
        (void)Wild; (void)Scatter; (void)Chocolate; (void)Cupcake; (void)Cupcake2; (void)LollipopGirl; (void)LollipopCandy; (void)GummyBear;
    }

    // Draw each winning payline over the symbols, underneath the cabinet bezel.
    // Four thin segments form the path across the five reels.
    for (int32 Line = 0; Line < FSugarLustSlotMachine::PaylineCount; ++Line)
    {
        for (int32 Reel = 0; Reel < FSugarLustSlotMachine::ReelCount - 1; ++Reel)
        {
            const int32 RowA = SlotGame->GetPaylineRow(Line, Reel);
            const int32 RowB = SlotGame->GetPaylineRow(Line, Reel + 1);
            const FVector2D A(Left + Reel * (ReelW + Gap) + ReelW * 0.5f, Top + (RowA + 0.5f) * CellH);
            const FVector2D B(Left + (Reel + 1) * (ReelW + Gap) + ReelW * 0.5f, Top + (RowB + 0.5f) * CellH);
            const FVector2D Delta = B - A;
            UBorder* Segment = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
            Segment->SetBrushColor(FLinearColor(1.0f, 0.78f, 0.12f, 0.82f));
            Segment->SetVisibility(ESlateVisibility::Collapsed);
            UCanvasPanelSlot* SegmentSlot = ReelArea->AddChildToCanvas(Segment);
            SegmentSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            SegmentSlot->SetPosition((A + B) * 0.5f);
            SegmentSlot->SetSize(FVector2D(Delta.Size(), 3.0f));
            Segment->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)));
            WinLineSegments.Add(Segment);
        }
    }

    if (ReelCellFrameTexture)
    {
        for (int32 Reel = 0; Reel < FSugarLustSlotMachine::ReelCount; ++Reel)
        {
            UImage* ReelFrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
            ReelFrameImage->SetBrushFromTexture(ReelCellFrameTexture, true);
            const float ReelX = 330.0f + 8.0f + Left + Reel * (ReelW + Gap);
            const float ReelY = 290.0f + 8.0f + Top;
            Place(ReelFrameImage, ReelX, ReelY, ReelW, CellH * 3.0f + ReelFrameThickness * 2.0f, 5);
        }
    }
    if (UTexture2D* ReelFrameTexture = LoadArt(TEXT("/Game/Art/sugar-lust-reel-frame.sugar-lust-reel-frame")))
    {
        UImage* ReelFrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        ReelFrameImage->SetBrushFromTexture(ReelFrameTexture, true);
        Place(ReelFrameImage, 330, 290, 1260, 610, 6);
    }
    if(UTexture2D* BottomBarTexture=LoadArt(TEXT("/Game/Art/slot-bottom-bar-candy-clean.slot-bottom-bar-candy-clean")))
    {
        UImage* BottomBar=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        BottomBar->SetBrushFromTexture(BottomBarTexture,true);
        Place(BottomBar,0,915,1920,165,3);
    }
    CandyBetMinusNormalTexture = LoadArt(TEXT("/Game/Art/SlotButtons/CandyBetMinusNormal.CandyBetMinusNormal"));
    CandyBetMinusPressedTexture = LoadArt(TEXT("/Game/Art/SlotButtons/CandyBetMinusPressed.CandyBetMinusPressed"));
    CandyBetPlusNormalTexture = LoadArt(TEXT("/Game/Art/SlotButtons/CandyBetPlusNormal.CandyBetPlusNormal"));
    CandyBetPlusPressedTexture = LoadArt(TEXT("/Game/Art/SlotButtons/CandyBetPlusPressed.CandyBetPlusPressed"));
    CandySpinNormalTexture = LoadArt(TEXT("/Game/Art/SlotButtons/CandySpinNormal.CandySpinNormal"));
    CandySpinPressedTexture = LoadArt(TEXT("/Game/Art/SlotButtons/CandySpinPressed.CandySpinPressed"));
    CandySpinAutoTexture = LoadArt(TEXT("/Game/Art/SlotButtons/CandySpinAuto.CandySpinAuto"));

    BetMinusArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BetMinusCandyArt"));
    SetCandyButtonImage(BetMinusArt, CandyBetMinusNormalTexture);
    Place(BetMinusArt, 174, 941, 112, 112, 4);
    BetPlusArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BetPlusCandyArt"));
    SetCandyButtonImage(BetPlusArt, CandyBetPlusNormalTexture);
    Place(BetPlusArt, 513, 941, 112, 112, 4);
    SpinButtonArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SpinCandyArt"));
    SetCandyButtonImage(SpinButtonArt, CandySpinNormalTexture);
    Place(SpinButtonArt, 1612, 926, 144, 144, 4);

    BetMinusButton=MakeBarHitTarget();
    BetMinusButton->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBetDown);
    BetMinusButton->OnPressed.AddDynamic(this,&USugarLustRootWidget::HandleBetDownPressed);
    BetMinusButton->OnReleased.AddDynamic(this,&USugarLustRootWidget::HandleBetDownReleased);
    Place(BetMinusButton,171,938,118,118,5);
    BetPlusButton=MakeBarHitTarget();
    BetPlusButton->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBetUp);
    BetPlusButton->OnPressed.AddDynamic(this,&USugarLustRootWidget::HandleBetUpPressed);
    BetPlusButton->OnReleased.AddDynamic(this,&USugarLustRootWidget::HandleBetUpReleased);
    Place(BetPlusButton,510,938,118,118,5);
    TotalBetText=Cast<UTextBlock>(MakeText(TEXT("200"),22,SugarLustStyle::White,true)); Place(TotalBetText,292,972,214,34,6);
    WinText=Cast<UTextBlock>(MakeText(TEXT("0"),25,SugarLustStyle::White,true)); Place(WinText,800,966,310,40,6);
    SpinStatus=Cast<UTextBlock>(MakeText(TEXT(""),14,SugarLustStyle::White,true)); Place(SpinStatus,765,941,380,25,6);
    // The sprite's example value was removed; show the live paylines in that badge.
    LinesBadgeButton=MakeBarHitTarget();
    LinesBadgeButton->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleCycleLines); Place(LinesBadgeButton,1298,937,132,126,5);
    LinesText=Cast<UTextBlock>(MakeText(TEXT("20"),22,SugarLustStyle::White,true));
    LinesText->SetVisibility(ESlateVisibility::HitTestInvisible); Place(LinesText,1337,984,76,38,6);
    LinesLabel=Cast<UTextBlock>(MakeText(TEXT("LINES"),11,SugarLustStyle::Gold,true));
    LinesLabel->SetVisibility(ESlateVisibility::HitTestInvisible); Place(LinesLabel,1315,1017,98,20,6);

    // The sheet's SPIN art includes "HOLD FOR AUTO"; wire the full button area
    // so tap spins once and a press-and-hold starts the existing 10-spin sequence.
    SpinButton=MakeBarHitTarget();
    SpinButton->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleSpin);
    SpinButton->OnPressed.AddDynamic(this,&USugarLustRootWidget::HandleSpinPressed);
    SpinButton->OnReleased.AddDynamic(this,&USugarLustRootWidget::HandleSpinReleased);
    Place(SpinButton,1474,915,438,165,5);
    SlotNoticeText=Cast<UTextBlock>(MakeText(TEXT(""),14,SugarLustStyle::Pink,true));
    SlotNoticeText->SetVisibility(ESlateVisibility::Collapsed); Place(SlotNoticeText,520,899,880,24,8);
    AutoSpinButton=nullptr;
    AutoSpinStatusText=nullptr;

    BonusCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("BonusOverlay"));
    UCanvasPanelSlot* BonusRootSlot=DesignCanvas->AddChildToCanvas(BonusCanvas); BonusRootSlot->SetPosition(FVector2D::ZeroVector); BonusRootSlot->SetSize(FVector2D(1920,1080)); BonusRootSlot->SetZOrder(20); BonusCanvas->SetVisibility(ESlateVisibility::Collapsed);
    UBorder* Dim=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()); Dim->SetBrushColor(FLinearColor(0.01f,0.0f,0.025f,0.88f)); auto* DimSlot=BonusCanvas->AddChildToCanvas(Dim); DimSlot->SetPosition(FVector2D::ZeroVector); DimSlot->SetSize(FVector2D(1920,1080));
    UBorder* BonusPanel=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()); BonusPanel->SetBrushColor(SugarLustStyle::Gold); BonusPanel->SetPadding(FMargin(6)); UBorder* BonusInner=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()); BonusInner->SetBrushColor(SugarLustStyle::Deep); BonusPanel->SetContent(BonusInner);
    auto* BonusPanelSlot=BonusCanvas->AddChildToCanvas(BonusPanel); BonusPanelSlot->SetPosition(FVector2D(270,125)); BonusPanelSlot->SetSize(FVector2D(1380,830));
    BonusHeaderText=Cast<UTextBlock>(MakeText(TEXT("SUGAR RUSH BONUS!"),42,SugarLustStyle::Gold,true)); auto* HSlot=BonusCanvas->AddChildToCanvas(BonusHeaderText); HSlot->SetPosition(FVector2D(410,155)); HSlot->SetSize(FVector2D(1100,65));
    BonusPromptText=Cast<UTextBlock>(MakeText(TEXT("Choose 3 candy jars to collect their multipliers"),22,SugarLustStyle::White)); auto* PSlot=BonusCanvas->AddChildToCanvas(BonusPromptText); PSlot->SetPosition(FVector2D(410,220)); PSlot->SetSize(FVector2D(1100,44));
    BonusJarSpriteSheet = LoadArt(TEXT("/Game/Art/candy_jar_bonus_spritesheet.candy_jar_bonus_spritesheet"));
    if (!BonusJarSpriteSheet)
    {
        UE_LOG(LogTemp, Warning, TEXT("Candy jar bonus sheet is not imported yet. Import candy_jar_bonus_spritesheet.png into /Game/Art."));
    }
    for(int32 Card=0;Card<FSugarLustSlotMachine::BonusCardCount;++Card)
    {
        const int32 Col=Card%3, Row=Card/3;
        const float CenterX=555.0f+Col*390.0f;
        const float TopY=286.0f+Row*188.0f;

        UImage* JarImage=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        if(BonusJarSpriteSheet)
        {
            JarImage->SetBrushFromTexture(BonusJarSpriteSheet, true);
            FSlateBrush JarBrush=JarImage->GetBrush();
            JarBrush.SetUVRegion(FBox2f(FVector2f(0.0f,0.0f),FVector2f(0.2f,0.2f)));
            JarImage->SetBrush(JarBrush);
        }
        JarImage->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* JarSlot=BonusCanvas->AddChildToCanvas(JarImage);
        JarSlot->SetPosition(FVector2D(CenterX-88.0f,TopY));
        JarSlot->SetSize(FVector2D(176.0f,176.0f));
        BonusCardImages.Add(JarImage);

        UButton* CardButton=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        RemoveButtonChrome(CardButton);
        if(!BonusJarSpriteSheet)
            CardButton->SetContent(MakeText(TEXT("CANDY JAR"),22,SugarLustStyle::Gold,true));
        auto* CardSlot=BonusCanvas->AddChildToCanvas(CardButton);
        CardSlot->SetPosition(FVector2D(CenterX-112.0f,TopY-2.0f));
        CardSlot->SetSize(FVector2D(224.0f,184.0f));
        CardSlot->SetZOrder(2);
        BonusCardButtons.Add(CardButton);

        UTextBlock* PrizeLabel=Cast<UTextBlock>(MakeText(TEXT(""),17,FLinearColor(0.16f,0.025f,0.09f),true));
        PrizeLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* PrizeSlot=BonusCanvas->AddChildToCanvas(PrizeLabel);
        PrizeSlot->SetPosition(FVector2D(CenterX-54.0f,TopY+97.0f));
        PrizeSlot->SetSize(FVector2D(108.0f,30.0f));
        PrizeSlot->SetZOrder(3);
        BonusCardPrizeLabels.Add(PrizeLabel);

        UTextBlock* Caption=Cast<UTextBlock>(MakeText(TEXT("TAP TO PICK"),15,SugarLustStyle::White,true));
        Caption->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* CaptionSlot=BonusCanvas->AddChildToCanvas(Caption);
        CaptionSlot->SetPosition(FVector2D(CenterX-108.0f,TopY+168.0f));
        CaptionSlot->SetSize(FVector2D(216.0f,30.0f));
        CaptionSlot->SetZOrder(3);
        BonusCardLabels.Add(Caption);
    }
    BonusPicksText=Cast<UTextBlock>(MakeText(TEXT("PICKS LEFT: 3"),23,SugarLustStyle::White,true)); auto* PickSlot=BonusCanvas->AddChildToCanvas(BonusPicksText); PickSlot->SetPosition(FVector2D(500,690)); PickSlot->SetSize(FVector2D(420,48));
    BonusTotalText=Cast<UTextBlock>(MakeText(TEXT("BONUS WIN: 0"),24,SugarLustStyle::Gold,true)); auto* TotalSlot=BonusCanvas->AddChildToCanvas(BonusTotalText); TotalSlot->SetPosition(FVector2D(950,690)); TotalSlot->SetSize(FVector2D(420,48));
    BonusContinueButton=MakeButton(TEXT("COLLECT WIN"),FLinearColor(0.06f,0.8f,0.14f),24); BonusContinueButton->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBonusContinue); auto* ContinueSlot=BonusCanvas->AddChildToCanvas(BonusContinueButton); ContinueSlot->SetPosition(FVector2D(740,770)); ContinueSlot->SetSize(FVector2D(440,70));
    // Bind six explicit handlers to UMG's dynamic button delegates.
    BonusCardButtons[0]->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBonusCard0); BonusCardButtons[1]->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBonusCard1);
    BonusCardButtons[2]->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBonusCard2); BonusCardButtons[3]->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBonusCard3);
    BonusCardButtons[4]->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBonusCard4); BonusCardButtons[5]->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleBonusCard5);

    // A brief full-screen beat gives the scatter trigger a clear handoff into
    // the pick game. Keep it above the bonus panel, but below the big-win UI.
    BonusTransitionCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BonusTransition"));
    UCanvasPanelSlot* TransitionRootSlot = DesignCanvas->AddChildToCanvas(BonusTransitionCanvas);
    TransitionRootSlot->SetPosition(FVector2D::ZeroVector);
    TransitionRootSlot->SetSize(FVector2D(1920.0f, 1080.0f));
    TransitionRootSlot->SetZOrder(25);
    BonusTransitionCanvas->SetVisibility(ESlateVisibility::Collapsed);
    UBorder* TransitionDim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    TransitionDim->SetBrushColor(FLinearColor(0.015f, 0.0f, 0.035f, 0.80f));
    UCanvasPanelSlot* TransitionDimSlot = BonusTransitionCanvas->AddChildToCanvas(TransitionDim);
    TransitionDimSlot->SetPosition(FVector2D::ZeroVector);
    TransitionDimSlot->SetSize(FVector2D(1920.0f, 1080.0f));
    UBorder* TransitionPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    TransitionPanel->SetBrushColor(SugarLustStyle::Gold);
    TransitionPanel->SetPadding(FMargin(5.0f));
    UBorder* TransitionInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    TransitionInner->SetBrushColor(FLinearColor(0.12f, 0.015f, 0.15f, 0.98f));
    TransitionPanel->SetContent(TransitionInner);
    UCanvasPanelSlot* TransitionPanelSlot = BonusTransitionCanvas->AddChildToCanvas(TransitionPanel);
    TransitionPanelSlot->SetPosition(FVector2D(390.0f, 365.0f));
    TransitionPanelSlot->SetSize(FVector2D(1140.0f, 350.0f));
    BonusTransitionTitle = Cast<UTextBlock>(MakeText(TEXT("SCATTER BONUS!"), 64.0f, SugarLustStyle::Gold, true));
    BonusTransitionTitle->SetJustification(ETextJustify::Center);
    UCanvasPanelSlot* TransitionTitleSlot = BonusTransitionCanvas->AddChildToCanvas(BonusTransitionTitle);
    TransitionTitleSlot->SetPosition(FVector2D(440.0f, 430.0f));
    TransitionTitleSlot->SetSize(FVector2D(1040.0f, 90.0f));
    BonusTransitionSubtitle = Cast<UTextBlock>(MakeText(TEXT("SCATTER BONUS ROUND UNLOCKED"), 30.0f, SugarLustStyle::White, true));
    BonusTransitionSubtitle->SetJustification(ETextJustify::Center);
    UCanvasPanelSlot* TransitionSubtitleSlot = BonusTransitionCanvas->AddChildToCanvas(BonusTransitionSubtitle);
    TransitionSubtitleSlot->SetPosition(FVector2D(440.0f, 535.0f));
    TransitionSubtitleSlot->SetSize(FVector2D(1040.0f, 60.0f));

    // Full-screen win art is selected by payout tier. The legacy panel remains
    // as a fallback until the imported textures are available in the project.
    BigWinCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BigWinOverlay"));
    UCanvasPanelSlot* BigWinRootSlot = DesignCanvas->AddChildToCanvas(BigWinCanvas);
    BigWinRootSlot->SetPosition(FVector2D::ZeroVector);
    BigWinRootSlot->SetSize(FVector2D(1920.0f, 1080.0f));
    BigWinRootSlot->SetZOrder(30);
    BigWinCanvas->SetVisibility(ESlateVisibility::Collapsed);

    BigWinBackdropImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BigWinTierArtwork"));
    UCanvasPanelSlot* BigWinBackdropSlot = BigWinCanvas->AddChildToCanvas(BigWinBackdropImage);
    BigWinBackdropSlot->SetPosition(FVector2D::ZeroVector);
    BigWinBackdropSlot->SetSize(FVector2D(1920.0f, 1080.0f));

    UBorder* BigWinDim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    BigWinDim->SetBrushColor(FLinearColor(0.025f, 0.003f, 0.045f, 0.06f));
    UCanvasPanelSlot* BigWinDimSlot = BigWinCanvas->AddChildToCanvas(BigWinDim);
    BigWinDimSlot->SetPosition(FVector2D::ZeroVector);
    BigWinDimSlot->SetSize(FVector2D(1920.0f, 1080.0f));

    BigWinFallbackPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    BigWinFallbackPanel->SetBrushColor(SugarLustStyle::Gold);
    BigWinFallbackPanel->SetPadding(FMargin(7.0f));
    UBorder* BigWinPanelInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    BigWinPanelInner->SetBrushColor(FLinearColor(0.075f, 0.008f, 0.09f, 0.97f));
    BigWinFallbackPanel->SetContent(BigWinPanelInner);
    UCanvasPanelSlot* BigWinPanelSlot = BigWinCanvas->AddChildToCanvas(BigWinFallbackPanel);
    BigWinPanelSlot->SetPosition(FVector2D(455.0f, 300.0f));
    BigWinPanelSlot->SetSize(FVector2D(1010.0f, 430.0f));

    if (UTexture2D* CharacterSheet = LoadArt(TEXT("/Game/Art/strawberry-win-spritesheet.strawberry-win-spritesheet")))
    {
        auto AddCharacter = [this, CharacterSheet](const TCHAR* Name, const FVector2D& Position, bool bMirror)
        {
            UImage* Character = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
            Character->SetBrushFromTexture(CharacterSheet, true);
            Character->SetColorAndOpacity(FLinearColor::White);
            UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
            Fit->SetStretch(EStretch::ScaleToFit);
            Fit->AddChild(Character);
            UCanvasPanelSlot* CharacterSlot = BigWinCanvas->AddChildToCanvas(Fit);
            CharacterSlot->SetPosition(Position);
            CharacterSlot->SetSize(FVector2D(390.0f, 690.0f));
            if (bMirror)
            {
                Character->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
                Character->SetRenderScale(FVector2D(-1.0f, 1.0f));
            }
            return Character;
        };
        BigWinCharacterLeft = AddCharacter(TEXT("BigWinStrawberryLeft"), FVector2D(75.0f, 210.0f), false);
        BigWinCharacterRight = AddCharacter(TEXT("BigWinStrawberryRight"), FVector2D(1455.0f, 210.0f), true);
    }

    BigWinTitleText = Cast<UTextBlock>(MakeText(TEXT("BIG WIN!"), 64.0f, SugarLustStyle::Gold, true));
    UCanvasPanelSlot* BigWinTitleSlot = BigWinCanvas->AddChildToCanvas(BigWinTitleText);
    BigWinTitleSlot->SetPosition(FVector2D(555.0f, 360.0f));
    BigWinTitleSlot->SetSize(FVector2D(810.0f, 110.0f));
    BigWinAmountText = Cast<UTextBlock>(MakeText(TEXT("0"), 72.0f, SugarLustStyle::White, true));
    UCanvasPanelSlot* BigWinAmountSlot = BigWinCanvas->AddChildToCanvas(BigWinAmountText);
    BigWinAmountSlot->SetPosition(FVector2D(555.0f, 470.0f));
    BigWinAmountSlot->SetSize(FVector2D(810.0f, 110.0f));
    BigWinAmountSlot->SetZOrder(11);
    BigWinAmountText->SetAutoWrapText(false);
    BigWinAmountText->SetVisibility(ESlateVisibility::Visible);

    UBorder* BigWinAmountFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    BigWinAmountFrame->SetBrushColor(FLinearColor(0.78f, 0.34f, 0.035f, 0.96f));
    BigWinAmountFrame->SetPadding(FMargin(4.0f));
    BigWinAmountCover = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    BigWinAmountCover->SetBrushColor(FLinearColor(0.07f, 0.008f, 0.012f, 0.97f));
    BigWinAmountFrame->SetContent(BigWinAmountCover);
    UCanvasPanelSlot* BigWinAmountCoverSlot = BigWinCanvas->AddChildToCanvas(BigWinAmountFrame);
    BigWinAmountCoverSlot->SetPosition(FVector2D(415.0f, 700.0f));
    BigWinAmountCoverSlot->SetSize(FVector2D(1090.0f, 165.0f));
    BigWinAmountCoverSlot->SetZOrder(10);
    BigWinAmountFrame->SetVisibility(ESlateVisibility::Collapsed);
    BigWinCoinsLabel = Cast<UTextBlock>(MakeText(TEXT("COINS WON"), 24.0f, SugarLustStyle::Pink, true));
    UCanvasPanelSlot* BigWinCoinsSlot = BigWinCanvas->AddChildToCanvas(BigWinCoinsLabel);
    BigWinCoinsSlot->SetPosition(FVector2D(555.0f, 580.0f));
    BigWinCoinsSlot->SetSize(FVector2D(810.0f, 46.0f));

    UpdateReelWidgets(); UpdateSlotUI(); UpdateBonusUI();
}

void USugarLustRootWidget::ShowBigWinPresentation()
{
    if (!SlotGame || !BigWinCanvas) return;
    const int64 Win = SlotGame->GetLastWin();
    const int64 Bet = SlotGame->GetTotalBet();
    if (Bet <= 0 || Win < Bet * 10) return;

    const bool bEpic = Win >= Bet * 100;
    const bool bSuper = !bEpic && Win >= Bet * 50;
    const bool bMega = !bEpic && !bSuper && Win >= Bet * 25;
    const TCHAR* ArtPath = bEpic
        ? TEXT("/Game/Art/big-win-epic.big-win-epic")
        : bSuper
            ? TEXT("/Game/Art/big-win-super.big-win-super")
            : bMega
                ? TEXT("/Game/Art/big-win-mega.big-win-mega")
                : TEXT("/Game/Art/big-win-basic.big-win-basic");
    UTexture2D* TierArtwork = LoadArt(ArtPath);
    const bool bHasTierArtwork = TierArtwork != nullptr;
    if (BigWinBackdropImage)
    {
        if (TierArtwork) BigWinBackdropImage->SetBrushFromTexture(TierArtwork, true);
        BigWinBackdropImage->SetVisibility(bHasTierArtwork ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (BigWinFallbackPanel) BigWinFallbackPanel->SetVisibility(bHasTierArtwork ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (BigWinAmountCover) BigWinAmountCover->GetParent()->SetVisibility(bHasTierArtwork ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (BigWinTitleText)
    {
        const TCHAR* Tier = bEpic ? TEXT("EPIC WIN!") : bSuper ? TEXT("SUPER WIN!") : bMega ? TEXT("MEGA WIN!") : TEXT("BIG WIN!");
        BigWinTitleText->SetText(FText::FromString(Tier));
        BigWinTitleText->SetVisibility(bHasTierArtwork ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
    if (BigWinCoinsLabel) BigWinCoinsLabel->SetVisibility(bHasTierArtwork ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (BigWinCharacterLeft) BigWinCharacterLeft->SetVisibility(bHasTierArtwork ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (BigWinCharacterRight) BigWinCharacterRight->SetVisibility(bHasTierArtwork ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (BigWinAmountText)
    {
        BigWinAmountText->SetVisibility(ESlateVisibility::Visible);
        BigWinAmountText->SetColorAndOpacity(bHasTierArtwork ? SugarLustStyle::Gold : SugarLustStyle::White);
        if (UCanvasPanelSlot* AmountSlot = Cast<UCanvasPanelSlot>(BigWinAmountText->Slot))
        {
            AmountSlot->SetPosition(bHasTierArtwork ? FVector2D(465.0f, 720.0f) : FVector2D(555.0f, 470.0f));
            AmountSlot->SetSize(bHasTierArtwork ? FVector2D(990.0f, 125.0f) : FVector2D(810.0f, 110.0f));
        }
    }

    bSpinMusicFadeOutPending = false;
    SpinMusicStopDelaySeconds = 0.0f;
    if (SpinMusicComponent && SpinMusicComponent->IsPlaying())
    {
        SpinMusicComponent->FadeOut(0.35f, 0.0f);
    }

    BigWinPresentationAmount = Win;
    BigWinPresentationSeconds = 0.0f;
    BigWinPresentationDuration = bEpic ? 9.0f : bSuper ? 8.0f : bMega ? 7.0f : 6.0f;
    BigWinPayoutCountDuration = 2.4f;
    if (BigWinAmountText) BigWinAmountText->SetText(FText::FromString(TEXT("0")));
    BigWinCanvas->SetVisibility(ESlateVisibility::Visible);
    UpdateBigWinUI(0.0f);
}

void USugarLustRootWidget::UpdateBigWinUI(float DeltaSeconds)
{
    if (!BigWinCanvas || BigWinCanvas->GetVisibility() != ESlateVisibility::Visible) return;
    BigWinPresentationSeconds += FMath::Max(0.0f, DeltaSeconds);

    const float CountProgress = FMath::Clamp(BigWinPresentationSeconds / BigWinPayoutCountDuration, 0.0f, 1.0f);
    const float FastEaseOut = 1.0f - FMath::Pow(1.0f - CountProgress, 3.0f);
    const int64 DisplayAmount = CountProgress >= 1.0f
        ? BigWinPresentationAmount
        : static_cast<int64>(static_cast<double>(BigWinPresentationAmount) * FastEaseOut);
    if (BigWinAmountText) BigWinAmountText->SetText(FText::AsNumber(DisplayAmount));
    const int32 BaseFrame = FMath::FloorToInt(BigWinPresentationSeconds * 12.0f);
    auto SetCharacterFrame = [BaseFrame](UImage* Character, int32 Phase)
    {
        if (!Character) return;
        const int32 Frame = (BaseFrame + Phase) % 25;
        const int32 Column = Frame % 5;
        const int32 Row = Frame / 5;
        constexpr float Inset = 1.5f / 1280.0f;
        FSlateBrush Brush = Character->GetBrush();
        Brush.SetUVRegion(FBox2f(
            FVector2f(Column / 5.0f + Inset, Row / 5.0f + Inset),
            FVector2f((Column + 1) / 5.0f - Inset, (Row + 1) / 5.0f - Inset)));
        Character->SetBrush(Brush);
    };
    SetCharacterFrame(BigWinCharacterLeft, 0);
    SetCharacterFrame(BigWinCharacterRight, 11);

    if (BigWinPresentationSeconds >= BigWinPresentationDuration)
    {
        BigWinCanvas->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void USugarLustRootWidget::BuildDailyBonusModal()
{
    DailyBonusCardFrames.Reset();
    DailyBonusCardLabels.Reset();
    DailyBonusCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DailyBonusOverlay"));
    UCanvasPanelSlot* RootSlot = DesignCanvas->AddChildToCanvas(DailyBonusCanvas);
    RootSlot->SetPosition(FVector2D::ZeroVector);
    RootSlot->SetSize(FVector2D(1920, 1080));
    RootSlot->SetZOrder(20);
    DailyBonusCanvas->SetVisibility(ESlateVisibility::Collapsed);

    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Dim->SetBrushColor(FLinearColor(0.005f, 0.0f, 0.015f, 0.78f));
    UCanvasPanelSlot* DimSlot = DailyBonusCanvas->AddChildToCanvas(Dim);
    DimSlot->SetPosition(FVector2D::ZeroVector);
    DimSlot->SetSize(FVector2D(1920, 1080));

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(SugarLustStyle::Gold);
    Panel->SetPadding(FMargin(5));
    UBorder* PanelFill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    PanelFill->SetBrushColor(FLinearColor(0.055f, 0.008f, 0.075f, 0.99f));
    Panel->SetContent(PanelFill);
    UCanvasPanelSlot* PanelSlot = DailyBonusCanvas->AddChildToCanvas(Panel);
    PanelSlot->SetPosition(FVector2D(410, 185));
    PanelSlot->SetSize(FVector2D(1100, 700));

    UTextBlock* Header = Cast<UTextBlock>(MakeText(TEXT("DAILY BONUS"), 40, SugarLustStyle::Gold, true));
    UCanvasPanelSlot* HeaderSlot = DailyBonusCanvas->AddChildToCanvas(Header);
    HeaderSlot->SetPosition(FVector2D(510, 215));
    HeaderSlot->SetSize(FVector2D(900, 58));
    UTextBlock* Subtitle = Cast<UTextBlock>(MakeText(TEXT("COME BACK EACH DAY TO BUILD YOUR STREAK"), 19, SugarLustStyle::White, true));
    UCanvasPanelSlot* SubtitleSlot = DailyBonusCanvas->AddChildToCanvas(Subtitle);
    SubtitleSlot->SetPosition(FVector2D(510, 278));
    SubtitleSlot->SetSize(FVector2D(900, 38));

    DailyBonusCloseButton = MakeButton(TEXT("X"), SugarLustStyle::Pink, 22);
    DailyBonusCloseButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleCloseDailyBonus);
    UCanvasPanelSlot* CloseSlot = DailyBonusCanvas->AddChildToCanvas(DailyBonusCloseButton);
    CloseSlot->SetPosition(FVector2D(1418, 207));
    CloseSlot->SetSize(FVector2D(60, 52));

    for (int32 Index = 0; Index < 7; ++Index)
    {
        UBorder* CardFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        CardFrame->SetBrushColor(FLinearColor(0.36f, 0.19f, 0.46f, 1.0f));
        CardFrame->SetPadding(FMargin(2));
        UBorder* CardFill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        CardFill->SetBrushColor(FLinearColor(0.075f, 0.018f, 0.095f, 1.0f));
        CardFill->SetPadding(FMargin(4));
        UTextBlock* CardLabel = Cast<UTextBlock>(MakeText(TEXT(""), 17, SugarLustStyle::White, true));
        CardFill->SetContent(CardLabel);
        CardFrame->SetContent(CardFill);
        UCanvasPanelSlot* CardSlot = DailyBonusCanvas->AddChildToCanvas(CardFrame);
        CardSlot->SetPosition(FVector2D(451 + Index * 145, 348));
        CardSlot->SetSize(FVector2D(132, 196));
        DailyBonusCardFrames.Add(CardFrame);
        DailyBonusCardLabels.Add(CardLabel);
    }

    DailyBonusStatusText = Cast<UTextBlock>(MakeText(TEXT(""), 23, SugarLustStyle::Gold, true));
    UCanvasPanelSlot* StatusSlot = DailyBonusCanvas->AddChildToCanvas(DailyBonusStatusText);
    StatusSlot->SetPosition(FVector2D(500, 570));
    StatusSlot->SetSize(FVector2D(920, 46));
    DailyBonusMessageText = Cast<UTextBlock>(MakeText(TEXT(""), 19, SugarLustStyle::White, true));
    UCanvasPanelSlot* MessageSlot = DailyBonusCanvas->AddChildToCanvas(DailyBonusMessageText);
    MessageSlot->SetPosition(FVector2D(500, 615));
    MessageSlot->SetSize(FVector2D(920, 38));

    DailyBonusClaimButton = MakeButton(TEXT("CLAIM BONUS"), FLinearColor(0.08f, 0.85f, 0.16f), 24);
    if (auto* ContentSlot = Cast<UButtonSlot>(DailyBonusClaimButton->GetContent()->Slot))
    { ContentSlot->SetHorizontalAlignment(HAlign_Fill); ContentSlot->SetVerticalAlignment(VAlign_Fill); }
    DailyBonusClaimButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleClaimDailyBonus);
    UCanvasPanelSlot* ClaimSlot = DailyBonusCanvas->AddChildToCanvas(DailyBonusClaimButton);
    ClaimSlot->SetPosition(FVector2D(735, 665));
    ClaimSlot->SetSize(FVector2D(450, 72));
    if (UBorder* Face = Cast<UBorder>(DailyBonusClaimButton->GetContent()))
    {
        if (UBorder* Fill = Cast<UBorder>(Face->GetContent())) DailyBonusClaimLabel = Cast<UTextBlock>(Fill->GetContent());
    }
    UTextBlock* Footer = Cast<UTextBlock>(MakeText(TEXT("STREAK RESETS IF YOU MISS A DAY"), 16, FLinearColor(0.72f, 0.62f, 0.78f), true));
    UCanvasPanelSlot* FooterSlot = DailyBonusCanvas->AddChildToCanvas(Footer);
    FooterSlot->SetPosition(FVector2D(500, 755));
    FooterSlot->SetSize(FVector2D(920, 32));
}

void USugarLustRootWidget::UpdateDailyBonusUI()
{
    if (!SlotGame || !DailyBonusCanvas) return;
    const int32 CurrentDay = SlotGame->GetDailyBonusStreakDay();
    const bool bCanClaim = SlotGame->CanClaimDailyBonus();
    for (int32 Index = 0; Index < DailyBonusCardFrames.Num(); ++Index)
    {
        const int32 Day = Index + 1;
        const bool bCurrent = Day == CurrentDay;
        const bool bAlreadyClaimed = !bCanClaim && Day <= CurrentDay;
        DailyBonusCardFrames[Index]->SetBrushColor(bCurrent ? SugarLustStyle::Gold : bAlreadyClaimed ? FLinearColor(0.18f, 0.55f, 0.3f) : FLinearColor(0.36f, 0.19f, 0.46f));
        const FString Amount = FText::AsNumber(FSugarLustSlotMachine::GetDailyBonusRewardForDay(Day)).ToString();
        const FString CardText = bAlreadyClaimed
            ? FString::Printf(TEXT("DAY %d\n\nOK\n%s\nCLAIMED"), Day, *Amount)
            : FString::Printf(TEXT("DAY %d\n\nC\n%s\nCOINS"), Day, *Amount);
        DailyBonusCardLabels[Index]->SetText(FText::FromString(CardText));
        DailyBonusCardLabels[Index]->SetColorAndOpacity(FSlateColor(bCurrent ? SugarLustStyle::Gold : SugarLustStyle::White));
    }

    if (bCanClaim)
    {
        DailyBonusStatusText->SetText(FText::FromString(FString::Printf(TEXT("DAY %d REWARD: %s COINS"), CurrentDay, *FText::AsNumber(SlotGame->GetDailyBonusReward()).ToString())));
    }
    else
    {
        const int32 Seconds = SlotGame->GetDailyBonusSecondsRemaining();
        DailyBonusStatusText->SetText(FText::FromString(FString::Printf(TEXT("NEXT BONUS IN %02d:%02d:%02d"), Seconds / 3600, (Seconds / 60) % 60, Seconds % 60)));
    }
    if (DailyBonusClaimButton) DailyBonusClaimButton->SetIsEnabled(bCanClaim && bPlayFabProfileReady && !bDailyBonusRequestInFlight);
    if (DailyBonusClaimLabel)
    {
        const TCHAR* Label = bDailyBonusRequestInFlight ? TEXT("CLAIMING...") : !bPlayFabProfileReady ? TEXT("CONNECTING...") : bCanClaim ? TEXT("CLAIM BONUS") : TEXT("CLAIMED TODAY");
        DailyBonusClaimLabel->SetText(FText::FromString(Label));
    }
}

void USugarLustRootWidget::UpdateReelWidgets()
{
    if(!SlotGame) return;
    const float CellH=190.0f;
    TArray<int32, TInlineAllocator<FSugarLustSlotMachine::PaylineCount>> WinningLines;
    if (!SlotGame->IsSpinning())
    {
        for (int32 Line = 0; Line < SlotGame->GetActiveLines(); ++Line)
            if (SlotGame->IsWinningPayline(Line)) WinningLines.Add(Line);
    }
    const int32 DisplayedWinningLine = WinningLines.Num() > 0
        ? WinningLines[FMath::FloorToInt(GrapeWinAnimationSeconds / 1.1f) % WinningLines.Num()]
        : INDEX_NONE;
    const UTexture2D* Wild=LoadArt(TEXT("/Game/Art/wild.wild")); const UTexture2D* Scatter=LoadArt(TEXT("/Game/Art/scatter.scatter")); const UTexture2D* Chocolate=LoadArt(TEXT("/Game/Art/chocolate.chocolate")); const UTexture2D* Cupcake=LoadArt(TEXT("/Game/Art/cupcake.cupcake")); const UTexture2D* Cupcake2=LoadArt(TEXT("/Game/Art/cupcake2.cupcake2")); const UTexture2D* LollipopGirl=LoadArt(TEXT("/Game/Art/lollipop.lollipop")); const UTexture2D* LollipopCandy=LoadArt(TEXT("/Game/Art/lollipop_candy.lollipop_candy")); const UTexture2D* GummyBear=LoadArt(TEXT("/Game/Art/gummybear.gummybear")); const UTexture2D* GrapeSheet=LoadArt(TEXT("/Game/Art/grape_spritesheet.grape_spritesheet"));
    const UTexture2D* Cherry=LoadArt(TEXT("/Game/Art/cherry.cherry")); const UTexture2D* CandyCane=LoadArt(TEXT("/Game/Art/candycane.candycane")); const UTexture2D* Macaron=LoadArt(TEXT("/Game/Art/macron.macron"));
    const UTexture2D* Ten=LoadArt(TEXT("/Game/Art/candy_10.candy_10")); const UTexture2D* Jack=LoadArt(TEXT("/Game/Art/candy_j.candy_j")); const UTexture2D* Queen=LoadArt(TEXT("/Game/Art/candy_q.candy_q")); const UTexture2D* King=LoadArt(TEXT("/Game/Art/candy_k.candy_k")); const UTexture2D* Ace=LoadArt(TEXT("/Game/Art/candy_a.candy_a"));
    for(int32 Reel=0;Reel<FSugarLustSlotMachine::ReelCount;++Reel)
    {
        const float Fraction=SlotGame->GetReelScrollFraction(Reel);
        for(int32 TileRow=0;TileRow<4;++TileRow)
        {
            const int32 Index=Reel*4+TileRow; if(!ReelTileSlots.IsValidIndex(Index)) continue;
            ReelTileSlots[Index]->SetPosition(FVector2D(0,(TileRow-1-Fraction)*CellH));
            const ESugarLustSymbol Symbol=SlotGame->GetSymbolAt(Reel,TileRow-1);
            const bool bSymbolChanged = ReelSymbolCache[Index] != static_cast<uint8>(Symbol);
            if (bSymbolChanged)
            {
                ReelSymbolCache[Index] = static_cast<uint8>(Symbol);
                ReelMarks[Index]->SetText(FText::FromString(FSugarLustSlotMachine::GetSymbolMark(Symbol)));
                UTexture2D* Texture = Symbol == ESugarLustSymbol::Wild ? const_cast<UTexture2D*>(Wild) :
                    Symbol == ESugarLustSymbol::Scatter ? const_cast<UTexture2D*>(Scatter) :
                    Symbol == ESugarLustSymbol::Chocolate ? const_cast<UTexture2D*>(Chocolate) :
                    Symbol == ESugarLustSymbol::Cupcake ? const_cast<UTexture2D*>(Cupcake) :
                    Symbol == ESugarLustSymbol::Cupcake2 ? const_cast<UTexture2D*>(Cupcake2) :
                    Symbol == ESugarLustSymbol::Lollipop ? const_cast<UTexture2D*>(LollipopGirl) :
                    Symbol == ESugarLustSymbol::LollipopCandy ? const_cast<UTexture2D*>(LollipopCandy) :
                    Symbol == ESugarLustSymbol::GummyBear ? const_cast<UTexture2D*>(GummyBear) :
                    Symbol == ESugarLustSymbol::Grape ? const_cast<UTexture2D*>(GrapeSheet) :
                    Symbol == ESugarLustSymbol::Cherry ? const_cast<UTexture2D*>(Cherry) :
                    Symbol == ESugarLustSymbol::CandyCane ? const_cast<UTexture2D*>(CandyCane) :
                    Symbol == ESugarLustSymbol::Macaron ? const_cast<UTexture2D*>(Macaron) :
                    Symbol == ESugarLustSymbol::Ten ? const_cast<UTexture2D*>(Ten) :
                    Symbol == ESugarLustSymbol::Jack ? const_cast<UTexture2D*>(Jack) :
                    Symbol == ESugarLustSymbol::Queen ? const_cast<UTexture2D*>(Queen) :
                    Symbol == ESugarLustSymbol::King ? const_cast<UTexture2D*>(King) :
                    Symbol == ESugarLustSymbol::Ace ? const_cast<UTexture2D*>(Ace) : nullptr;
                if (Texture)
                {
                    ReelArt[Index]->SetBrushFromTexture(Texture, true);
                    // Grape uses a cropped atlas region. Reset the UVs whenever a
                    // tile changes to another textured symbol or that crop would
                    // carry over and zoom the Wild/Scatter artwork.
                    if (Symbol != ESugarLustSymbol::Grape)
                    {
                        FSlateBrush FullTextureBrush = ReelArt[Index]->GetBrush();
                        FullTextureBrush.SetUVRegion(FBox2f(FVector2f(0.0f, 0.0f), FVector2f(1.0f, 1.0f)));
                        ReelArt[Index]->SetBrush(FullTextureBrush);
                    }
                    ReelArt[Index]->SetColorAndOpacity(FLinearColor::White);
                    ReelArt[Index]->SetVisibility(ESlateVisibility::Visible);
                    ReelMarks[Index]->SetVisibility(ESlateVisibility::Collapsed);
                }
                else
                {
                    ReelArt[Index]->SetVisibility(ESlateVisibility::Collapsed);
                    ReelMarks[Index]->SetVisibility(ESlateVisibility::Visible);
                }
            }

            // The atlas is a 6-column by 5-row sheet. Use its first frame at rest,
            // and loop through the 26 populated frames while it participates in a win.
            if (Symbol == ESugarLustSymbol::Grape && GrapeSheet && ReelArt[Index]->IsVisible())
            {
                const bool bWinningGrape = !SlotGame->IsSpinning() && SlotGame->IsWinHighlight(Reel, TileRow - 1);
                // Keep the 26-frame win loop readable; 5 fps gives one full
                // animation cycle a little over five seconds instead of racing by.
                // Keep the 26-frame win animation deliberately relaxed: at 2.5 FPS
                // one full loop takes just over ten seconds instead of rushing by.
                const int32 Frame = bWinningGrape ? FMath::FloorToInt(GrapeWinAnimationSeconds * 2.5f) % 26 : 0;
                const int32 Row = Frame < 6 ? 0 : 1 + (Frame - 6) / 5;
                const int32 Column = Frame < 6 ? Frame : (Frame - 6) % 5;
                constexpr float InsetX = 2.0f / 6490.0f;
                constexpr float InsetY = 2.0f / 5408.0f;
                const FVector2f UVMin(Column / 6.0f + InsetX, Row / 5.0f + InsetY);
                const FVector2f UVMax((Column + 1) / 6.0f - InsetX, (Row + 1) / 5.0f - InsetY);
                FSlateBrush Brush = ReelArt[Index]->GetBrush();
                Brush.SetUVRegion(FBox2f(UVMin, UVMax));
                ReelArt[Index]->SetBrush(Brush);
            }

            const int32 HighlightRow = DisplayedWinningLine != INDEX_NONE && Reel < SlotGame->GetWinningPaylineLength(DisplayedWinningLine)
                ? SlotGame->GetPaylineRow(DisplayedWinningLine, Reel)
                : INDEX_NONE;
            const bool bHighlighted = !SlotGame->IsSpinning() &&
                (HighlightRow == TileRow - 1 || ((Symbol == ESugarLustSymbol::Scatter || Symbol >= ESugarLustSymbol::FreeSpins) && SlotGame->IsWinHighlight(Reel, TileRow - 1)));
            const float Pulse = bHighlighted ? (0.72f + 0.28f * FMath::Sin(GrapeWinAnimationSeconds * 8.0f)) : 1.0f;
            ReelArt[Index]->SetColorAndOpacity(bHighlighted
                ? FLinearColor(1.0f, 0.88f + 0.12f * Pulse, 0.38f + 0.62f * Pulse, 1.0f)
                : FLinearColor::White);
            ReelMarks[Index]->SetColorAndOpacity(FSlateColor(bHighlighted ? FLinearColor(1.0f, 0.95f, 0.45f, 1.0f) : Symbol >= ESugarLustSymbol::JackpotMini ? FSugarLustSlotMachine::GetSymbolAccent(Symbol) : SugarLustStyle::Gold));
        }
    }

    for (int32 Line = 0; Line < FSugarLustSlotMachine::PaylineCount; ++Line)
    {
        const int32 WinningLength = DisplayedWinningLine == Line
            ? SlotGame->GetWinningPaylineLength(Line)
            : 0;
        for (int32 Segment = 0; Segment < FSugarLustSlotMachine::ReelCount - 1; ++Segment)
        {
            const int32 Index = Line * (FSugarLustSlotMachine::ReelCount - 1) + Segment;
            if (WinLineSegments.IsValidIndex(Index))
            {
                // A payline ends at its final matching symbol. Do not draw a
                // misleading tail into the first nonmatching reel.
                const bool bVisible = Line == DisplayedWinningLine && Segment < WinningLength - 1;
                WinLineSegments[Index]->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
            }
        }
    }
}

void USugarLustRootWidget::UpdateSlotUI()
{
    if(!SlotGame) return;
    UpdateFreeSpinsUI();
    auto Number=[](int64 Value){return FText::AsNumber(Value).ToString();};
    if(CoinsText) CoinsText->SetText(FText::FromString(Number(SlotGame->GetCoins())));
    if(BetText) BetText->SetText(FText::FromString(FString::Printf(TEXT("BET / LINE  %s"),*Number(SlotGame->GetBetPerLine()))));
    if(LinesText) LinesText->SetText(FText::AsNumber(SlotGame->GetActiveLines()));
    if(TotalBetText) TotalBetText->SetText(FText::FromString(Number(SlotGame->GetTotalBet())));
    if(WinText) WinText->SetText(FText::FromString(Number(SlotGame->GetLastWin())));
    if(AutoSpinStatusText)
    {
        const FString AutoMessage = AutoSpinsRemaining > 0
            ? FString::Printf(TEXT("AUTO SPIN: %d LEFT | CLICK TO STOP"), AutoSpinsRemaining)
            : bAutoSpinWaitingForAnimation ? TEXT("STOPPING AFTER THIS SPIN") : TEXT("");
        AutoSpinStatusText->SetText(FText::FromString(AutoMessage));
        AutoSpinStatusText->SetVisibility(AutoMessage.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
    if(SpinStatus)
    {
        if(!SugarSpinError.IsEmpty()) SpinStatus->SetText(FText::FromString(SugarSpinError));
        else if(AutoSpinsRemaining>0) SpinStatus->SetText(FText::FromString(FString::Printf(TEXT("AUTO SPIN  |  %d LEFT  |  TAP TO STOP"),AutoSpinsRemaining)));
        else if(bAutoSpinWaitingForAnimation) SpinStatus->SetText(FText::FromString(TEXT("STOPPING AFTER THIS SPIN")));
        else if(bSpinRequestRetrying) SpinStatus->SetText(FText::FromString(TEXT("RECONNECTING TO THE CASINO...")));
        else if(SlotGame->IsSpinning()) SpinStatus->SetText(FText::FromString(TEXT("REELS SPINNING...")));
        else if(SlotGame->IsBonusRoundActive()) SpinStatus->SetText(FText::FromString(TEXT("SCATTERS! PICK THREE CANDY JARS")));
        else if(SlotGame->IsBonusResultReady()) SpinStatus->SetText(FText::FromString(TEXT("BONUS COMPLETE - COLLECT YOUR WIN")));
        else SpinStatus->SetText(FText::GetEmpty());
        const bool bShowStatus = bSpinRequestRetrying || !SugarSpinError.IsEmpty() || AutoSpinsRemaining>0 || bAutoSpinWaitingForAnimation || SlotGame->IsSpinning() ||
            SlotGame->IsBonusRoundActive() || SlotGame->IsBonusResultReady();
        SpinStatus->SetVisibility(bShowStatus ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if(SpinButton) SpinButton->SetIsEnabled(bPlayFabProfileReady &&
        (SugarFreeSpins.Remaining>0 || AutoSpinsRemaining>0 || (!bPlayFabRequestInFlight && SlotGame->CanSpin())));
    if (SpinButtonArt)
    {
        UTexture2D* SpinStateTexture = (AutoSpinsRemaining > 0 || bAutoSpinWaitingForAnimation)
            ? CandySpinAutoTexture
            : bSpinButtonHeld ? CandySpinPressedTexture : CandySpinNormalTexture;
        SetCandyButtonImage(SpinButtonArt, SpinStateTexture);
    }
    const bool bCanChangeWager = PendingPlayFabSpinId.IsEmpty() && !bPlayFabRequestInFlight && !SlotGame->IsSpinning() &&
        !SlotGame->IsBonusRoundActive() && !SlotGame->IsBonusResultReady() && SugarFreeSpins.Remaining == 0;
    if(MaxBetButton) MaxBetButton->SetIsEnabled(bCanChangeWager);
    if(AutoSpinButton) AutoSpinButton->SetIsEnabled(bPlayFabProfileReady &&
        (AutoSpinsRemaining > 0 || (!bPlayFabRequestInFlight && !SlotGame->IsSpinning() &&
            !SlotGame->IsBonusRoundActive() && !SlotGame->IsBonusResultReady() && SlotGame->CanSpin())));
    if(BetMinusButton) BetMinusButton->SetIsEnabled(bCanChangeWager);
    if(BetPlusButton) BetPlusButton->SetIsEnabled(bCanChangeWager);
    if(LinesBadgeButton) LinesBadgeButton->SetIsEnabled(bCanChangeWager);
    if(LinesMinusButton) LinesMinusButton->SetIsEnabled(bCanChangeWager);
    if(LinesPlusButton) LinesPlusButton->SetIsEnabled(bCanChangeWager);
    UpdateReelWidgets(); UpdateBonusUI(); UpdatePlayerProfileUI();
}

void USugarLustRootWidget::ApplyJackpotData(const TSharedPtr<FJsonObject>& Profile)
{
    if (!Profile.IsValid()) return;
    const TSharedPtr<FJsonObject>* Jackpots = nullptr;
    if (!Profile->TryGetObjectField(TEXT("Jackpots"), Jackpots) || !Jackpots || !Jackpots->IsValid()) return;

    double Mini = 0.0, Minor = 0.0, Major = 0.0, Grand = 0.0;
    if (!(*Jackpots)->TryGetNumberField(TEXT("Mini"), Mini) ||
        !(*Jackpots)->TryGetNumberField(TEXT("Minor"), Minor) ||
        !(*Jackpots)->TryGetNumberField(TEXT("Major"), Major) ||
        !(*Jackpots)->TryGetNumberField(TEXT("Grand"), Grand))
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayFab jackpot response was incomplete; keeping the last displayed meters."));
        return;
    }

    JackpotMini = static_cast<int64>(Mini);
    JackpotMinor = static_cast<int64>(Minor);
    JackpotMajor = static_cast<int64>(Major);
    JackpotGrand = static_cast<int64>(Grand);
    UpdateJackpotUI();
}


void USugarLustRootWidget::ApplyFreeSpinData(const TSharedPtr<FJsonObject>& Profile)
{
    if (!Profile.IsValid()) return;
    const TSharedPtr<FJsonObject>* Features = nullptr;
    if (!Profile->TryGetObjectField(TEXT("FreeSpins"), Features) || !Features || !Features->IsValid()) return;
    auto Read = [this, Features](const TCHAR* Game, FFreeSpinStatus& State, bool bSugar)
    {
        const TSharedPtr<FJsonObject>* Data = nullptr;
        if (!(*Features)->TryGetObjectField(Game, Data) || !Data || !Data->IsValid()) return;
        double Remaining = 0, Awarded = 0, LastAward = 0, Win = 0, Bet = 0, Lines = 0;
        if (!(*Data)->TryGetNumberField(TEXT("Remaining"), Remaining) ||
            !(*Data)->TryGetNumberField(TEXT("TotalAwarded"), Awarded) ||
            !(*Data)->TryGetNumberField(TEXT("TotalWin"), Win) ||
            !(*Data)->TryGetNumberField(TEXT("BetPerLine"), Bet) ||
            !(*Data)->TryGetNumberField(TEXT("ActiveLines"), Lines)) return;
        (*Data)->TryGetNumberField(TEXT("LastAward"), LastAward);
        (*Data)->TryGetBoolField(TEXT("LastWasFreeSpin"), State.bLastWasFreeSpin);
        State.Remaining = FMath::Clamp(static_cast<int32>(Remaining), 0, 100);
        State.TotalAwarded = FMath::Max(0, static_cast<int32>(Awarded));
        State.LastAward = FMath::Max(0, static_cast<int32>(LastAward));
        State.TotalWin = FMath::Max<int64>(0, static_cast<int64>(Win));
        if (bSugar)
        {
            if (SlotGame) SlotGame->ApplyFreeSpinsState(State.Remaining, static_cast<int32>(Bet), static_cast<int32>(Lines));
            if (State.Remaining > 0 || State.bLastWasFreeSpin) AutoSpinsRemaining = 0;
        }
        else
        {
            if (State.Remaining > 0) NeonBet = FMath::Clamp(static_cast<int32>(Bet), 100, 10000);
            if (State.Remaining > 0 || State.bLastWasFreeSpin) NeonAutoSpinsRemaining = 0;
        }
    };
    Read(TEXT("SugarLust"), SugarFreeSpins, true);
    Read(TEXT("NeonNights"), NeonFreeSpins, false);
}

void USugarLustRootWidget::UpdateFreeSpinsUI()
{
    auto Display = [](UTextBlock* Label, const FFreeSpinStatus& State, bool bSpinning, bool bHeatPause)
    {
        if (!Label) return;
        if (State.TotalAwarded == 0 || (bSpinning && !bHeatPause && !State.bPaused))
        {
            Label->SetVisibility(ESlateVisibility::Collapsed);
            return;
        }
        const FString Text = bHeatPause
            ? FString::Printf(TEXT("FREE SPINS PAUSED FOR NEON HEAT  |  %d LEFT"), State.Remaining)
            : State.Remaining > 0
            ? FString::Printf(TEXT("FREE SPINS: %d LEFT  |  FEATURE WIN: %s%s"), State.Remaining,
                *FText::AsNumber(State.TotalWin).ToString(), State.bPaused ? TEXT("  |  TAP SPIN TO RESUME") : TEXT("  |  TAP SPIN TO PAUSE"))
            : FString::Printf(TEXT("FREE SPINS COMPLETE  |  TOTAL WIN: %s"), *FText::AsNumber(State.TotalWin).ToString());
        Label->SetText(FText::FromString(Text));
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
    };
    // Only touch widgets belonging to the current tree after screen changes.
    if (bShowingSlot) Display(SugarFreeSpinsText, SugarFreeSpins, SlotGame && SlotGame->IsSpinning(), false);
    if (bShowingNeonNightsPreview) Display(NeonFreeSpinsText, NeonFreeSpins, bNeonSpinAnimating, bNeonHeatBonusActive || bNeonHeatBonusComplete);
}

void USugarLustRootWidget::ApplyCloudMissionData(const TSharedPtr<FJsonObject>& Profile)
{
    ApplyFreeSpinData(Profile);
    if (!SlotGame || !Profile.IsValid()) return;

    const TArray<TSharedPtr<FJsonValue>>* MissionArray = nullptr;
    if (!Profile->TryGetArrayField(TEXT("Missions"), MissionArray) || !MissionArray || MissionArray->Num() != 5)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayFab profile did not include the authoritative daily missions."));
        return;
    }

    TArray<FSugarLustSlotMachine::FMission> ServerMissions;
    ServerMissions.Reserve(MissionArray->Num());
    for (const TSharedPtr<FJsonValue>& Value : *MissionArray)
    {
        const TSharedPtr<FJsonObject> MissionObject = Value.IsValid() ? Value->AsObject() : nullptr;
        if (!MissionObject.IsValid()) return;

        FSugarLustSlotMachine::FMission Mission;
        double Target = 0.0, Progress = 0.0, Reward = 0.0;
        if (!MissionObject->TryGetStringField(TEXT("Id"), Mission.Id) ||
            !MissionObject->TryGetStringField(TEXT("Description"), Mission.Description) ||
            !MissionObject->TryGetNumberField(TEXT("Target"), Target) ||
            !MissionObject->TryGetNumberField(TEXT("Progress"), Progress) ||
            !MissionObject->TryGetNumberField(TEXT("RewardCoins"), Reward) ||
            !MissionObject->TryGetBoolField(TEXT("Claimed"), Mission.bClaimed))
        {
            UE_LOG(LogTemp, Warning, TEXT("PlayFab mission data was malformed; keeping the last known mission state."));
            return;
        }

        Mission.Target = FMath::Max(1, static_cast<int32>(Target));
        Mission.Progress = FMath::Clamp(static_cast<int32>(Progress), 0, Mission.Target);
        Mission.RewardCoins = FMath::Max<int64>(0, static_cast<int64>(Reward));
        ServerMissions.Add(MoveTemp(Mission));
    }

    FString ResetDate;
    Profile->TryGetStringField(TEXT("MissionsDate"), ResetDate);
    SlotGame->ApplyCloudMissions(ServerMissions, ResetDate);
    bPlayFabMissionsReady = true;
}

void USugarLustRootWidget::UpdateJackpotUI()
{
    if (JackpotMiniText) JackpotMiniText->SetText(FText::AsNumber(JackpotMini));
    if (JackpotMinorText) JackpotMinorText->SetText(FText::AsNumber(JackpotMinor));
    if (JackpotMajorText) JackpotMajorText->SetText(FText::AsNumber(JackpotMajor));
    if (JackpotGrandText) JackpotGrandText->SetText(FText::AsNumber(JackpotGrand));
}

void USugarLustRootWidget::UpdatePlayerProfileUI()
{
    if(!SlotGame) return;
    const int32 Level=SlotGame->GetLevel();
    if (bShowingSlot)
    {
        if(SlotXpText)
        {
            const FString Progress=Level>=200?FString::Printf(TEXT("LEVEL %d\nMAX LEVEL"),Level):FString::Printf(TEXT("LEVEL %d\n%s / %s XP"),Level,*FText::AsNumber(SlotGame->GetXpInLevel()).ToString(),*FText::AsNumber(SlotGame->GetXpNeededInLevel()).ToString());
            SlotXpText->SetText(FText::FromString(Progress));
        }
        return;
    }
    if(PlayerLevelText)
    {
        FString DisplayName = PlayerDisplayName;
        DisplayName.TrimStartAndEndInline();
        if (DisplayName.IsEmpty()) DisplayName = TEXT("Guest");
        if (DisplayName.Len() > 20)
        {
            DisplayName.LeftInline(19);
            DisplayName += TEXT("...");
        }
        PlayerLevelText->SetText(FText::FromString(DisplayName));
    }
    if(PlayerBadgeLevelText) PlayerBadgeLevelText->SetText(FText::AsNumber(Level));
    if(PlayerXpText)
    {
        const FString Progress=Level>=200?TEXT("MAX LEVEL"):FString::Printf(TEXT("%s / %s XP"),*FText::AsNumber(SlotGame->GetXpInLevel()).ToString(),*FText::AsNumber(SlotGame->GetXpNeededInLevel()).ToString());
        PlayerXpText->SetText(FText::FromString(Progress));
    }
    if(PlayerXpBar) PlayerXpBar->SetPercent(SlotGame->GetLevelProgress());
    if(LobbyCoinsText) LobbyCoinsText->SetText(FText::AsNumber(SlotGame->GetCoins()));
    if(LobbyGemsText) LobbyGemsText->SetText(LobbyGemBalance>=0 ? FText::AsNumber(LobbyGemBalance) : FText::FromString(TEXT("--")));
    UpdateLobbyHostGreeting();
}

void USugarLustRootWidget::PickBonusCard(int32 Card)
{
    if (!SlotGame || !SlotGame->IsBonusRoundActive() || bBonusPickRequestInFlight ||
        Card < 0 || Card >= BonusCardButtons.Num() || SlotGame->IsBonusCardRevealed(Card)) return;
    BonusJarAnimatingCard = Card;
    BonusJarAnimationSeconds = 0.0f;
    RequestPlayFabBonusPick(Card);
}

void USugarLustRootWidget::UpdateBonusUI()
{
    if(!SlotGame || !BonusCanvas) return;
    const bool bVisible=SlotGame->IsBonusRoundActive()||SlotGame->IsBonusResultReady();
    if (!bVisible)
    {
        bBonusWasVisible = false;
        BonusTransitionSeconds = 0.0f;
        BonusJarAnimatingCard = INDEX_NONE;
        BonusJarAnimationSeconds = 0.0f;
        BonusCanvas->SetVisibility(ESlateVisibility::Collapsed);
        if (BonusTransitionCanvas) BonusTransitionCanvas->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    if (!bBonusWasVisible)
    {
        bBonusWasVisible = true;
        // A completed bonus restored from the server can go straight to its
        // result; active rounds get the scatter reveal beat.
        BonusTransitionSeconds = SlotGame->IsBonusRoundActive() ? 1.65f : 0.0f;
    }
    const bool bTransitioning = BonusTransitionSeconds > 0.0f && SlotGame->IsBonusRoundActive();
    BonusCanvas->SetVisibility(bTransitioning ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (BonusTransitionCanvas)
    {
        BonusTransitionCanvas->SetVisibility(bTransitioning ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bTransitioning)
        {
            BonusTransitionSubtitle->SetText(FText::FromString(FString::Printf(TEXT("%d SCATTERS - BONUS ROUND UNLOCKED"), SlotGame->GetBonusScatterCount())));
            const float Pulse = 0.88f + 0.12f * FMath::Sin(GetWorld() ? GetWorld()->GetTimeSeconds() * 8.0f : 0.0f);
            BonusTransitionCanvas->SetRenderOpacity(Pulse);
        }
    }
    if (bTransitioning) return;
    const bool bDone=SlotGame->IsBonusResultReady();
    BonusHeaderText->SetText(FText::FromString(bDone?TEXT("BONUS WIN COLLECTED!"):TEXT("SUGAR RUSH BONUS!")));
    BonusPromptText->SetText(FText::FromString(bDone?TEXT("Your three candy jar prizes have been added to your balance"):TEXT("Choose 3 candy jars to collect their multipliers")));
    BonusPicksText->SetText(FText::FromString(FString::Printf(TEXT("PICKS LEFT: %d"),SlotGame->GetBonusPicksRemaining())));
    BonusTotalText->SetText(FText::FromString(FString::Printf(TEXT("BONUS WIN: %s"),*FText::AsNumber(SlotGame->GetBonusRoundWin()).ToString())));
    for(int32 I=0;I<BonusCardButtons.Num();++I)
    {
        const bool bRevealed=SlotGame->IsBonusCardRevealed(I); const bool bCanPick=SlotGame->IsBonusRoundActive()&&bPlayFabProfileReady&&!bRevealed&&!bBonusPickRequestInFlight;
        BonusCardButtons[I]->SetIsEnabled(bCanPick);
        const bool bAnimating=BonusJarAnimatingCard==I;
        const int32 Frame=bAnimating
            ? FMath::Clamp(5+FMath::FloorToInt(FMath::Clamp(BonusJarAnimationSeconds/1.45f,0.0f,0.999f)*20.0f),5,24)
            : (bRevealed?24:0);
        if(BonusCardImages.IsValidIndex(I) && BonusJarSpriteSheet)
        {
            const int32 Column=Frame%5;
            const int32 Row=Frame/5;
            constexpr float Inset=1.5f/1280.0f;
            FSlateBrush JarBrush=BonusCardImages[I]->GetBrush();
            JarBrush.SetUVRegion(FBox2f(
                FVector2f(Column/5.0f+Inset,Row/5.0f+Inset),
                FVector2f((Column+1)/5.0f-Inset,(Row+1)/5.0f-Inset)));
            BonusCardImages[I]->SetBrush(JarBrush);
            BonusCardImages[I]->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        if(BonusCardPrizeLabels.IsValidIndex(I))
        {
            BonusCardPrizeLabels[I]->SetText(bRevealed
                ? FText::FromString(FString::Printf(TEXT("x%d"),SlotGame->GetBonusMultiplier(I)))
                : FText::GetEmpty());
            const bool bTagReady=bRevealed && (!bAnimating || BonusJarAnimationSeconds>=1.05f);
            BonusCardPrizeLabels[I]->SetVisibility(bTagReady?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        }
        if(BonusCardLabels.IsValidIndex(I))
        {
            const FString Caption=bRevealed
                ? FString::Printf(TEXT("%s COINS"),*FText::AsNumber(SlotGame->GetBonusPrize(I)).ToString())
                : (bAnimating?FString(TEXT("OPENING...")):FString(TEXT("TAP TO PICK")));
            BonusCardLabels[I]->SetText(FText::FromString(Caption));
        }
    }
    BonusContinueButton->SetVisibility(bDone?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
}
void USugarLustRootWidget::BuildMissionsModal()
{
    if (!DesignCanvas) return;

    MissionsCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    Place(MissionsCanvas, 0, 0, 1920, 1080, 40);
    auto AddToModal = [this](UWidget* Widget, float X, float Y, float Width, float Height, int32 Z)
    {
        if (!MissionsCanvas || !Widget) return;
        UCanvasPanelSlot* Slot = MissionsCanvas->AddChildToCanvas(Widget);
        Slot->SetPosition(FVector2D(X, Y));
        Slot->SetSize(FVector2D(Width, Height));
        Slot->SetZOrder(Z);
    };
    UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Dimmer->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.84f));
    AddToModal(Dimmer, 0, 0, 1920, 1080, 0);

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(SugarLustStyle::Gold);
    Panel->SetPadding(FMargin(4.0f));
    UBorder* Face = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Face->SetBrushColor(FLinearColor(0.025f, 0.004f, 0.04f, 1.0f));
    Face->SetPadding(FMargin(28.0f));
    Panel->SetContent(Face);
    AddToModal(Panel, 480, 150, 960, 740, 1);

    UTextBlock* Title = Cast<UTextBlock>(MakeText(TEXT("DAILY MISSIONS"), 38, SugarLustStyle::Gold, true));
    AddToModal(Title, 560, 175, 800, 52, 2);
    UTextBlock* Intro = Cast<UTextBlock>(MakeText(TEXT("PLAY SUGAR LUST TO COMPLETE TODAY'S CHALLENGES"), 19, SugarLustStyle::White, true));
    AddToModal(Intro, 550, 230, 820, 34, 2);
    GemEarningProgressText = Cast<UTextBlock>(MakeText(*GemEarningSummary, 17, SugarLustStyle::Gold, true));
    AddToModal(GemEarningProgressText, 535, 270, 850, 62, 3);

    MissionLabels.Reset();
    MissionBorders.Reset();
    MissionClaimButtons.Reset();
    const TArray<FSugarLustSlotMachine::FMission> Missions = SlotGame ? SlotGame->GetMissions() : TArray<FSugarLustSlotMachine::FMission>();
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const float Y = 342.0f + Index * 76.0f;
        UBorder* Row = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Row->SetBrushColor(FLinearColor(0.055f, 0.012f, 0.08f, 0.99f));
        AddToModal(Row, 535, Y, 850, 64, 2);
        MissionBorders.Add(Row);

        const FString InitialText = Missions.IsValidIndex(Index) ? Missions[Index].Description : FString();
        UTextBlock* Label = Cast<UTextBlock>(MakeText(InitialText, 17, SugarLustStyle::White, true, HAlign_Left));
        AddToModal(Label, 560, Y + 4.0f, 590, 56, 3);
        MissionLabels.Add(Label);

        UButton* Claim = MakeButton(TEXT("CLAIM"), SugarLustStyle::Pink, 17);
        AddToModal(Claim, 1170, Y + 7.0f, 185, 50, 4);
        switch (Index)
        {
        case 0: Claim->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleClaimMission0); break;
        case 1: Claim->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleClaimMission1); break;
        case 2: Claim->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleClaimMission2); break;
        case 3: Claim->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleClaimMission3); break;
        case 4: Claim->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleClaimMission4); break;
        }
        MissionClaimButtons.Add(Claim);
    }

    MissionsStatusText = Cast<UTextBlock>(MakeText(TEXT("MISSIONS SYNC TO THE SERVER. CLAIMS ARE VERIFIED THERE."),
        15, SugarLustStyle::White, false));
    AddToModal(MissionsStatusText, 535, 730, 850, 34, 3);
    MissionsCloseButton = MakeButton(TEXT("CLOSE"), SugarLustStyle::Gold, 19);
    MissionsCloseButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleCloseMissions);
    AddToModal(MissionsCloseButton, 850, 780, 220, 58, 3);
    MissionsCanvas->SetVisibility(ESlateVisibility::Collapsed);
    UpdateMissionsUI();
}

void USugarLustRootWidget::BuildSettingsModal()
{
    if (!DesignCanvas) return;
    SettingsCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    Place(SettingsCanvas, 0, 0, 1920, 1080, 45);
    auto AddToModal = [this](UWidget* Widget, float X, float Y, float Width, float Height, int32 Z)
    {
        if (!SettingsCanvas || !Widget) return;
        UCanvasPanelSlot* Slot = SettingsCanvas->AddChildToCanvas(Widget);
        Slot->SetPosition(FVector2D(X, Y));
        Slot->SetSize(FVector2D(Width, Height));
        Slot->SetZOrder(Z);
    };
    UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Dimmer->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.84f));
    AddToModal(Dimmer, 0, 0, 1920, 1080, 0);

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(SugarLustStyle::Gold);
    Panel->SetPadding(FMargin(4.0f));
    UBorder* Face = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Face->SetBrushColor(FLinearColor(0.025f, 0.004f, 0.04f, 1.0f));
    Panel->SetContent(Face);
    AddToModal(Panel, 610, 300, 700, 480, 1);

    AddToModal(MakeText(TEXT("SETTINGS"), 38, SugarLustStyle::Gold, true), 660, 330, 600, 60, 2);
    SettingsMusicButton = MakeButton(TEXT("MUSIC"), SugarLustStyle::Pink, 21);
    SettingsMusicButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleToggleMusic);
    AddToModal(SettingsMusicButton, 720, 425, 480, 72, 2);
    if (UBorder* FaceBorder = Cast<UBorder>(SettingsMusicButton->GetContent()))
        if (UBorder* FillBorder = Cast<UBorder>(FaceBorder->GetContent()))
            SettingsMusicLabel = Cast<UTextBlock>(FillBorder->GetContent());
    SettingsSfxButton = MakeButton(TEXT("SOUND EFFECTS"), SugarLustStyle::Pink, 21);
    SettingsSfxButton->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleToggleSfx);
    AddToModal(SettingsSfxButton, 720, 525, 480, 72, 2);
    if (UBorder* FaceBorder = Cast<UBorder>(SettingsSfxButton->GetContent()))
        if (UBorder* FillBorder = Cast<UBorder>(FaceBorder->GetContent()))
            SettingsSfxLabel = Cast<UTextBlock>(FillBorder->GetContent());
    UButton* Close = MakeButton(TEXT("CLOSE"), SugarLustStyle::Gold, 19);
    Close->OnClicked.AddDynamic(this, &USugarLustRootWidget::HandleCloseSettings);
    AddToModal(Close, 840, 655, 240, 62, 2);
    SettingsCanvas->SetVisibility(ESlateVisibility::Collapsed);
    UpdateSettingsUI();
}

void USugarLustRootWidget::UpdateSettingsUI()
{
    if (SettingsMusicLabel)
        SettingsMusicLabel->SetText(FText::FromString(bMusicEnabled ? TEXT("MUSIC: ON") : TEXT("MUSIC: OFF")));
    if (SettingsSfxLabel)
        SettingsSfxLabel->SetText(FText::FromString(bSfxEnabled ? TEXT("SOUND EFFECTS: ON") : TEXT("SOUND EFFECTS: OFF")));
}

void USugarLustRootWidget::UpdateMissionsUI()
{
    if (!SlotGame || !MissionsCanvas) return;
    SlotGame->InitMissionsIfNeeded();
    TArray<FSugarLustSlotMachine::FMission> Missions = SlotGame->GetMissions();
    for (int32 i = 0; i < MissionLabels.Num() && i < Missions.Num(); ++i)
    {
        const auto& M = Missions[i];
        FString L = FString::Printf(TEXT("%s  [%d/%d]  +%lld COINS"), *M.Description, M.Progress, M.Target, M.RewardCoins);
        if (MissionLabels[i]) MissionLabels[i]->SetText(FText::FromString(L));
        if (i < MissionBorders.Num() && MissionBorders[i])
        {
            MissionBorders[i]->SetBrushColor(M.bClaimed
                ? FLinearColor(0.04f, 0.22f, 0.11f, 1.0f)
                : (M.Progress >= M.Target
                    ? FLinearColor(0.28f, 0.16f, 0.035f, 1.0f)
                    : FLinearColor(0.055f, 0.012f, 0.08f, 1.0f)));
        }
        if (i < MissionClaimButtons.Num() && MissionClaimButtons[i])
        {
            MissionClaimButtons[i]->SetIsEnabled(bPlayFabProfileReady && bPlayFabMissionsReady && !bMissionClaimRequestInFlight &&
                !M.bClaimed && M.Progress >= M.Target);
        }
    }
    if (MissionsStatusText && !bMissionClaimRequestInFlight)
    {
        const FString CurrentStatus = MissionsStatusText->GetText().ToString();
        const bool bStatusIsConnectionHint = CurrentStatus.StartsWith(TEXT("MISSIONS SYNC")) ||
            CurrentStatus.StartsWith(TEXT("CONNECT TO PLAYFAB")) ||
            CurrentStatus.StartsWith(TEXT("UPDATE PLAYFAB CLOUDSCRIPT"));
        if (bStatusIsConnectionHint)
        {
            const TCHAR* Status = !bPlayFabProfileReady
                ? TEXT("CONNECT TO PLAYFAB TO CLAIM SERVER-VERIFIED COINS.")
                : !bPlayFabMissionsReady
                    ? TEXT("UPDATE PLAYFAB CLOUDSCRIPT TO ENABLE SERVER CLAIMS.")
                    : TEXT("MISSIONS SYNC TO THE SERVER. CLAIMS ARE VERIFIED THERE.");
            MissionsStatusText->SetText(FText::FromString(Status));
        }
    }
}


void USugarLustRootWidget::HandleEnter3DLounge()
{
    if (!Lounge3D) Lounge3D = GetWorld()->SpawnActor<ALustHouseLounge>();
    if (!Lounge3D) return;
    Lounge3D->SetActorHiddenInGame(false);
    Lounge3D->SetActorTickEnabled(true);
    Lounge3D->SetView(0);
    if (auto* PC = GetOwningPlayer()) PC->SetViewTarget(Lounge3D);
    ClearScreenWidgets();
    WideBackgroundImage->SetVisibility(ESlateVisibility::Collapsed);
    Place(MakeText(TEXT("LUST HOUSE  /  3D LOUNGE PREVIEW"), 28, SugarLustStyle::White, true), 60, 40, 1200, 60, 1);
    Place(MakeText(TEXT("Choose a view, then open a game or rewards."), 20, SugarLustStyle::White), 60, 105, 1400, 45, 1);
    const TCHAR* Labels[] = {TEXT("ENTRANCE"),TEXT("SLOTS"),TEXT("LUSTVEND"),TEXT("STAGE"),TEXT("PLAY SUGAR LUST"),TEXT("OPEN REWARDS"),TEXT("BACK TO LOBBY")};
    for (int32 I=0; I<7; ++I)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>();
        Button->SetBackgroundColor(SugarLustStyle::Panel);
        Button->AddChild(MakeText(Labels[I], 18, SugarLustStyle::White, true));
        switch(I)
        {
        case 0: Button->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleLoungeOverview); break;
        case 1: Button->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleLoungeSlots); break;
        case 2: Button->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleLoungeVend); break;
        case 3: Button->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleLoungeStage); break;
        case 4: Button->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleOpenSlot); break;
        case 5: Button->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleGemsPlusButton); break;
        case 6: Button->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleExit3DLounge); break;
        }
        Place(Button, 40+I*265, 950, 250, 80, 2);
    }
}
void USugarLustRootWidget::HandleLoungeOverview() { if(Lounge3D) Lounge3D->SetView(0); }
void USugarLustRootWidget::HandleLoungeSlots() { if(Lounge3D) Lounge3D->SetView(1); }
void USugarLustRootWidget::HandleLoungeVend() { if(Lounge3D) Lounge3D->SetView(2); }
void USugarLustRootWidget::HandleLoungeStage() { if(Lounge3D) Lounge3D->SetView(3); }
void USugarLustRootWidget::HandleExit3DLounge() { BuildLobby(); }

void USugarLustRootWidget::ClearScreenWidgets()
{
    // Invalidate every screen-owned observer before releasing the canvas children.
    // Network callbacks continue updating account data while the lounge is visible.
    GemEarningProgressText = nullptr;
    BonusCanvas = nullptr;
    BonusTransitionCanvas = nullptr;
    BonusTransitionTitle = nullptr;
    BonusTransitionSubtitle = nullptr;
    BigWinCanvas = nullptr;
    BigWinBackdropImage = nullptr;
    BigWinAmountCover = nullptr;
    BigWinFallbackPanel = nullptr;
    DailyBonusCanvas = nullptr;
    LobbyNoticePanel = nullptr;
    LobbyNoticeText = nullptr;
    LobbyHostGreetingPanel = nullptr;
    LobbyHostGreetingText = nullptr;
    SugarFreeSpinsText = nullptr;
    NeonFreeSpinsText = nullptr;
    ProgressiveJackpotCanvas = nullptr;
    NeonCoinsText = nullptr;
    NeonXpText = nullptr;
    NeonBetText = nullptr;
    NeonTotalBetText = nullptr;
    NeonWinText = nullptr;
    NeonStatusText = nullptr;
    NeonSpinButton = nullptr;
    NeonBetDownButton = nullptr;
    NeonBetUpButton = nullptr;
    NeonMaxBetButton = nullptr;
    NeonAutoSpinButton = nullptr;
    BigWinCharacterLeft = nullptr;
    BigWinCharacterRight = nullptr;
    BigWinTitleText = nullptr;
    BigWinAmountText = nullptr;
    BigWinCoinsLabel = nullptr;
    DailyBonusButton = nullptr;
    DailyBonusClaimButton = nullptr;
    DailyBonusCloseButton = nullptr;
    DisclaimerContinueButton = nullptr;
    DailyBonusStatusText = nullptr;
    DailyBonusMessageText = nullptr;
    DailyBonusClaimLabel = nullptr;
    SpinButton = nullptr;
    AutoSpinButton = nullptr;
    MaxBetButton = nullptr;
    BetMinusButton = nullptr;
    BetPlusButton = nullptr;
    BetMinusArt = nullptr;
    BetPlusArt = nullptr;
    SpinButtonArt = nullptr;
    LinesBadgeButton = nullptr;
    LinesMinusButton = nullptr;
    LinesPlusButton = nullptr;
    BonusContinueButton = nullptr;
    SpinStatus = nullptr;
    SlotNoticeText = nullptr;
    CoinsText = nullptr;
    LobbyCoinsText = nullptr;
    LobbyGemsText = nullptr;
    PlayerLevelText = nullptr;
    PlayerBadgeLevelText = nullptr;
    PlayerXpText = nullptr;
    SlotXpText = nullptr;
    JackpotMiniText = nullptr;
    JackpotMinorText = nullptr;
    JackpotMajorText = nullptr;
    JackpotGrandText = nullptr;
    PlayerXpBar = nullptr;
    BetText = nullptr;
    LinesText = nullptr;
    LinesLabel = nullptr;
    TotalBetText = nullptr;
    WinText = nullptr;
    AutoSpinStatusText = nullptr;
    BonusHeaderText = nullptr;
    BonusPromptText = nullptr;
    BonusPicksText = nullptr;
    BonusTotalText = nullptr;
    MissionsButtonLobby = nullptr;
    MissionsCanvas = nullptr;
    SettingsCanvas = nullptr;
    MissionsCloseButton = nullptr;
    SettingsMusicButton = nullptr;
    SettingsSfxButton = nullptr;
    SettingsMusicLabel = nullptr;
    SettingsSfxLabel = nullptr;
    MissionsStatusText = nullptr;
    NeonJackpotSymbolLabels.Reset();
    NeonWinFrames.Reset();
    NeonWinSegments.Reset();
    NeonSymbolImages.Reset();
    NeonHeatPrizeLabels.Reset();
    ReelViewports.Reset();
    ReelTileSlots.Reset();
    ReelArt.Reset();
    ReelMarks.Reset();
    WinLineSegments.Reset();
    BonusCardButtons.Reset();
    BonusCardLabels.Reset();
    BonusCardImages.Reset();
    BonusCardPrizeLabels.Reset();
    DailyBonusCardFrames.Reset();
    DailyBonusCardLabels.Reset();
    MissionClaimButtons.Reset();
    MissionLabels.Reset();
    MissionBorders.Reset();
    for (auto& Label : NeonJackpotTexts) Label = nullptr;
    if (DesignCanvas) DesignCanvas->ClearChildren();
}

void USugarLustRootWidget::HandleEnterSocialRoom()
{
    if (SocialRoom) return;
    if (Lounge3D) { Lounge3D->SetActorHiddenInGame(true); Lounge3D->SetActorTickEnabled(false); }
    // Offset the isolated room so the earlier procedural preview cannot overlap it.
    SocialRoom = GetWorld()->SpawnActorDeferred<ASocialCasinoPrototype>(ASocialCasinoPrototype::StaticClass(), FTransform(FVector(10000,0,0)));
    if(SocialRoom){SocialRoom->UseEventVenue();SocialRoom->FinishSpawning(FTransform(FVector(10000,0,0)));}
    if (!SocialRoom) return;
    SetVisibility(ESlateVisibility::Hidden);
    SocialRoom->Configure(this);
}
void USugarLustRootWidget::OpenSocialSlot(ASocialCasinoPrototype* Room)
{
    if (Room != SocialRoom || !Room->IsSeated() || CabinetHost) return;
    if (Room->IsSelectedMegaPot())
    {
        if(!bPlayFabProfileReady){Room->ReturnFromGame();return;}
        if(!MegaPotCabinet)MegaPotCabinet=CreateWidget<UMegaLustPotWidget>(GetOwningPlayer());
        MegaPotCabinet->ConfigureLive(this);MegaPotCabinet->SetCabinetLayout();
        SetVisibility(ESlateVisibility::Hidden);
        CabinetHost=GetWorld()->SpawnActorDeferred<ACasinoCabinetDisplay>(ACasinoCabinetDisplay::StaticClass(),FTransform(FVector(200000,0,0)));
        if(CabinetHost){CabinetHost->Session=this;CabinetHost->MegaSession=MegaPotCabinet;CabinetHost->bUseExistingSession=true;CabinetHost->FinishSpawning(FTransform(FVector(200000,0,0)));}
        else Room->ReturnFromGame();
        return;
    }
    if (Room->IsSelectedNeonNights()) ShowNeonNightsPreview();
    else HandleOpenSlot();
    SetRenderTransformPivot(FVector2D::ZeroVector);
    SetRenderScale(FVector2D(1,1)); SetRenderTranslation(FVector2D::ZeroVector);
    // The root stays in the viewport to preserve its account, callbacks, and room.
    // Only its game canvas moves to the cabinet presentation.
    SetVisibility(ESlateVisibility::Hidden);
    CabinetHost = GetWorld()->SpawnActorDeferred<ACasinoCabinetDisplay>(ACasinoCabinetDisplay::StaticClass(),FTransform(FVector(200000,0,0)));
    if (CabinetHost)
    {
        CabinetHost->Session = this;
        CabinetHost->BankCabinetMesh = Room->GetSelectedCabinetMesh();
        CabinetHost->bUseExistingSession = true;
        CabinetHost->FinishSpawning(FTransform(FVector(200000,0,0)));
    }
    else { SetVisibility(ESlateVisibility::Visible); }
}
void USugarLustRootWidget::CloseCabinetView() { if(CabinetHost && CabinetHost->MegaSession && !CabinetHost->MegaSession->PrepareToLeaveCabinet())return;HandleBack(); }
void USugarLustRootWidget::CloseSocialRoom()
{
    if (CabinetHost) { auto* Host=CabinetHost.Get(); Host->UnmountSession(); Host->Destroy(); }
    SocialRoom=nullptr;
    SetRenderScale(FVector2D(1,1)); SetRenderTranslation(FVector2D::ZeroVector);
    SetVisibility(ESlateVisibility::Visible);
    ShowLobby();
}

#include "CasinoCabinetControls.h"
UUserWidget* USugarLustRootWidget::CreateCabinetControls()
{
    if (CabinetControls) return CabinetControls;
    if (!SlotGame || !DesignCanvas) return nullptr;
    // Preserve modal overlays at their authored size; expand only the base game.
    for (UWidget* Child : DesignCanvas->GetAllChildren())
    {
        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Child->Slot);
        if (!CanvasSlot || CanvasSlot->GetZOrder() >= 20) continue;
        FVector2D P = CanvasSlot->GetPosition(), S = CanvasSlot->GetSize();
        if ((P.Y < 126 && S.Y < 150) || P.Y >= 899)
        {
            Child->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }
        // Keep authored coordinates: reel animation updates these slots every frame.

    }
    DesignCanvas->SetRenderTransformPivot(FVector2D::ZeroVector);
    DesignCanvas->SetRenderScale(FVector2D(1.35, 1.38));
    DesignCanvas->SetRenderTranslation(FVector2D(-337.5, -179.4));
    DesignSizeBox->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    UCasinoCabinetControls* Deck = CreateWidget<UCasinoCabinetControls>(GetOwningPlayer());
    Deck->Session = this;
    CabinetControls = Deck;
    return Deck;
}


void USugarLustRootWidget::HandleOpenMegaPot()
{
    if(SocialRoom || CabinetHost || bShowingSlot || bShowingNeonNightsPreview)return;
    if(!MegaPotPreview)MegaPotPreview=CreateWidget<UMegaLustPotWidget>(GetOwningPlayer());
    MegaPotPreview->Lobby=this;
    SetVisibility(ESlateVisibility::Hidden);
    MegaPotPreview->AddToViewport(60);
}


void USugarLustRootWidget::HandleOpenMegaPotLive()
{
    if(SocialRoom || CabinetHost || bShowingSlot || bShowingNeonNightsPreview)return;
    if(!bPlayFabProfileReady){ShowSlotNotice(TEXT("PLEASE WAIT FOR YOUR CASINO LOGIN."));return;}
    if(!MegaPotLive)MegaPotLive=CreateWidget<UMegaLustPotWidget>(GetOwningPlayer());
    MegaPotLive->ConfigureLive(this);SetVisibility(ESlateVisibility::Hidden);MegaPotLive->AddToViewport(60);
}
