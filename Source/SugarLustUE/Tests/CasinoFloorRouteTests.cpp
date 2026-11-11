#if WITH_DEV_AUTOMATION_TESTS
#include "../CasinoFloorRoute.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCasinoFloorRouteTest,"SugarLust.Venue.Routes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCasinoFloorRouteTest::RunTest(const FString& Parameters)
{
    FCasinoFloorRoute R;R.Blockers.Add({FVector2D(0,850),FVector2D(100,100),0});TArray<FVector> Path;
    TestTrue(TEXT("Route detours around a furniture bank"),R.Plan(FVector(0,1200,0),FVector(0,450,0),Path));
    FVector P(0,1200,0);bool Safe=true;for(const FVector& Q:Path){Safe &= R.Segment(FVector2D(P.X,P.Y),FVector2D(Q.X,Q.Y));P=Q;}TestTrue(TEXT("Every route segment clears furniture"),Safe);
    TestFalse(TEXT("Cannot walk into furniture"),R.Plan(FVector(0,1200,0),FVector(0,850,0),Path));
    TestFalse(TEXT("Cannot walk outside the authored venue"),R.Plan(FVector(0,1200,0),FVector(2000,850,0),Path));
    TestTrue(TEXT("Floor to stage route exists through central access"),R.Plan(FVector(700,600,0),FVector(500,-200,41),Path));
    P=FVector(700,600,0);Safe=true;bool SawRamp=false;
    for(const FVector& Q:Path){Safe &= R.Segment(FVector2D(P.X,P.Y),FVector2D(Q.X,Q.Y));if(P.Y>=350&&Q.Y<=150)SawRamp=FMath::Abs(P.X)<=100&&FMath::Abs(Q.X)<=100;P=Q;}
    TestTrue(TEXT("Stage route remains within walkable area"),Safe);
    TestEqual(TEXT("Stage height"),FCasinoFloorRoute::Height(-200),41.f);TestEqual(TEXT("Floor height"),FCasinoFloorRoute::Height(500),0.f);
    TestFalse(TEXT("Stage edge cannot be entered sideways"),R.Segment(FVector2D(700,400),FVector2D(700,100)));
    return true;
}
#endif
