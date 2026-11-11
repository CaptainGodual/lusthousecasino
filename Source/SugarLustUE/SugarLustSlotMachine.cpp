#include "SugarLustSlotMachine.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    constexpr int32 Paylines[FSugarLustSlotMachine::PaylineCount][FSugarLustSlotMachine::ReelCount] =
    {
        {1,1,1,1,1}, {0,0,0,0,0}, {2,2,2,2,2}, {0,1,2,1,0}, {2,1,0,1,2},
        {0,0,1,2,2}, {2,2,1,0,0}, {1,0,0,0,1}, {1,2,2,2,1}, {0,1,1,1,0},
        {2,1,1,1,2}, {0,0,1,1,1}, {2,2,1,1,1}, {1,1,1,0,0}, {1,1,1,2,2},
        {0,1,0,1,0}, {2,1,2,1,2}, {0,2,1,0,2}, {2,0,1,2,0}, {1,0,1,0,1}
    };

    constexpr int32 Payouts[22][3] =
    {
        {5,20,100}, {10,50,250}, {15,75,400}, {20,100,500}, {25,125,750},
        {30,150,1000}, {40,200,1500}, {100,500,2500}, {0,0,0}, {45,225,1800}, {15,75,400}, {10,50,250},
        {2,10,50}, {3,15,75}, {4,20,100}, {5,25,125}, {6,30,150}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}
    };

    constexpr int32 BonusMultipliers[FSugarLustSlotMachine::BonusCardCount] = {5,8,10,12,20,50};
    constexpr int64 DailyBonusRewards[7] = {50'000,75'000,100'000,150'000,200'000,300'000,500'000};
}

FSugarLustSlotMachine::FSugarLustSlotMachine()
{
    FString SavedProfile;
    if (FFileHelper::LoadFileToString(SavedProfile, *(FPaths::ProjectSavedDir() / TEXT("PlayerProfile.txt"))))
    {
        TArray<FString> ProfileFields;
        SavedProfile.ParseIntoArray(ProfileFields, TEXT("|"), false);
        if (ProfileFields.Num() >= 1)
        {
            // Versioned format: "v1|coins|date|streak"
            if (ProfileFields[0] == TEXT("v1") && ProfileFields.Num() >= 4)
            {
                Coins = FMath::Max<int64>(0, FCString::Atoi64(*ProfileFields[1]));
                DailyBonusLastClaimDate = ProfileFields[2];
                DailyBonusLastStreakDay = FMath::Clamp(FCString::Atoi(*ProfileFields[3]), 0, 7);
            }
            else if (ProfileFields.Num() >= 3)
            {
                // Legacy format: "coins|date|streak"
                Coins = FMath::Max<int64>(0, FCString::Atoi64(*ProfileFields[0]));
                DailyBonusLastClaimDate = ProfileFields[1];
                DailyBonusLastStreakDay = FMath::Clamp(FCString::Atoi(*ProfileFields[2]), 0, 7);
            }
        }
    }

    FString SavedXP;
    if (FFileHelper::LoadFileToString(SavedXP, *(FPaths::ProjectSavedDir() / TEXT("PlayerXP.txt"))))
    {
        TArray<FString> XpFields;
        SavedXP.ParseIntoArray(XpFields, TEXT("|"), false);
        if (XpFields.Num() >= 1)
        {
            if (XpFields[0] == TEXT("v1") && XpFields.Num() >= 2)
                XP = FMath::Max<int64>(0, FCString::Atoi64(*XpFields[1]));
            else
                XP = FMath::Max<int64>(0, FCString::Atoi64(*XpFields[0]));
        }
        Level = CalculateLevel(XP);
    }

    LoadMissions();
    InitMissionsIfNeeded();

    ReelStrips =
    {
        {ESugarLustSymbol::Cherry, ESugarLustSymbol::Cupcake2, ESugarLustSymbol::Grape, ESugarLustSymbol::LollipopCandy,
         ESugarLustSymbol::GummyBear, ESugarLustSymbol::Ten, ESugarLustSymbol::Ten, ESugarLustSymbol::Cupcake,
         ESugarLustSymbol::Jack, ESugarLustSymbol::Jack, ESugarLustSymbol::Chocolate, ESugarLustSymbol::Scatter,
         ESugarLustSymbol::GummyBear, ESugarLustSymbol::Wild, ESugarLustSymbol::Cherry},
        {ESugarLustSymbol::Grape, ESugarLustSymbol::Cupcake, ESugarLustSymbol::Lollipop, ESugarLustSymbol::GummyBear,
         ESugarLustSymbol::Queen, ESugarLustSymbol::Queen, ESugarLustSymbol::LollipopCandy, ESugarLustSymbol::Cupcake2,
         ESugarLustSymbol::King, ESugarLustSymbol::Wild, ESugarLustSymbol::Lollipop, ESugarLustSymbol::Ten,
         ESugarLustSymbol::King, ESugarLustSymbol::Chocolate, ESugarLustSymbol::Scatter},
        {ESugarLustSymbol::Grape, ESugarLustSymbol::GummyBear, ESugarLustSymbol::Cupcake, ESugarLustSymbol::Chocolate,
         ESugarLustSymbol::Cupcake, ESugarLustSymbol::Lollipop, ESugarLustSymbol::Cupcake2, ESugarLustSymbol::LollipopCandy,
         ESugarLustSymbol::Wild, ESugarLustSymbol::Cupcake, ESugarLustSymbol::Ace, ESugarLustSymbol::Ace,
         ESugarLustSymbol::Lollipop, ESugarLustSymbol::Cupcake, ESugarLustSymbol::Scatter},
        {ESugarLustSymbol::Cupcake2, ESugarLustSymbol::Grape, ESugarLustSymbol::GummyBear, ESugarLustSymbol::CandyCane,
         ESugarLustSymbol::GummyBear, ESugarLustSymbol::Cupcake, ESugarLustSymbol::LollipopCandy, ESugarLustSymbol::Lollipop,
         ESugarLustSymbol::Queen, ESugarLustSymbol::Jack, ESugarLustSymbol::Wild, ESugarLustSymbol::King,
         ESugarLustSymbol::Chocolate, ESugarLustSymbol::Macaron, ESugarLustSymbol::Macaron},
        {ESugarLustSymbol::Chocolate, ESugarLustSymbol::Cupcake2, ESugarLustSymbol::Grape, ESugarLustSymbol::Chocolate,
         ESugarLustSymbol::CandyCane, ESugarLustSymbol::Wild, ESugarLustSymbol::Chocolate, ESugarLustSymbol::GummyBear,
         ESugarLustSymbol::Macaron, ESugarLustSymbol::Jack, ESugarLustSymbol::Lollipop, ESugarLustSymbol::King,
         ESugarLustSymbol::Scatter, ESugarLustSymbol::Ace, ESugarLustSymbol::Wild}
    };

    // Double each weighted strip to 30 positions, rotating the second half so it
    // does not visibly repeat the first 15-symbol pattern in the same order.
    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
    {
        const TArray<ESugarLustSymbol> FirstHalf = ReelStrips[Reel];
        const int32 HalfLength = FirstHalf.Num();
        for (int32 Index = 0; Index < HalfLength; ++Index)
        {
            ReelStrips[Reel].Add(FirstHalf[(Index + HalfLength / 2) % HalfLength]);
        }
    }

    constexpr int32 FreePositions[5] = {5,4,10,8,9};
    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
    {
        ReelStrips[Reel][FreePositions[Reel]] = ESugarLustSymbol::FreeSpins;
        ReelStrips[Reel][15 + (FreePositions[Reel] + 8) % 15] = ESugarLustSymbol::FreeSpins;
    }
    // Keep this Scatter weighting identical to Nakama's 30-stop strips.
    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
    {
        bool bFoundScatter = false;
        for (ESugarLustSymbol& Symbol : ReelStrips[Reel])
        {
            if (Symbol != ESugarLustSymbol::Scatter) continue;
            if (bFoundScatter) Symbol = ESugarLustSymbol::CandyCane;
            else bFoundScatter = true;
        }
        if (!bFoundScatter) ReelStrips[Reel][13] = ESugarLustSymbol::Scatter;
    }
    constexpr int32 JackpotPositions[5][4] = {{1,8,14,17},{0,7,10,18},{0,3,6,21},{0,3,19,22},{0,3,6,20}};
    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
        for (int32 Tier = 0; Tier < 4; ++Tier)
            ReelStrips[Reel][JackpotPositions[Reel][Tier]] = static_cast<ESugarLustSymbol>(18 + Tier);
    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
    {
        ReelTarget[Reel] = FMath::RandRange(0, ReelStrips[Reel].Num() - 1);
        ReelScroll[Reel] = ReelTarget[Reel];
        ReelStopped[Reel] = true;
        for (int32 Row = 0; Row < RowCount; ++Row)
        {
            Window[Reel][Row] = ReelStrips[Reel][(ReelTarget[Reel] + Row) % ReelStrips[Reel].Num()];
        }
    }
}

