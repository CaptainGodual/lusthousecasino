#pragma once

#include "CoreMinimal.h"

enum class ESugarLustSymbol : uint8
{
    Cherry,
    Lollipop,
    Cupcake,
    GummyBear,
    Chocolate,
    CandyCane,
    Macaron,
    Wild,
    Scatter,
    Grape,
    Cupcake2,
    LollipopCandy,
    Ten,
    Jack,
    Queen,
    King,
    Ace,
    FreeSpins,
    JackpotMini,
    JackpotMinor,
    JackpotMajor,
    JackpotGrand
};

/** Standalone Sugar Lust rules and reel motion model, ported from the MonoGame slot machine. */
class FSugarLustSlotMachine
{
public:
    static constexpr int32 ReelCount = 5;
    static constexpr int32 RowCount = 3;
    static constexpr int32 PaylineCount = 20;
    static constexpr int32 BonusCardCount = 6;
    static constexpr int32 BonusPickCount = 3;

    FSugarLustSlotMachine();

    bool Spin();
    bool BeginAuthoritativeSpin(const TArray<int32>& StopIndices, int64 CoinsAfter, int64 XpAfter,
        int64 SpinWin, int32 ScatterCount, const FString& ClaimDate, int32 StreakDay);
    void ResumeAuthoritativeBonus(int32 ScatterCount, int32 PicksRemaining, int64 BonusWin,
        const TArray<int32>& RevealedCards, const TArray<int64>& RevealedPrizes);
    bool ApplyAuthoritativeBonusPick(int32 Card, int64 Prize, int32 PicksRemaining, int64 BonusWin, int64 CoinsAfter);
    void Update(float DeltaSeconds);
    void ApplyFreeSpinsState(int32 Remaining, int32 LockedBet, int32 LockedLines);
    void SetBetPerLine(int32 Value);
    void SetActiveLines(int32 Value);
    bool PickBonusCard(int32 Card);
    void DismissBonusResult();
    bool TryClaimDailyBonus();
    void ApplyCloudProfile(int64 NewCoins, int64 NewXP, const FString& ClaimDate, int32 StreakDay);

    ESugarLustSymbol GetSymbolAt(int32 Reel, int32 Row) const;
    bool IsWinHighlight(int32 Reel, int32 Row) const;
    bool IsWinningPayline(int32 Payline) const;
    int32 GetPaylineRow(int32 Payline, int32 Reel) const;
    int32 GetWinningPaylineLength(int32 Payline) const;
    bool IsReelStopped(int32 Reel) const;
    float GetReelScrollFraction(int32 Reel) const;
    bool IsSpinning() const { return bSpinning; }
    bool IsBonusRoundActive() const { return bBonusRoundActive; }
    bool IsBonusResultReady() const { return bBonusResultReady; }
    bool CanSpin() const { return !bSpinning && !bBonusRoundActive && !bBonusResultReady && (FreeSpinsRemaining > 0 || Coins >= GetTotalBet()); }

    int32 GetBetPerLine() const { return BetPerLine; }
    int32 GetActiveLines() const { return ActiveLines; }
    int64 GetTotalBet() const { return static_cast<int64>(BetPerLine) * ActiveLines; }
    int64 GetCoins() const { return Coins; }
    bool CanClaimDailyBonus() const;
    int32 GetDailyBonusStreakDay() const;
    int64 GetDailyBonusReward() const;
    int32 GetDailyBonusSecondsRemaining() const;
    static int64 GetDailyBonusRewardForDay(int32 Day);
    int64 GetXP() const { return XP; }
    int32 GetLevel() const { return Level; }
    int64 GetXpInLevel() const;
    int64 GetXpNeededInLevel() const;
    float GetLevelProgress() const;
    int64 GetLastWin() const { return LastWin; }
    int32 GetBonusScatterCount() const { return BonusScatterCount; }
    int32 GetBonusPicksRemaining() const { return BonusPicksRemaining; }
    int64 GetBonusRoundWin() const { return BonusRoundWin; }
    bool IsBonusCardRevealed(int32 Card) const;
    int64 GetBonusPrize(int32 Card) const;
    int32 GetBonusMultiplier(int32 Card) const;

    // PlayFab owns online mission progress and rewards. Local missions remain an offline demo fallback.
    struct FMission
    {
        FString Id;
        FString Description;
        int32 Target = 0;
        int32 Progress = 0;
        int64 RewardCoins = 0;
        bool bClaimed = false;
    };
    void InitMissionsIfNeeded();
    void UpdateMissionProgress(const FString& MissionType, int32 Amount = 1);
    TArray<FMission> GetMissions() const { return Missions; }
    void ApplyCloudMissions(const TArray<FMission>& ServerMissions, const FString& ResetDate);

    static FString GetSymbolName(ESugarLustSymbol Symbol);
    static FString GetSymbolMark(ESugarLustSymbol Symbol);
    static FLinearColor GetSymbolAccent(ESugarLustSymbol Symbol);

private:
    TArray<TArray<ESugarLustSymbol>> ReelStrips;
    ESugarLustSymbol Window[ReelCount][RowCount];
    bool WinHighlight[ReelCount][RowCount] = {};
    bool WinningPaylines[PaylineCount] = {};
    int32 WinningPaylineLengths[PaylineCount] = {};
    bool ReelStopped[ReelCount] = {};
    int32 StoppedReels[ReelCount] = {};
    double ReelScroll[ReelCount] = {};
    double ReelStartScroll[ReelCount] = {};
    double ReelTargetScroll[ReelCount] = {};
    double ReelRunUpTime[ReelCount] = {};
    int32 ReelTarget[ReelCount] = {};
    double SpinElapsed = 0.0;

    int32 FreeSpinsRemaining = 0;
    int32 BetPerLine = 10;
    int32 ActiveLines = PaylineCount;
    int64 Coins = 10'050'000;
    FString DailyBonusLastClaimDate;
    int32 DailyBonusLastStreakDay = 0;
    int64 XP = 0;
    int32 Level = 1;
    int64 LastWin = 0;
    int64 PendingAuthoritativeWin = 0;
    int64 BonusPrizes[BonusCardCount] = {};
    bool BonusRevealed[BonusCardCount] = {};
    bool bSpinning = false;
    bool bBonusRoundActive = false;
    bool bBonusResultReady = false;
    int32 BonusPicksRemaining = 0;
    int32 BonusScatterCount = 0;
    int64 BonusRoundWin = 0;
    bool bAuthoritativeSpin = false;
    int32 AuthoritativeScatterCount = 0;

    // Device-local mission progress for the demo.
    TArray<FMission> Missions;
    FString MissionsLastResetDate;
    bool bMissionsServerAuthoritative = false;

    void StopReel(int32 Reel);
    void StartSpinAtTargets(const TArray<int32>& StopIndices);
    void EvaluateWin();
    void StartBonusRound(int32 ScatterCount);
    void StartAuthoritativeBonusRound(int32 ScatterCount);
    void SaveXP() const;
    bool SaveProfile(int64 CoinsToSave, const FString& ClaimDate, int32 StreakDay) const;
    void SaveProfile() const;
    static int64 XpRequiredFor(int32 Level);
    static int32 CalculateLevel(int64 TotalXP);
    static int32 GetPayout(ESugarLustSymbol Symbol, int32 Count);

    void SaveMissions() const;
    void LoadMissions();
};
