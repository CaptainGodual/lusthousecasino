#include "HotStreakSubsystem.h"
#include "CasinoAccountApi.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UHotStreakSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PollHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UHotStreakSubsystem::TickRefresh), 1.0f);
    ForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UHotStreakSubsystem::EnterForeground);
    BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UHotStreakSubsystem::EnterBackground);
}
void UHotStreakSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(PollHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    ClearAccount(); OnChanged.Clear(); Super::Deinitialize();
}
void UHotStreakSubsystem::SetAccount(const FString& Player, const FString& Ticket)
{
    if (Player == PlayerId && Ticket == SessionTicket) return;
    ClearAccount(); PlayerId = Player; SessionTicket = Ticket; LoadRequestIds(); Refresh();
}
void UHotStreakSubsystem::ClearAccount()
{
    ++LoginGeneration; PlayerId.Reset(); SessionTicket.Reset(); LastError.Reset();
    Snapshot = FHotStreakSnapshot(); DisplayedSnapshot = FHotStreakSnapshot();
    Presentations.Reset(); SeenPresentationIds.Reset(); PendingRequestIds.Reset();
    bRequestInFlight = false; bDeferDisplay = false; LastRefreshAt = 0; LastReactionAt = -1000;
    OnChanged.Broadcast();
}
bool UHotStreakSubsystem::IsCurrentSession(uint64 ExpectedGeneration, const FString& Player) const
{ return ExpectedGeneration == LoginGeneration && !PlayerId.IsEmpty() && PlayerId == Player && !SessionTicket.IsEmpty(); }
bool UHotStreakSubsystem::MatchesSession(uint64 ExpectedGeneration, const FString& Ticket) const
{ return ExpectedGeneration == LoginGeneration && !PlayerId.IsEmpty() && !SessionTicket.IsEmpty() && SessionTicket == Ticket; }
bool UHotStreakSubsystem::RestoreSession(FString& Player, FString& Ticket) const
{ if (PlayerId.IsEmpty() || SessionTicket.IsEmpty()) return false; Player = PlayerId; Ticket = SessionTicket; return true; }
void UHotStreakSubsystem::EnterForeground() { bForeground = true; Refresh(); }
void UHotStreakSubsystem::EnterBackground() { bForeground = false; }
bool UHotStreakSubsystem::TickRefresh(float)
{
    if (bForeground && Snapshot.bAvailable && Snapshot.Phase != TEXT("Archived") && FPlatformTime::Seconds() - LastRefreshAt >= 60) Refresh();
    return true;
}
void UHotStreakSubsystem::Send(const FString& Function, const TSharedRef<FJsonObject>& Parameters,
    TFunction<void(TSharedPtr<FJsonObject>)> Success)
{
    if (bRequestInFlight) return;
    if (PlayerId.IsEmpty() || SessionTicket.IsEmpty()) { LastError = TEXT("Sign in to see your saved event progress."); OnChanged.Broadcast(); return; }
    bRequestInFlight = true; LastError.Reset(); OnChanged.Broadcast();
    const uint64 Generation = LoginGeneration; const FString Player = PlayerId;
    const TWeakObjectPtr<UHotStreakSubsystem> WeakThis(this);
    FCasinoAccountApi::Execute(this, SessionTicket, Function, Parameters,
        [WeakThis, Generation, Player, Success = MoveTemp(Success)](TSharedPtr<FJsonObject> Result)
        {
            if (!WeakThis.IsValid() || !WeakThis->IsCurrentSession(Generation, Player)) return;
            WeakThis->bRequestInFlight = false; Success(Result); WeakThis->OnChanged.Broadcast();
        },
        [WeakThis, Generation, Player](const FString& Error)
        {
            if (!WeakThis.IsValid() || !WeakThis->IsCurrentSession(Generation, Player)) return;
            WeakThis->bRequestInFlight = false; WeakThis->LastError = Error; WeakThis->OnChanged.Broadcast();
        }, [WeakThis, Generation, Player]() { return WeakThis.IsValid() && WeakThis->IsCurrentSession(Generation, Player); });
}
void UHotStreakSubsystem::Refresh()
{
    if (bRequestInFlight || PlayerId.IsEmpty() || !bForeground) return;
    LastRefreshAt = FPlatformTime::Seconds();
    const TWeakObjectPtr<UHotStreakSubsystem> WeakThis(this);
    Send(TEXT("GetHotStreakState"), MakeShared<FJsonObject>(), [WeakThis](TSharedPtr<FJsonObject> Result)
    {
        if (!WeakThis.IsValid()) return;
        const TSharedPtr<FJsonObject>* Profile = nullptr;
        if (!Result.IsValid() || !Result->TryGetObjectField(TEXT("Profile"), Profile) || !Profile)
        { WeakThis->LastError = TEXT("Event state is unavailable. Your slot result is unaffected."); return; }
        WeakThis->ApplyProfile(*Profile);
    });
}
bool UHotStreakSubsystem::ApplySnapshot(const TSharedPtr<FJsonObject>& Json)
{
    FHotStreakSnapshot Incoming;
    if (!FHotStreakSnapshot::Parse(Json, Incoming)) return false;
    if (Incoming.AccountRevision < Snapshot.AccountRevision ||
        (Incoming.bAvailable && Incoming.EventId == Snapshot.EventId && Incoming.ProgressRevision < Snapshot.ProgressRevision)) return false;
    const bool bNewEvent = Incoming.bAvailable && Incoming.EventId != Snapshot.EventId;
    if (bNewEvent) { Presentations.Reset(); SeenPresentationIds.Reset(); }
    if (!Incoming.bAvailable && Snapshot.bAvailable)
    {
        Snapshot.AccountRevision = Incoming.AccountRevision; Snapshot.Phase = TEXT("Disabled"); Snapshot.ServerTime = Incoming.ServerTime;
        for (auto& Tier : Snapshot.Tiers) Tier.bClaimable = false;
        LastError = TEXT("Event is unavailable. Your banked progress is retained.");
    }
    else Snapshot = MoveTemp(Incoming);
    SnapshotAt = FPlatformTime::Seconds();
    if (!bDeferDisplay) DisplayedSnapshot = Snapshot;
    OnChanged.Broadcast(); return true;
}
void UHotStreakSubsystem::ApplyProfile(const TSharedPtr<FJsonObject>& Profile)
{
    const TSharedPtr<FJsonObject>* Event = nullptr;
    if (Profile.IsValid() && Profile->TryGetObjectField(TEXT("HotStreak"), Event) && Event) ApplySnapshot(*Event);
}
void UHotStreakSubsystem::ApplySettledSpin(const TSharedPtr<FJsonObject>& Result, const FString& SpinId)
{
    if (!Result.IsValid()) return;
    const TSharedPtr<FJsonObject>* Profile = nullptr;
    if (Result->TryGetObjectField(TEXT("Profile"), Profile) && Profile) ApplyProfile(*Profile);
    bool bReplayed = false, bLegacy = false;
    Result->TryGetBoolField(TEXT("Replayed"), bReplayed); Result->TryGetBoolField(TEXT("RecoveredLegacyReceipt"), bLegacy);
    const TSharedPtr<FJsonObject>* Event = nullptr; FHotStreakDelta Delta;
    if (bReplayed || bLegacy || !Result->TryGetObjectField(TEXT("HotStreakDelta"), Event) || !Event || !FHotStreakDelta::Parse(*Event, Delta) ||
        !Delta.bEligible || Delta.SpinId != SpinId || Delta.EventId != Snapshot.EventId || Delta.RulesRevision != Snapshot.RulesRevision || Delta.ProgressRevision > Snapshot.ProgressRevision) return;
    const FString Id = Delta.EventId + TEXT("|") + SpinId;
    if (SeenPresentationIds.Contains(Id)) return;
    SeenPresentationIds.Add(Id); if (SeenPresentationIds.Num() > 256) SeenPresentationIds.RemoveAt(0);
    if (Delta.TriggerKind == TEXT("None") && Delta.NewlyUnlockedTierIds.IsEmpty()) return;
    if (Delta.TriggerKind == TEXT("Extension") && Delta.NewlyUnlockedTierIds.IsEmpty() && !Presentations.IsEmpty() &&
        Presentations.Last().TriggerKind == TEXT("Extension") && Presentations.Last().NewlyUnlockedTierIds.IsEmpty()) Presentations.Last() = Delta;
    else Presentations.Add(Delta);
    if (Presentations.Num() > 16)
    {
        const int32 Ordinary = Presentations.IndexOfByPredicate([](const FHotStreakDelta& Item) { return Item.NewlyUnlockedTierIds.IsEmpty() && Item.TriggerKind == TEXT("Extension"); });
        const int32 WithoutTier = Presentations.IndexOfByPredicate([](const FHotStreakDelta& Item) { return Item.NewlyUnlockedTierIds.IsEmpty(); });
        Presentations.RemoveAt(Ordinary != INDEX_NONE ? Ordinary : WithoutTier != INDEX_NONE ? WithoutTier : 0);
    }
}
void UHotStreakSubsystem::RevealSettledProgress()
{ if (bDeferDisplay) { bDeferDisplay = false; DisplayedSnapshot = Snapshot; OnChanged.Broadcast(); } }
bool UHotStreakSubsystem::PopPresentation(FHotStreakDelta& Out, double Now)
{
    while (!Presentations.IsEmpty())
    {
        FHotStreakDelta Delta = Presentations[0]; Presentations.RemoveAt(0);
        if (Delta.EventId != Snapshot.EventId || Snapshot.Phase == TEXT("Archived")) continue;
        const bool bStrong = Delta.TriggerKind == TEXT("Entrance") || !Delta.NewlyUnlockedTierIds.IsEmpty();
        if (!bStrong && Now - LastReactionAt < 15.0) continue;
        LastReactionAt = Now; Out = MoveTemp(Delta); return true;
    }
    return false;
}
FString UHotStreakSubsystem::RequestStorePath() const
{ return FPaths::ProjectSavedDir() / TEXT("HotStreakClaims") / (FPaths::MakeValidFileName(PlayerId) + TEXT(".json")); }
bool UHotStreakSubsystem::SaveRequestIds() const
{
    if (PlayerId.IsEmpty()) return false;
    const FString Path = RequestStorePath(); IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    auto Json = MakeShared<FJsonObject>();
    for (const auto& Pair : PendingRequestIds) Json->SetStringField(Pair.Key, Pair.Value);
    FString Text;
    return FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Text)) && FFileHelper::SaveStringToFile(Text, *Path);
}
void UHotStreakSubsystem::LoadRequestIds()
{
    if (PlayerId.IsEmpty()) return;
    FString Text; TSharedPtr<FJsonObject> Json;
    if (!FFileHelper::LoadFileToString(Text, *RequestStorePath()) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid()) return;
    for (const auto& Pair : Json->Values)
    {
        FString Value;
        if (PendingRequestIds.Num() >= 128) break;
        FGuid Parsed;
        if (Pair.Key.Len() <= 150 && Pair.Value.IsValid() && Pair.Value->TryGetString(Value) && FGuid::ParseExact(Value, EGuidFormats::Digits, Parsed)) PendingRequestIds.Add(FString(Pair.Key.ToView()), Value);
    }
}
bool UHotStreakSubsystem::GetRequestId(const FString& Key, FString& Out)
{
    if (const FString* Existing = PendingRequestIds.Find(Key)) { Out = *Existing; return true; }
    if (PendingRequestIds.Num() >= 128) { LastError = TEXT("Too many pending event requests. Recover an existing claim first."); return false; }
    Out = FGuid::NewGuid().ToString(EGuidFormats::Digits); PendingRequestIds.Add(Key, Out);
    if (SaveRequestIds()) return true;
    PendingRequestIds.Remove(Key); LastError = TEXT("Could not save the claim for recovery. Please retry."); return false;
}
void UHotStreakSubsystem::RequestClaim(const FString& TierId)
{
    if (bRequestInFlight) return;
    const FHotStreakTier* Tier = Snapshot.FindTier(TierId);
    if (!Tier || Snapshot.PresentationVersion != 1 || (!Tier->bClaimable && !PendingRequestIds.Contains(Snapshot.EventId + TEXT("|") + TierId))) return;
    const FString EventId = Snapshot.EventId, RewardId = Tier->RewardId, Key = EventId + TEXT("|") + TierId;
    FString Id; if (!GetRequestId(Key, Id)) { OnChanged.Broadcast(); return; }
    auto Parameters = MakeShared<FJsonObject>(); Parameters->SetStringField(TEXT("ClaimId"), Id);
    Parameters->SetStringField(TEXT("EventId"), EventId); Parameters->SetStringField(TEXT("TierId"), TierId);
    const TWeakObjectPtr<UHotStreakSubsystem> WeakThis(this);
    Send(TEXT("ClaimHotStreakReward"), Parameters, [WeakThis, EventId, RewardId, TierId, Key](TSharedPtr<FJsonObject> Result)
    {
        if (!WeakThis.IsValid()) return;
        bool bClaimed = false; FString ReturnedEvent, ReturnedTier, ReturnedReward;
        const TSharedPtr<FJsonObject>* Profile = nullptr;
        if (!Result.IsValid() || !Result->TryGetBoolField(TEXT("Claimed"), bClaimed) || !bClaimed ||
            !Result->TryGetStringField(TEXT("EventId"), ReturnedEvent) || ReturnedEvent != EventId ||
            !Result->TryGetStringField(TEXT("TierId"), ReturnedTier) || ReturnedTier != TierId ||
            !Result->TryGetStringField(TEXT("RewardId"), ReturnedReward) || ReturnedReward != RewardId ||
            !Result->TryGetObjectField(TEXT("Profile"), Profile) || !Profile)
        { WeakThis->LastError = TEXT("Claim confirmation is incomplete. Retry to recover the original claim."); return; }
        WeakThis->ApplyProfile(*Profile); WeakThis->PendingRequestIds.Remove(Key); WeakThis->SaveRequestIds();
    });
}
void UHotStreakSubsystem::RequestEquipMild()
{
    const FString RewardId = TEXT("pepper_internal_mild_badge_v1");
    if (Snapshot.PresentationVersion != 1 || !Snapshot.OwnedRewardIds.Contains(RewardId) || bRequestInFlight) return;
    const FString Key = TEXT("equip|") + RewardId; FString Id;
    if (!GetRequestId(Key, Id)) { OnChanged.Broadcast(); return; }
    auto Parameters = MakeShared<FJsonObject>(); Parameters->SetStringField(TEXT("EquipId"), Id);
    Parameters->SetStringField(TEXT("RewardId"), RewardId); Parameters->SetStringField(TEXT("Slot"), TEXT("EventBadge"));
    const TWeakObjectPtr<UHotStreakSubsystem> WeakThis(this);
    Send(TEXT("EquipCasinoCosmetic"), Parameters, [WeakThis, Key](TSharedPtr<FJsonObject> Result)
    {
        if (!WeakThis.IsValid()) return;
        bool bEquipped = false; const TSharedPtr<FJsonObject>* Profile = nullptr;
        if (!Result.IsValid() || !Result->TryGetBoolField(TEXT("Equipped"), bEquipped) || !bEquipped || !Result->TryGetObjectField(TEXT("Profile"), Profile) || !Profile)
        { WeakThis->LastError = TEXT("Equipment confirmation is incomplete. Please retry."); return; }
        WeakThis->ApplyProfile(*Profile); WeakThis->PendingRequestIds.Remove(Key); WeakThis->SaveRequestIds();
    });
}
FString UHotStreakSubsystem::GetPhaseText() const
{
    if (!Snapshot.bAvailable) return TEXT("No event is available. Refresh to check again.");
    const FDateTime Now = Snapshot.ServerTime + FTimespan::FromSeconds(FMath::Max(0.0, FPlatformTime::Seconds() - SnapshotAt));
    const FDateTime Target = Snapshot.Phase == TEXT("Upcoming") ? Snapshot.StartsAt : Snapshot.Phase == TEXT("Active") ? Snapshot.EndsAt : Snapshot.ClaimUntil;
    const int64 Hours = FMath::Max<int64>(0, static_cast<int64>((Target - Now).GetTotalHours()));
    if (Snapshot.Phase == TEXT("Upcoming")) return FString::Printf(TEXT("Starts in %lld days %lld hours"), Hours / 24, Hours % 24);
    if (Snapshot.Phase == TEXT("Active")) return FString::Printf(TEXT("Ends in %lld days %lld hours"), Hours / 24, Hours % 24);
    if (Snapshot.Phase == TEXT("ClaimOnly")) return FString::Printf(TEXT("Earning ended | Claim window: %lld hours remaining"), Hours);
    return Snapshot.Phase == TEXT("Archived") ? TEXT("Event ended | Collected rewards stay yours") : TEXT("Earning paused | Banked Heat is safe");
}
FString UHotStreakSubsystem::GetHudText() const
{
    const auto& State = DisplayedSnapshot;
    if (!State.bAvailable) return TEXT("HOT STREAK WITH PEPPER\nOpen Events to check availability");
    FString Text = FString::Printf(TEXT("HOT STREAK\nSTREAK %lld  |  HEAT %lld\n"), State.CurrentStreak, State.Heat);
    if (State.Phase == TEXT("Active")) Text += State.CurrentStreak < 3 ? FString::Printf(TEXT("%lld paid wins to Heat"), 3 - State.CurrentStreak) : TEXT("Each paid win adds 1 Heat");
    else Text += State.Phase == TEXT("Upcoming") ? TEXT("Event starts soon") : State.Phase == TEXT("ClaimOnly") ? TEXT("Earning ended | Collect rewards") : State.Phase == TEXT("Archived") ? TEXT("Event ended | Collection saved") : TEXT("Earning paused | Heat is safe");
    if (State.EquippedRewardId == TEXT("pepper_internal_mild_badge_v1")) Text += TEXT("\nPEPPER'S MILD TITLE");
    return Text;
}