bool FSugarLustSlotMachine::Spin()
{
    if (!CanSpin()) return false;

    Coins -= GetTotalBet();
    SaveProfile();
    XP += FMath::Min<int64>(250, 10 + GetTotalBet() / 1000);
    Level = CalculateLevel(XP);
    SaveXP();
    UpdateMissionProgress(TEXT("xp"), static_cast<int32>(FMath::Min<int64>(250, 10 + GetTotalBet() / 1000)));
    if (GetTotalBet() >= 100000) UpdateMissionProgress(TEXT("high_bet"), 1);
    LastWin = 0;
    PendingAuthoritativeWin = 0;
    FMemory::Memzero(WinHighlight, sizeof(WinHighlight));
    FMemory::Memzero(WinningPaylines, sizeof(WinningPaylines));
    FMemory::Memzero(WinningPaylineLengths, sizeof(WinningPaylineLengths));
    FMemory::Memzero(StoppedReels, sizeof(StoppedReels));
    SpinElapsed = 0.0;
    bSpinning = true;

    TArray<int32> Targets;
    for (int32 Reel = 0; Reel < ReelCount; ++Reel) Targets.Add(FMath::RandRange(0, ReelStrips[Reel].Num() - 1));
    StartSpinAtTargets(Targets);
    return true;
}

