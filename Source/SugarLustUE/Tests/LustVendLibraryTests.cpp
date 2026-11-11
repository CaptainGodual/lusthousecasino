#if WITH_DEV_AUTOMATION_TESTS
#include "../LustVendWidget.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLustVendLibraryTest,"SugarLust.LustVend.RewardLibrary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLustVendLibraryTest::RunTest(const FString& Parameters)
{
    ULustVendWidget* Widget=NewObject<ULustVendWidget>();
    Widget->Skus={TEXT("vid-chip"),TEXT("holo-disk")};
    Widget->Names={TEXT("Vid Chip"),TEXT("Holo Disk")};
    Widget->bReady=true;Widget->Balance=125;Widget->Owned.Add(TEXT("vid-chip"));
    TestFalse(TEXT("Unowned packages cannot be opened"),Widget->OpenReward(1));
    TestFalse(TEXT("Invalid selection cannot be opened"),Widget->OpenReward(INDEX_NONE));
    TestTrue(TEXT("Previously purchased package opens from server ownership"),Widget->OpenReward(0));
    TestTrue(TEXT("Package reveal opens above the library"),Widget->bShowingLibrary && Widget->bViewingReward);
    TestEqual(TEXT("Package starts with an opening presentation"),Widget->RewardOpeningSeconds,0.f);
    // Media is intentionally absent; show an honest placeholder, not a fake grant.
    Widget->RewardImages.Reset();Widget->RewardVideos.Reset();
    Widget->FinishRewardOpening();
    TestTrue(TEXT("Empty package explains that content is coming"),Widget->RewardMessage.Contains(TEXT("CONTENT COMING SOON")));
    TestEqual(TEXT("Opening does not spend gems again"),Widget->Balance,static_cast<int64>(125));
    TestTrue(TEXT("Opening does not consume the entitlement"),Widget->Owned.Contains(TEXT("vid-chip")));
    Widget->CloseRewardViewer();
    TestFalse(TEXT("Returning to collection closes the viewer"),Widget->bViewingReward);
    TestTrue(TEXT("Owned content can be reopened"),Widget->OpenReward(0));
    Widget->Owned.Reset();Widget->LoadRewardPage();
    TestFalse(TEXT("Stale ownership cannot continue revealing content"),Widget->bViewingReward);
    Widget->Owned.Add(TEXT("vid-chip"));Widget->bBusy=true;
    TestFalse(TEXT("Inventory refresh must finish before opening"),Widget->OpenReward(0));
    Widget->bBusy=false;Widget->bReady=false;
    TestFalse(TEXT("Unverified inventory cannot open packages"),Widget->OpenReward(0));
    TestTrue(TEXT("Parent traversal is rejected"),ULustVendWidget::ResolveRewardVideoPath(TEXT("../private.mp4")).IsEmpty());
    TestTrue(TEXT("External URLs are rejected"),ULustVendWidget::ResolveRewardVideoPath(TEXT("https://example.com/video.mp4")).IsEmpty());
    TestTrue(TEXT("Absolute paths are rejected"),ULustVendWidget::ResolveRewardVideoPath(TEXT("C:/private.mp4")).IsEmpty());
    TestTrue(TEXT("Configured videos resolve under Content Movies"),ULustVendWidget::ResolveRewardVideoPath(TEXT("Rewards/vid-chip.mp4")).EndsWith(TEXT("/Movies/Rewards/vid-chip.mp4")));
    if(FParse::Param(FCommandLine::Get(),TEXT("RewardLibraryScreenshots")))
    {
        Widget->bReady=true;Widget->bBusy=false;
        Widget->Skus={TEXT("vid-chip"),TEXT("holo-disk"),TEXT("pinup-pack"),TEXT("avatar-item"),TEXT("theme-pack"),TEXT("special-item")};
        Widget->Names={TEXT("Vid Chip"),TEXT("Holo Disk"),TEXT("Pinup Pack"),TEXT("Avatar Item"),TEXT("Theme Pack"),TEXT("Special Item")};
        Widget->Prices={60,90,120,180,300,600};
        Widget->Owned.Add(TEXT("pinup-pack"));Widget->bShowingLibrary=true;
        TSharedRef<SWidget> UI=Widget->RebuildWidget();
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("RewardLibraryQA");
        IFileManager::Get().MakeDirectory(*Folder,true);
        auto Capture=[this,&UI,&Folder](const TCHAR* Name)
        {
            UI->Invalidate(EInvalidateWidgetReason::Layout);
            UI->SlatePrepass();
            FWidgetRenderer Renderer(false);
            UTextureRenderTarget2D* Target=Renderer.DrawWidget(UI,FVector2D(1280,900));
            FBufferArchive Png;
            const bool bExported=Target && FImageUtils::ExportRenderTarget2DAsPNG(Target,Png);
            TestTrue(TEXT("Render reward UI for visual verification"),bExported && FFileHelper::SaveArrayToFile(Png,*(Folder/Name)));
        };
        Capture(TEXT("library.png"));
        Widget->OpenReward(0);Widget->RewardImages.Reset();Widget->RewardVideos.Reset();Widget->FinishRewardOpening();
        Capture(TEXT("empty-package.png"));
        Widget->CloseRewardViewer();
    }
    return true;
}
#endif
