#if WITH_DEV_AUTOMATION_TESTS
#include "../SocialCasinoPrototype.h"
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVenueInputSurfaceTest,"SugarLust.Venue.InputSurface",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVenueInputSurfaceTest::RunTest(const FString& Parameters)
{
    auto* Controls=NewObject<USocialCasinoControls>();Controls->AddToRoot();Controls->WidgetTree=NewObject<UWidgetTree>(Controls);
    Controls->NativeOnInitialized();
    auto* Canvas=Cast<UCanvasPanel>(Controls->WidgetTree->RootWidget);
    TestNotNull(TEXT("Floor input tree exists before Slate is built"),Canvas);
    if(Canvas)TestEqual(TEXT("Floor surface, menu and status are created during initialization"),Canvas->GetChildrenCount(),3);
    TestNotNull(TEXT("Status exists before construction"),Controls->Status.Get());
    Controls->TakeWidget();
    TestTrue(TEXT("Controls are hit-test visible"),Controls->GetVisibility()==ESlateVisibility::Visible);
    Controls->RemoveFromRoot();return true;
}
#endif