bool FSugarLustSlotMachine::BeginAuthoritativeSpin(const TArray<int32>& StopIndices, int64 CoinsAfter, int64 XpAfter,
    int64 SpinWin, int32 ScatterCount, const FString& ClaimDate, int32 StreakDay)
{
    if (bSpinning || bBonusRoundActive || bBonusResultReady || StopIndices.Num() != ReelCount) return false;
    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
    {
        if (StopIndices[Reel] < 0 || StopIndices[Reel] >= ReelStrips[Reel].Num()) return false;
    }

    const int64 PreviousXP = XP;
    Coins = FMath::Max<int64>(0, CoinsAfter);
    XP = FMath::Max<int64>(0, XpAfter);
    Level = CalculateLevel(XP);
    UpdateMissionProgress(TEXT("spin"), 1);
    if (XP > PreviousXP)
        UpdateMissionProgress(TEXT("xp"), static_cast<int32>(FMath::Min<int64>(XP - PreviousXP, MAX_int32)));
    if (GetTotalBet() >= 100000) UpdateMissionProgress(TEXT("high_bet"), 1);
    if (SpinWin > 0) UpdateMissionProgress(TEXT("win"), static_cast<int32>(FMath::Min<int64>(SpinWin, MAX_int32)));
    if (ScatterCount >= 3) UpdateMissionProgress(TEXT("scatter"), 1);
    DailyBonusLastClaimDate = ClaimDate;
    DailyBonusLastStreakDay = FMath::Clamp(StreakDay, 0, 7);
    SaveProfile();
    SaveXP();

    // Keep the server result hidden until the presentation animation finishes.
    LastWin = 0;
    PendingAuthoritativeWin = FMath::Max<int64>(0, SpinWin);
    AuthoritativeScatterCount = FMath::Clamp(ScatterCount, 0, ReelCount * RowCount);
    bAuthoritativeSpin = true;
    FMemory::Memzero(WinHighlight, sizeof(WinHighlight));
    FMemory::Memzero(WinningPaylines, sizeof(WinningPaylines));
    FMemory::Memzero(WinningPaylineLengths, sizeof(WinningPaylineLengths));
    FMemory::Memzero(StoppedReels, sizeof(StoppedReels));
    StartSpinAtTargets(StopIndices);
    return true;
}

void FSugarLustSlotMachine::StartSpinAtTargets(const TArray<int32>& StopIndices)
{
    SpinElapsed = 0.0;
    bSpinning = true;
    constexpr double SpinSpeed = 24.0;
    constexpr double DecelerationDuration = 0.72;
    const double DecelerationDistance = SpinSpeed * DecelerationDuration / 3.0;

    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
    {
        ReelStopped[Reel] = false;
        ReelTarget[Reel] = StopIndices[Reel];
        ReelStartScroll[Reel] = ReelScroll[Reel];

        const double StripCount = ReelStrips[Reel].Num();
        const double MinimumTravel = StripCount * 1.5 + Reel * (StripCount / 8.0);
        double TargetScroll = ReelTarget[Reel];
        while (TargetScroll < ReelStartScroll[Reel] + MinimumTravel)
        {
            TargetScroll += StripCount;
        }
        ReelTargetScroll[Reel] = TargetScroll;
        ReelRunUpTime[Reel] = FMath::Max(0.0, (TargetScroll - ReelStartScroll[Reel] - DecelerationDistance) / SpinSpeed);
    }
}

void FSugarLustSlotMachine::Update(float DeltaSeconds)
{
    if (!bSpinning) return;

    SpinElapsed += FMath::Clamp(static_cast<double>(DeltaSeconds), 0.0, 0.1);
    bool bAllStopped = true;
    constexpr double SpinSpeed = 24.0;
    constexpr double DecelerationDuration = 0.72;
    const double DecelerationDistance = SpinSpeed * DecelerationDuration / 3.0;

    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
    {
        if (ReelStopped[Reel]) continue;
        bAllStopped = false;

        if (SpinElapsed < ReelRunUpTime[Reel])
        {
            ReelScroll[Reel] = ReelStartScroll[Reel] + SpinSpeed * SpinElapsed;
            continue;
        }

        const double T = (SpinElapsed - ReelRunUpTime[Reel]) / DecelerationDuration;
        if (T >= 1.0)
        {
            StopReel(Reel);
            continue;
        }

        const double ClampedT = FMath::Clamp(T, 0.0, 1.0);
        const double EaseOut = 1.0 - FMath::Pow(1.0 - ClampedT, 3.0);
        const double DecelerationStart = ReelTargetScroll[Reel] - DecelerationDistance;
        ReelScroll[Reel] = DecelerationStart + DecelerationDistance * EaseOut;
    }

    if (bAllStopped)
    {
        bSpinning = false;
        EvaluateWin();
        if (bAuthoritativeSpin)
        {
            LastWin = PendingAuthoritativeWin;
            PendingAuthoritativeWin = 0;
        }
        if (bAuthoritativeSpin && AuthoritativeScatterCount >= 3)
        {
            StartAuthoritativeBonusRound(AuthoritativeScatterCount);
        }
        bAuthoritativeSpin = false;
    }
}

void FSugarLustSlotMachine::ApplyFreeSpinsState(int32 Remaining, int32 LockedBet, int32 LockedLines)
{
    FreeSpinsRemaining = FMath::Clamp(Remaining, 0, 100);
    if (FreeSpinsRemaining > 0)
    {
        BetPerLine = FMath::Clamp(LockedBet, 1, 10000);
        ActiveLines = FMath::Clamp(LockedLines, 1, PaylineCount);
    }
}

