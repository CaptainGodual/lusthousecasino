#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "CasinoFloorRoute.h"
#include "SocialCasinoPrototype.generated.h"
class USugarLustRootWidget;
class ASocialCasinoPrototype;
class ASpinDuelRun;
class UCameraComponent;
class UStaticMesh;
class UMaterialInterface;
class USceneComponent;
class UTextBlock;

UCLASS()
class SUGARLUSTUE_API USocialCasinoControls : public UUserWidget
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<ASocialCasinoPrototype> Casino;
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event) override;
    UPROPERTY() TObjectPtr<UTextBlock> Status;
    UFUNCTION() void SeatOne();
    UFUNCTION() void SeatThree();
    UFUNCTION() void Play();
    UFUNCTION() void Stand();
    UFUNCTION() void Leave();
    UFUNCTION() void SpinDuel();
};

UCLASS()
class SUGARLUSTUE_API ASocialCasinoPrototype : public AActor
{
    GENERATED_BODY()
public:
    ASocialCasinoPrototype();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void Configure(USugarLustRootWidget* Owner);
    void WalkFromScreen(FVector2D Screen);
    UFUNCTION(BlueprintCallable) void RequestSeat(int32 Index);
    UFUNCTION(BlueprintCallable) void BuildPreview();
    UFUNCTION(BlueprintCallable) void StepPreview(float Seconds) { Tick(Seconds); }
    void UseEventVenue() { bUseVenue = true; }
    UFUNCTION(BlueprintCallable) void BuildVenuePreview();
    UFUNCTION(BlueprintCallable) bool RequestWalkTo(FVector LocalPoint);
    UFUNCTION(BlueprintCallable) void SetGroupBonusView(bool Enabled) { bGroupBonusFocus = Enabled; }
    bool IsGroupBonusView() const { return bGroupBonusFocus; }
    UFUNCTION(BlueprintPure) int32 GetSelectedBankSize() const { return Seat>=0 ? BankMembers(Seat).Num() : 0; }
    UFUNCTION(BlueprintPure) bool IsSelectedNeonNights() const;
    UFUNCTION(BlueprintPure) bool IsSelectedMegaPot() const;
    UFUNCTION(BlueprintPure) UStaticMesh* GetSelectedCabinetMesh() const;
    bool IsVenueReady() const { return bUseVenue && Avatar != nullptr; }
    UFUNCTION(BlueprintPure) FVector GetAvatarPosition() const { return Avatar ? AvatarPosition() : FVector::ZeroVector; }
    UFUNCTION(BlueprintCallable) void Play();
    UFUNCTION(BlueprintCallable) void Stand();
    void Leave();
    UFUNCTION(BlueprintCallable) void GoToSpinDuel();
    UFUNCTION(BlueprintCallable) void OpenSpinDuel();
    void QueueSpinDuelEntry() { bMenuEventEntry = true; }
    void ResumeFromSpinDuel();
    void SetVenueVisible(bool Visible);
    UFUNCTION(BlueprintCallable) void ReturnFromGame();
    UFUNCTION(BlueprintPure) bool IsSeated() const { return Seat >= 0; }
private:
    UPROPERTY() TObjectPtr<ASpinDuelRun> SpinDuelRun;
    bool bDuelPadArmed = true;
    bool bMenuEventEntry = false;
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
    UPROPERTY() TObjectPtr<UStaticMesh> CabinetMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
    UPROPERTY() TArray<TObjectPtr<class USkeletalMesh>> People;
    UPROPERTY() TObjectPtr<class UPoseableMeshComponent> PlayerPose;
    UPROPERTY() TObjectPtr<class UAnimSequence> FemaleSeatedAnimation;
    UPROPERTY() TObjectPtr<class UAnimSequence> MaleSeatedAnimation;
    UPROPERTY() TArray<TObjectPtr<class USkeletalMeshComponent>> SeatedVisuals;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> PlayerSeatedVisual;
    int32 PeopleCursor = 0;
    void ApplyPersonPose(class UPoseableMeshComponent* Mesh, bool bSeated, float Stride);
    UPROPERTY() TObjectPtr<USceneComponent> Avatar;
    UPROPERTY() TObjectPtr<USceneComponent> StandingLegs;
    UPROPERTY() TObjectPtr<USceneComponent> SittingLegs;
    UPROPERTY() TObjectPtr<USocialCasinoControls> Controls;
    TWeakObjectPtr<USugarLustRootWidget> Lobby;
    bool bUseVenue = false;
    bool bGroupBonusFocus = false;
    TArray<int32> BankMembers(int32 Index) const;
    void BankCamera(int32 Index, FVector& Position, FVector& Target) const;
    bool bVenueLoading = false;
    float VenueLoadSeconds = 0;
    UPROPERTY() TObjectPtr<class ULevelStreamingDynamic> VenueStream;
    UPROPERTY() TArray<TObjectPtr<AActor>> VenueMachines;
    TArray<FTransform> VenueSeatTransforms;
    FCasinoFloorRoute VenueRoute;
    TArray<FVector> Waypoints;
    int32 WaypointIndex = 0;
    bool InitializeVenue(class ULevel* Level);
    bool Occupied(int32 I) const { return bUseVenue ? (I==1 || I==4) : (I==1 || I==3); }
    float SeatYaw(int32 I) const { return bUseVenue && VenueSeatTransforms.IsValidIndex(I) ? VenueSeatTransforms[I].Rotator().Yaw : 0; }
    FVector Approach(int32 I) const;
    FVector AvatarPosition() const;
    FVector Destination;
    int32 PendingSeat = -1;
    int32 Seat = -1;
    bool bPlaying = false;
    float WalkPhase = 0;
    FVector SeatPosition(int32 Index) const;
    void Status(const FString& Text);
    USceneComponent* MakeGroup(USceneComponent* Parent);
    void Part(USceneComponent* Parent, FVector Position, FVector Size, FLinearColor Color, bool bSphere=false);
    USceneComponent* MakeAvatar(FVector Position, FLinearColor Color, bool bSeated);
    void Label(const FString& Text,FVector Position,float Size=22);
};
