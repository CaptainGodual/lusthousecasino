#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotStreakWidget.generated.h"

class UHotStreakSubsystem;
class UTextBlock;
class UButton;
DECLARE_MULTICAST_DELEGATE(FHotStreakPanelAction);

UCLASS()
class SUGARLUSTUE_API UHotStreakWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void BindEvent(UHotStreakSubsystem* Event);
    FHotStreakPanelAction OnClosed, OnPracticeRequested;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
private:
    friend class FHotStreakClientTest;
    UPROPERTY(Transient) TObjectPtr<UHotStreakSubsystem> Subsystem;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PhaseText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ProgressText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TierText;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ClaimText;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ClaimButtons;
    UPROPERTY(Transient) TObjectPtr<UButton> EquipButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EquipText;
    FDelegateHandle ChangedHandle;
    void RefreshView();
    UFUNCTION() void Close();
    UFUNCTION() void Practice();
    UFUNCTION() void Retry();
    UFUNCTION() void ClaimMild();
    UFUNCTION() void ClaimHot();
    UFUNCTION() void ClaimExtraHot();
    UFUNCTION() void ClaimInferno();
    UFUNCTION() void EquipMild();
};