void FSugarLustSlotMachine::SetBetPerLine(int32 Value)
{
    if (FreeSpinsRemaining == 0 && !bSpinning && !bBonusRoundActive && !bBonusResultReady)
        BetPerLine = FMath::Clamp(Value, 1, 10'000);
}

void FSugarLustSlotMachine::SetActiveLines(int32 Value)
{
    if (FreeSpinsRemaining == 0 && !bSpinning && !bBonusRoundActive && !bBonusResultReady)
        ActiveLines = FMath::Clamp(Value, 1, PaylineCount);
}

ESugarLustSymbol FSugarLustSlotMachine::GetSymbolAt(int32 Reel, int32 Row) const
{
    if (Reel < 0 || Reel >= ReelCount) return ESugarLustSymbol::Cherry;
    const TArray<ESugarLustSymbol>& Strip = ReelStrips[Reel];
    if (ReelStopped[Reel] && Row >= 0 && Row < RowCount) return Window[Reel][Row];

    const int32 BaseIndex = FMath::FloorToInt(ReelScroll[Reel]) % Strip.Num();
    return Strip[(BaseIndex + Row + Strip.Num()) % Strip.Num()];
}

bool FSugarLustSlotMachine::IsWinHighlight(int32 Reel, int32 Row) const
{
    return Reel >= 0 && Reel < ReelCount && Row >= 0 && Row < RowCount && WinHighlight[Reel][Row];
}

bool FSugarLustSlotMachine::IsWinningPayline(int32 Payline) const
{
    return Payline >= 0 && Payline < PaylineCount && WinningPaylines[Payline];
}

int32 FSugarLustSlotMachine::GetPaylineRow(int32 Payline, int32 Reel) const
{
    return Payline >= 0 && Payline < PaylineCount && Reel >= 0 && Reel < ReelCount
        ? Paylines[Payline][Reel]
        : INDEX_NONE;
}

int32 FSugarLustSlotMachine::GetWinningPaylineLength(int32 Payline) const
{
    return Payline >= 0 && Payline < PaylineCount ? WinningPaylineLengths[Payline] : 0;
}

bool FSugarLustSlotMachine::IsReelStopped(int32 Reel) const
{
    return Reel >= 0 && Reel < ReelCount && ReelStopped[Reel];
}

float FSugarLustSlotMachine::GetReelScrollFraction(int32 Reel) const
{
    if (Reel < 0 || Reel >= ReelCount || ReelStopped[Reel]) return 0.0f;
    const double Scroll = ReelScroll[Reel];
    return static_cast<float>(Scroll - FMath::FloorToDouble(Scroll));
}

void FSugarLustSlotMachine::StopReel(int32 Reel)
{
    ReelScroll[Reel] = ReelTargetScroll[Reel];
    ReelStopped[Reel] = true;
    StoppedReels[Reel] = 1;
    for (int32 Row = 0; Row < RowCount; ++Row)
    {
        Window[Reel][Row] = ReelStrips[Reel][(ReelTarget[Reel] + Row) % ReelStrips[Reel].Num()];
    }
}

void FSugarLustSlotMachine::EvaluateWin()
{
    FMemory::Memzero(WinHighlight, sizeof(WinHighlight));
    FMemory::Memzero(WinningPaylines, sizeof(WinningPaylines));
    FMemory::Memzero(WinningPaylineLengths, sizeof(WinningPaylineLengths));
    int64 TotalWin = 0;

    for (int32 LineIndex = 0; LineIndex < ActiveLines; ++LineIndex)
    {
        const int32* Line = Paylines[LineIndex];
        if (Window[0][Line[0]] == ESugarLustSymbol::Scatter || Window[0][Line[0]] >= ESugarLustSymbol::FreeSpins) continue;

        ESugarLustSymbol PayingSymbol = Window[0][Line[0]];
        if (PayingSymbol == ESugarLustSymbol::Wild)
        {
            for (int32 Reel = 1; Reel < ReelCount; ++Reel)
            {
                const ESugarLustSymbol Candidate = Window[Reel][Line[Reel]];
                if (Candidate == ESugarLustSymbol::Scatter || Candidate >= ESugarLustSymbol::FreeSpins) break;
                if (Candidate != ESugarLustSymbol::Wild)
                {
                    PayingSymbol = Candidate;
                    break;
                }
            }
        }

        int32 MatchCount = 0;
        for (int32 Reel = 0; Reel < ReelCount; ++Reel)
        {
            const ESugarLustSymbol Symbol = Window[Reel][Line[Reel]];
            if (Symbol == ESugarLustSymbol::Scatter || Symbol >= ESugarLustSymbol::FreeSpins) break;
            if (Symbol != ESugarLustSymbol::Wild && Symbol != PayingSymbol) break;
            ++MatchCount;
        }

        if (MatchCount >= 3 && GetPayout(PayingSymbol, MatchCount) > 0)
        {
            WinningPaylines[LineIndex] = true;
            WinningPaylineLengths[LineIndex] = MatchCount;
            bool bUsedWild = false;
            for (int32 Reel = 0; Reel < MatchCount; ++Reel)
            {
                bUsedWild |= Window[Reel][Line[Reel]] == ESugarLustSymbol::Wild;
                WinHighlight[Reel][Line[Reel]] = true;
            }
            TotalWin += static_cast<int64>(GetPayout(PayingSymbol, MatchCount)) * BetPerLine * (bUsedWild ? 2 : 1);
        }
    }

    int32 ScatterCount = 0;
    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
    {
        for (int32 Row = 0; Row < RowCount; ++Row)
        {
            if (Window[Reel][Row] == ESugarLustSymbol::Scatter) ++ScatterCount;
        }
    }

    if (ScatterCount >= 3)
    {
        TotalWin += static_cast<int64>(GetPayout(ESugarLustSymbol::Scatter, ScatterCount)) * GetTotalBet();
        for (int32 Reel = 0; Reel < ReelCount; ++Reel)
        {
            for (int32 Row = 0; Row < RowCount; ++Row)
            {
                if (Window[Reel][Row] == ESugarLustSymbol::Scatter) WinHighlight[Reel][Row] = true;
            }
        }
    }

    for (int32 Tier = 0; Tier < 4; ++Tier)
    {
        int32 Matches = 0;
        const ESugarLustSymbol JackpotSymbol = static_cast<ESugarLustSymbol>(18 + Tier);
        for (int32 Reel = 0; Reel < ReelCount; ++Reel)
            for (int32 Row = 0; Row < RowCount; ++Row) if (Window[Reel][Row] == JackpotSymbol) ++Matches;
        if (Matches >= 3)
            for (int32 Reel = 0; Reel < ReelCount; ++Reel)
                for (int32 Row = 0; Row < RowCount; ++Row)
                    if (Window[Reel][Row] == JackpotSymbol) WinHighlight[Reel][Row] = true;
    }
    int32 FreeSymbols = 0;
    for (int32 Reel = 0; Reel < ReelCount; ++Reel)
        for (int32 Row = 0; Row < RowCount; ++Row)
            if (Window[Reel][Row] == ESugarLustSymbol::FreeSpins) ++FreeSymbols;
    if (FreeSymbols >= 3)
        for (int32 Reel = 0; Reel < ReelCount; ++Reel)
            for (int32 Row = 0; Row < RowCount; ++Row)
                if (Window[Reel][Row] == ESugarLustSymbol::FreeSpins) WinHighlight[Reel][Row] = true;

    if (!bAuthoritativeSpin)
    {
        LastWin = TotalWin;
        Coins += TotalWin;
        SaveProfile();
        if (ScatterCount >= 3) 
        {
            StartBonusRound(ScatterCount);
            UpdateMissionProgress(TEXT("scatter"), 1);
        }
        UpdateMissionProgress(TEXT("spin"), 1);
        if (TotalWin > 0)
            UpdateMissionProgress(TEXT("win"), static_cast<int32>(FMath::Min<int64>(TotalWin, MAX_int32)));
    }
}

void FSugarLustSlotMachine::StartBonusRound(int32 ScatterCount)
{
    BonusScatterCount = ScatterCount;
    BonusRoundWin = 0;
    BonusPicksRemaining = BonusPickCount;
    FMemory::Memzero(BonusRevealed, sizeof(BonusRevealed));

    TArray<int32> Multipliers;
    for (int32 Multiplier : BonusMultipliers) Multipliers.Add(Multiplier);
    for (int32 Index = Multipliers.Num() - 1; Index > 0; --Index)
    {
        const int32 SwapIndex = FMath::RandRange(0, Index);
        Multipliers.Swap(Index, SwapIndex);
    }
    for (int32 Card = 0; Card < BonusCardCount; ++Card)
    {
        BonusPrizes[Card] = GetTotalBet() * Multipliers[Card];
    }

    bBonusRoundActive = true;
    bBonusResultReady = false;
}

void FSugarLustSlotMachine::StartAuthoritativeBonusRound(int32 ScatterCount)
{
    BonusScatterCount = ScatterCount;
    BonusRoundWin = 0;
    BonusPicksRemaining = BonusPickCount;
    FMemory::Memzero(BonusPrizes, sizeof(BonusPrizes));
    FMemory::Memzero(BonusRevealed, sizeof(BonusRevealed));
    bBonusRoundActive = true;
    bBonusResultReady = false;
}

void FSugarLustSlotMachine::ResumeAuthoritativeBonus(int32 ScatterCount, int32 PicksRemaining, int64 BonusWin,
    const TArray<int32>& RevealedCards, const TArray<int64>& RevealedPrizes)
{
    StartAuthoritativeBonusRound(ScatterCount);
    BonusPicksRemaining = FMath::Clamp(PicksRemaining, 0, BonusPickCount);
    BonusRoundWin = FMath::Max<int64>(0, BonusWin);
    const int32 Count = FMath::Min(RevealedCards.Num(), RevealedPrizes.Num());
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const int32 Card = RevealedCards[Index];
        if (Card >= 0 && Card < BonusCardCount)
        {
            BonusRevealed[Card] = true;
            BonusPrizes[Card] = FMath::Max<int64>(0, RevealedPrizes[Index]);
        }
    }
    bBonusRoundActive = BonusPicksRemaining > 0;
    bBonusResultReady = !bBonusRoundActive;
    if (bBonusResultReady) LastWin += BonusRoundWin;
}

