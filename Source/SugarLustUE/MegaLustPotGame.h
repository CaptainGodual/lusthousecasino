#pragma once
#include "CoreMinimal.h"
// Practice-only rules. These balances never enter the casino economy.
class FMegaLustPotGame
{
public:
 enum Symbol { Crown, Heart, Ring, Seven, Chalice, Coins, Rose, Key, Ace, King, Queen, Jack, Ten, Wild, Crimson, Royal, HeartCoin, SymbolCount };
 enum class Phase { Base, Hold, Result };
 explicit FMegaLustPotGame(int32 Seed=731);
 bool Spin();
 bool ResolveBase(const TArray<int32>& Grid);
 bool Respin();
 void ResolveRespin(const TArray<int64>& Added, bool Collector);
 void Collect();
 bool SetBet(int32 Value);
 bool CanChangeBet() const { return Mode==Phase::Base && FreeSpins==0 && RoyalSpins==0; }
 static int64 Evaluate(const TArray<int32>& Grid,int32 Bet,TArray<int32>* Lines=nullptr,TArray<int32>* Cells=nullptr);
 static FString Name(int32 S);
 static const int32 Paylines[20][5];
 FRandomStream Rng;
 TArray<int32> Grid;
 TArray<int64> Locked;
 TArray<int32> WinningLines;
 TArray<int32> WinningCells;
 Phase Mode=Phase::Base;
 int64 Credits=100000, LastWin=0, BonusAward=0;
 int32 Bet=10, CrimsonCharge=0, RoyalCharge=0, MegaCharge=0;
 int32 FreeSpins=0, RoyalSpins=0, Respins=0, CollectorMultiplier=1, FeatureBet=0;
 FString Message=TEXT("Fill the Pot. Ignite the Fortune.");
 FString Jackpot;
 int32 LockedCount() const;
 int64 LockedTotal() const;
private:
 int32 RollSymbol(bool ExtraWild);
 void BeginHold(const TArray<int32>& Source);
 void FinishHold();
};
