#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "SpinDuelRules.h"
#include "SpinDuelRun.generated.h"
class ASocialCasinoPrototype;
class ASpinDuelRun;
class UTextBlock;
class UButton;
class UCameraComponent;
class UStaticMeshComponent;
class ULevelStreamingDynamic;

UCLASS()
class SUGARLUSTUE_API USpinDuelPanel : public UUserWidget
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<ASpinDuelRun> Run;
 virtual void NativeOnInitialized() override;
 void Refresh();
 UFUNCTION() void Primary(); UFUNCTION() void Secondary(); UFUNCTION() void Leave();
 UFUNCTION() void HoldOne(); UFUNCTION() void HoldTwo(); UFUNCTION() void HoldThree();
 UPROPERTY() TObjectPtr<UTextBlock> Heading;
 UPROPERTY() TObjectPtr<UTextBlock> Details;
 UPROPERTY() TObjectPtr<UTextBlock> PrimaryLabel;
 UPROPERTY() TObjectPtr<UTextBlock> SecondaryLabel;
 UPROPERTY() TObjectPtr<UButton> PrimaryButton;
 UPROPERTY() TObjectPtr<UButton> SecondaryButton;
 UPROPERTY() TArray<TObjectPtr<UButton>> ReelButtons;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> ReelLabels;
};

UCLASS()
class SUGARLUSTUE_API ASpinDuelRun : public AActor
{
 GENERATED_BODY()
public:
 ASpinDuelRun();
 virtual void BeginPlay() override;
 virtual void Tick(float Dt) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 void Configure(ASocialCasinoPrototype* InCasino);
 UFUNCTION(BlueprintCallable) void Primary();
 UFUNCTION(BlueprintCallable) void Secondary();
 UFUNCTION(BlueprintCallable) void Leave();
 UFUNCTION(BlueprintCallable) void Hold(int32 Index);
 UFUNCTION(BlueprintPure) FString GetPhase() const {return Phase;}
 UFUNCTION(BlueprintPure) int32 GetRunHealth() const {return Combat.HP;}
 UFUNCTION(BlueprintPure) int32 GetKeys() const {return Combat.Keys;}
 FString Title() const; FString Detail() const; FString PrimaryText() const; FString SecondaryText() const;
 UFUNCTION(BlueprintPure) FString ReelText(int32 Index) const;
 bool IsBusy() const {return SpinDelay>0||Phase==TEXT("Loading")||Phase==TEXT("Travel");}
 bool InCombat() const {return Phase==TEXT("Duel");}
private:
 TWeakObjectPtr<ASocialCasinoPrototype> Casino;
 UPROPERTY() TObjectPtr<UCameraComponent> Camera;
 UPROPERTY() TObjectPtr<USpinDuelPanel> Panel;
 UPROPERTY() TObjectPtr<ULevelStreamingDynamic> Forest;
 UPROPERTY() TObjectPtr<USceneComponent> Walker;
 UPROPERTY() TObjectPtr<USceneComponent> Challenger;
 UPROPERTY() TObjectPtr<UStaticMesh> Cube;
 UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
 UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
 FSpinDuelState Combat;
 FRandomStream Random;
 FString Phase=TEXT("Confirm"),Message;
 TArray<FVector> Stops;
 FVector TravelFrom,TravelTo;
 float TravelAlpha=0,SpinDelay=0,LoadTime=0,ReelRefresh=0;
 TArray<int32> Pending;
 bool bReady=false,bClosing=false;
 void StartForest();void ReadyForest();void StartTravel(bool Hard);void Arrive();void ResolveSpin();void Refresh();
 FVector Ground(FVector P) const;
 USceneComponent* Person(FVector P,bool Rival);
 UStaticMeshComponent* Part(USceneComponent* Parent,FVector P,FVector Size,FLinearColor Color,bool Round=false);
 void Label(const FString& Text,FVector P,float Size=28);
};
