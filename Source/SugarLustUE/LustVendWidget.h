#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "LustVendWidget.generated.h"

class FJsonObject;
class UTexture2D;
class UMediaPlayer;
class UMediaTexture;
class UMediaSoundComponent;

/** Gems and inventory are owned exclusively by the LUSTVEND service. */
UCLASS()
class SUGARLUSTUE_API ULustVendWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(const FString& Ticket, const FString& PlayerId);
    TFunction<void(int64)> OnWalletChanged;
    void ShowRewardLibrary();
    static void SendAccount(const FString& Ticket, const FString& Route, const TSharedPtr<FJsonObject>& Body,
        TFunction<void(TSharedPtr<FJsonObject>, int32)> Done);
    static FString DescribeEarnings(const TSharedPtr<FJsonObject>& Json);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float Delta) override;
private:
    friend class FLustVendLibraryTest;
    TSharedRef<SWidget> BuildRewardLibrary();
    TSharedRef<SWidget> BuildRewardViewer();
    bool OpenReward(int32 Index);
    void FinishRewardOpening();
    void LoadRewardPage();
    void CloseRewardViewer();
    void StopRewardMedia();
    static FString ResolveRewardVideoPath(const FString& RelativePath);
    UFUNCTION() void HandleRewardMediaFailed(FString FailedUrl);
    UFUNCTION() void HandleRewardMediaOpened(FString OpenedUrl);
    bool bShowingLibrary = false;
    bool bViewingReward = false;
    bool bRewardAudioMuted = false;
    int32 ViewedReward = INDEX_NONE;
    int32 RewardPage = 0;
    float RewardOpeningSeconds = -1.f;
    FString AutoOpenSku;
    FString RewardMessage;
    TArray<FString> RewardImages, RewardVideos;
    FSlateBrush RewardPreviewBrush;
    FSlateBrush OpeningPackageBrush;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> RewardPreviewTexture;
    UPROPERTY(Transient) TObjectPtr<UMediaPlayer> RewardMediaPlayer;
    UPROPERTY(Transient) TObjectPtr<UMediaTexture> RewardMediaTexture;
    UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> RewardMediaSound;
    void Refresh();
    void Buy();
    void Collect();
    void Send(const FString& Route, const TSharedPtr<FJsonObject>& Body,
        TFunction<void(TSharedPtr<FJsonObject>, int32)> Done);
    void SavePending();
    void SetFrame(int32 Frame);
    void SelectPrize(int32 Index);
    void CompleteTutorialLesson();
    FString TutorialText() const;
    FString Session, PendingPath, RequestId, PendingSku, PendingVersion;
    FString Version, Receipt, Status = TEXT("Connecting to your gem wallet...");
    TArray<FString> Skus, Names;
    TArray<int32> Prices;
    TSet<FString> Owned;
    int32 Selected = INDEX_NONE;
    int64 Balance = 0;
    bool bBusy = false;
    bool bConfirm = false;
    bool bReady = false;
    bool bShowingEarnings = false;
    int32 TutorialStep = 0;
    FString EarningSummary = TEXT("Loading earning progress...");
    float Animation = -1.f;
    float CollectAnimation = -1.f;
    FSlateBrush MachineBrush;
    FSlateBrush SceneBrush;
    FSlateBrush ProductBrushes[6];
    FSlateBrush DispensedBrush;
    UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> Frames;
    UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> ProductTextures;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> SceneTexture;
};