bool FSugarLustSlotMachine::ApplyAuthoritativeBonusPick(int32 Card, int64 Prize, int32 PicksRemaining, int64 BonusWin, int64 CoinsAfter)
{
    if (!bBonusRoundActive || Card < 0 || Card >= BonusCardCount || BonusRevealed[Card]) return false;
    BonusRevealed[Card] = true;
    BonusPrizes[Card] = FMath::Max<int64>(0, Prize);
    BonusPicksRemaining = FMath::Clamp(PicksRemaining, 0, BonusPickCount);
    BonusRoundWin = FMath::Max<int64>(0, BonusWin);
    Coins = FMath::Max<int64>(0, CoinsAfter);
    SaveProfile();
    if (BonusPicksRemaining == 0)
    {
        bBonusRoundActive = false;
        bBonusResultReady = true;
        LastWin += BonusRoundWin;
    }
    return true;
}

bool FSugarLustSlotMachine::PickBonusCard(int32 Card)
{
    if (!bBonusRoundActive || Card < 0 || Card >= BonusCardCount || BonusRevealed[Card]) return false;

    BonusRevealed[Card] = true;
    --BonusPicksRemaining;
    BonusRoundWin += BonusPrizes[Card];
    if (BonusPicksRemaining == 0)
    {
        bBonusRoundActive = false;
        bBonusResultReady = true;
        LastWin += BonusRoundWin;
        Coins += BonusRoundWin;
        SaveProfile();
    }
    return true;
}

void FSugarLustSlotMachine::DismissBonusResult()
{
    bBonusResultReady = false;
}

bool FSugarLustSlotMachine::CanClaimDailyBonus() const
{
    return DailyBonusLastClaimDate != FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
}

int32 FSugarLustSlotMachine::GetDailyBonusStreakDay() const
{
    if (!CanClaimDailyBonus()) return FMath::Clamp(DailyBonusLastStreakDay, 1, 7);
    const FString Yesterday = (FDateTime::Now() - FTimespan::FromDays(1)).ToString(TEXT("%Y-%m-%d"));
    return DailyBonusLastClaimDate == Yesterday ? DailyBonusLastStreakDay % 7 + 1 : 1;
}

