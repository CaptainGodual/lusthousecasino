#include "CasinoCabinetControls.h"
#include "SugarLustRootWidget.h"
#include "SugarLustSlotMachine.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
void UCasinoCabinetControls::NativeOnInitialized()
{
 Super::NativeOnInitialized();
 auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);
 auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(1920);Size->SetHeightOverride(1080);Scale->AddChild(Size);
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->AddChild(Canvas);WidgetTree->RootWidget=Scale;
 auto Place=[Canvas](UWidget* W,float X,float Y,float Width,float Height){auto* S=Canvas->AddChildToCanvas(W);S->SetPosition({X,Y});S->SetSize({Width,Height});};
 auto Panel=[&](float X,float Y,float W,float H,FLinearColor Color){auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrushColor(Color);Place(B,X,Y,W,H);};
 const FLinearColor Gold(1,0.67f,0.16f),Pink(0.05f,0.6f,1),Dark(0.012f,0.025f,0.05f),White(1,0.95f,0.83f);
 Panel(0,0,1920,1080,Dark);Panel(0,0,1920,12,Pink);Panel(0,1068,1920,12,Gold);
 auto Text=[&](const FString& Name,const FString& Value,float X,float Y,float W,float H,int Font,FLinearColor Color){auto* T=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),*Name);T->SetText(FText::FromString(Value));T->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold",Font)));T->SetColorAndOpacity(Color);T->SetJustification(ETextJustify::Center);T->SetVisibility(ESlateVisibility::HitTestInvisible);Place(T,X,Y,W,H);return T;};
 Title=Text("DeckTitle","SUGAR LUST",60,38,1800,72,46,Gold);
 Text("BalanceLabel","COIN BALANCE",70,160,550,60,29,White);Text("WagerLabel","TOTAL BET",685,160,550,60,29,White);Text("WinLabel","LAST WIN",1300,160,550,60,29,White);
 Balance=Text("DeckBalance","0",70,220,550,90,52,Gold);Wager=Text("DeckWager","0",685,220,550,90,52,White);Win=Text("DeckWin","0",1300,220,550,90,52,Gold);
 auto Button=[&](const FString& Name,const FString& Label,float X,float Y,float W,float H,FLinearColor Color,UTextBlock** Out=nullptr){auto* B=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),*Name);FButtonStyle Style=B->GetStyle();
 Style.SetNormal(FSlateRoundedBoxBrush(Color,22.f,Gold,5.f));Style.SetHovered(FSlateRoundedBoxBrush(Color*1.2f,22.f,FLinearColor::White,6.f));Style.SetPressed(FSlateRoundedBoxBrush(Color*.65f,22.f,Gold,3.f));B->SetStyle(Style);auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Label));T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",40));T->SetColorAndOpacity(White);T->SetJustification(ETextJustify::Center);B->AddChild(T);Place(B,X,Y,W,H);if(Out)*Out=T;return B;};
 Plus=Button("DeckBetPlus","BET +",70,365,300,165,FLinearColor(0.2f,0.13f,0.035f));Minus=Button("DeckBetMinus","BET -",70,565,300,165,FLinearColor(0.2f,0.13f,0.035f));Max=Button("DeckMaxBet","MAX BET",410,365,330,365,FLinearColor(0.35f,0.22f,0.04f));
 Spin=Button("DeckSpin","SPIN",800,365,550,365,FLinearColor(0.7f,0.4f,0.045f));UTextBlock* A=nullptr;Auto=Button("DeckAuto","AUTO: OFF",1370,365,480,365,FLinearColor(0.015f,0.18f,0.4f),&A);AutoLabel=A;
 Minus->OnClicked.AddDynamic(this,&UCasinoCabinetControls::BetDown);Plus->OnClicked.AddDynamic(this,&UCasinoCabinetControls::BetUp);Max->OnClicked.AddDynamic(this,&UCasinoCabinetControls::MaxBet);Spin->OnClicked.AddDynamic(this,&UCasinoCabinetControls::DoSpin);Auto->OnClicked.AddDynamic(this,&UCasinoCabinetControls::DoAuto);
 Status=Text("DeckStatus","",70,755,1780,65,28,Gold);
 Text("GroupEventStatus","GROUP BONUS  |  NO EVENT ACTIVE",70,865,1780,60,30,White);
 auto* Progress=WidgetTree->ConstructWidget<UProgressBar>();Progress->SetPercent(0);Progress->SetFillColorAndOpacity(Pink);Place(Progress,200,960,1520,24);
}
void UCasinoCabinetControls::NativeTick(const FGeometry& G,float Delta)
{
 Super::NativeTick(G,Delta);if(!Session||!Session->SlotGame)return;auto* Game=Session->SlotGame;
 if(Session->bShowingNeonNightsPreview)
 {
  Title->SetText(FText::FromString(TEXT("NEON NIGHTS")));
  Balance->SetText(FText::AsNumber(Session->NeonCoins));Wager->SetText(FText::AsNumber(static_cast<int64>(Session->NeonBet)*5));Win->SetText(FText::AsNumber(Session->NeonDisplayedWin));
  const bool Change=Session->NeonBetUpButton&&Session->NeonBetUpButton->GetIsEnabled();Minus->SetIsEnabled(Change);Plus->SetIsEnabled(Change);Max->SetIsEnabled(Change);
  Spin->SetIsEnabled(Session->NeonSpinButton&&Session->NeonSpinButton->GetIsEnabled());Auto->SetIsEnabled(Session->NeonAutoSpinButton&&Session->NeonAutoSpinButton->GetIsEnabled());
  AutoLabel->SetText(FText::FromString(Session->NeonAutoSpinsRemaining>0?FString::Printf(TEXT("STOP AUTO (%d)"),Session->NeonAutoSpinsRemaining):TEXT("AUTO: OFF")));
  Status->SetText(!Session->bPlayFabProfileReady?FText::FromString(TEXT("CONNECTING TO CASINO...")):Session->NeonStatusText?Session->NeonStatusText->GetText():FText::FromString(TEXT("1,024 WAYS | SELECT YOUR BET")));
  return;
 }
 Title->SetText(FText::FromString(TEXT("SUGAR LUST")));
 Balance->SetText(FText::AsNumber(Game->GetCoins()));Wager->SetText(FText::AsNumber(Game->GetTotalBet()));Win->SetText(FText::AsNumber(Game->GetLastWin()));
 const bool Change=Session->BetPlusButton&&Session->BetPlusButton->GetIsEnabled();Minus->SetIsEnabled(Change);Plus->SetIsEnabled(Change);Max->SetIsEnabled(Change);
 const bool Ready=Session->bPlayFabProfileReady&&!Session->bPlayFabRequestInFlight&&!Game->IsSpinning()&&!Game->IsBonusRoundActive()&&!Game->IsBonusResultReady();
 Spin->SetIsEnabled(Session->SpinButton&&Session->SpinButton->GetIsEnabled());Auto->SetIsEnabled(Session->AutoSpinsRemaining>0||(Ready&&Game->CanSpin()));
 AutoLabel->SetText(FText::FromString(Session->AutoSpinsRemaining>0?FString::Printf(TEXT("STOP AUTO (%d)"),Session->AutoSpinsRemaining):TEXT("AUTO: OFF")));
 Status->SetText(FText::FromString(!Session->SugarSpinError.IsEmpty()?Session->SugarSpinError:!Session->bPlayFabProfileReady?TEXT("CONNECTING TO CASINO..."):Session->AutoSpinsRemaining>0?TEXT("AUTO SPIN ACTIVE"):Game->IsSpinning()?TEXT("REELS SPINNING..."):TEXT("20 PAYLINES  |  SELECT YOUR BET")));
}
void UCasinoCabinetControls::BetDown(){if(Session&&Minus->GetIsEnabled()){if(Session->bShowingNeonNightsPreview)Session->HandleNeonBetDown();else Session->HandleBetDown();}}
void UCasinoCabinetControls::BetUp(){if(Session&&Plus->GetIsEnabled()){if(Session->bShowingNeonNightsPreview)Session->HandleNeonBetUp();else Session->HandleBetUp();}}
void UCasinoCabinetControls::MaxBet(){if(Session&&Max->GetIsEnabled()){if(Session->bShowingNeonNightsPreview)Session->HandleNeonMaxBet();else Session->HandleMaxBet();}}
void UCasinoCabinetControls::DoSpin(){if(Session&&Spin->GetIsEnabled()){if(Session->bShowingNeonNightsPreview)Session->HandleNeonSpin();else Session->HandleSpin();}}
void UCasinoCabinetControls::DoAuto(){if(Session&&Auto->GetIsEnabled()){if(Session->bShowingNeonNightsPreview)Session->HandleNeonAutoSpin();else Session->HandleAutoSpin();}}

FString UCasinoCabinetControls::GetDisplayedWager() const { return Wager ? Wager->GetText().ToString() : FString(); }
