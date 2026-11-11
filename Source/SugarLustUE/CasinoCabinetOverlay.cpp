#include "CasinoCabinetOverlay.h"
#include "CasinoCabinetDisplay.h"
#include "SugarLustRootWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/CoreStyle.h"
#include "Blueprint/SlateBlueprintLibrary.h"
void UCasinoCabinetGameView::NativeOnInitialized()
{
 Super::NativeOnInitialized();auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);WidgetTree->RootWidget=Scale;
 ContentBox=WidgetTree->ConstructWidget<USizeBox>();ContentBox->SetWidthOverride(1920);ContentBox->SetHeightOverride(1080);ContentBox->SetClipping(EWidgetClipping::ClipToBoundsAlways);Scale->AddChild(ContentBox);
}
void UCasinoCabinetGameView::SetGameCanvas(UWidget* Content){if(ContentBox)ContentBox->SetContent(Content);}
void UCasinoCabinetOverlay::NativeOnInitialized()
{
 Super::NativeOnInitialized();auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
 auto* InputSurface=WidgetTree->ConstructWidget<UBorder>();InputSurface->SetBrushColor(FLinearColor::Transparent);InputSurface->SetVisibility(ESlateVisibility::Visible);
 auto* InputSlot=Canvas->AddChildToCanvas(InputSurface);InputSlot->SetAnchors(FAnchors(.37f,0,1,1));InputSlot->SetOffsets(FMargin(0));
 auto* Back=WidgetTree->ConstructWidget<UButton>();Back->SetBackgroundColor(FLinearColor(.12f,.015f,.15f));auto* Label=WidgetTree->ConstructWidget<UTextBlock>();Label->SetText(FText::FromString(TEXT("BACK TO BANK")));Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",20));Label->SetColorAndOpacity(FLinearColor(1,.8f,.35f));Back->AddChild(Label);Back->OnClicked.AddDynamic(this,&UCasinoCabinetOverlay::Close);
 auto* BackSlot=Canvas->AddChildToCanvas(Back);BackSlot->SetAnchors(FAnchors(.37f,0));BackSlot->SetPosition({12,12});BackSlot->SetSize({225,55});BackSlot->SetZOrder(10);
}
void UCasinoCabinetOverlay::SetCaptureTexture(UTextureRenderTarget2D*) {}
bool UCasinoCabinetOverlay::Point(const FPointerEvent& Event)
{
 if(!Host)return false;FVector2D Pixel,Viewport;
 USlateBlueprintLibrary::AbsoluteToViewport(GetOwningPlayer(),Event.GetScreenSpacePosition(),Pixel,Viewport);
 return Host->PointFromViewportPixel(Pixel);
}
FReply UCasinoCabinetOverlay::NativeOnMouseMove(const FGeometry&,const FPointerEvent& E){return Point(E)?FReply::Handled():FReply::Unhandled();}
FReply UCasinoCabinetOverlay::NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent& E)
{
 if(E.GetEffectingButton()!=EKeys::LeftMouseButton||!Point(E))return FReply::Unhandled();Host->PressDisplay();bPointerDown=true;return FReply::Handled().CaptureMouse(TakeWidget());
}
FReply UCasinoCabinetOverlay::NativeOnMouseButtonUp(const FGeometry&,const FPointerEvent& E)
{
 if(!bPointerDown)return FReply::Unhandled();Point(E);if(Host)Host->ReleaseDisplay();bPointerDown=false;return FReply::Handled().ReleaseMouseCapture();
}
FReply UCasinoCabinetOverlay::NativeOnTouchStarted(const FGeometry& G,const FPointerEvent& E){if(!Point(E))return FReply::Unhandled();Host->PressDisplay();bPointerDown=true;return FReply::Handled().CaptureMouse(TakeWidget());}
FReply UCasinoCabinetOverlay::NativeOnTouchMoved(const FGeometry& G,const FPointerEvent& E){return NativeOnMouseMove(G,E);}
FReply UCasinoCabinetOverlay::NativeOnTouchEnded(const FGeometry& G,const FPointerEvent& E){return NativeOnMouseButtonUp(G,E);}
void UCasinoCabinetOverlay::Close(){if(Host&&Host->Session)Host->Session->CloseCabinetView();}

