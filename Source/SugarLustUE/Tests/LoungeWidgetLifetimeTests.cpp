#if WITH_DEV_AUTOMATION_TESTS
#include "../SugarLustRootWidget.h"
#include "Misc/AutomationTest.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "UObject/GarbageCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoungeWidgetLifetimeTest,
    "SugarLust.Lounge.WidgetLifetime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLoungeWidgetLifetimeTest::RunTest(const FString& Parameters)
{
    auto* Widget = NewObject<USugarLustRootWidget>();
    Widget->AddToRoot();
    Widget->WidgetTree = NewObject<UWidgetTree>(Widget);
    Widget->DesignCanvas = Widget->WidgetTree->ConstructWidget<UCanvasPanel>();
    Widget->WidgetTree->RootWidget = Widget->DesignCanvas;
    auto* Label = Widget->WidgetTree->ConstructWidget<UTextBlock>();
    Widget->DesignCanvas->AddChild(Label);
    Widget->JackpotMiniText = Widget->JackpotMinorText = Label;
    Widget->JackpotMajorText = Widget->JackpotGrandText = Label;
    Widget->PlayerLevelText = Widget->PlayerXpText = Label;
    Widget->LobbyGemsText = Widget->GemEarningProgressText = Label;
    Widget->NeonJackpotTexts[0] = Label;
    Widget->MissionLabels.Add(Label);
    Widget->ClearScreenWidgets();
    TestNull(TEXT("Removed jackpot label is no longer observed"), Widget->JackpotMiniText);
    TestNull(TEXT("Removed profile label is no longer observed"), Widget->PlayerLevelText);
    TestNull(TEXT("Removed neon label is no longer observed"), Widget->NeonJackpotTexts[0]);
    TestTrue(TEXT("Removed mission observers are cleared"), Widget->MissionLabels.IsEmpty());
    TestEqual(TEXT("Old screen children are detached"), Widget->DesignCanvas->GetChildrenCount(), 0);
    CollectGarbage(RF_NoFlags);
    Widget->JackpotMini = 123;
    Widget->UpdateJackpotUI();
    TestEqual(TEXT("Delayed jackpot data remains available without old UI"), Widget->JackpotMini, int64(123));
    auto* Replacement = Widget->WidgetTree->ConstructWidget<UTextBlock>();
    Widget->DesignCanvas->AddChild(Replacement);
    Widget->JackpotMiniText = Replacement;
    Widget->UpdateJackpotUI();
    TestTrue(TEXT("Rebuilt screen receives current jackpot"), Replacement->GetText().EqualTo(FText::AsNumber(int64(123))));
    Widget->ClearScreenWidgets();
    Widget->RemoveFromRoot();
    return true;
}
#endif
