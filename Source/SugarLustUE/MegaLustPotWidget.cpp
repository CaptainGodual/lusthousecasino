#include "MegaLustPotWidget.h"
#include "SugarLustRootWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/ButtonSlot.h"
#include "Engine/Texture2D.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
namespace MegaStyle {const FLinearColor Gold(1,.66f,.18f),Ivory(1,.92f,.75f),Ruby(.35f,.008f,.045f),Black(.012f,.005f,.012f);}
void UMegaLustPotWidget::Place(UWidget* W,float X,float Y,float Width,float Height){auto* S=Canvas->AddChildToCanvas(W);S->SetPosition({X,Y});S->SetSize({Width,Height});}
UTextBlock* UMegaLustPotWidget::Text(const FString& V,float X,float Y,float W,float H,int32 Font,FLinearColor Color){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(V));T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",Font));T->SetColorAndOpacity(Color);T->SetJustification(ETextJustify::Center);T->SetVisibility(ESlateVisibility::HitTestInvisible);Place(T,X,Y,W,H);return T;}
FSlateBrush UMegaLustPotWidget::AtlasBrush(float X,float Y,float W,float H) const
{
 FSlateBrush B;B.SetResourceObject(Atlas);B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(W,H);B.SetUVRegion(FBox2f(FVector2f(X/1448.f,Y/1086.f),FVector2f((X+W)/1448.f,(Y+H)/1086.f)));return B;
}
UImage* UMegaLustPotWidget::Art(float SX,float SY,float SW,float SH,float X,float Y,float W,float H)
{
 auto* I=WidgetTree->ConstructWidget<UImage>();I->SetBrush(AtlasBrush(SX,SY,SW,SH));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(I,X,Y,W,H);return I;
}
UButton* UMegaLustPotWidget::Button(const FString& V,float X,float Y,float W,float H,TObjectPtr<UTextBlock>* Label)
{
 auto* B=WidgetTree->ConstructWidget<UButton>();FButtonStyle Style=B->GetStyle();
 Style.SetNormal(FSlateRoundedBoxBrush(MegaStyle::Ruby,16.f,MegaStyle::Gold,2.f));Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.55f,.02f,.07f),16.f,MegaStyle::Ivory,3.f));Style.SetPressed(FSlateRoundedBoxBrush(MegaStyle::Black,16.f,MegaStyle::Gold,3.f));
 FVector4 R(0,0,0,0);
 if(V==TEXT("SPIN"))R=FVector4(1065,410,365,265);
 else if(V==TEXT("BET −"))R=FVector4(1040,690,145,125);
 else if(V==TEXT("BET +"))R=FVector4(1180,690,130,125);
 else if(V==TEXT("MAX BET"))R=FVector4(1303,690,145,125);
 else if(V==TEXT("BACK TO LOBBY"))R=FVector4(1080,816,165,135);
 else if(V==TEXT("RULES / PAYS"))R=FVector4(1240,816,175,135);
 const bool Painted=Atlas&&R.Z>0;
 if(Painted){auto Brush=AtlasBrush(R.X,R.Y,R.Z,R.W);Style.SetNormal(Brush);Brush.TintColor=FLinearColor(1.13f,1.13f,1.13f,1);Style.SetHovered(Brush);Brush.TintColor=FLinearColor(.7f,.7f,.7f,1);Style.SetPressed(Brush);Brush.TintColor=FLinearColor(.4f,.4f,.4f,1);Style.SetDisabled(Brush);}
 B->SetStyle(Style);B->SetToolTipText(FText::FromString(V));auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(V));T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",22));T->SetColorAndOpacity(MegaStyle::Ivory);T->SetJustification(ETextJustify::Center);
 if(Painted&&V==TEXT("SPIN")){auto* Face=WidgetTree->ConstructWidget<UBorder>();Face->SetBrush(FSlateRoundedBoxBrush(MegaStyle::Ruby,14.f,MegaStyle::Gold,1.f));Face->SetContent(T);Face->SetHorizontalAlignment(HAlign_Center);Face->SetVerticalAlignment(VAlign_Center);auto* S=WidgetTree->ConstructWidget<USizeBox>();S->SetWidthOverride(220);S->SetHeightOverride(70);S->SetContent(Face);SpinActionBadge=S;B->SetContent(S);auto* ContentSlot=Cast<UButtonSlot>(S->Slot);ContentSlot->SetHorizontalAlignment(HAlign_Center);ContentSlot->SetVerticalAlignment(VAlign_Center);T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",28));}
 else {B->AddChild(T);if(Painted)T->SetVisibility(ESlateVisibility::Hidden);}
 Place(B,X,Y,W,H);if(Label)*Label=T;return B;
}
void UMegaLustPotWidget::NativeOnInitialized()
{
 Super::NativeOnInitialized();using namespace MegaStyle;
 SymbolAtlas=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/MegaPot/UI/T_RoyalLust_Symbols.T_RoyalLust_Symbols"));
 Atlas=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/MegaPot/UI/T_RoyalLust_UIAtlas.T_RoyalLust_UIAtlas"));
 auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(1920);Size->SetHeightOverride(1080);Scale->AddChild(Size);Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->AddChild(Canvas);WidgetTree->RootWidget=Scale;
 auto Panel=[&](float X,float Y,float W,float H,FLinearColor C,float Radius=20.f){auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrush(FSlateRoundedBoxBrush(C,Radius,Gold,2.f));Place(B,X,Y,W,H);return B;};
 Panel(0,0,1920,1080,Black,0);
 // Keep the full background visible without stretching its original proportions.
 if(auto* Background=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/MegaPot/UI/T_RoyalLust_Background.T_RoyalLust_Background")))
 {
  auto* Backdrop=WidgetTree->ConstructWidget<UImage>();Backdrop->SetBrushFromTexture(Background);Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
  const float Width=1080.f*static_cast<float>(Background->GetSizeX())/FMath::Max(1,Background->GetSizeY());Place(Backdrop,(1920.f-Width)*.5f,0,Width,1080);
  auto* Scrim=WidgetTree->ConstructWidget<UBorder>();Scrim->SetBrushColor(FLinearColor(.025f,.002f,.012f,.48f));Scrim->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Scrim,0,0,1920,1080);
 }
 else Panel(35,22,1850,1036,FLinearColor(.035f,.002f,.012f));
 if(Atlas)Art(30,25,990,353,650,0,620,221);else Text(TEXT("MEGA LUST POT • ROYAL LUST"),360,40,1200,100,48,Gold);
 Button(TEXT("BACK TO LOBBY"),65,45,200,155)->OnClicked.AddDynamic(this,&UMegaLustPotWidget::Close);
 Button(TEXT("RULES / PAYS"),1645,45,200,155)->OnClicked.AddDynamic(this,&UMegaLustPotWidget::ToggleHelp);
 ModeLabel=Text(TEXT("PRACTICE BUILD • Separate credits • Art preview"),340,213,1240,30,18,FLinearColor(.8f,.68f,.55f));
 const FVector4 SourcePots[]={FVector4(35,378,310,272),FVector4(350,375,325,275),FVector4(677,375,348,275)};
 const TCHAR* Desc[]={TEXT("12 crests → 8 extra-wild free spins"),TEXT("16 royal crests → 3 spins at 2x–10x"),TEXT("24 hearts OR 6 in one spin → HOLD")};
 for(int32 I=0;I<3;++I){float X=500+I*340;auto R=SourcePots[I];if(Atlas)Art(R.X,R.Y,R.Z,R.W,X,243,240,190);else Panel(X,243,240,190,Ruby);PotLabels.Add(Text(TEXT(""),X+20,389,200,26,18,Ivory));auto* P=WidgetTree->ConstructWidget<UProgressBar>();P->SetFillColorAndOpacity(Gold);Place(P,X+20,421,200,6);Pots.Add(P);Text(Desc[I],X-30,431,300,28,14,Ivory);}
 if(Atlas)Art(20,655,660,373,460,454,1000,550);else Panel(460,454,1000,550,Gold,14);
 for(int32 Row=0;Row<3;++Row)for(int32 Reel=0;Reel<5;++Reel)
 {
  float X=526.7f+Reel*176.4f,Y=526.3f+Row*138.6f;Cells.Add(Panel(X+4,Y+4,168,130,FLinearColor(0,0,0,.2f),2));
  auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);auto* Image=WidgetTree->ConstructWidget<UImage>();Image->SetVisibility(ESlateVisibility::HitTestInvisible);Fit->SetContent(Image);Fit->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Fit,X+10,Y+5,156,122);SymbolImages.Add(Image);
  SymbolValueBackers.Add(Panel(X+15,Y+91,146,32,FLinearColor(.025f,.005f,.012f,.92f),8));
  auto* Label=Text(TEXT(""),X+15,Y+93,146,36,20,Ivory);Label->SetShadowColorAndOpacity(FLinearColor::Black);Label->SetShadowOffset({1,1});Symbols.Add(Label);
 }
 Text(TEXT("FIXED HOLD FEATURE AWARDS\nMINI 25x • MAJOR 100x\nMEGA 1,000x TOTAL BET"),65,755,315,95,18,Gold);
 Features=Text(TEXT(""),390,1000,1120,30,17,Ivory);
 Wallet=Text(TEXT(""),0,0,1,1,1,Gold);Wallet->SetVisibility(ESlateVisibility::Collapsed);
 if(Atlas){Art(1050,73,390,115,1510,225,330,97);Art(1050,187,390,101,1510,337,330,86);Art(1050,289,390,108,1510,441,330,91);}
 else {Panel(1510,225,330,97,Black);Panel(1510,337,330,86,Black);Panel(1510,441,330,91,Black);}
 BalanceValue=Text(TEXT(""),1545,263,260,45,30,Ivory);BetValue=Text(TEXT(""),1545,367,260,45,30,Ivory);WinValue=Text(TEXT(""),1545,475,260,45,30,Ivory);
 Button(TEXT("BET −"),1505,845,110,95)->OnClicked.AddDynamic(this,&UMegaLustPotWidget::BetDown);Button(TEXT("BET +"),1620,845,110,95)->OnClicked.AddDynamic(this,&UMegaLustPotWidget::BetUp);Button(TEXT("MAX BET"),1735,845,110,95)->OnClicked.AddDynamic(this,&UMegaLustPotWidget::MaxBet);
 SpinButton=Button(TEXT("SPIN"),1510,565,330,260,&SpinLabel);SpinButton->OnClicked.AddDynamic(this,&UMegaLustPotWidget::Spin);Button(TEXT("AUTO: OFF"),1510,950,330,60,&AutoLabel)->OnClicked.AddDynamic(this,&UMegaLustPotWidget::ToggleAuto);
 auto* DemoCrimson=Button(TEXT("TRY CRIMSON"),65,392,235,58);DemoCrimson->OnClicked.AddDynamic(this,&UMegaLustPotWidget::TryCrimson);PracticeOnly.Add(DemoCrimson);
 auto* DemoRoyal=Button(TEXT("TRY ROYAL"),65,466,235,58);DemoRoyal->OnClicked.AddDynamic(this,&UMegaLustPotWidget::TryRoyal);PracticeOnly.Add(DemoRoyal);
 auto* DemoMega=Button(TEXT("TRY MEGA"),65,540,235,58);DemoMega->OnClicked.AddDynamic(this,&UMegaLustPotWidget::TryMega);PracticeOnly.Add(DemoMega);
 PracticeOnly.Add(Text(TEXT("GUIDED FEATURE\nPREVIEWS\nPractice only"),65,625,235,120,18,Gold));
 auto* Reset=Button(TEXT("RESET PRACTICE"),65,940,315,58);Reset->OnClicked.AddDynamic(this,&UMegaLustPotWidget::ResetPractice);PracticeOnly.Add(Reset);
 if(Atlas){Art(684,700,355,86,75,850,300,72);Art(683,803,358,210,75,215,300,176);}
 Status=Text(TEXT(""),120,1036,1680,30,16,Ivory);Status->SetAutoWrapText(true);
 Help=Text(TEXT("20 PAYLINES • LEFT TO RIGHT • 3+ MATCHES\nQueen Wild substitutes for regular symbols. Pot symbols do not pay on lines.\n3/4/5 matches in line-bet multiples:\nCrown 10/30/100 • Heart 8/25/80 • Ring 7/20/60 • Seven 6/18/50\nChalice 5/15/40 • Gold 4/12/30 • Rose 4/10/25 • Key 3/8/20\nA,K,Q 2/5/15 • J,10 1/4/10 • Queen Wild 15/50/150\n\nPot counters are real practice-session progress, shared across bet levels.\nCrimson awards 8 free spins with extra Wilds. Royal awards 3 multiplier spins.\nSix Heart Coins or 24 accumulated hearts start Hold & Respin with 6+ locked coins.\nThree respins. New coins reset to three. Collectors increase the final multiplier (max 10x).\n9+ hearts: Mini 25x • 12+ hearts: Major 100x • all 15: Mega 1,000x total bet.\nOnly the highest tier is awarded, plus locked coins × collector multiplier.\nFeatures lock the current bet. Queued free/multiplier spins resume after collection.\n\nPrototype credits reset when the application restarts. No live coins, gems or rewards.\nFrequencies and payouts are provisional; no RTP target has been approved."),170,215,1580,660,24,Ivory);
 auto* Shade=WidgetTree->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(.018f,.003f,.01f,1));Shade->SetContent(Help); // Move help into an opaque, dismissible-by-rules overlay.
 Place(Shade,145,205,1630,680);Shade->SetVisibility(ESlateVisibility::Collapsed);Shade->SetPadding(FMargin(28));Help->SetAutoWrapText(true);Help->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",23));Help->SetVisibility(ESlateVisibility::HitTestInvisible);
 Refresh();
}
void UMegaLustPotWidget::Refresh()
{
 using namespace MegaStyle;if(bLive)UpdateLivePots();const bool Hold=Game.Mode!=FMegaLustPotGame::Phase::Base;
 for(int32 I=0;I<15;++I)
 {
  PaintSymbol(I,Game.Grid[I],Hold);
  const bool Winning=!Hold&&Game.WinningCells.Contains(I);FLinearColor C=Hold&&Game.Locked[I]>0?FLinearColor(.45f,.19f,.015f,.35f):FLinearColor(.012f,.005f,.012f,.08f);Cells[I]->SetBrush(FSlateRoundedBoxBrush(C,10.f,Winning?Ivory:Gold,Winning?5.f:0.f));
 }
 int32 Charge[]={Game.CrimsonCharge,Game.RoyalCharge,Game.MegaCharge},Goal[]={12,16,24};
 for(int32 I=0;I<3;++I){Pots[I]->SetPercent(static_cast<float>(Charge[I])/Goal[I]);PotLabels[I]->SetText(FText::FromString(FString::Printf(TEXT("%d / %d collected"),Charge[I],Goal[I])));}
 Wallet->SetText(FText::FromString(FString::Printf(TEXT("%s  %s    |    TOTAL BET  %d    |    WIN  %s"),bLive?TEXT("COINS"):TEXT("PRACTICE"),*FText::AsNumber(Game.Credits).ToString(),Game.Bet*20,*FText::AsNumber(Game.LastWin).ToString())));
 BalanceValue->SetText(FText::AsNumber(Game.Credits));BetValue->SetText(FText::AsNumber(Game.Bet*20));WinValue->SetText(FText::AsNumber(Game.LastWin));
 Features->SetText(FText::FromString(Hold?FString::Printf(TEXT("HEARTS %d / 15    •    RESPINS %d    •    COLLECTOR %dx    •    LOCKED %s"),Game.LockedCount(),Game.Respins,Game.CollectorMultiplier,*FText::AsNumber(Game.LockedTotal()).ToString()):FString::Printf(TEXT("20 PAYLINES    •    CRIMSON FREE SPINS %d    •    ROYAL MULTIPLIER SPINS %d"),Game.FreeSpins,Game.RoyalSpins)));
 if(SpinActionBadge)SpinActionBadge->SetVisibility(Game.Mode==FMegaLustPotGame::Phase::Base&&Game.FreeSpins==0?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
 SpinLabel->SetText(FText::FromString(Game.Mode==FMegaLustPotGame::Phase::Hold?TEXT("RESPIN"):Game.Mode==FMegaLustPotGame::Phase::Result?TEXT("COLLECT"):Game.FreeSpins>0?TEXT("FREE SPIN"):TEXT("SPIN")));SpinButton->SetIsEnabled(Animation<=0&&(!bLive||(!bBusy&&bReady)));AutoLabel->SetText(FText::FromString(Auto?TEXT("AUTO: ON"):TEXT("AUTO: OFF")));Status->SetText(FText::FromString(Game.Message));
}
void UMegaLustPotWidget::Spin(){if(Animation>0||ShowingHelp)return;if(bLive){SpinLive();return;}if(Game.Mode==FMegaLustPotGame::Phase::Result){Game.Collect();Refresh();return;}Animation=Game.Mode==FMegaLustPotGame::Phase::Hold?2.5f:3.5f;AutoDelay=1.8f;Game.Message=Game.Mode==FMegaLustPotGame::Phase::Hold?TEXT("Golden hearts falling..."):TEXT("The royal reels are spinning...");Refresh();}
void UMegaLustPotWidget::NativeTick(const FGeometry& G,float D)
{
 Super::NativeTick(G,D);Clock+=D;
 if(bLive){Animation=FMath::Max(0.f,Animation-D);if(PendingPresentation&&Animation<=0)FinishLiveSpin();if(bBusy&&Game.Mode==FMegaLustPotGame::Phase::Base){for(int32 I=0;I<15;++I)PaintSymbol(I,(static_cast<int32>(Clock*12)+I*3)%FMegaLustPotGame::SymbolCount);}if(Auto&&!bBusy&&bReady){AutoDelay-=D;if(AutoDelay<=0)SpinLive();}return;}
 if(Animation>0){Animation-=D;if(Game.Mode==FMegaLustPotGame::Phase::Base){for(int32 I=0;I<15;++I)PaintSymbol(I,(static_cast<int32>(Clock*12)+I*3)%FMegaLustPotGame::SymbolCount);}
 if(Animation<=0){bool Ok=true;if(Game.Mode==FMegaLustPotGame::Phase::Hold)Game.Respin();else if(Tour){TArray<int32> Grid;for(int32 I=0;I<15;++I)Grid.Add(I%13);if(Tour==1){Game.CrimsonCharge=11;Grid[0]=FMegaLustPotGame::Crimson;}if(Tour==2){Game.RoyalCharge=15;Grid[0]=FMegaLustPotGame::Royal;}if(Tour==3)for(int32 I=0;I<6;++I)Grid[I]=FMegaLustPotGame::HeartCoin;Ok=Game.ResolveBase(Grid);Tour=0;}else Ok=Game.Spin();if(!Ok){Auto=false;Game.Message=TEXT("Not enough practice credits for this bet.");}if(Game.Mode==FMegaLustPotGame::Phase::Result)Auto=false;Refresh();}}
 else if(Auto){AutoDelay-=D;if(AutoDelay<=0)Spin();}
}
void UMegaLustPotWidget::BetDown(){if(Animation<=0&&!ShowingHelp&&!bBusy&&(!bLive||bReady))Game.SetBet(FMath::Max(1,Game.Bet-1));Refresh();}
void UMegaLustPotWidget::BetUp(){if(Animation<=0&&!ShowingHelp&&!bBusy&&(!bLive||bReady))Game.SetBet(FMath::Min(100,Game.Bet+1));Refresh();}
void UMegaLustPotWidget::MaxBet(){if(Animation<=0&&!ShowingHelp&&!bBusy&&(!bLive||bReady))Game.SetBet(FMath::Min<int64>(100,Game.Credits/20));Refresh();}
void UMegaLustPotWidget::ToggleAuto(){if(ShowingHelp||(bLive&&(!bReady||bBusy)))return;Auto=!Auto;AutoDelay=.2f;Refresh();}
void UMegaLustPotWidget::ToggleHelp(){ShowingHelp=!ShowingHelp;if(auto* P=Help->GetParent())P->SetVisibility(ShowingHelp?ESlateVisibility::Visible:ESlateVisibility::Collapsed);if(ShowingHelp)Auto=false;Refresh();}
void UMegaLustPotWidget::StartTour(int32 K){if(bLive||Animation>0||ShowingHelp||!Game.CanChangeBet())return;Auto=false;Tour=K;Spin();}
void UMegaLustPotWidget::TryCrimson(){StartTour(1);}void UMegaLustPotWidget::TryRoyal(){StartTour(2);}void UMegaLustPotWidget::TryMega(){StartTour(3);}
FString UMegaLustPotWidget::GetPhase()const{return Game.Mode==FMegaLustPotGame::Phase::Hold?TEXT("Hold"):Game.Mode==FMegaLustPotGame::Phase::Result?TEXT("Result"):TEXT("Base");}
void UMegaLustPotWidget::Close(){if(Animation>0||bBusy)return;Auto=false;if(bCabinetLayout){if(Lobby)Lobby->CloseCabinetView();return;}RemoveFromParent();if(Lobby)Lobby->SetVisibility(ESlateVisibility::Visible);}


void UMegaLustPotWidget::ResetPractice(){if(bLive||Animation>0||ShowingHelp)return;Auto=false;Tour=0;Game=FMegaLustPotGame(FMath::Rand());Refresh();}


void UMegaLustPotWidget::PaintSymbol(int32 Cell,int32 Symbol,bool Hold)
{
 if(!Symbols.IsValidIndex(Cell))return;
 const bool Coin=Hold&&Game.Locked.IsValidIndex(Cell)&&Game.Locked[Cell]>0;
 SymbolValueBackers[Cell]->SetVisibility(Coin?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
 if(Hold&&!Coin){SymbolImages[Cell]->SetVisibility(ESlateVisibility::Collapsed);Symbols[Cell]->SetVisibility(ESlateVisibility::Collapsed);return;}
 if(Hold)Symbol=FMegaLustPotGame::HeartCoin;
 if(SymbolAtlas&&Symbol>=0&&Symbol<FMegaLustPotGame::SymbolCount)
 {
  // Enum order: Crown, Ruby Heart, Ring, Seven, Chalice, Gold, Rose, Key,
  // A,K,Q,J,10, Queen Wild, Crimson Pot, Royal Pot, Golden Heart Coin.
  static const FVector4 Crops[]={
   FVector4(280,0,250,296),FVector4(744,0,245,296),FVector4(530,0,214,296),FVector4(1225,0,223,296),FVector4(989,0,236,296),
   FVector4(0,296,246,228),FVector4(246,296,246,228),FVector4(492,296,216,228),
   FVector4(285,524,245,234),FVector4(530,524,218,234),FVector4(748,524,232,234),FVector4(980,524,200,234),FVector4(1180,524,268,234),
   FVector4(0,0,280,296),FVector4(934,296,233,228),FVector4(1167,296,281,228),FVector4(708,296,226,228)};
  const FVector4 R=Crops[Symbol];FSlateBrush B;B.SetResourceObject(SymbolAtlas);B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(R.Z,R.W);B.SetUVRegion(FBox2f(FVector2f(R.X/1448.f,R.Y/1086.f),FVector2f((R.X+R.Z)/1448.f,(R.Y+R.W)/1086.f)));SymbolImages[Cell]->SetBrush(B);SymbolImages[Cell]->SetVisibility(ESlateVisibility::HitTestInvisible);
  Symbols[Cell]->SetVisibility(Coin||Symbol==FMegaLustPotGame::Wild?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
  if(Symbol==FMegaLustPotGame::Wild)SymbolValueBackers[Cell]->SetVisibility(ESlateVisibility::HitTestInvisible);
  Symbols[Cell]->SetText(Coin?FText::AsNumber(Game.Locked[Cell]):FText::FromString(TEXT("WILD")));
 }
 else {SymbolImages[Cell]->SetVisibility(ESlateVisibility::Collapsed);Symbols[Cell]->SetVisibility(ESlateVisibility::HitTestInvisible);Symbols[Cell]->SetText(Coin?FText::AsNumber(Game.Locked[Cell]):FText::FromString(FMegaLustPotGame::Name(Symbol)));}
}

bool UMegaLustPotWidget::PrepareToLeaveCabinet(){if(Animation>0||bBusy)return false;Auto=false;Refresh();return true;}
void UMegaLustPotWidget::SetCabinetLayout()
{
 if(bCabinetLayout)return;bCabinetLayout=true;
 for(UWidget* Child:Canvas->GetAllChildren())
 {
  auto* S=Cast<UCanvasPanelSlot>(Child->Slot);if(!S)continue;FVector2D P=S->GetPosition(),Z=S->GetSize();
  if(FMath::IsNearlyEqual(P.Y,431.f)){Child->SetVisibility(ESlateVisibility::Collapsed);}
  else if(Child->GetFName()==TEXT("MegaSync")){S->SetPosition({1510,130});S->SetSize({330,65});}
  else if(Child->GetToolTipText().ToString()==TEXT("RULES / PAYS")){S->SetPosition({1510,15});S->SetSize({140,100});}
  else if(P.X>=460 && P.X<1465 && P.Y>=454 && P.Y<1000){S->SetPosition({30+(P.X-460)*1.35f,200+(P.Y-454)*1.35f});S->SetSize(Z*1.35f);}
  else if(P.X>=500 && P.X<1460 && P.Y>=243 && P.Y<454){S->SetPosition({120+(P.X-500)*1.32f,(P.Y-243)*.9f});S->SetSize(Z*.9f);}
  else if(Child==Features){S->SetPosition({20,980});S->SetSize({1430,40});}
  else if(P.X<400 && Z.X<500 && Child!=Help && Child!=Status)Child->SetVisibility(ESlateVisibility::Collapsed);
  else if(P.Y<225 && Z.X<1700 && Child!=Help)Child->SetVisibility(ESlateVisibility::Collapsed);
 }
}
