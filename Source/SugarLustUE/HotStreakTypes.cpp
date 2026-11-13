#include "HotStreakTypes.h"
#include "Dom/JsonObject.h"

namespace
{
    bool Number(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, int64& Value)
    {
        double Raw = 0;
        if (!Json->TryGetNumberField(Key, Raw) || !FMath::IsFinite(Raw) || Raw < 0 || Raw > 9007199254740991.0 || FMath::FloorToDouble(Raw) != Raw) return false;
        Value = static_cast<int64>(Raw); return true;
    }
    bool Strings(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, TArray<FString>& Values, int32 Limit)
    {
        const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
        if (!Json->TryGetArrayField(Key, Array) || !Array || Array->Num() > Limit) return false;
        for (const auto& Value : *Array)
        {
            FString Text;
            if (!Value.IsValid() || !Value->TryGetString(Text) || Text.IsEmpty() || Text.Len() > 64 || Values.Contains(Text)) return false;
            Values.Add(Text);
        }
        return true;
    }
    bool Date(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, FDateTime& Value)
    { FString Text; return Json->TryGetStringField(Key, Text) && FDateTime::ParseIso8601(*Text, Value); }
}

const FHotStreakTier* FHotStreakSnapshot::FindTier(const FString& Id) const
{ return Tiers.FindByPredicate([&Id](const FHotStreakTier& Tier) { return Tier.Id == Id; }); }

bool FHotStreakSnapshot::Parse(const TSharedPtr<FJsonObject>& Json, FHotStreakSnapshot& Out)
{
    FHotStreakSnapshot Value;
    if (!Json.IsValid() || !Json->TryGetBoolField(TEXT("Available"), Value.bAvailable) ||
        !Number(Json, TEXT("AccountRevision"), Value.AccountRevision) || !Date(Json, TEXT("ServerTime"), Value.ServerTime)) return false;
    if (!Value.bAvailable) { Out = MoveTemp(Value); return true; }
    int64 Trigger = 0, Version = 0;
    const TSharedPtr<FJsonObject>* Progress = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Tiers = nullptr;
    if (!Json->TryGetStringField(TEXT("EventId"), Value.EventId) || Value.EventId.IsEmpty() || Value.EventId.Len() > 64 ||
        !Json->TryGetStringField(TEXT("Phase"), Value.Phase) ||
        !(Value.Phase == TEXT("Upcoming") || Value.Phase == TEXT("Active") || Value.Phase == TEXT("ClaimOnly") || Value.Phase == TEXT("Archived") || Value.Phase == TEXT("Disabled")) ||
        !Number(Json, TEXT("RulesRevision"), Value.RulesRevision) || Value.RulesRevision < 1 ||
        !Number(Json, TEXT("TriggerWins"), Trigger) || Trigger != 3 || !Number(Json, TEXT("ClientPresentationVersion"), Version) || Version < 1 || Version > MAX_int32 ||
        !Date(Json, TEXT("StartsAt"), Value.StartsAt) || !Date(Json, TEXT("EndsAt"), Value.EndsAt) || !Date(Json, TEXT("ClaimUntil"), Value.ClaimUntil) ||
        !(Value.StartsAt < Value.EndsAt && Value.EndsAt < Value.ClaimUntil) ||
        !Json->TryGetObjectField(TEXT("Progress"), Progress) || !Progress || !Progress->IsValid() ||
        !Number(*Progress, TEXT("CurrentStreak"), Value.CurrentStreak) || !Number(*Progress, TEXT("BestStreak"), Value.BestStreak) || Value.BestStreak < Value.CurrentStreak ||
        !Number(*Progress, TEXT("Heat"), Value.Heat) || !Number(*Progress, TEXT("ProgressRevision"), Value.ProgressRevision) ||
        !Json->TryGetArrayField(TEXT("Tiers"), Tiers) || !Tiers || Tiers->Num() != 4 ||
        !Strings(Json, TEXT("OwnedRewardIds"), Value.OwnedRewardIds, 96) || !Strings(Json, TEXT("EligibleGames"), Value.EligibleGames, 2)) return false;
    Value.PresentationVersion = static_cast<int32>(Version);
    const TCHAR* Ids[] = {TEXT("Mild"), TEXT("Hot"), TEXT("ExtraHot"), TEXT("Inferno")};
    int64 LastThreshold = 0;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const auto Item = (*Tiers)[Index];
        if (!Item.IsValid() || Item->Type != EJson::Object) return false;
        const auto Object = Item->AsObject(); FHotStreakTier Tier;
        if (!Object.IsValid() || !Object->TryGetStringField(TEXT("TierId"), Tier.Id) || Tier.Id != Ids[Index] ||
            !Object->TryGetStringField(TEXT("RewardId"), Tier.RewardId) || Tier.RewardId.IsEmpty() ||
            !Object->TryGetStringField(TEXT("RewardName"), Tier.RewardName) || !Object->TryGetStringField(TEXT("RewardKind"), Tier.RewardKind) ||
            !Object->TryGetStringField(TEXT("EquipSlot"), Tier.EquipSlot) || !Number(Object, TEXT("HeatRequired"), Tier.HeatRequired) || Tier.HeatRequired <= LastThreshold ||
            !Object->TryGetBoolField(TEXT("RewardReady"), Tier.bReady) || !Object->TryGetBoolField(TEXT("Unlocked"), Tier.bUnlocked) ||
            !Object->TryGetBoolField(TEXT("Claimed"), Tier.bClaimed) || !Object->TryGetBoolField(TEXT("Claimable"), Tier.bClaimable) ||
            (Tier.bUnlocked && Value.Heat < Tier.HeatRequired) || (Tier.bClaimed && !Tier.bUnlocked) ||
            (Tier.bClaimable && (!Tier.bReady || !Tier.bUnlocked || Tier.bClaimed))) return false;
        LastThreshold = Tier.HeatRequired; Value.Tiers.Add(MoveTemp(Tier));
    }
    Json->TryGetStringField(TEXT("EquippedRewardId"), Value.EquippedRewardId);
    Out = MoveTemp(Value); return true;
}

