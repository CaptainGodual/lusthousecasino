#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "HotStreakTypes.h"
#include "HotStreakSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FHotStreakChanged);

/** Account-scoped trusted event cache. Widgets never award Heat or ownership. */
UCLASS()
class SUGARLUSTUE_API UHotStreakSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void SetAccount(const FString& Player, const FString& Ticket);
    void ClearAccount();
    void Refresh();
    bool ApplySnapshot(const TSharedPtr<FJsonObject>& Json);
    void ApplyProfile(const TSharedPtr<FJsonObject>& Profile);
    void ApplySettledSpin(const TSharedPtr<FJsonObject>& Result, const FString& SpinId);
    void SetResultPending() { bDeferDisplay = true; }
    void RevealSettledProgress();
    bool PopPresentation(FHotStreakDelta& Out, double Now);
    void RequestClaim(const FString& TierId);
    void RequestEquipMild();
    bool IsBusy() const { return bRequestInFlight; }
    uint64 GetLoginGeneration() const { return LoginGeneration; }
    bool HasPendingClaim(const FString& TierId) const { return PendingRequestIds.Contains(Snapshot.EventId + TEXT("|") + TierId); }
    bool RestoreSession(FString& Player, FString& Ticket) const;
    bool IsCurrentSession(uint64 ExpectedGeneration, const FString& Player) const;
    bool MatchesSession(uint64 ExpectedGeneration, const FString& Ticket) const;
    const FHotStreakSnapshot& GetSnapshot() const { return Snapshot; }
    const FHotStreakSnapshot& GetDisplayedSnapshot() const { return DisplayedSnapshot; }
    FString GetHudText() const;
    FString GetPhaseText() const;
    const FString& GetError() const { return LastError; }
    FHotStreakChanged OnChanged;
private:
    friend class FHotStreakClientTest;
    FHotStreakSnapshot Snapshot, DisplayedSnapshot;
    TArray<FHotStreakDelta> Presentations;
    TArray<FString> SeenPresentationIds;
    TMap<FString, FString> PendingRequestIds;
    FString PlayerId, SessionTicket, LastError;
    uint64 LoginGeneration = 0;
    bool bRequestInFlight = false, bForeground = true, bDeferDisplay = false;
    double LastRefreshAt = 0, SnapshotAt = 0, LastReactionAt = -1000;
    FTSTicker::FDelegateHandle PollHandle;
    FDelegateHandle ForegroundHandle, BackgroundHandle;
    bool TickRefresh(float Delta);
    void EnterForeground();
    void EnterBackground();
    void Send(const FString& Function, const TSharedRef<FJsonObject>& Parameters,
        TFunction<void(TSharedPtr<FJsonObject>)> Success);
    FString RequestStorePath() const;
    bool SaveRequestIds() const;
    void LoadRequestIds();
    bool GetRequestId(const FString& Key, FString& Out);
};
