#if WITH_DEV_AUTOMATION_TESTS
#include "../MegaLustPotWidget.h"
#include "../SugarLustRootWidget.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMegaPotServerViewTest,"SugarLust.MegaPot.ServerView",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMegaPotServerViewTest::RunTest(const FString&)
{
 auto* Root=NewObject<USugarLustRootWidget>();Root->AddToRoot();Root->PlayFabPlayerId=TEXT("MegaPotAutomation_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
 auto* W=NewObject<UMegaLustPotWidget>();W->AddToRoot();W->WidgetTree=NewObject<UWidgetTree>(W);W->NativeOnInitialized();W->ConfigureLive(Root);
 TestTrue(TEXT("Online identity is explicit"),W->IsLiveMode());TestFalse(TEXT("Login failure fails closed"),W->IsServerReady());TestFalse(TEXT("Spin disabled without server"),W->SpinButton->GetIsEnabled());
 for(auto& Child:W->PracticeOnly)TestTrue(TEXT("Guided outcomes and reset absent online"),Child->GetVisibility()==ESlateVisibility::Collapsed);
 auto Fixture=[](const TCHAR* Name){FString Text;FFileHelper::LoadFileToString(Text,*FPaths::Combine(FPaths::ProjectDir(),TEXT("Backend/Tests/fixtures"),Name));TSharedPtr<FJsonObject> O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O);return O;};
 auto State=Fixture(TEXT("mega-pot-state.json"));TestTrue(TEXT("Server-generated fixture loaded"),State.IsValid());if(!State){W->RemoveFromRoot();Root->RemoveFromRoot();return false;}
 TestTrue(TEXT("Valid synchronization applied"),W->ApplyServer(State,true));TestEqual(TEXT("Only server wallet accepted"),W->Game.Credits,int64(10050000));TestTrue(TEXT("Spin enabled after validated sync"),W->SpinButton->GetIsEnabled());
 int64 Credits=W->Game.Credits;W->ResetPractice();W->TryMega();TestEqual(TEXT("Practice helpers cannot mutate online balance"),W->Game.Credits,Credits);TestTrue(TEXT("Practice helpers cannot fabricate a bonus"),W->Game.Mode==FMegaLustPotGame::Phase::Base);
 W->PendingId=TEXT("test-pending");TestTrue(TEXT("Pending receipt persisted before submission"),W->SavePending());TestTrue(TEXT("Receipt exists"),IFileManager::Get().FileExists(*W->JournalPath()));
 auto Invalid=Fixture(TEXT("mega-pot-state.json"));Invalid->SetNumberField(TEXT("RulesVersion"),99);TestFalse(TEXT("Mismatched rules rejected"),W->ApplyServer(Invalid,true));TestEqual(TEXT("Malformed response leaves wallet untouched"),W->Game.Credits,Credits);TestTrue(TEXT("Malformed response retains recovery receipt"),IFileManager::Get().FileExists(*W->JournalPath()));
 Invalid=Fixture(TEXT("mega-pot-state.json"));Invalid->GetObjectField(TEXT("Profile"))->SetNumberField(TEXT("Coins"),-1);TestFalse(TEXT("Negative server wallet rejected"),W->ApplyServer(Invalid,true));
 Invalid=Fixture(TEXT("mega-pot-state.json"));Invalid->GetObjectField(TEXT("Profile"))->GetObjectField(TEXT("MegaPot"))->SetStringField(TEXT("Phase"),TEXT("Unknown"));TestFalse(TEXT("Unknown feature phase rejected"),W->ApplyServer(Invalid,true));
 Invalid=Fixture(TEXT("mega-pot-state.json"));Invalid->GetObjectField(TEXT("Profile"))->GetObjectField(TEXT("MegaPot"))->SetArrayField(TEXT("Grid"),{});TestFalse(TEXT("Incomplete board rejected"),W->ApplyServer(Invalid,true));
 TestTrue(TEXT("Sync resolves durable pending receipt"),W->ApplyServer(State,true));TestFalse(TEXT("Resolved receipt deleted"),IFileManager::Get().FileExists(*W->JournalPath()));
 auto Result=Fixture(TEXT("mega-pot-result.json"));TestTrue(TEXT("Server result applied"),W->ApplyServer(Result,false));TestTrue(TEXT("Restored result needs only presentation collection"),W->Game.Mode==FMegaLustPotGame::Phase::Result);TestEqual(TEXT("Server settled coins"),W->Game.Credits,int64(1200));W->SpinLive();TestEqual(TEXT("Collect never credits locally"),W->Game.Credits,int64(1200));TestTrue(TEXT("Collection returns to base view"),W->Game.Mode==FMegaLustPotGame::Phase::Base);
 W->bReady=false;W->Spin();TestEqual(TEXT("Offline online-mode spin never simulates"),W->Animation,0.f);TestEqual(TEXT("No offline wallet fallback"),W->Game.Credits,int64(1200));
 W->PendingId=TEXT("presentation-test");W->SavePending();
 auto Delayed=Fixture(TEXT("mega-pot-result.json"));Delayed->SetStringField(TEXT("SpinId"),W->PendingId);
 W->PendingPresentation=Delayed;W->Animation=3.5f;W->bBusy=true;W->bReady=true;
 const int64 BeforePresentation=W->Game.Credits;
 W->FinishLiveSpin();TestTrue(TEXT("Fast server reply remains buffered during animation"),W->PendingPresentation.IsValid());TestTrue(TEXT("Input remains locked during presentation"),W->bBusy);TestEqual(TEXT("Wallet presentation does not reveal result early"),W->Game.Credits,BeforePresentation);
 W->Animation=0;W->FinishLiveSpin();TestFalse(TEXT("Result presented after minimum duration"),W->PendingPresentation.IsValid());TestFalse(TEXT("Presentation unlocks input"),W->bBusy);TestTrue(TEXT("Receipt cleared only after validated reveal"),W->PendingId.IsEmpty());
 W->SetCabinetLayout();TestTrue(TEXT("Cabinet keeps recovery visible"),W->GetWidgetFromName(TEXT("MegaSync"))->GetVisibility()==ESlateVisibility::Visible);
 W->bBusy=true;TestFalse(TEXT("Cabinet cannot exit during settlement"),W->PrepareToLeaveCabinet());W->bBusy=false;W->Animation=1;TestFalse(TEXT("Cabinet cannot exit before reveal"),W->PrepareToLeaveCabinet());W->Animation=0;W->Auto=true;TestTrue(TEXT("Idle cabinet can return to bank"),W->PrepareToLeaveCabinet());TestFalse(TEXT("Leaving stops automatic spins"),W->Auto);
 W->ClearPending();W->RemoveFromRoot();Root->RemoveFromRoot();return true;
}
#endif
