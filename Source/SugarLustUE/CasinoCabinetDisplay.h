#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "CasinoCabinetDisplay.generated.h"
class UWidgetComponent;
class UCameraComponent;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class UCasinoCabinetOverlay;
class UCasinoCabinetGameView;
class UWidgetInteractionComponent;
class USugarLustRootWidget;
class UMegaLustPotWidget;
UCLASS()
class SUGARLUSTUE_API UCasinoCabinetMarquee : public UUserWidget
{
 GENERATED_BODY()
protected:
 virtual void NativeOnInitialized() override;
};
UCLASS()
class SUGARLUSTUE_API ACasinoCabinetDisplay : public AActor
{
 GENERATED_BODY()
public:
 ACasinoCabinetDisplay();
 virtual void BeginPlay() override;
 virtual void Tick(float) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 bool bUseExistingSession=false;
 UPROPERTY(Transient) TObjectPtr<UStaticMesh> BankCabinetMesh;
 UPROPERTY(Transient) TObjectPtr<UMegaLustPotWidget> MegaSession;
 UFUNCTION(BlueprintPure, Category="Casino|Cabinet") USugarLustRootWidget* GetGameSession() const { return Session; }
 UFUNCTION(BlueprintPure, Category="Casino|Cabinet") USceneCaptureComponent2D* GetCapture() const { return Capture; }
 UFUNCTION(BlueprintPure, Category="Casino|Cabinet") UTextureRenderTarget2D* GetCaptureTarget() const { return CaptureTarget; }

 void UnmountSession();
 void ClearDisplayPointer();
 UFUNCTION(BlueprintCallable, Category="Casino|Cabinet") bool PointFromViewportPixel(FVector2D Pixel);
 UFUNCTION(BlueprintCallable, Category="Casino|Cabinet") void PointFromCaptureUV(FVector2D UV);
 UPROPERTY(Transient) TObjectPtr<USceneCaptureComponent2D> Capture;
 UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> CaptureTarget;
 UPROPERTY(Transient) TObjectPtr<UCasinoCabinetOverlay> Overlay;
 UPROPERTY(Transient) TObjectPtr<UCasinoCabinetGameView> GameView;

 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Cabinet;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> UpperDisplay;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> LowerDisplay;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> MarqueeDisplay;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetInteractionComponent> Interaction;
 UPROPERTY(Transient) TObjectPtr<USugarLustRootWidget> Session;
 // Useful for automation and screen-space hosts. UV is normalized in the lower display.
 UFUNCTION(BlueprintCallable, Category="Casino|Cabinet") void PointAtLowerDisplay(FVector2D UV);
 UFUNCTION(BlueprintCallable, Category="Casino|Cabinet") void PressDisplay();
 UFUNCTION(BlueprintCallable, Category="Casino|Cabinet") void ReleaseDisplay();
private:
 float PresentationScale=.95f;
 float PresentationFocusZ=103.f;
 float PresentationCenterX=0.f;
 bool bWasTouchDown=false;
 bool bMounted=false;
};