int64 FSugarLustSlotMachine::GetDailyBonusReward() const
{
    return GetDailyBonusRewardForDay(GetDailyBonusStreakDay());
}

int64 FSugarLustSlotMachine::GetDailyBonusRewardForDay(int32 Day)
{
    return DailyBonusRewards[FMath::Clamp(Day, 1, 7) - 1];
}

int32 FSugarLustSlotMachine::GetDailyBonusSecondsRemaining() const
{
    const FDateTime Now = FDateTime::Now();
    const FDateTime Tomorrow = Now.GetDate() + FTimespan::FromDays(1);
    return FMath::Max(0, static_cast<int32>((Tomorrow - Now).GetTotalSeconds()));
}

bool FSugarLustSlotMachine::TryClaimDailyBonus()
{
    if (!CanClaimDailyBonus()) return false;
    const int32 StreakDay = GetDailyBonusStreakDay();
    const FString ClaimDate = FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
    const int64 UpdatedCoins = Coins + GetDailyBonusRewardForDay(StreakDay);
    if (!SaveProfile(UpdatedCoins, ClaimDate, StreakDay)) return false;
    Coins = UpdatedCoins;
    DailyBonusLastClaimDate = ClaimDate;
    DailyBonusLastStreakDay = StreakDay;
    return true;
}

void FSugarLustSlotMachine::ApplyCloudProfile(int64 NewCoins, int64 NewXP, const FString& ClaimDate, int32 StreakDay)
{
    Coins = FMath::Max<int64>(0, NewCoins);
    XP = FMath::Max<int64>(0, NewXP);
    Level = CalculateLevel(XP);
    DailyBonusLastClaimDate = ClaimDate;
    DailyBonusLastStreakDay = FMath::Clamp(StreakDay, 0, 7);
    SaveProfile();
    SaveXP();
}

bool FSugarLustSlotMachine::SaveProfile(int64 CoinsToSave, const FString& ClaimDate, int32 StreakDay) const
{
    const FString SaveDirectory = FPaths::ProjectSavedDir();
    IFileManager::Get().MakeDirectory(*SaveDirectory, true);
    const FString Data = FString::Printf(TEXT("v1|%lld|%s|%d"), static_cast<long long>(CoinsToSave), *ClaimDate, StreakDay);
    return FFileHelper::SaveStringToFile(Data, *(SaveDirectory / TEXT("PlayerProfile.txt")));
}

void FSugarLustSlotMachine::SaveProfile() const
{
    SaveProfile(Coins, DailyBonusLastClaimDate, DailyBonusLastStreakDay);
}

int64 FSugarLustSlotMachine::GetXpInLevel() const
{
    return FMath::Max<int64>(0, XP - XpRequiredFor(Level));
}

int64 FSugarLustSlotMachine::GetXpNeededInLevel() const
{
    return Level >= 200 ? 0 : XpRequiredFor(Level + 1) - XpRequiredFor(Level);
}

float FSugarLustSlotMachine::GetLevelProgress() const
{
    const int64 Needed = GetXpNeededInLevel();
    return Needed <= 0 ? 1.0f : FMath::Clamp(static_cast<float>(GetXpInLevel()) / static_cast<float>(Needed), 0.0f, 1.0f);
}

void FSugarLustSlotMachine::SaveXP() const
{
    const FString SaveDirectory = FPaths::ProjectSavedDir();
    IFileManager::Get().MakeDirectory(*SaveDirectory, true);
    FFileHelper::SaveStringToFile(FString::Printf(TEXT("v1|%lld"), static_cast<long long>(XP)), *(SaveDirectory / TEXT("PlayerXP.txt")));
}

int64 FSugarLustSlotMachine::XpRequiredFor(int32 TargetLevel)
{
    if (TargetLevel <= 1) return 0;
    return static_cast<int64>(1000.0 * FMath::Pow(1.18, TargetLevel - 2));
}

int32 FSugarLustSlotMachine::CalculateLevel(int64 TotalXP)
{
    int32 Result = 1;
    while (Result < 200 && TotalXP >= XpRequiredFor(Result + 1)) ++Result;
    return Result;
}

bool FSugarLustSlotMachine::IsBonusCardRevealed(int32 Card) const
{
    return Card >= 0 && Card < BonusCardCount && BonusRevealed[Card];
}

int64 FSugarLustSlotMachine::GetBonusPrize(int32 Card) const
{
    return Card >= 0 && Card < BonusCardCount ? BonusPrizes[Card] : 0;
}

int32 FSugarLustSlotMachine::GetBonusMultiplier(int32 Card) const
{
    const int64 TotalBet = GetTotalBet();
    return Card >= 0 && Card < BonusCardCount && TotalBet > 0
        ? static_cast<int32>(BonusPrizes[Card] / TotalBet)
        : 0;
}

int32 FSugarLustSlotMachine::GetPayout(ESugarLustSymbol Symbol, int32 Count)
{
    if (Count < 3 || Symbol >= ESugarLustSymbol::FreeSpins) return 0;
    if (Symbol == ESugarLustSymbol::Scatter)
    {
        return Count == 3 ? 5 : Count == 4 ? 20 : 100;
    }
    if (Count > 5) return 0;
    return Payouts[static_cast<int32>(Symbol)][Count - 3];
}

