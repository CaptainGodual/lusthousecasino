#include "CasinoCabinetDisplay.h"
#include "SugarLustRootWidget.h"
#include "MegaLustPotWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Camera/CameraComponent.h"
#include "CasinoCabinetOverlay.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/SizeBox.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"

#include "Blueprint/WidgetTree.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
void UCasinoCabinetMarquee::NativeOnInitialized()
{
 Super::NativeOnInitialized();auto* C=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=C;
 auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrushColor(FLinearColor(.018f,.008f,.035f));auto* S=C->AddChildToCanvas(B);S->SetSize({1600,864});
 auto Label=[&](const FString& Text,float Y,int Font,FLinearColor Color){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Text));T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",Font));T->SetColorAndOpacity(Color);T->SetJustification(ETextJustify::Center);auto* P=C->AddChildToCanvas(T);P->SetPosition({60,Y});P->SetSize({1480,180});};
 Label(TEXT("LUST HOUSE CASINO"),150,64,FLinearColor(1,.65f,.15f));Label(TEXT("SUGAR LUST"),345,122,FLinearColor(1,.12f,.48f));Label(TEXT("SWEET WINS. NEON NIGHTS."),590,42,FLinearColor(1,.88f,.7f));
}
ACasinoCabinetDisplay::ACasinoCabinetDisplay()
{
 PrimaryActorTick.bCanEverTick=true;Cabinet=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabinet"));SetRootComponent(Cabinet);
 auto Display=[&](const TCHAR* Name,FVector Pos,FRotator Rot,FVector2D Size,float Scale){auto* W=CreateDefaultSubobject<UWidgetComponent>(Name);W->SetupAttachment(Cabinet);W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(Size);W->SetRelativeLocation(Pos);W->SetRelativeRotation(Rot);W->SetRelativeScale3D(FVector(Scale));W->SetTickWhenOffscreen(true);W->SetTickMode(ETickMode::Enabled);W->SetTwoSided(false);return W;};
 UpperDisplay=Display(TEXT("UpperDisplay"),{0,9.95f,104.72f},{0,90,0},{1920,1080},.0262f);
 LowerDisplay=Display(TEXT("LowerDisplay"),{0,14.15f,72.85f},{15,90,0},{1920,1080},.0262f);
 MarqueeDisplay=Display(TEXT("MarqueeDisplay"),{0,9.95f,138.7f},{0,90,0},{1600,864},.027f);
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("CabinetCamera"));Camera->SetupAttachment(Cabinet);Camera->SetRelativeLocation({0,280,85});Camera->SetRelativeRotation({0,-90,0});Camera->SetFieldOfView(60);
 Interaction=CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("ScreenInteraction"));Interaction->SetupAttachment(Cabinet);Interaction->InteractionSource=EWidgetInteractionSource::Custom;Interaction->InteractionDistance=2000;Interaction->PointerIndex=3;
}
void ACasinoCabinetDisplay::BeginPlay()
{
 Super::BeginPlay();auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC)return;
 PC->bShowMouseCursor=true;
 if (bUseExistingSession && Session)
 {
  if(!Cabinet->GetStaticMesh())Cabinet->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Lounge/HelixDisplayStudy/cabinet_clean/StaticMeshes/cabinet_clean.cabinet_clean")));
  Cabinet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  GameView=CreateWidget<UCasinoCabinetGameView>(PC);
  if(MegaSession)
  {
   PresentationScale=.9f;PresentationFocusZ=40.f;
   Cabinet->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Lounge/MegaPotCabinetFixed/SM_MegaPotCabinetFixed.SM_MegaPotCabinetFixed")));

   // Mesh is centered at its origin; screens are positioned relative to that center.
   UpperDisplay->SetRelativeLocation({0,-4,69});UpperDisplay->SetRelativeRotation({0,90,0});UpperDisplay->SetRelativeScale3D(FVector(.039f,.0415f,.043f));
   LowerDisplay->SetRelativeLocation({0,-2,22});LowerDisplay->SetRelativeRotation({15,90,0});LowerDisplay->SetRelativeScale3D(FVector(.038f));
   // The corrected cabinet already carries the royal artwork on its curved upper screen.
   // Keep that fitted surface instead of covering it with a floating flat poster.
   UpperDisplay->SetVisibility(false);
   auto* RoyalScreen=NewObject<UStaticMeshComponent>(this,TEXT("RoyalScreen"));AddInstanceComponent(RoyalScreen);RoyalScreen->SetupAttachment(Cabinet);
   RoyalScreen->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Lounge/MegaRoyalScreen/royal-screen/StaticMeshes/MegaRoyalScreen.MegaRoyalScreen")));
   RoyalScreen->SetCollisionEnabled(ECollisionEnabled::NoCollision);RoyalScreen->SetCastShadow(false);RoyalScreen->RegisterComponent();
   LowerDisplay->SetWidget(MegaSession);MarqueeDisplay->SetVisibility(false);
  }
  else if(BankCabinetMesh)
  {
   Cabinet->SetStaticMesh(BankCabinetMesh);
   PresentationCenterX=BankCabinetMesh->GetBounds().Origin.X;
   GameView->SetGameCanvas(Session->DesignCanvas);UpperDisplay->SetWidget(GameView);
   Session->DesignCanvas->SetRenderScale({1,1});Session->DesignCanvas->SetRenderTranslation(FVector2D::ZeroVector);
   // Preserve each cabinet's painted upper panels; put the complete interactive game on its game screen.
   LowerDisplay->SetVisibility(false);MarqueeDisplay->SetVisibility(false);
   if(Session->bShowingNeonNightsPreview)
   {
    PresentationScale=.6f;PresentationFocusZ=20.f;
    UpperDisplay->SetRelativeLocation({PresentationCenterX,6,5});UpperDisplay->SetRelativeRotation({28,90,0});UpperDisplay->SetRelativeScale3D({.03f,.0302f,.061f});
   }
   else
   {
    PresentationScale=1.f;PresentationFocusZ=112.f;
    UpperDisplay->SetRelativeLocation({PresentationCenterX,13,96.5f});UpperDisplay->SetRelativeRotation({6,90,0});UpperDisplay->SetRelativeScale3D({.028f,.0285f,.027f});
   }
  }
  else {GameView->SetGameCanvas(Session->DesignCanvas);UpperDisplay->SetWidget(GameView);
  LowerDisplay->SetWidget(Session->CreateCabinetControls());MarqueeDisplay->SetWidget(CreateWidget<UCasinoCabinetMarquee>(PC));
  }
  if(!MegaSession && !BankCabinetMesh && Session->bShowingNeonNightsPreview)
  {
   auto* Banner=MarqueeDisplay->GetUserWidgetObject();
   for(UWidget* Child : Cast<UCanvasPanel>(Banner->WidgetTree->RootWidget)->GetAllChildren())
    if(auto* Text=Cast<UTextBlock>(Child))
     if(Text->GetText().ToString()==TEXT("SUGAR LUST"))Text->SetText(FText::FromString(TEXT("NEON NIGHTS")));
  }
  Cabinet->SetLightingChannels(false,true,false);
  for(FVector P : {FVector(-90,130,190),FVector(100,100,140)})
  {
   auto* Light=NewObject<UPointLightComponent>(this);Light->SetupAttachment(Cabinet);Light->SetRelativeLocation(P);Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetLightingChannels(false,true,false);Light->SetIntensity((MegaSession||BankCabinetMesh)?120:500);Light->SetAttenuationRadius(250);Light->SetCastShadows(false);Light->RegisterComponent();
  }
  Overlay=CreateWidget<UCasinoCabinetOverlay>(PC);Overlay->Host=this;Overlay->AddToViewport(50);bMounted=true;
 }
 else
 {
  PC->SetViewTarget(this);Session=CreateWidget<USugarLustRootWidget>(PC);UpperDisplay->SetWidget(Session);
  if(UFunction* Open=Session->FindFunction(TEXT("HandleOpenSlot")))Session->ProcessEvent(Open,nullptr);
  LowerDisplay->SetWidget(Session->CreateCabinetControls());MarqueeDisplay->SetWidget(CreateWidget<UCasinoCabinetMarquee>(PC));
 }
 FInputModeGameAndUI Input;Input.SetHideCursorDuringCapture(false);PC->SetInputMode(Input);
}
void ACasinoCabinetDisplay::PointAtLowerDisplay(FVector2D UV)
{
 const FVector Local(0,(.5f-UV.X)*1920,(.5f-UV.Y)*1080);
 FHitResult Hit(LowerDisplay->GetOwner(),LowerDisplay,LowerDisplay->GetComponentTransform().TransformPosition(Local),LowerDisplay->GetForwardVector());Hit.bBlockingHit=true;Hit.Distance=1;Hit.TraceStart=Hit.Location+LowerDisplay->GetForwardVector()*10;Hit.TraceEnd=Hit.Location-LowerDisplay->GetForwardVector()*10;Interaction->SetCustomHitResult(Hit);
}
void ACasinoCabinetDisplay::PressDisplay(){Interaction->TickComponent(0.f,LEVELTICK_All,nullptr);Interaction->PressPointerKey(EKeys::LeftMouseButton);}
void ACasinoCabinetDisplay::ReleaseDisplay(){Interaction->ReleasePointerKey(EKeys::LeftMouseButton);}
void ACasinoCabinetDisplay::Tick(float Delta)
{
 Super::Tick(Delta);
 if(bUseExistingSession)
 {
  if(auto* ViewPC=UGameplayStatics::GetPlayerController(this,0))
  {
   FVector Eye;FRotator ViewRotation;ViewPC->GetPlayerViewPoint(Eye,ViewRotation);
   const FRotationMatrix ViewAxes(ViewRotation);const FVector Forward=ViewAxes.GetUnitAxis(EAxis::X),Right=ViewAxes.GetUnitAxis(EAxis::Y),Up=ViewAxes.GetUnitAxis(EAxis::Z);
   int32 W=0,H=0;ViewPC->GetViewportSize(W,H);const float Aspect=H>0?float(W)/H:16.f/9.f;
   const float FOV=ViewPC->PlayerCameraManager?ViewPC->PlayerCameraManager->GetFOVAngle():100.f;
   const float TanH=FMath::Tan(FMath::DegreesToRadians(FOV*.5f)),Depth=75.f;
   const float Scale=PresentationScale*(TanH/Aspect)/.64f;
   SetActorScale3D(FVector(Scale));SetActorRotation(FRotationMatrix::MakeFromYZ(-Forward,Up).Rotator());
   SetActorLocation(Eye+Forward*Depth+Right*(.44f*Depth*TanH-PresentationCenterX*Scale)-Up*(PresentationFocusZ*Scale));
  }
  if(Session && bMounted && !MegaSession)Session->NativeTick(FGeometry::MakeRoot(FVector2D(1920,1080),FSlateLayoutTransform()),Delta);
  return;
 }
 auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC)return;
 float X=0,Y=0;bool Touch=false;PC->GetInputTouchState(ETouchIndex::Touch1,X,Y,Touch);if(!Touch&&!PC->GetMousePosition(X,Y))return;
 FVector Start,Direction;if(PC->DeprojectScreenPositionToWorld(X,Y,Start,Direction))
 {
  FHitResult Hit;GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Direction*2000,ECC_Visibility);Interaction->SetCustomHitResult(Hit);
 }
 if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)||(Touch&&!bWasTouchDown))PressDisplay();
 if(PC->WasInputKeyJustReleased(EKeys::LeftMouseButton)||(!Touch&&bWasTouchDown))ReleaseDisplay();bWasTouchDown=Touch;
}

