#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CasinoCabinetControls.generated.h"
class USugarLustRootWidget;
class UTextBlock;
class UButton;
UCLASS()
class SUGARLUSTUE_API UCasinoCabinetControls : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(Transient) TObjectPtr<USugarLustRootWidget> Session;
 UFUNCTION(BlueprintPure, Category="Casino|Cabinet") FString GetDisplayedWager() const;
protected:
 virtual void NativeOnInitialized() override;
 virtual void NativeTick(const FGeometry&,float) override;
private:
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Balance;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Wager;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Win;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> AutoLabel;
 UPROPERTY(Transient) TObjectPtr<UButton> Minus;
 UPROPERTY(Transient) TObjectPtr<UButton> Plus;
 UPROPERTY(Transient) TObjectPtr<UButton> Max;
 UPROPERTY(Transient) TObjectPtr<UButton> Spin;
 UPROPERTY(Transient) TObjectPtr<UButton> Auto;
 UFUNCTION() void BetDown();
 UFUNCTION() void BetUp();
 UFUNCTION() void MaxBet();
 UFUNCTION() void DoSpin();
 UFUNCTION() void DoAuto();
};
