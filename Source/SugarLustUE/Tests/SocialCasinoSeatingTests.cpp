#if WITH_DEV_AUTOMATION_TESTS
#include "../SocialCasinoPrototype.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSocialCasinoSeatTest,"SugarLust.SocialPrototype.Seating",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSocialCasinoSeatTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("Test world"),World))return false;
    auto* Room=World->SpawnActor<ASocialCasinoPrototype>(FVector(10000,0,0),FRotator::ZeroRotator);
    if(!TestNotNull(TEXT("Prototype room"),Room)){World->DestroyWorld(false);return false;}
    Room->DispatchBeginPlay();
    auto VisibleAnimatedPeople=[Room]() { int32 Count=0; TInlineComponentArray<USkeletalMeshComponent*> Meshes(Room); for(auto* Mesh:Meshes) if(Mesh->IsVisible()) ++Count; return Count; };
    TestEqual(TEXT("Two neighbouring NPCs use animated seated meshes"),VisibleAnimatedPeople(),2);
    Room->RequestSeat(1);
    for(int32 I=0;I<100;++I)Room->Tick(.05f);
    TestFalse(TEXT("NPC occupied seat cannot be taken"),Room->IsSeated());
    Room->RequestSeat(0);
    for(int32 I=0;I<100;++I)Room->Tick(.05f);
    TestTrue(TEXT("Walking to a free seat completes seating"),Room->IsSeated());
    TestEqual(TEXT("Player switches to seated animation"),VisibleAnimatedPeople(),3);
    Room->Stand();TestEqual(TEXT("Standing hides the seated player mesh"),VisibleAnimatedPeople(),2);TestFalse(TEXT("Stand leaves seated state"),Room->IsSeated());
    Room->RequestSeat(2);Room->Stand();
    for(int32 I=0;I<100;++I)Room->Tick(.05f);
    TestFalse(TEXT("Cancelling approach prevents later automatic seating"),Room->IsSeated());
    Room->RequestSeat(2);
    for(int32 I=0;I<100;++I)Room->Tick(.05f);
    TestTrue(TEXT("Another free machine can be selected after standing"),Room->IsSeated());
    World->DestroyWorld(false);
    return true;
}
#endif
