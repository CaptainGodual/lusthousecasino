#pragma once
#include "CoreMinimal.h"

// Local practice combat only. Never accepts or grants casino currency.
struct FSpinDuelState
{
 int32 HP=100, Shield=0, EnemyHP=0, EnemyMax=0, Encounter=0, Keys=0, Score=0, Turn=0, Tonic=1, Held=-1;
 bool bHard=false;
 int32 Reels[3]={0,1,2};
 void BeginEncounter(int32 Index){Encounter=Index;EnemyMax=Index==3?85:35+Index*15;EnemyHP=EnemyMax+(bHard&&Index==1?15:0);Turn=0;Shield=0;Held=-1;}
 int32 Intent() const {return Turn%3;} // strike, guard, heavy strike
 int32 EnemyDamage() const {return Intent()==1?0:Intent()==2?18+Encounter*2:10+Encounter*2;}
 bool CanSpin() const {return HP>0&&EnemyHP>0;}
 FString Resolve(const TArray<int32>& Symbols)
 {
  if(!CanSpin()||Symbols.Num()!=3)return TEXT("No active duel.");
  int32 Attack=3,Block=0,Heal=0;
  for(int32 I=0;I<3;++I){Reels[I]=FMath::Clamp(Symbols[I],0,4);switch(Reels[I]){case 0:Attack+=8;break;case 1:Block+=10;break;case 2:Heal+=9;break;case 3:Attack+=5;Block+=5;break;case 4:Attack+=8;Heal+=4;break;}}
  const bool Match=Reels[0]==Reels[1]&&Reels[1]==Reels[2];if(Match)Attack+=15;
  const int32 IntentNow=Intent(),Incoming=EnemyDamage();if(IntentNow==1)Attack=FMath::Max(1,Attack-10);
  HP=FMath::Min(100,HP+Heal);Shield=FMath::Min(40,Shield+Block);EnemyHP=FMath::Max(0,EnemyHP-Attack);
  int32 Taken=0;if(EnemyHP>0){Taken=FMath::Max(0,Incoming-Shield);Shield=FMath::Max(0,Shield-Incoming);HP=FMath::Max(0,HP-Taken);}
  ++Turn;Held=-1;return FString::Printf(TEXT("%sAttack %d | Guard +%d | Heal +%d | Damage taken %d"),Match?TEXT("TRIPLE MATCH! "):TEXT(""),Attack,Block,Heal,Taken);
 }
 void AwardEncounter(){if(EnemyHP!=0||HP<=0)return;++Keys;Score+=100+Encounter*50+(bHard&&Encounter==1?75:0);HP=FMath::Min(100,HP+12);EnemyHP=-1;}
 bool UseTonic(){if(Tonic<=0||HP<=0||HP>=100)return false;--Tonic;HP=FMath::Min(100,HP+25);return true;}
};
