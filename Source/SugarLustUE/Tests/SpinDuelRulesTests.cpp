#if WITH_DEV_AUTOMATION_TESTS
#include "../SpinDuelRules.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpinDuelRulesTest,"SugarLust.SpinDuel.PracticeRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSpinDuelRulesTest::RunTest(const FString& Parameters)
{
 FSpinDuelState S;S.BeginEncounter(0);S.HP=95;S.Resolve({2,2,2});TestEqual(TEXT("Healing capped at 100 before retaliation"),S.HP,90);TestEqual(TEXT("Heart triple includes match attack"),S.EnemyHP,17);
 S.BeginEncounter(0);S.Shield=35;S.Resolve({1,1,1});TestEqual(TEXT("Shield cap then incoming strike"),S.Shield,30);
 S.BeginEncounter(0);S.EnemyHP=1;S.HP=1;S.Resolve({0,0,0});TestEqual(TEXT("Defeated rival cannot retaliate"),S.HP,1);S.AwardEncounter();const int Score=S.Score;S.AwardEncounter();TestEqual(TEXT("Reward awarded only once"),S.Score,Score);TestEqual(TEXT("One key per cleared rival"),S.Keys,1);
 S=FSpinDuelState();S.HP=50;TestTrue(TEXT("First tonic available"),S.UseTonic());TestEqual(TEXT("Tonic heals 25"),S.HP,75);TestFalse(TEXT("Tonic cannot be reused"),S.UseTonic());
 S=FSpinDuelState();for(int I=0;I<4;++I){S.BeginEncounter(I);for(int Turn=0;Turn<40&&S.CanSpin();++Turn)S.Resolve({0,0,0});TestTrue(TEXT("Attack strategy can complete each practice encounter"),S.EnemyHP==0&&S.HP>0);S.AwardEncounter();}TestEqual(TEXT("Four encounters completed"),S.Keys,4);
 S=FSpinDuelState();S.BeginEncounter(0);S.HP=0;const int E=S.EnemyHP;S.Resolve({0,0,0});TestEqual(TEXT("Defeat blocks further damage"),S.EnemyHP,E);
 return true;
}
#endif
