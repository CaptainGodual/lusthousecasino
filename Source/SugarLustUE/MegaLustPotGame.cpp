#include "MegaLustPotGame.h"
const int32 FMegaLustPotGame::Paylines[20][5]={
 {1,1,1,1,1},{0,0,0,0,0},{2,2,2,2,2},{0,1,2,1,0},{2,1,0,1,2},
 {0,0,1,0,0},{2,2,1,2,2},{1,0,0,0,1},{1,2,2,2,1},{0,1,1,1,0},
 {2,1,1,1,2},{1,0,1,0,1},{1,2,1,2,1},{0,2,0,2,0},{2,0,2,0,2},
 {0,1,0,1,0},{2,1,2,1,2},{0,2,2,2,0},{2,0,0,0,2},{1,1,0,1,1}};
FMegaLustPotGame::FMegaLustPotGame(int32 Seed):Rng(Seed){Grid.Init(0,15);Locked.Init(0,15);for(int32& S:Grid)S=Rng.RandRange(0,12);}
FString FMegaLustPotGame::Name(int32 S){static const TCHAR* N[]={TEXT("CROWN"),TEXT("RUBY HEART"),TEXT("DIAMOND"),TEXT("ROYAL 7"),TEXT("CHALICE"),TEXT("GOLD"),TEXT("ROSE"),TEXT("ROYAL KEY"),TEXT("A"),TEXT("K"),TEXT("Q"),TEXT("J"),TEXT("10"),TEXT("QUEEN\nWILD"),TEXT("CRIMSON\nCREST"),TEXT("ROYAL\nCREST"),TEXT("HEART COIN")};return S>=0&&S<SymbolCount?N[S]:TEXT("?");}
int64 FMegaLustPotGame::Evaluate(const TArray<int32>& G,int32 B,TArray<int32>* Lines,TArray<int32>* Cells)
{
 if(Cells)Cells->Reset();if(Lines)Lines->Reset();if(G.Num()!=15||B<=0)return 0;for(int32 S:G)if(S<0||S>=SymbolCount)return 0;
 static const int32 Pays[14][3]={{10,30,100},{8,25,80},{7,20,60},{6,18,50},{5,15,40},{4,12,30},{4,10,25},{3,8,20},{2,5,15},{2,5,15},{2,5,15},{1,4,10},{1,4,10},{15,50,150}};
 int64 Total=0;
 for(int32 L=0;L<20;++L){int64 Best=0;int32 BestCount=0;
  for(int32 Target=0;Target<=Wild;++Target){int32 Count=0;for(int32 R=0;R<5;++R){int32 S=G[Paylines[L][R]*5+R];if(S==Target||S==Wild)++Count;else break;}if(Count>=3){int64 Win=static_cast<int64>(Pays[Target][Count-3])*B;if(Win>Best){Best=Win;BestCount=Count;}}}
  if(Best>0){Total+=Best;if(Lines)Lines->Add(L);if(Cells)for(int32 R=0;R<BestCount;++R)Cells->AddUnique(Paylines[L][R]*5+R);}
 }return Total;
}
int32 FMegaLustPotGame::RollSymbol(bool ExtraWild)
{
 const int32 N=Rng.RandRange(0,999);if(N<(ExtraWild?95:25))return Wild;if(N<145)return HeartCoin;if(N<205)return Crimson;if(N<255)return Royal;return Rng.RandRange(0,12);
}
bool FMegaLustPotGame::SetBet(int32 V){if(!CanChangeBet()||V<1||V>100||V*20>Credits)return false;Bet=V;return true;}
bool FMegaLustPotGame::Spin(){if(Mode!=Phase::Base)return false;TArray<int32> G;for(int32 I=0;I<15;++I)G.Add(RollSymbol(FreeSpins>0));return ResolveBase(G);}
bool FMegaLustPotGame::ResolveBase(const TArray<int32>& G)
{
 if(Mode!=Phase::Base||G.Num()!=15)return false;for(int32 S:G)if(S<0||S>=SymbolCount)return false;
 const bool Free=FreeSpins>0;if(!Free&&Credits<Bet*20)return false;
 if(Free)--FreeSpins;else Credits-=Bet*20;
 const int32 Multiplier=RoyalSpins>0?Rng.RandRange(2,10):1;if(RoyalSpins>0)--RoyalSpins;
 Grid=G;LastWin=Evaluate(G,Bet,&WinningLines,&WinningCells)*Multiplier;Credits+=LastWin;
 int32 Hearts=0;for(int32 S:G){if(S==Crimson)++CrimsonCharge;if(S==Royal)++RoyalCharge;if(S==HeartCoin){++MegaCharge;++Hearts;}}
 Message=FString::Printf(TEXT("%s  |  %d winning lines  |  %dx royal multiplier"),Free?TEXT("CRIMSON FREE SPIN"):TEXT("SPIN COMPLETE"),WinningLines.Num(),Multiplier);
 while(CrimsonCharge>=12){CrimsonCharge-=12;FreeSpins+=8;Message+=TEXT("  |  CRIMSON: +8 FREE SPINS");}
 while(RoyalCharge>=16){RoyalCharge-=16;RoyalSpins+=3;Message+=TEXT("  |  ROYAL: 3 MULTIPLIER SPINS");}
 if(Hearts>=6||MegaCharge>=24){MegaCharge=0;BeginHold(G);}
 return true;
}
int32 FMegaLustPotGame::LockedCount()const{int32 N=0;for(int64 V:Locked)if(V>0)++N;return N;}
int64 FMegaLustPotGame::LockedTotal()const{int64 N=0;for(int64 V:Locked)N+=V;return N;}
void FMegaLustPotGame::BeginHold(const TArray<int32>& Source)
{
 FeatureBet=Bet*20;Locked.Init(0,15);CollectorMultiplier=1;BonusAward=0;Jackpot.Empty();Respins=3;Mode=Phase::Hold;
 for(int32 I=0;I<15;++I)if(Source[I]==HeartCoin)Locked[I]=FeatureBet*Rng.RandRange(1,5);
 // The charged Mega Pot grants the same six-coin starting board as a reel trigger.
 while(LockedCount()<6){int32 I=Rng.RandRange(0,14);if(!Locked[I])Locked[I]=FeatureBet*Rng.RandRange(1,5);}
 Message=TEXT("MEGA POT! Six hearts locked. New coins reset your three respins.");if(LockedCount()==15)FinishHold();
}
bool FMegaLustPotGame::Respin()
{
 if(Mode!=Phase::Hold)return false;TArray<int64> Added;Added.Init(0,15);for(int32 I=0;I<15;++I)if(!Locked[I]&&Rng.FRand()<.13f)Added[I]=FeatureBet*Rng.RandRange(1,10);ResolveRespin(Added,Rng.FRand()<.08f);return true;
}
void FMegaLustPotGame::ResolveRespin(const TArray<int64>& Added,bool Collector)
{
 if(Mode!=Phase::Hold||Added.Num()!=15)return;for(int64 V:Added)if(V<0||V>static_cast<int64>(FeatureBet)*10)return;
 int32 New=0;for(int32 I=0;I<15;++I)if(!Locked[I]&&Added[I]>0){Locked[I]=Added[I];++New;}
 Respins=New>0?3:Respins-1;if(Collector)CollectorMultiplier=FMath::Min(10,CollectorMultiplier+1);
 Message=FString::Printf(TEXT("%d new hearts | %d respins | Collector %dx"),New,Respins,CollectorMultiplier);
 if(Respins<=0||LockedCount()==15)FinishHold();
}
void FMegaLustPotGame::FinishHold()
{
 const int32 Count=LockedCount();const int64 Extra=Count==15?1000:Count>=12?100:Count>=9?25:0;
 Jackpot=Count==15?TEXT("MEGA POT"):Count>=12?TEXT("MAJOR POT"):Count>=9?TEXT("MINI POT"):TEXT("HEART COLLECTION");
 BonusAward=LockedTotal()*CollectorMultiplier+Extra*FeatureBet;Credits+=BonusAward;LastWin=BonusAward;Mode=Phase::Result;
 Message=FString::Printf(TEXT("%s WON! +%s practice credits"),*Jackpot,*FText::AsNumber(BonusAward).ToString());
}
void FMegaLustPotGame::Collect(){if(Mode==Phase::Result){Mode=Phase::Base;Respins=0;Message=TEXT("Treasure collected. Your queued pot features are ready.");}}