bool FHotStreakDelta::Parse(const TSharedPtr<FJsonObject>& Json, FHotStreakDelta& Out)
{
    FHotStreakDelta Value;
    if (!Json.IsValid() || !Json->TryGetStringField(TEXT("EventId"), Value.EventId) || Value.EventId.IsEmpty() || Value.EventId.Len() > 64 ||
        !Json->TryGetStringField(TEXT("SpinId"), Value.SpinId) || Value.SpinId.IsEmpty() || Value.SpinId.Len() > 64 ||
        !Json->TryGetStringField(TEXT("TriggerKind"), Value.TriggerKind) ||
        !(Value.TriggerKind == TEXT("None") || Value.TriggerKind == TEXT("Entrance") || Value.TriggerKind == TEXT("Extension") || Value.TriggerKind == TEXT("StreakBreak")) ||
        !Json->TryGetBoolField(TEXT("Eligible"), Value.bEligible) || !Number(Json, TEXT("RulesRevision"), Value.RulesRevision) || Value.RulesRevision < 1 ||
        !Number(Json, TEXT("ProgressRevision"), Value.ProgressRevision) || !Number(Json, TEXT("StreakBefore"), Value.StreakBefore) ||
        !Number(Json, TEXT("StreakAfter"), Value.StreakAfter) || !Number(Json, TEXT("HeatAdded"), Value.HeatAdded) || Value.HeatAdded > 1 ||
        !Strings(Json, TEXT("NewlyUnlockedTierIds"), Value.NewlyUnlockedTierIds, 4)) return false;
    for (const FString& Tier : Value.NewlyUnlockedTierIds)
        if (Tier != TEXT("Mild") && Tier != TEXT("Hot") && Tier != TEXT("ExtraHot") && Tier != TEXT("Inferno")) return false;
    if (Value.bEligible && (Value.ProgressRevision < 1 ||
        Value.HeatAdded != (Value.StreakAfter >= 3 ? 1 : 0) ||
        (Value.StreakAfter != 0 && Value.StreakAfter != Value.StreakBefore + 1) ||
        (Value.TriggerKind == TEXT("Entrance") && (Value.StreakBefore != 2 || Value.StreakAfter != 3)) ||
        (Value.TriggerKind == TEXT("StreakBreak") && (Value.StreakBefore < 3 || Value.StreakAfter != 0)))) return false;
    Out = MoveTemp(Value); return true;
}
