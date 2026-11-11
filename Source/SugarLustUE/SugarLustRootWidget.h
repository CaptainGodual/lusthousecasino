#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SugarLustRootWidget.generated.h"

class UCanvasPanel;
class USizeBox;
class UWidget;
class UTexture2D;
class UImage;
class UTextBlock;
class UButton;
class UCanvasPanelSlot;
class UBorder;
class UProgressBar;
class UAudioComponent;
class USoundWave;
class USoundWaveProcedural;
class FSugarLustSlotMachine;
class FJsonObject;
class UVerticalBox;
class ULustVendWidget;

UCLASS()
class SUGARLUSTUE_API USugarLustRootWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    USugarLustRootWidget(const FObjectInitializer& ObjectInitializer);
    void OpenSocialSlot(class ASocialCasinoPrototype* Room);
    void CloseSocialRoom();
    UFUNCTION(BlueprintCallable, Category="Casino|Cabinet")
    UUserWidget* CreateCabinetControls();
    UFUNCTION(BlueprintPure, Category="Casino|Cabinet") bool IsCabinetNeonNights() const { return bShowingNeonNightsPreview; }
    friend class UCasinoCabinetControls;
    friend class UMegaLustPotWidget;
    friend class FMegaPotServerViewTest;
    friend class FPendingSpinIdentityTest;
    friend class ACasinoCabinetDisplay;
    UPROPERTY(Transient) TObjectPtr<class ACasinoCabinetDisplay> CabinetHost;
    UFUNCTION(BlueprintCallable, Category="Casino|Cabinet") void CloseCabinetView();

    UPROPERTY(Transient) TObjectPtr<UUserWidget> CabinetControls;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