FString FSugarLustSlotMachine::GetSymbolName(ESugarLustSymbol Symbol)
{
    if (Symbol >= ESugarLustSymbol::JackpotMini && Symbol <= ESugarLustSymbol::JackpotGrand)
    {
        const TCHAR* Tiers[] = {TEXT("MINI"),TEXT("MINOR"),TEXT("MAJOR"),TEXT("GRAND")};
        return FString(Tiers[static_cast<int32>(Symbol) - 18]) + TEXT("\nJACKPOT");
    }
    if (Symbol == ESugarLustSymbol::FreeSpins) return TEXT("FREE SPINS");
    switch (Symbol)
    {
    case ESugarLustSymbol::Cherry: return TEXT("CHERRY");
    case ESugarLustSymbol::Lollipop: return TEXT("LOLLIPOP GIRL");
    case ESugarLustSymbol::Cupcake: return TEXT("CUPCAKE GIRL");
    case ESugarLustSymbol::GummyBear: return TEXT("GUMMY BEAR");
    case ESugarLustSymbol::Chocolate: return TEXT("CHOCOLATE");
    case ESugarLustSymbol::CandyCane: return TEXT("CANDY CANE");
    case ESugarLustSymbol::Macaron: return TEXT("MACARON");
    case ESugarLustSymbol::Wild: return TEXT("WILD");
    case ESugarLustSymbol::Scatter: return TEXT("SCATTER");
    case ESugarLustSymbol::Grape: return TEXT("GRAPE");
    case ESugarLustSymbol::Cupcake2: return TEXT("CUPCAKE");
    case ESugarLustSymbol::LollipopCandy: return TEXT("LOLLIPOP");
    case ESugarLustSymbol::Ten: return TEXT("TEN");
    case ESugarLustSymbol::Jack: return TEXT("JACK");
    case ESugarLustSymbol::Queen: return TEXT("QUEEN");
    case ESugarLustSymbol::King: return TEXT("KING");
    case ESugarLustSymbol::Ace: return TEXT("ACE");
    default: return TEXT("?");
    }
}

void FSugarLustSlotMachine::InitMissionsIfNeeded()
{
    if (bMissionsServerAuthoritative) return;
    const FString Today = FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
    if (MissionsLastResetDate == Today && Missions.Num() > 0) return;

    Missions.Reset();
    MissionsLastResetDate = Today;

    FMission M1; M1.Id = TEXT("spin_10"); M1.Description = TEXT("Spin 10 times"); M1.Target = 10; M1.Progress = 0; M1.RewardCoins = 25000; M1.bClaimed = false; Missions.Add(M1);
    FMission M2; M2.Id = TEXT("win_50000"); M2.Description = TEXT("Win 50,000 coins total"); M2.Target = 50000; M2.Progress = 0; M2.RewardCoins = 50000; M2.bClaimed = false; Missions.Add(M2);
    FMission M3; M3.Id = TEXT("scatter_3"); M3.Description = TEXT("Trigger bonus 3 times"); M3.Target = 3; M3.Progress = 0; M3.RewardCoins = 75000; M3.bClaimed = false; Missions.Add(M3);
    FMission M4; M4.Id = TEXT("bet_high"); M4.Description = TEXT("Play 5 high-bet spins"); M4.Target = 5; M4.Progress = 0; M4.RewardCoins = 40000; M4.bClaimed = false; Missions.Add(M4);
    FMission M5; M5.Id = TEXT("level_up"); M5.Description = TEXT("Gain XP from spins"); M5.Target = 1000; M5.Progress = 0; M5.RewardCoins = 60000; M5.bClaimed = false; Missions.Add(M5);

    SaveMissions();
}

void FSugarLustSlotMachine::UpdateMissionProgress(const FString& MissionType, int32 Amount)
{
    if (bMissionsServerAuthoritative || Amount <= 0) return;
    const FString Today = FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
    if (MissionsLastResetDate != Today) InitMissionsIfNeeded();

    for (FMission& M : Missions)
    {
        if (M.bClaimed) continue;
        if (MissionType == TEXT("spin") && M.Id == TEXT("spin_10"))
        {
            M.Progress = FMath::Min(M.Target, M.Progress + Amount);
        }
        else if (MissionType == TEXT("high_bet") && M.Id == TEXT("bet_high"))
        {
            M.Progress = FMath::Min(M.Target, M.Progress + Amount);
        }
        else if (MissionType == TEXT("win") && M.Id == TEXT("win_50000"))
        {
            M.Progress = FMath::Min(M.Target, M.Progress + Amount);
        }
        else if (MissionType == TEXT("scatter") && M.Id == TEXT("scatter_3"))
        {
            M.Progress = FMath::Min(M.Target, M.Progress + Amount);
        }
        else if (MissionType == TEXT("xp") && M.Id == TEXT("level_up"))
        {
            M.Progress = FMath::Min(M.Target, M.Progress + Amount);
        }
    }
    SaveMissions();
}

void FSugarLustSlotMachine::ApplyCloudMissions(const TArray<FMission>& ServerMissions, const FString& ResetDate)
{
    Missions = ServerMissions;
    MissionsLastResetDate = ResetDate;
    bMissionsServerAuthoritative = true;
    SaveMissions();
}

void FSugarLustSlotMachine::SaveMissions() const
{
    const FString SaveDirectory = FPaths::ProjectSavedDir();
    IFileManager::Get().MakeDirectory(*SaveDirectory, true);
    FString Data = FString::Printf(TEXT("v1|%s\n"), *MissionsLastResetDate);
    for (const FMission& M : Missions)
    {
        Data += FString::Printf(TEXT("%s|%s|%d|%d|%lld|%d\n"), *M.Id, *M.Description, M.Target, M.Progress, M.RewardCoins, M.bClaimed ? 1 : 0);
    }
    FFileHelper::SaveStringToFile(Data, *(SaveDirectory / TEXT("Missions.txt")));
}

