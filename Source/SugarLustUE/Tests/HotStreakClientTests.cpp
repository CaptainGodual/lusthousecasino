#if WITH_DEV_AUTOMATION_TESTS
#include "../HotStreakSubsystem.h"
#include "../HotStreakWidget.h"
#include "../HotStreakPresenter.h"
#include "../CasinoAccountApi.h"
#include "../SugarLustRootWidget.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"
#include "RenderingThread.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif

namespace
{
    TSharedRef<FJsonObject> EventSnapshot(int64 Revision, int64 Streak, int64 Heat)
    {
        auto Json=MakeShared<FJsonObject>();Json->SetBoolField(TEXT("Available"),true);
        Json->SetStringField(TEXT("EventId"),TEXT("pepper_hot_streak_internal_v1"));
        Json->SetStringField(TEXT("Phase"),TEXT("Active"));Json->SetNumberField(TEXT("AccountRevision"),Revision);
        Json->SetNumberField(TEXT("RulesRevision"),1);Json->SetNumberField(TEXT("TriggerWins"),3);Json->SetNumberField(TEXT("ClientPresentationVersion"),1);
        Json->SetStringField(TEXT("ServerTime"),TEXT("2026-11-02T00:00:00Z"));Json->SetStringField(TEXT("StartsAt"),TEXT("2026-11-01T00:00:00Z"));
        Json->SetStringField(TEXT("EndsAt"),TEXT("2026-11-08T00:00:00Z"));Json->SetStringField(TEXT("ClaimUntil"),TEXT("2026-11-10T00:00:00Z"));
        auto Progress=MakeShared<FJsonObject>();Progress->SetNumberField(TEXT("CurrentStreak"),Streak);Progress->SetNumberField(TEXT("BestStreak"),FMath::Max<int64>(Streak,3));
        Progress->SetNumberField(TEXT("Heat"),Heat);Progress->SetNumberField(TEXT("ProgressRevision"),Revision);Json->SetObjectField(TEXT("Progress"),Progress);
        TArray<TSharedPtr<FJsonValue>> Tiers;const TCHAR* Ids[]={TEXT("Mild"),TEXT("Hot"),TEXT("ExtraHot"),TEXT("Inferno")};const int32 Targets[]={1,5,15,30};
        for(int32 I=0;I<4;++I)
        {
            auto Tier=MakeShared<FJsonObject>();Tier->SetStringField(TEXT("TierId"),Ids[I]);Tier->SetNumberField(TEXT("HeatRequired"),Targets[I]);
            Tier->SetStringField(TEXT("RewardId"),I==0?TEXT("pepper_internal_mild_badge_v1"):FString::Printf(TEXT("preview_%d"),I));
            Tier->SetStringField(TEXT("RewardName"),I==0?TEXT("Pepper's Mild title"):TEXT("Pepper collection preview"));
            Tier->SetStringField(TEXT("RewardKind"),I==0?TEXT("Presentation"):TEXT("Wearable"));Tier->SetStringField(TEXT("EquipSlot"),I==0?TEXT("EventBadge"):TEXT("Outfit"));
            Tier->SetBoolField(TEXT("RewardReady"),I==0);Tier->SetBoolField(TEXT("Unlocked"),Heat>=Targets[I]);Tier->SetBoolField(TEXT("Claimed"),false);
            Tier->SetBoolField(TEXT("Claimable"),I==0 && Heat>=Targets[I]);Tiers.Add(MakeShared<FJsonValueObject>(Tier));
        }
        Json->SetArrayField(TEXT("Tiers"),Tiers);Json->SetArrayField(TEXT("OwnedRewardIds"),{});
        Json->SetArrayField(TEXT("EligibleGames"),{MakeShared<FJsonValueString>(TEXT("SugarLust")),MakeShared<FJsonValueString>(TEXT("NeonNights"))});
        Json->SetStringField(TEXT("EquippedRewardId"),TEXT(""));return Json;
    }
    TSharedRef<FJsonObject> Settled(int64 Revision,const FString& SpinId,const FString& Trigger,int64 Streak,int64 Heat,bool bReplay=false)
    {
        auto Result=MakeShared<FJsonObject>(),Profile=MakeShared<FJsonObject>(),Delta=MakeShared<FJsonObject>();
        Profile->SetObjectField(TEXT("HotStreak"),EventSnapshot(Revision,Streak,Heat));Result->SetObjectField(TEXT("Profile"),Profile);Result->SetBoolField(TEXT("Replayed"),bReplay);
        Delta->SetStringField(TEXT("EventId"),TEXT("pepper_hot_streak_internal_v1"));Delta->SetStringField(TEXT("SpinId"),SpinId);
        Delta->SetStringField(TEXT("TriggerKind"),Trigger);Delta->SetBoolField(TEXT("Eligible"),true);Delta->SetNumberField(TEXT("RulesRevision"),1);
        Delta->SetNumberField(TEXT("ProgressRevision"),Revision);Delta->SetNumberField(TEXT("StreakBefore"),Streak>0?Streak-1:4);
        Delta->SetNumberField(TEXT("StreakAfter"),Streak);Delta->SetNumberField(TEXT("HeatAdded"),Streak>=3?1:0);
        TArray<TSharedPtr<FJsonValue>> Unlocked;if(Trigger==TEXT("Entrance"))Unlocked.Add(MakeShared<FJsonValueString>(TEXT("Mild")));
        Delta->SetArrayField(TEXT("NewlyUnlockedTierIds"),Unlocked);Result->SetObjectField(TEXT("HotStreakDelta"),Delta);return Result;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHotStreakClientTest,"SugarLust.HotStreak.ClientState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHotStreakClientTest::RunTest(const FString& Parameters)
{
    auto* GameInstance=NewObject<UGameInstance>();
    auto* Event=NewObject<UHotStreakSubsystem>(GameInstance);
    TestTrue(TEXT("Strict parser accepts trusted event state"),Event->ApplySnapshot(EventSnapshot(2,2,0)));
    Event->SetResultPending();const auto Entrance=Settled(3,TEXT("third"),TEXT("Entrance"),3,1);
    Event->ApplySettledSpin(Entrance,TEXT("third"));
    TestEqual(TEXT("Authoritative state is cached immediately"),Event->GetSnapshot().Heat,int64(1));
    TestEqual(TEXT("HUD does not reveal a pending reel result"),Event->GetDisplayedSnapshot().Heat,int64(0));
    Event->RevealSettledProgress();TestEqual(TEXT("Reel stop reveals banked Heat"),Event->GetDisplayedSnapshot().Heat,int64(1));
    FHotStreakDelta Cue;TestTrue(TEXT("Third win queues Pepper entrance"),Event->PopPresentation(Cue,100));
    TestEqual(TEXT("Entrance carries one tier cue"),Cue.NewlyUnlockedTierIds.Num(),1);
    Event->ApplySettledSpin(Entrance,TEXT("third"));TestFalse(TEXT("Duplicate spin never repeats Pepper"),Event->PopPresentation(Cue,120));
    TestFalse(TEXT("Older snapshot cannot roll progress back"),Event->ApplySnapshot(EventSnapshot(1,0,0)));
    auto Corrupt=EventSnapshot(4,3,1);Corrupt->SetNumberField(TEXT("AccountRevision"),3.5);
    TestFalse(TEXT("Fractional revisions are rejected"),Event->ApplySnapshot(Corrupt));
    auto Unknown=EventSnapshot(4,3,1);Unknown->SetStringField(TEXT("FutureOptionalField"),TEXT("safe"));
    TestTrue(TEXT("Unknown optional fields do not invalidate trusted progress"),Event->ApplySnapshot(Unknown));
    Event->ApplySettledSpin(Settled(5,TEXT("old-receipt"),TEXT("Entrance"),4,2,true),TEXT("old-receipt"));
    TestFalse(TEXT("Receipt replay refreshes counters without an old entrance"),Event->PopPresentation(Cue,140));
    TestEqual(TEXT("Receipt snapshot can be newer than its immutable delta"),Event->GetSnapshot().Heat,int64(2));
    Event->ApplySettledSpin(MakeShared<FJsonObject>(),TEXT("optional-missing"));
    TestEqual(TEXT("Missing optional event payload leaves counters intact"),Event->GetSnapshot().Heat,int64(2));
    Event->ApplySettledSpin(Settled(6,TEXT("extend1"),TEXT("Extension"),5,3),TEXT("extend1"));
    Event->ApplySettledSpin(Settled(7,TEXT("extend2"),TEXT("Extension"),6,4),TEXT("extend2"));
    TestEqual(TEXT("Rapid ordinary reactions coalesce"),Event->Presentations.Num(),1);
    TestTrue(TEXT("Ordinary cue is available after cooldown"),Event->PopPresentation(Cue,150));
    Event->ApplySettledSpin(Settled(8,TEXT("break"),TEXT("StreakBreak"),0,4),TEXT("break"));
    TestFalse(TEXT("Ordinary reactions respect fifteen second cooldown"),Event->PopPresentation(Cue,155));
    Event->PlayerId=TEXT("old_account");Event->SessionTicket=TEXT("test_only_session");const uint64 Generation=Event->LoginGeneration;
    TestTrue(TEXT("Account request guard accepts its original generation"),Event->IsCurrentSession(Generation,TEXT("old_account")));
    TestTrue(TEXT("Cabinet transport can validate its shared ticket"),Event->MatchesSession(Generation,TEXT("test_only_session")));
    TestFalse(TEXT("Another account ticket cannot apply a callback"),Event->MatchesSession(Generation,TEXT("another_ticket")));
    Event->ClearAccount();TestFalse(TEXT("Logout rejects old in-flight callbacks"),Event->IsCurrentSession(Generation,TEXT("old_account")));
    TestFalse(TEXT("Logout clears cached event ownership"),Event->GetSnapshot().bAvailable);
    TestTrue(TEXT("Logout clears queued presentations"),Event->Presentations.IsEmpty());
    Event->PlayerId=TEXT("hotstreak_automation_claim_recovery");
    FString FirstId,ReloadedId;
    TestTrue(TEXT("Claim ID is saved before transport"),Event->GetRequestId(TEXT("season|Mild"),FirstId));
    Event->PendingRequestIds.Reset();Event->LoadRequestIds();
    TestTrue(TEXT("Claim ID can be recovered after reopening"),Event->GetRequestId(TEXT("season|Mild"),ReloadedId));
    TestEqual(TEXT("Manual retry uses the persisted claim ID"),FirstId,ReloadedId);
    Event->ClearAccount();
    TestTrue(TEXT("Claims explicitly retry their stable ID"),FCasinoAccountApi::ShouldRetry(TEXT("ClaimHotStreakReward"),TEXT("claim"),0,503,TEXT(""),true));
    TestFalse(TEXT("Claim retries remain bounded"),FCasinoAccountApi::ShouldRetry(TEXT("ClaimHotStreakReward"),TEXT("claim"),2,503,TEXT(""),true));
    TestFalse(TEXT("Claim without a saved ID cannot automatically retry"),FCasinoAccountApi::ShouldRetry(TEXT("ClaimHotStreakReward"),TEXT(""),0,503,TEXT(""),true));
    auto* Root=NewObject<USugarLustRootWidget>();
    TestTrue(TEXT("Idle result is ready for event presentation"),Root->IsHotStreakPresentationReady());
    Root->bBonusPickRequestInFlight=true;TestFalse(TEXT("Candy Jar request blocks Pepper"),Root->IsHotStreakPresentationReady());Root->bBonusPickRequestInFlight=false;
    Root->bNeonHeatBonusActive=true;TestFalse(TEXT("Neon Heat blocks Pepper"),Root->IsHotStreakPresentationReady());Root->bNeonHeatBonusActive=false;
    Root->bNeonPayoutAnimating=true;TestFalse(TEXT("Payout counting blocks Pepper"),Root->IsHotStreakPresentationReady());Root->bNeonPayoutAnimating=false;
    Root->PendingSugarJackpotPrize=1;TestFalse(TEXT("Pending jackpot has priority over Pepper"),Root->IsHotStreakPresentationReady());Root->PendingSugarJackpotPrize=0;
    Root->bNeonHeatBonusComplete=true;TestFalse(TEXT("Bonus completion hold blocks Pepper"),Root->IsHotStreakPresentationReady());Root->bNeonHeatBonusComplete=false;
    Root->PepperPresenter=NewObject<UHotStreakPresenter>(Root);
    FHotStreakDelta VisibleCue;VisibleCue.TriggerKind=TEXT("Entrance");Root->PepperPresenter->Present(VisibleCue);
    TestEqual(TEXT("Pepper never captures spin input"),Root->PepperPresenter->GetVisibility(),ESlateVisibility::HitTestInvisible);
    Root->bNeonHeatBonusActive=true;Root->UpdateHotStreakPresentation();
    TestFalse(TEXT("A new mandatory bonus dismisses an already visible reaction"),Root->PepperPresenter->IsPresenting());Root->bNeonHeatBonusActive=false;
    Event->ApplySnapshot(EventSnapshot(10,3,1));
    auto* Panel=NewObject<UHotStreakWidget>();Panel->WidgetTree=NewObject<UWidgetTree>(Panel);Panel->NativeOnInitialized();Panel->BindEvent(Event);
    TestEqual(TEXT("Event panel includes all four collection tiers"),Panel->ClaimButtons.Num(),4);
    if(FParse::Param(FCommandLine::Get(),TEXT("HotStreakScreenshots")))
    {
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("HotStreakVerification");IFileManager::Get().MakeDirectory(*Folder,true);
        auto Render=[&](UUserWidget* Widget,FVector2D Size,const TCHAR* Filename)
        {
#if WITH_EDITOR
            // A freshly imported texture can still be compiling on the first editor frame.
            FTextureCompilingManager::Get().FinishAllCompilation();
#endif
            FlushRenderingCommands();
            TSharedRef<SWidget> View=Widget->TakeWidget();View->SlatePrepass();FWidgetRenderer Renderer(false);
            Renderer.DrawWidget(View,Size);FlushRenderingCommands();View->SlatePrepass();
            UTextureRenderTarget2D* Target=Renderer.DrawWidget(View,Size);FlushRenderingCommands();FBufferArchive Png;
            return Target && FImageUtils::ExportRenderTarget2DAsPNG(Target,Png) && FFileHelper::SaveArrayToFile(Png,*(Folder/Filename));
        };
        TestTrue(TEXT("Render the actual Hot Streak event panel"),Render(Panel,FVector2D(1920,1080),TEXT("event-panel.png")));
        Root->PepperPresenter->WidgetTree=NewObject<UWidgetTree>(Root->PepperPresenter);Root->PepperPresenter->NativeOnInitialized();
        VisibleCue.TriggerKind=TEXT("Extension");VisibleCue.NewlyUnlockedTierIds={TEXT("ExtraHot")};Root->PepperPresenter->Present(VisibleCue);
        TestTrue(TEXT("Render the Pepper tier unlock cue"),Render(Root->PepperPresenter,FVector2D(270,330),TEXT("pepper-unlock.png")));
        Root->PepperPresenter->Dismiss();
    }
    Panel->BindEvent(nullptr);
    return true;
}
#endif
