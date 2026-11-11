#if WITH_DEV_AUTOMATION_TESTS
#include "../MegaLustPotGame.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMegaPotRulesTest,"SugarLust.MegaPot.Rules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMegaPotRulesTest::RunTest(const FString&)
{
 using G=FMegaLustPotGame;
 TArray<int32> Grid;for(int32 I=0;I<15;++I)Grid.Add((I%5)+8);
 TArray<int32> AllWild;AllWild.Init(G::Wild,15);
 TestEqual(TEXT("Wilds pay best once per line"),G::Evaluate(AllWild,1),static_cast<int64>(3000));
 TSet<FString> Lines;for(int32 L=0;L<20;++L){FString Key;for(int32 R=0;R<5;++R){TestTrue(TEXT("Line stays inside grid"),G::Paylines[L][R]>=0&&G::Paylines[L][R]<3);Key+=FString::FromInt(G::Paylines[L][R]);}Lines.Add(Key);}TestEqual(TEXT("20 unique lines"),Lines.Num(),20);
 G Base;const int64 Start=Base.Credits;TestTrue(TEXT("Base spin"),Base.ResolveBase(Grid));TestEqual(TEXT("Debit exactly total bet"),Base.Credits,Start-200);TArray<int32> Bad=Grid;Bad[0]=99;TestFalse(TEXT("Invalid symbols rejected"),Base.ResolveBase(Bad));TestEqual(TEXT("Invalid outcome leaves credits"),Base.Credits,Start-200);
 G Crimson;Crimson.CrimsonCharge=11;auto C=Grid;C[0]=G::Crimson;Crimson.ResolveBase(C);TestEqual(TEXT("Crimson awards eight"),Crimson.FreeSpins,8);TestEqual(TEXT("Real meter resets"),Crimson.CrimsonCharge,0);TestFalse(TEXT("Feature bet locked"),Crimson.SetBet(20));int64 Before=Crimson.Credits;Crimson.ResolveBase(Grid);TestEqual(TEXT("Free spin no debit"),Crimson.Credits,Before);TestEqual(TEXT("Free spin decremented"),Crimson.FreeSpins,7);
 G Royal;Royal.RoyalCharge=15;C=Grid;C[0]=G::Royal;Royal.ResolveBase(C);TestEqual(TEXT("Royal awards three"),Royal.RoyalSpins,3);Royal.ResolveBase(AllWild);TestTrue(TEXT("Royal payout 2x to 10x"),Royal.LastWin>=60000&&Royal.LastWin<=300000);TestEqual(TEXT("Royal consumed once"),Royal.RoyalSpins,2);
 G Hold;C=Grid;for(int32 I=0;I<6;++I)C[I]=G::HeartCoin;Hold.ResolveBase(C);TestTrue(TEXT("Six hearts trigger"),Hold.Mode==G::Phase::Hold);TestEqual(TEXT("Six retained"),Hold.LockedCount(),6);TestFalse(TEXT("Base spins blocked in hold"),Hold.ResolveBase(Grid));TestFalse(TEXT("Hold bet locked"),Hold.SetBet(50));TArray<int64> Empty;Empty.Init(0,15);Hold.ResolveRespin(Empty,false);TestEqual(TEXT("Empty respin decrements"),Hold.Respins,2);auto Added=Empty;Added[6]=200;const int64 First=Hold.Locked[0];Hold.ResolveRespin(Added,true);TestEqual(TEXT("New coin resets"),Hold.Respins,3);TestEqual(TEXT("Existing coin unchanged"),Hold.Locked[0],First);TestEqual(TEXT("Collector increases"),Hold.CollectorMultiplier,2);
 for(int32 I=0;I<3;++I)Hold.ResolveRespin(Empty,false);TestTrue(TEXT("Three misses finish"),Hold.Mode==G::Phase::Result);int64 Settled=Hold.Credits;Hold.ResolveRespin(Empty,true);Hold.Collect();Hold.Collect();TestEqual(TEXT("Settlement and collection idempotent"),Hold.Credits,Settled);
 G Full;Full.ResolveBase(C);Added.Init(200,15);Full.ResolveRespin(Added,false);TestTrue(TEXT("Full board ends immediately"),Full.Mode==G::Phase::Result);TestEqual(TEXT("Mega tier"),Full.Jackpot,FString(TEXT("MEGA POT")));TestEqual(TEXT("Mega pays locked total plus 1000x"),Full.BonusAward,Full.LockedTotal()+200000);
 G Meter;Meter.MegaCharge=23;C=Grid;C[0]=G::HeartCoin;Meter.ResolveBase(C);TestTrue(TEXT("24 accumulated hearts also trigger"),Meter.Mode==G::Phase::Hold);TestEqual(TEXT("Meter awards six starters"),Meter.LockedCount(),6);
 G Sim(1234);Sim.Credits=1000000000000LL;int32 Features=0;for(int32 I=0;I<10000;++I){if(Sim.Mode==G::Phase::Base)Sim.Spin();else if(Sim.Mode==G::Phase::Hold){Sim.Respin();++Features;}else Sim.Collect();TestTrue(TEXT("Nonnegative practice funds"),Sim.Credits>=0);TestTrue(TEXT("Crimson meter bounded"),Sim.CrimsonCharge>=0&&Sim.CrimsonCharge<12);TestTrue(TEXT("Royal meter bounded"),Sim.RoyalCharge>=0&&Sim.RoyalCharge<16);TestTrue(TEXT("Mega meter bounded"),Sim.MegaCharge>=0&&Sim.MegaCharge<24);}TestTrue(TEXT("Seeded run exercises hold features"),Features>0);
 return true;
}
#endif
