#if WITH_DEV_AUTOMATION_TESTS
#include "../SugarLustRootWidget.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Components/CanvasPanel.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Blueprint/WidgetTree.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNeonWinFeedbackTest, "SugarLust.NeonNights.WinFeedback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNeonWinFeedbackTest::RunTest(const FString& Parameters)
{
    USugarLustRootWidget* Widget = NewObject<USugarLustRootWidget>();
    auto ResetGrid = [Widget]()
    {
        Widget->NeonBet = 100;
        Widget->NeonBaseWin = 0;
        Widget->bNeonCurrentSpinIsBonus = false;
        Widget->NeonVisibleGrid.Reset();
        // Each reel has a different filler, so it cannot create a ways win.
        for (int32 Row = 0; Row < 4; ++Row)
            for (int32 Reel = 0; Reel < 5; ++Reel) Widget->NeonVisibleGrid.Add(5 + Reel);
    };

    ResetGrid();
    Widget->NeonVisibleGrid[0] = 0;
    Widget->NeonVisibleGrid[6] = 11;
    Widget->NeonVisibleGrid[12] = 0;
    Widget->NeonBaseWin = 200;
    Widget->BuildNeonWinPresentation();
    TestEqual(TEXT("Three-reel win with a wild produces one symbol group"), Widget->NeonWinningGroups.Num(), 1);
    if (Widget->NeonWinningGroups.Num() == 1)
    {
        const auto& Group = Widget->NeonWinningGroups[0];
        TestEqual(TEXT("Path stops before the nonmatching fourth reel"), Group.CellsByReel.Num(), 3);
        TestEqual(TEXT("One matching position per reel gives one way"), Group.Ways, 1);
        TestEqual(TEXT("Wild cell is highlighted in its actual row"), Group.CellsByReel[1][0], 6);
    }

    ResetGrid();
    Widget->NeonVisibleGrid[0] = Widget->NeonVisibleGrid[5] = Widget->NeonVisibleGrid[1] = Widget->NeonVisibleGrid[18] = 0;
    Widget->NeonVisibleGrid[12] = 11;
    Widget->NeonBaseWin = 1200;
    Widget->BuildNeonWinPresentation();
    TestEqual(TEXT("Four-reel multiway win has one symbol group"), Widget->NeonWinningGroups.Num(), 1);
    if (Widget->NeonWinningGroups.Num() == 1)
    {
        TestEqual(TEXT("Two matching cells on reel one produce two ways"), Widget->NeonWinningGroups[0].Ways, 2);
        TestEqual(TEXT("Four matching reels are retained"), Widget->NeonWinningGroups[0].CellsByReel.Num(), 4);
    }
    Widget->NeonBaseWin = 1201;
    AddExpectedError(TEXT("displayed grid does not match authoritative BaseWin"), EAutomationExpectedErrorFlags::Contains, 1);
    Widget->BuildNeonWinPresentation();
    TestTrue(TEXT("Server payout mismatch suppresses misleading highlights"), Widget->NeonWinningGroups.IsEmpty());

    ResetGrid();
    Widget->NeonVisibleGrid[0] = Widget->NeonVisibleGrid[7] = Widget->NeonVisibleGrid[14] = 12;
    Widget->NeonBaseWin = 1000;
    Widget->BuildNeonWinPresentation();
    TestEqual(TEXT("Scatters highlight all three positions without a ways line"), Widget->NeonWinningScatterCells.Num(), 3);
    TestTrue(TEXT("Scatter-only payout has no connecting paths"), Widget->NeonWinningGroups.IsEmpty());

    Widget->bNeonCurrentSpinIsBonus = true;
    Widget->BuildNeonWinPresentation();
    TestTrue(TEXT("Hold-and-spin results do not pretend to be base-game ways wins"), Widget->NeonWinningGroups.IsEmpty() && Widget->NeonWinningScatterCells.IsEmpty());
    ResetGrid();
    Widget->BuildNeonWinPresentation();
    TestTrue(TEXT("Losing spin clears the previous winning combination"), Widget->NeonWinningGroups.IsEmpty() && Widget->NeonWinningScatterCells.IsEmpty());

    ResetGrid();
    Widget->NeonVisibleGrid.Init(13, 20);
    Widget->NeonFreeSpins.LastAward = 20;
    Widget->BuildNeonWinPresentation();
    TestEqual(TEXT("Free Spins symbols get trigger highlights without fake line wins"), Widget->NeonWinningFreeSpinCells.Num(), 20);
    TestTrue(TEXT("Free Spins symbols are not paying regular symbols"), Widget->NeonWinningGroups.IsEmpty());

    ResetGrid();
    Widget->NeonVisibleGrid[0] = Widget->NeonVisibleGrid[1] = Widget->NeonVisibleGrid[2] = 18;
    Widget->NeonBaseWin = 1000;
    Widget->BuildNeonWinPresentation();
    TestEqual(TEXT("Three-reel Neon Muse win highlights the new regular symbol"), Widget->NeonWinningGroups.Num(), 1);
    if (Widget->NeonWinningGroups.Num() == 1)
        TestEqual(TEXT("Muse path contains three reels"), Widget->NeonWinningGroups[0].CellsByReel.Num(), 3);
    Widget->NeonVisibleGrid[1] = 11;
    Widget->BuildNeonWinPresentation();
    TestEqual(TEXT("Wild substitution preserves the Muse winning path"), Widget->NeonWinningGroups.Num(), 1);
    Widget->NeonVisibleGrid[2] = 12;
    Widget->NeonBaseWin = 0;
    Widget->BuildNeonWinPresentation();
    TestTrue(TEXT("Only two Muse reels have no winning highlight"), Widget->NeonWinningGroups.IsEmpty());

    const TSharedRef<FJsonObject> Profile = MakeShared<FJsonObject>();
    const TSharedRef<FJsonObject> Features = MakeShared<FJsonObject>();
    auto FreeState = [](int32 Remaining, int32 Bet)
    {
        TSharedRef<FJsonObject> State = MakeShared<FJsonObject>();
        State->SetNumberField(TEXT("Remaining"), Remaining);
        State->SetNumberField(TEXT("TotalAwarded"), 20);
        State->SetNumberField(TEXT("TotalWin"), 1250);
        State->SetNumberField(TEXT("BetPerLine"), Bet);
        State->SetNumberField(TEXT("ActiveLines"), 20);
        return State;
    };
    Features->SetObjectField(TEXT("SugarLust"), FreeState(10, 12));
    Features->SetObjectField(TEXT("NeonNights"), FreeState(15, 500));
    Profile->SetObjectField(TEXT("FreeSpins"), Features);
    Widget->NeonAutoSpinsRemaining = 8;
    Widget->ApplyFreeSpinData(Profile);
    TestEqual(TEXT("Sugar Lust free-spin count resumes independently"), Widget->SugarFreeSpins.Remaining, 10);
    TestEqual(TEXT("Neon free-spin count resumes independently"), Widget->NeonFreeSpins.Remaining, 15);
    TestEqual(TEXT("Neon restores the server-locked wager"), Widget->NeonBet, 500);
    TestEqual(TEXT("Paid auto spins stop when free spins start"), Widget->NeonAutoSpinsRemaining, 0);
    TestEqual(TEXT("Feature win survives profile refresh"), Widget->NeonFreeSpins.TotalWin, static_cast<int64>(1250));


    Widget->SugarFreeSpins.bPaused = false;
    Widget->HandleSpin();
    TestTrue(TEXT("Sugar Lust SPIN pauses automatic free spins"), Widget->SugarFreeSpins.bPaused);
    Widget->HandleSpin();
    TestFalse(TEXT("Sugar Lust SPIN resumes automatic free spins"), Widget->SugarFreeSpins.bPaused);
    Widget->NeonFreeSpins.bPaused = false;
    Widget->HandleNeonSpin();
    TestTrue(TEXT("Neon SPIN pauses automatic free spins"), Widget->NeonFreeSpins.bPaused);
    Widget->HandleNeonSpin();
    TestFalse(TEXT("Neon SPIN resumes automatic free spins"), Widget->NeonFreeSpins.bPaused);

    Widget->NeonFreeSpins.DelaySeconds = 1.0f;
    Widget->bNeonHeatBonusActive = true;
    Widget->UpdateNeonNights(0.25f);
    TestEqual(TEXT("Neon Heat freezes the free-spin countdown"), Widget->NeonFreeSpins.DelaySeconds, 1.0f);
    TestEqual(TEXT("Neon Heat leaves queued free spins untouched"), Widget->NeonFreeSpins.Remaining, 15);
    Widget->bNeonHeatBonusActive = false;
    Widget->bNeonHeatBonusComplete = true;
    Widget->NeonHeatBonusCompleteSeconds = 4.0f;
    Widget->bNeonPayoutAnimating = true;
    Widget->UpdateNeonNights(0.25f);
    TestEqual(TEXT("Heat completion hold does not expire during its payout"), Widget->NeonHeatBonusCompleteSeconds, 4.0f);
    TestEqual(TEXT("Free-spin countdown stays frozen during Heat payout"), Widget->NeonFreeSpins.DelaySeconds, 1.0f);
    Widget->bNeonPayoutAnimating = false;
    Widget->UpdateNeonNights(0.25f);
    TestEqual(TEXT("Free spins remain paused for the completion display"), Widget->NeonFreeSpins.DelaySeconds, 1.0f);


    ResetGrid();
    Widget->NeonVisibleGrid[0] = Widget->NeonVisibleGrid[6] = Widget->NeonVisibleGrid[12] = 14;
    Widget->PendingNeonJackpotTier = TEXT("Mini");
    Widget->PendingNeonJackpotPrize = 125000000;
    Widget->BuildNeonWinPresentation();
    TestEqual(TEXT("Only the matching tier's three symbols highlight"), Widget->NeonWinningJackpotCells.Num(), 3);
    TestTrue(TEXT("Progressive symbols are separate from ordinary line wins"), Widget->NeonWinningGroups.IsEmpty());
    Widget->ProgressiveJackpotCanvas = NewObject<UCanvasPanel>(Widget);
    Widget->ProgressiveJackpotCanvas->SetVisibility(ESlateVisibility::Visible);
    TestTrue(TEXT("Jackpot reveal blocks automatic continuation"), Widget->IsProgressiveJackpotVisible());
    Widget->HandleCloseProgressiveJackpot();
    TestFalse(TEXT("Continue releases the jackpot reveal"), Widget->IsProgressiveJackpotVisible());
    TestEqual(TEXT("Queued free spins get a pause before resuming"), Widget->NeonFreeSpins.DelaySeconds, 1.0f);

    for (const FString Function : {FString(TEXT("Spin")), FString(TEXT("NeonNightsSpin"))})
    {
        TestTrue(TEXT("Transient bridge failure retries the original spin"), USugarLustRootWidget::ShouldRetrySpinRequest(Function, TEXT("same-id"), 0, 200, TEXT("CloudScriptHTTPRequestError"), true));
        TestTrue(TEXT("Lost transport response can recover the stored receipt"), USugarLustRootWidget::ShouldRetrySpinRequest(Function, TEXT("same-id"), 1, 0, TEXT(""), false));
        TestTrue(TEXT("Gateway unavailable response is retryable"), USugarLustRootWidget::ShouldRetrySpinRequest(Function, TEXT("same-id"), 0, 503, TEXT(""), true));
        TestFalse(TEXT("Retry limit prevents an endless reconnect loop"), USugarLustRootWidget::ShouldRetrySpinRequest(Function, TEXT("same-id"), 2, 200, TEXT("CloudScriptHTTPRequestError"), true));
        TestFalse(TEXT("Rules and Javascript errors remain visible"), USugarLustRootWidget::ShouldRetrySpinRequest(Function, TEXT("same-id"), 0, 200, TEXT("JavascriptException"), true));
        TestFalse(TEXT("Missing spin identity cannot retry automatically"), USugarLustRootWidget::ShouldRetrySpinRequest(Function, TEXT(""), 0, 503, TEXT(""), true));
        TestFalse(TEXT("Invalid wagers are not automatically repeated"), USugarLustRootWidget::ShouldRetrySpinRequest(Function, TEXT("same-id"), 0, 400, TEXT(""), true));
    }
    TestFalse(TEXT("Non-spin reward requests do not gain automatic retries"), USugarLustRootWidget::ShouldRetrySpinRequest(TEXT("ClaimDailyBonus"), TEXT("same-id"), 0, 503, TEXT(""), true));

    // Verify the real synthesizer PCM, then export it for listening review.
    constexpr int32 CueRate = 22050;
    for (const float Seconds : {1.0f, 1.5f, 2.7f, 4.0f})
    {
        const TArray<uint8> Cue = USugarLustRootWidget::BuildPayoutWinPcm(Seconds, 0.30f, CueRate);
        TestEqual(TEXT("Procedural cue ends on the requested payout frame"), Cue.Num(), FMath::RoundToInt(Seconds * CueRate) * static_cast<int32>(sizeof(int16)));
        const int16* Samples = reinterpret_cast<const int16*>(Cue.GetData());
        int32 Peak = 0;
        for (int32 I = 0; I < Cue.Num() / sizeof(int16); ++I) Peak = FMath::Max(Peak, FMath::Abs(static_cast<int32>(Samples[I])));
        TestTrue(TEXT("Synthesized motif is audible with headroom"), Peak > 1000 && Peak < 30000);
        TestEqual(TEXT("Win cue begins at zero to avoid a click"), Samples[0], static_cast<int16>(0));
        TestEqual(TEXT("Win cue releases to zero at payout completion"), Samples[Cue.Num() / sizeof(int16) - 1], static_cast<int16>(0));
    }
    const TArray<uint8> Preview = USugarLustRootWidget::BuildPayoutWinPcm(2.7f, 0.30f, CueRate);
    TArray<uint8> Wav;
    auto Tag = [&Wav](const ANSICHAR* Text) { Wav.Append(reinterpret_cast<const uint8*>(Text), 4); };
    auto U32 = [&Wav](uint32 Value) { Wav.Append(reinterpret_cast<const uint8*>(&Value), sizeof(Value)); };
    auto U16 = [&Wav](uint16 Value) { Wav.Append(reinterpret_cast<const uint8*>(&Value), sizeof(Value)); };
    Tag("RIFF"); U32(36 + Preview.Num()); Tag("WAVE"); Tag("fmt "); U32(16);
    U16(1); U16(1); U32(CueRate); U32(CueRate * 2); U16(2); U16(16); Tag("data"); U32(Preview.Num()); Wav.Append(Preview);
    const FString PreviewFolder = FPaths::ProjectSavedDir() / TEXT("AudioPreviews");
    IFileManager::Get().MakeDirectory(*PreviewFolder, true);
    TestTrue(TEXT("Export actual procedural cue for listening review"), FFileHelper::SaveArrayToFile(Wav, *(PreviewFolder / TEXT("procedural-win-reference-style.wav"))));

    if(FParse::Param(FCommandLine::Get(),TEXT("LobbyLayoutScreenshots")))
    {
        Widget->WidgetTree=NewObject<UWidgetTree>(Widget,TEXT("LobbyQATree"));
        Widget->NativeOnInitialized();
        Widget->bHasShownLobbyHostGreeting=true;
        Widget->BuildLobby();
        TSharedRef<SWidget> Lobby=Widget->DesignCanvas->TakeWidget();
        Lobby->SlatePrepass();
        FWidgetRenderer Renderer(false);
        UTextureRenderTarget2D* Target=Renderer.DrawWidget(Lobby,FVector2D(1920,1080));
        FBufferArchive Png;
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("LobbyLayoutQA");
        IFileManager::Get().MakeDirectory(*Folder,true);
        TestTrue(TEXT("Render compact lobby HUD for visual verification"),Target && FImageUtils::ExportRenderTarget2DAsPNG(Target,Png) && FFileHelper::SaveArrayToFile(Png,*(Folder/TEXT("compact-header.png"))));
    }
    return true;
}
#endif