void FSugarLustSlotMachine::LoadMissions()
{
    Missions.Reset();
    MissionsLastResetDate.Empty();
    FString SavedMissions;
    if (FFileHelper::LoadFileToString(SavedMissions, *(FPaths::ProjectSavedDir() / TEXT("Missions.txt"))))
    {
        TArray<FString> Lines;
        SavedMissions.ParseIntoArray(Lines, TEXT("\n"), false);
        if (Lines.Num() > 0)
        {
            TArray<FString> Header;
            Lines[0].ParseIntoArray(Header, TEXT("|"), false);
            if (Header.Num() >= 2 && Header[0] == TEXT("v1"))
                MissionsLastResetDate = Header[1];
        }
        for (int32 i = 1; i < Lines.Num(); ++i)
        {
            TArray<FString> Fields;
            Lines[i].ParseIntoArray(Fields, TEXT("|"), false);
            if (Fields.Num() >= 6)
            {
                FMission M;
                M.Id = Fields[0];
                M.Description = Fields[1];
                M.Target = FCString::Atoi(*Fields[2]);
                M.Progress = FCString::Atoi(*Fields[3]);
                M.RewardCoins = FCString::Atoi64(*Fields[4]);
                M.bClaimed = FCString::Atoi(*Fields[5]) == 1;
                Missions.Add(M);
            }
        }
    }
}

FString FSugarLustSlotMachine::GetSymbolMark(ESugarLustSymbol Symbol)
{
    if (Symbol >= ESugarLustSymbol::JackpotMini && Symbol <= ESugarLustSymbol::JackpotGrand)
    {
        const TCHAR* Tiers[] = {TEXT("MINI"),TEXT("MINOR"),TEXT("MAJOR"),TEXT("GRAND")};
        return FString(Tiers[static_cast<int32>(Symbol) - 18]) + TEXT("\nJACKPOT");
    }
    if (Symbol == ESugarLustSymbol::FreeSpins) return TEXT("FREE\nSPINS");
    switch (Symbol)
    {
    case ESugarLustSymbol::Cherry: return TEXT("CH");
    case ESugarLustSymbol::Lollipop: return TEXT("LG");
    case ESugarLustSymbol::Cupcake: return TEXT("CG");
    case ESugarLustSymbol::GummyBear: return TEXT("GB");
    case ESugarLustSymbol::Chocolate: return TEXT("CO");
    case ESugarLustSymbol::CandyCane: return TEXT("CC");
    case ESugarLustSymbol::Macaron: return TEXT("MA");
    case ESugarLustSymbol::Wild: return TEXT("WILD");
    case ESugarLustSymbol::Scatter: return TEXT("BONUS");
    case ESugarLustSymbol::Grape: return TEXT("GRAPE");
    case ESugarLustSymbol::Cupcake2: return TEXT("CK");
    case ESugarLustSymbol::LollipopCandy: return TEXT("LP");
    case ESugarLustSymbol::Ten: return TEXT("10");
    case ESugarLustSymbol::Jack: return TEXT("J");
    case ESugarLustSymbol::Queen: return TEXT("Q");
    case ESugarLustSymbol::King: return TEXT("K");
    case ESugarLustSymbol::Ace: return TEXT("A");
    default: return TEXT("?");
    }
}

FLinearColor FSugarLustSlotMachine::GetSymbolAccent(ESugarLustSymbol Symbol)
{
    if (Symbol >= ESugarLustSymbol::JackpotMini && Symbol <= ESugarLustSymbol::JackpotGrand)
    {
        const FLinearColor Colors[] = {FLinearColor(0.15f,0.8f,1),FLinearColor(0.2f,1,0.25f),FLinearColor(0.85f,0.2f,1),FLinearColor(1,0.75f,0.15f)};
        return Colors[static_cast<int32>(Symbol) - 18];
    }
    switch (Symbol)
    {
    case ESugarLustSymbol::Cherry: return FLinearColor(1.0f,0.20f,0.24f);
    case ESugarLustSymbol::Lollipop: return FLinearColor(1.0f,0.49f,0.20f);
    case ESugarLustSymbol::Cupcake: return FLinearColor(1.0f,0.20f,0.82f);
    case ESugarLustSymbol::GummyBear: return FLinearColor(0.20f,0.91f,0.50f);
    case ESugarLustSymbol::Chocolate: return FLinearColor(0.58f,0.30f,0.19f);
    case ESugarLustSymbol::CandyCane: return FLinearColor(1.0f,0.82f,0.13f);
    case ESugarLustSymbol::Macaron: return FLinearColor(0.20f,1.0f,0.87f);
    case ESugarLustSymbol::Wild: return FLinearColor(0.15f,0.68f,1.0f);
    case ESugarLustSymbol::Scatter: return FLinearColor(1.0f,0.42f,0.57f);
    case ESugarLustSymbol::Grape: return FLinearColor(0.60f,0.28f,1.0f);
    case ESugarLustSymbol::Cupcake2: return FLinearColor(1.0f,0.55f,0.30f);
    case ESugarLustSymbol::LollipopCandy: return FLinearColor(0.92f,0.20f,0.54f);
    case ESugarLustSymbol::Ten: return FLinearColor(0.70f,0.76f,1.0f);
    case ESugarLustSymbol::Jack: return FLinearColor(0.58f,0.83f,1.0f);
    case ESugarLustSymbol::Queen: return FLinearColor(1.0f,0.58f,0.82f);
    case ESugarLustSymbol::King: return FLinearColor(1.0f,0.64f,0.28f);
    case ESugarLustSymbol::Ace: return FLinearColor(1.0f,0.90f,0.35f);
    default: return FLinearColor::White;
    }
}
