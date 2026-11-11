#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MegaLustPotGame.h"
#include "MegaLustPotWidget.generated.h"
class UImage;class UTexture2D;class UCanvasPanel;class UTextBlock;class UButton;class UBorder;class UProgressBar;class USugarLustRootWidget;
UCLASS()
class SUGARLUSTUE_API UMegaLustPotWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(Transient) TObjectPtr<USugarLustRootWidget> Lobby;
 void ConfigureLive(USugarLustRootWidget* Owner);
 void SetCabinetLayout();
 bool PrepareToLeaveCabinet();
 bool bCabinetLayout=false;
 UFUNCTION(BlueprintCallable) void Synchronize();
 UFUNCTION(BlueprintPure) bool IsLiveMode() const { return bLive; }
 UFUNCTION(BlueprintPure) bool IsServerReady() const { return bReady; }
 UFUNCTION(BlueprintCallable) void Spin();
 UFUNCTION(BlueprintCallable) void Close();
 UFUNCTION(BlueprintCallable) void ResetPractice();
 UFUNCTION(BlueprintCallable) void TryCrimson();
 UFUNCTION(BlueprintCallable) void TryRoyal();
 UFUNCTION(BlueprintCallable) void TryMega();
 UFUNCTION(BlueprintPure) FString GetPhase() const;
 UFUNCTION(BlueprintPure) int64 GetPracticeCredits() const { return Game.Credits; }
protected:
 virtual void NativeOnInitialized() override;
 virtual void NativeTick(const FGeometry&,float) override;
private:
 friend class FMegaPotServerViewTest;
 bool ApplyServer(const TSharedPtr<class FJsonObject>& Result,bool Sync);
 void SpinLive();
 void FinishLiveSpin();
 TSharedPtr<FJsonObject> PendingPresentation;
 void UpdateLivePots();
 FString JournalPath() const;
 bool SavePending();
 bool ClearPending();
 bool bLive=false,bBusy=false,bReady=false;
 int64 ServerRevision=0;
 int32 Generation=0;
 FString PendingId;
 TSharedPtr<FJsonObject> LatestState;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeLabel;
 UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> PracticeOnly;
 FMegaLustPotGame Game{9127};
 UPROPERTY(Transient) TObjectPtr<UTexture2D> SymbolAtlas;
 UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SymbolImages;
 UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> SymbolValueBackers;
 void PaintSymbol(int32 Cell,int32 Symbol,bool Hold=false);
 UPROPERTY(Transient) TObjectPtr<UTexture2D> Atlas;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> BalanceValue;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> BetValue;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> WinValue;
 FSlateBrush AtlasBrush(float X,float Y,float W,float H) const;
 UImage* Art(float SX,float SY,float SW,float SH,float X,float Y,float W,float H);
 UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
 UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Symbols;
 UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Cells;
 UPROPERTY(Transient) TArray<TObjectPtr<UProgressBar>> Pots;
 UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> PotLabels;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Wallet;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Features;
 UPROPERTY(Transient) TObjectPtr<UWidget> SpinActionBadge;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> SpinLabel;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> AutoLabel;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Help;
 UPROPERTY(Transient) TObjectPtr<UButton> SpinButton;
 bool Auto=false,ShowingHelp=false;
 float Animation=0,AutoDelay=0,Clock=0;
 int32 Tour=0;
 void Refresh();
 void StartTour(int32 Kind);
 UFUNCTION() void BetDown();UFUNCTION() void BetUp();UFUNCTION() void MaxBet();UFUNCTION() void ToggleAuto();UFUNCTION() void ToggleHelp();
 UTextBlock* Text(const FString&,float,float,float,float,int32,FLinearColor);
 UButton* Button(const FString&,float,float,float,float,TObjectPtr<UTextBlock>* Label=nullptr);
 void Place(UWidget*,float,float,float,float);
};