private:
    UPROPERTY(Transient) TObjectPtr<class UMegaLustPotWidget> MegaPotPreview;
    UFUNCTION() void HandleOpenMegaPot();
    UPROPERTY(Transient) TObjectPtr<UMegaLustPotWidget> MegaPotLive;
    UPROPERTY(Transient) TObjectPtr<UMegaLustPotWidget> MegaPotCabinet;
    UFUNCTION() void HandleOpenMegaPotLive();
    UPROPERTY(Transient) TObjectPtr<class ASocialCasinoPrototype> SocialRoom;
    UFUNCTION() void HandleEnterSocialRoom();
    UFUNCTION() void HandleMenuFriends();
    UFUNCTION() void HandleMenuEvent();
    UPROPERTY(Transient) TObjectPtr<class ALustHouseLounge> Lounge3D;
    UFUNCTION() void HandleEnter3DLounge();
    UFUNCTION() void HandleLoungeOverview();
    UFUNCTION() void HandleLoungeSlots();
    UFUNCTION() void HandleLoungeVend();
    UFUNCTION() void HandleLoungeStage();
    UFUNCTION() void HandleExit3DLounge();
    UPROPERTY(Transient) TObjectPtr<UTexture2D> LobbyVendTexture;
    UPROPERTY(Transient) TObjectPtr<ULustVendWidget> ActiveVendWidget;
    int64 LobbyGemBalance = -1;
    FString GemEarningSummary = TEXT("Daily login +10 gems. Finish 10 / 25 / 50 spins for gem milestones.");
    UTextBlock* GemEarningProgressText = nullptr;
    void RefreshLustVendBalance();
    UCanvasPanel* DesignCanvas = nullptr;
    USizeBox* DesignSizeBox = nullptr;
    UImage* WideBackgroundImage = nullptr;
    UCanvasPanel* BonusCanvas = nullptr;
    UCanvasPanel* BonusTransitionCanvas = nullptr;
    UTextBlock* BonusTransitionTitle = nullptr;
    UTextBlock* BonusTransitionSubtitle = nullptr;
    UCanvasPanel* BigWinCanvas = nullptr;
    UImage* BigWinBackdropImage = nullptr;
    UBorder* BigWinAmountCover = nullptr;
    UBorder* BigWinFallbackPanel = nullptr;
    UCanvasPanel* DailyBonusCanvas = nullptr;
    UBorder* LobbyNoticePanel = nullptr;
    UTextBlock* LobbyNoticeText = nullptr;
    UBorder* LobbyHostGreetingPanel = nullptr;
    UTextBlock* LobbyHostGreetingText = nullptr;
    FSugarLustSlotMachine* SlotGame = nullptr;
    bool bShowingSlot = false;
    bool bShowingNeonNightsPreview = false;
    struct FFreeSpinStatus
    {
        int32 Remaining = 0;
        int32 TotalAwarded = 0;
        int32 LastAward = 0;
        int64 TotalWin = 0;
        bool bLastWasFreeSpin = false;
        bool bPaused = false;
        float DelaySeconds = 1.0f;
    };
    FString SugarSpinError;
    FString NeonSpinError;
    FFreeSpinStatus SugarFreeSpins;
    FFreeSpinStatus NeonFreeSpins;
    UTextBlock* SugarFreeSpinsText = nullptr;
    UTextBlock* NeonFreeSpinsText = nullptr;
    void ApplyFreeSpinData(const TSharedPtr<FJsonObject>& Profile);
    void UpdateFreeSpinsUI();
    bool bNeonSpinAnimating = false;
    bool bNeonResultReady = false;
    bool bNeonAutoSpin = false;
    bool bNeonPayoutAnimating = false;
    float NeonSpinSeconds = 0.0f;
    float NeonGridAnimationTick = 0.0f;
    float NeonPayoutSeconds = 0.0f;
    int32 NeonBet = 100;
    int32 NeonAutoSpinsRemaining = 0;
    int64 NeonCoins = 0;
    int64 NeonXp = 0;
    int64 NeonWin = 0;
    int64 NeonDisplayedWin = 0;
    int64 NeonBaseWin = 0;
    bool bNeonCurrentSpinIsBonus = false;
    float NeonPayoutDuration = 1.25f;
    float NeonWinPresentationSeconds = 0.0f;
    friend class FNeonWinFeedbackTest;
    struct FNeonWinningGroup
    {
        TArray<TArray<int32>> CellsByReel;
        int32 Ways = 1;
        FLinearColor Color = FLinearColor::White;
    };
    TArray<FNeonWinningGroup> NeonWinningGroups;
    TArray<int32> NeonWinningScatterCells;
    TArray<int32> NeonWinningFreeSpinCells;
    TArray<int32> NeonWinningJackpotCells;
    TArray<UTextBlock*> NeonJackpotSymbolLabels;
    FString PendingSugarJackpotTier;
    FString PendingNeonJackpotTier;
    int64 PendingSugarJackpotPrize = 0;
    int64 PendingNeonJackpotPrize = 0;
    UCanvasPanel* ProgressiveJackpotCanvas = nullptr;
    bool IsProgressiveJackpotVisible() const;
    void ShowProgressiveJackpotWin(const FString& Tier, int64 Prize);
    UFUNCTION() void HandleCloseProgressiveJackpot();
    TArray<UBorder*> NeonWinFrames;
    TArray<UBorder*> NeonWinSegments;
    void BuildNeonWinPresentation();
    void UpdateNeonWinPresentation(float DeltaSeconds);
    void StopPayoutWinTune();
    bool bNeonHeatBonusActive = false;
    bool bNeonHeatBonusComplete = false;
    int32 NeonHeatRespinsRemaining = 0;
    int64 NeonHeatBonusTotal = 0;
    float NeonHeatBonusDelaySeconds = 0.0f;
    float NeonHeatBonusCompleteSeconds = 0.0f;
    TArray<int32> NeonHeatLockedIndices;
    TArray<int64> NeonHeatLockedPrizes;
    FString PendingNeonSpinId;
    TSharedPtr<FJsonObject> PendingNeonSpinParameters;
    TArray<int32> NeonVisibleGrid;
    TArray<int32> NeonTargetGrid;
    TArray<UImage*> NeonSymbolImages;
    TArray<UTextBlock*> NeonHeatPrizeLabels;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTexture2D>> NeonSymbolTextures;
    UTextBlock* NeonCoinsText = nullptr;
    UTextBlock* NeonXpText = nullptr;
    UTextBlock* NeonBetText = nullptr;
    UTextBlock* NeonTotalBetText = nullptr;
    UTextBlock* NeonWinText = nullptr;
    UTextBlock* NeonStatusText = nullptr;
    UTextBlock* NeonJackpotTexts[4] = {};
    UButton* NeonSpinButton = nullptr;
    UButton* NeonBetDownButton = nullptr;
    UButton* NeonBetUpButton = nullptr;
    UButton* NeonMaxBetButton = nullptr;
    UButton* NeonAutoSpinButton = nullptr;
    bool bShowingDisclaimer = false;
    bool bPlayFabLoginStarted = false;
    bool bPlayFabProfileReady = false;
    bool bPlayFabMissionsReady = false;
    bool bPlayFabRequestInFlight = false;
    bool bSpinRequestRetrying = false;
    bool bJackpotRequestInFlight = false;
    bool bBonusPickRequestInFlight = false;
    bool bDailyBonusRequestInFlight = false;
    bool bMissionClaimRequestInFlight = false;
    int32 AutoSpinsRemaining = 0;
    bool bAutoSpinWaitingForAnimation = false;
    bool bSpinButtonHeld = false;
    bool bAutoSpinStartedFromHold = false;
    float SpinButtonHoldSeconds = 0.0f;
    float SlotNoticeSeconds = 0.0f;
    float LobbyNoticeSeconds = 0.0f;
    float LobbyHostGreetingSeconds = 0.0f;
    bool bHasShownLobbyHostGreeting = false;
    int32 LobbyHostGreetingLineIndex = INDEX_NONE;
    float JackpotRefreshSeconds = 0.0f;
    float BonusTransitionSeconds = 0.0f;
    bool bBonusWasVisible = false;
    float DisclaimerShownAt = 0.0f;
    FString PlayFabPlayerId;
    FString PlayerDisplayName = TEXT("Guest");
    FString PlayFabSessionTicket;
    FString PendingPlayFabSpinId;
    TSharedPtr<FJsonObject> PendingSugarSpinParameters;
    int64 JackpotMini = 125689745;
    int64 JackpotMinor = 689745321;
    int64 JackpotMajor = 1208745321;
    int64 JackpotGrand = 2809689745;

    TArray<UCanvasPanel*> ReelViewports;
    TArray<UCanvasPanelSlot*> ReelTileSlots;
    TArray<UImage*> ReelArt;
    TArray<UTextBlock*> ReelMarks;
    TArray<uint8> ReelSymbolCache;
    TArray<UBorder*> WinLineSegments;
    float GrapeWinAnimationSeconds = 0.0f;
    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SpinMusicSound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> SpinMusicComponent = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundWaveProcedural> PayoutWinTuneSound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> PayoutWinTuneComponent = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundWave> NeonWelcomeSound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> NeonWelcomeComponent = nullptr;
    void PlayNeonWelcome();
    void StopNeonWelcome();
    float SpinMusicStopDelaySeconds = 0.0f;
    bool bSpinMusicFadeOutPending = false;
    bool bSpinMusicStartedForCurrentSpin = false;
    UImage* BigWinCharacterLeft = nullptr;
    UImage* BigWinCharacterRight = nullptr;
    UTextBlock* BigWinTitleText = nullptr;
    UTextBlock* BigWinAmountText = nullptr;
    UTextBlock* BigWinCoinsLabel = nullptr;
    float BigWinPresentationSeconds = 0.0f;
    float BigWinPresentationDuration = 5.5f;
    float BigWinPayoutCountDuration = 2.4f;
    int64 BigWinPresentationAmount = 0;

    TArray<UButton*> BonusCardButtons;
    TArray<UTextBlock*> BonusCardLabels;
    TArray<UImage*> BonusCardImages;
    TArray<UTextBlock*> BonusCardPrizeLabels;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> BonusJarSpriteSheet = nullptr;
    int32 BonusJarAnimatingCard = INDEX_NONE;
    float BonusJarAnimationSeconds = 0.0f;
    TArray<UBorder*> DailyBonusCardFrames;
    TArray<UTextBlock*> DailyBonusCardLabels;
    UButton* DailyBonusButton = nullptr;
    UButton* DailyBonusClaimButton = nullptr;
    UButton* DailyBonusCloseButton = nullptr;
    UButton* DisclaimerContinueButton = nullptr;
    UTextBlock* DailyBonusStatusText = nullptr;
    UTextBlock* DailyBonusMessageText = nullptr;
    UTextBlock* DailyBonusClaimLabel = nullptr;
    UButton* SpinButton = nullptr;
    UButton* AutoSpinButton = nullptr;
    UButton* MaxBetButton = nullptr;
    UButton* BetMinusButton = nullptr;
    UButton* BetPlusButton = nullptr;
    UImage* BetMinusArt = nullptr;
    UImage* BetPlusArt = nullptr;
    UImage* SpinButtonArt = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> CandyBetMinusNormalTexture = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> CandyBetMinusPressedTexture = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> CandyBetPlusNormalTexture = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> CandyBetPlusPressedTexture = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> CandySpinNormalTexture = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> CandySpinPressedTexture = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> CandySpinAutoTexture = nullptr;
    UButton* LinesBadgeButton = nullptr;
    UButton* LinesMinusButton = nullptr;
    UButton* LinesPlusButton = nullptr;
    UButton* BonusContinueButton = nullptr;
    UTextBlock* SpinStatus = nullptr;
    UTextBlock* SlotNoticeText = nullptr;
    UTextBlock* CoinsText = nullptr;
    UTextBlock* LobbyCoinsText = nullptr;
    UTextBlock* LobbyGemsText = nullptr;
    UTextBlock* PlayerLevelText = nullptr;
    UTextBlock* PlayerBadgeLevelText = nullptr;
    UTextBlock* PlayerXpText = nullptr;
    UTextBlock* SlotXpText = nullptr;
    UTextBlock* JackpotMiniText = nullptr;
    UTextBlock* JackpotMinorText = nullptr;
    UTextBlock* JackpotMajorText = nullptr;
    UTextBlock* JackpotGrandText = nullptr;
    UProgressBar* PlayerXpBar = nullptr;
    UTextBlock* BetText = nullptr;
    UTextBlock* LinesText = nullptr;
    UTextBlock* LinesLabel = nullptr;
    UTextBlock* TotalBetText = nullptr;
    UTextBlock* WinText = nullptr;
    UTextBlock* AutoSpinStatusText = nullptr;
    UTextBlock* BonusHeaderText = nullptr;
    UTextBlock* BonusPromptText = nullptr;
    UTextBlock* BonusPicksText = nullptr;
    UTextBlock* BonusTotalText = nullptr;
    bool bMusicEnabled = true;
    bool bSfxEnabled = true;

    UButton* MissionsButtonLobby = nullptr;
    UCanvasPanel* MissionsCanvas = nullptr;
    UCanvasPanel* SettingsCanvas = nullptr;
    UButton* MissionsCloseButton = nullptr;
    UButton* SettingsMusicButton = nullptr;
    UButton* SettingsSfxButton = nullptr;
    UTextBlock* SettingsMusicLabel = nullptr;
    UTextBlock* SettingsSfxLabel = nullptr;
    TArray<UButton*> MissionClaimButtons;
    TArray<UTextBlock*> MissionLabels;
    TArray<UBorder*> MissionBorders;
    UTextBlock* MissionsStatusText = nullptr;

    friend class FLoungeWidgetLifetimeTest;
    void ClearScreenWidgets();
    void BuildDisclaimer();
    void BuildLobby();
    void BuildSlot();
    void BuildNeonNightsPreview();
    void UpdateNeonNights(float DeltaSeconds);
    void UpdateNeonNightsUI();
    void ApplyNeonHeatBonusState(const TSharedPtr<FJsonObject>& State);
    void RequestNeonNightsSpin();
    UFUNCTION() void HandleNeonBetDown();
    UFUNCTION() void HandleNeonBetUp();
    UFUNCTION() void HandleNeonMaxBet();
    UFUNCTION() void HandleNeonSpin();
    UFUNCTION() void HandleNeonAutoSpin();
    void ShowLobby();
    void ShowLobbyHostGreeting();
    void UpdateLobbyHostGreeting();
    void ShowSlot();
    void ShowNeonNightsPreview();
    void UpdateSlotUI();
    void UpdateReelWidgets();
    void StartSpinMusic();
    void ScheduleSpinMusicFadeOut();
    void FadeOutSpinMusic();
    void UpdateBigWinUI(float DeltaSeconds);
    void ShowBigWinPresentation();
    void UpdateBonusUI();
    void UpdatePlayerProfileUI();
    void ApplyJackpotData(const TSharedPtr<FJsonObject>& Profile);
    void ApplyCloudMissionData(const TSharedPtr<FJsonObject>& Profile);
    void UpdateJackpotUI();
    void StartPlayFabLogin();
    void RequestPlayFabProfile();
    void RequestPlayFabJackpots();
    void RequestPlayFabSpin();
    void RequestPlayFabBonusPick(int32 Card);
    void RequestPlayFabDailyBonus();
    void ExecutePlayFabCloudScript(const FString& FunctionName, const TSharedRef<FJsonObject>& Parameters,
        TFunction<void(TSharedPtr<FJsonObject>)> OnSuccess, TFunction<void(const FString&)> OnFailure);
    void ExecutePlayFabCloudScriptAttempt(const FString& FunctionName, const TSharedRef<FJsonObject>& Parameters,
        TFunction<void(TSharedPtr<FJsonObject>)> OnSuccess, TFunction<void(const FString&)> OnFailure, int32 Attempt);
    static bool ShouldRetrySpinRequest(const FString& FunctionName, const FString& SpinId, int32 Attempt,
        int32 HttpCode, const FString& CloudError, bool bConnected);
    static bool PreparePendingSpin(FString& SpinId, TSharedPtr<FJsonObject>& Pending,
        const TSharedRef<FJsonObject>& Desired);

    void BuildDailyBonusModal();
    void UpdateDailyBonusUI();
    void PickBonusCard(int32 Card);
    UFUNCTION() void HandleOpenSlot();
    UFUNCTION() void HandleOpenNeonNights();
    UFUNCTION() void HandleContinueToLobby();
    UFUNCTION() void HandleOpenDailyBonus();
    UFUNCTION() void HandleClaimDailyBonus();
    UFUNCTION() void HandleCloseDailyBonus();
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandleFreeCoinsButton();
    UFUNCTION() void HandleStoreButton();
    UFUNCTION() void HandleCoinPlusButton();
    UFUNCTION() void HandleGemsPlusButton();
    UFUNCTION() void HandleOpenRewardLibrary();
    UFUNCTION() void HandlePiggyBankButton();
    UFUNCTION() void HandleLevelProgressButton();
    UFUNCTION() void HandleSettingsButton();
    UFUNCTION() void HandleLobbyHomeButton();
    UFUNCTION() void HandleJackpotsButton();
    UFUNCTION() void HandleTournamentsButton();
    UFUNCTION() void HandleEventsButton();
    UFUNCTION() void HandleMissionsButton();
    UFUNCTION() void HandleInboxButton();
    UFUNCTION() void HandleVipButton();
    UFUNCTION() void HandleComingSoonCard();
    UFUNCTION() void HandleOpenSettings();
    UFUNCTION() void HandleCloseSettings();
    UFUNCTION() void HandleToggleMusic();
    UFUNCTION() void HandleToggleSfx();
    UFUNCTION() void HandleOpenMissions();
    UFUNCTION() void HandleCloseMissions();
    UFUNCTION() void HandleClaimMission0();
    UFUNCTION() void HandleClaimMission1();
    UFUNCTION() void HandleClaimMission2();
    UFUNCTION() void HandleClaimMission3();
    UFUNCTION() void HandleClaimMission4();
    UFUNCTION() void HandleSpin();
    UFUNCTION() void HandleSpinPressed();
    UFUNCTION() void HandleSpinReleased();
    UFUNCTION() void HandleAutoSpin();
    UFUNCTION() void HandleMaxBet();
    void PlayPayoutWinTune(int64 WinAmount, float DurationSeconds = 0.0f);
    static TArray<uint8> BuildPayoutWinPcm(float DurationSeconds, float Gain, int32 SampleRate);
    UFUNCTION() void HandleBetDown();
    UFUNCTION() void HandleBetUp();
    UFUNCTION() void HandleBetDownPressed();
    UFUNCTION() void HandleBetDownReleased();
    UFUNCTION() void HandleBetUpPressed();
    UFUNCTION() void HandleBetUpReleased();
    UFUNCTION() void HandleLinesDown();
    UFUNCTION() void HandleLinesUp();
    UFUNCTION() void HandleCycleLines();
    UFUNCTION() void HandleBonusCard0();
    UFUNCTION() void HandleBonusCard1();
    UFUNCTION() void HandleBonusCard2();
    UFUNCTION() void HandleBonusCard3();
    UFUNCTION() void HandleBonusCard4();
    UFUNCTION() void HandleBonusCard5();
    UFUNCTION() void HandleBonusContinue();
    UWidget* MakeText(const FString& Text, float Size, FLinearColor Color, bool bBold = false, EHorizontalAlignment HAlign = HAlign_Center);
    UWidget* MakePanel(const FString& Text, FLinearColor BorderColor, FLinearColor FillColor, float FontSize = 28.0f);
    UButton* MakeButton(const FString& Text, FLinearColor Accent, float FontSize = 24.0f);
    UButton* MakeArtButton(const TCHAR* AssetPath, const FString& FallbackText);
    void RemoveButtonChrome(UButton* Button);
    void ShowSlotNotice(const FString& Message);
    void Place(UWidget* Widget, float X, float Y, float Width, float Height, int32 Z = 0);
    UWidget* MakeCard(const FString& Title, const FString& Type, FLinearColor Accent, bool bFeatured, int32 X, int32 Y, int32 W, int32 H);
    UTexture2D* LoadArt(const TCHAR* AssetPath);
    void SetCandyButtonImage(UImage* Image, UTexture2D* Texture);
    void SetWideBackground(const TCHAR* AssetPath, FLinearColor Tint = FLinearColor::White);
    void BuildSettingsModal();
    void UpdateSettingsUI();
    void BuildMissionsModal();
    void UpdateMissionsUI();
    void ClaimMissionAtIndex(int32 MissionIndex);
};



