#pragma once
#include "CoreMinimal.h"

class FJsonObject;

struct FHotStreakTier
{
    FString Id, RewardId, RewardName, RewardKind, EquipSlot;
    int64 HeatRequired = 0;
    bool bReady = false, bUnlocked = false, bClaimed = false, bClaimable = false;
};

struct FHotStreakSnapshot
{
    bool bAvailable = false;
    FString EventId, Phase = TEXT("Disabled"), EquippedRewardId;
    FDateTime ServerTime, StartsAt, EndsAt, ClaimUntil;
    int64 AccountRevision = 0, ProgressRevision = 0, RulesRevision = 0;
    int64 CurrentStreak = 0, BestStreak = 0, Heat = 0;
    int32 TriggerWins = 3, PresentationVersion = 1;
    TArray<FHotStreakTier> Tiers;
    TArray<FString> OwnedRewardIds, EligibleGames;
    const FHotStreakTier* FindTier(const FString& Id) const;
    static bool Parse(const TSharedPtr<FJsonObject>& Json, FHotStreakSnapshot& Out);
};

struct FHotStreakDelta
{
    FString EventId, SpinId, TriggerKind;
    bool bEligible = false;
    int64 RulesRevision = 0, ProgressRevision = 0, StreakBefore = 0, StreakAfter = 0, HeatAdded = 0;
    TArray<FString> NewlyUnlockedTierIds;
    static bool Parse(const TSharedPtr<FJsonObject>& Json, FHotStreakDelta& Out);
};
