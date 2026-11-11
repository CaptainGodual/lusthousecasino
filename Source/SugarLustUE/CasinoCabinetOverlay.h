#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CasinoCabinetOverlay.generated.h"
class ACasinoCabinetDisplay;
class UImage;
class USizeBox;
UCLASS()
class SUGARLUSTUE_API UCasinoCabinetGameView : public UUserWidget
{
 GENERATED_BODY()
public:
 void SetGameCanvas(UWidget* Content);
protected:
 virtual void NativeOnInitialized() override;
private:
 UPROPERTY(Transient) TObjectPtr<USizeBox> ContentBox;
};
UCLASS()
class SUGARLUSTUE_API UCasinoCabinetOverlay : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(Transient) TObjectPtr<ACasinoCabinetDisplay> Host;
 void SetCaptureTexture(class UTextureRenderTarget2D* Target);
protected:
 virtual void NativeOnInitialized() override;
 virtual FReply NativeOnMouseMove(const FGeometry&,const FPointerEvent&) override;
 virtual FReply NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
 virtual FReply NativeOnMouseButtonUp(const FGeometry&,const FPointerEvent&) override;
 virtual FReply NativeOnTouchStarted(const FGeometry&,const FPointerEvent&) override;
 virtual FReply NativeOnTouchMoved(const FGeometry&,const FPointerEvent&) override;
 virtual FReply NativeOnTouchEnded(const FGeometry&,const FPointerEvent&) override;
private:
 UPROPERTY(Transient) TObjectPtr<UImage> CaptureImage;
 bool Point(const FPointerEvent& Event);
 bool bPointerDown=false;
 UFUNCTION() void Close();
};