void ACasinoCabinetDisplay::PointFromCaptureUV(FVector2D UV)
{
 if(!Capture)return;FVector Start,Direction;FHitResult Hit;
 if(UGameplayStatics::DeprojectSceneCaptureComponentToWorld(Capture,UV,Start,Direction))GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Direction*1500,ECC_Visibility);
 Interaction->SetCustomHitResult(Hit);
}
void ACasinoCabinetDisplay::UnmountSession()
{
 if(!bMounted)return;bMounted=false;ReleaseDisplay();
 if(IsValid(Session) && !MegaSession)
 {
  if(GameView)GameView->SetGameCanvas(nullptr);
  if(Session->DesignSizeBox && Session->DesignCanvas)Session->DesignSizeBox->SetContent(Session->DesignCanvas);
  Session->CabinetControls=nullptr;Session->CabinetHost=nullptr;
  if(Session->DesignCanvas){Session->DesignCanvas->SetRenderScale({1,1});Session->DesignCanvas->SetRenderTranslation(FVector2D::ZeroVector);}
 }
 if(Session)Session->CabinetHost=nullptr;
 UpperDisplay->SetWidget(nullptr);LowerDisplay->SetWidget(nullptr);MarqueeDisplay->SetWidget(nullptr);
 if(Overlay){Overlay->Host=nullptr;Overlay->RemoveFromParent();Overlay=nullptr;}
}
void ACasinoCabinetDisplay::EndPlay(const EEndPlayReason::Type Reason){UnmountSession();Super::EndPlay(Reason);}


void ACasinoCabinetDisplay::ClearDisplayPointer(){Interaction->SetCustomHitResult(FHitResult());Interaction->TickComponent(0.f,LEVELTICK_All,nullptr);}

bool ACasinoCabinetDisplay::PointFromViewportPixel(FVector2D Pixel)
{
 auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC)return false;FVector Start,Direction;FHitResult Hit;
 if(PC->DeprojectScreenPositionToWorld(Pixel.X,Pixel.Y,Start,Direction))GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Direction*1500,ECC_Visibility);
 if(Hit.GetComponent()!=UpperDisplay && Hit.GetComponent()!=LowerDisplay && Hit.GetComponent()!=MarqueeDisplay){ClearDisplayPointer();return false;}
 Interaction->SetCustomHitResult(Hit);return true;
}
