#if WITH_DEV_AUTOMATION_TESTS
#include "../SugarLustRootWidget.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPendingSpinIdentityTest, "SugarLust.Recovery.PendingSpinIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPendingSpinIdentityTest::RunTest(const FString& Parameters)
{
    for (const bool bNeon : {false, true})
    {
        FString Id;
        TSharedPtr<FJsonObject> Pending;
        auto Desired = MakeShared<FJsonObject>();
        const TCHAR* BetKey = bNeon ? TEXT("Bet") : TEXT("BetPerLine");
        Desired->SetNumberField(BetKey, 100);
        if (!bNeon) Desired->SetNumberField(TEXT("ActiveLines"), 20);
        TestTrue(TEXT("First request captures its immutable wager"), USugarLustRootWidget::PreparePendingSpin(Id, Pending, Desired));
        const FString OriginalId = Id;
        TestFalse(TEXT("A spin has a nonempty identity before sending"), OriginalId.IsEmpty());
        Desired->SetNumberField(BetKey, 10000);
        if (!bNeon) Desired->SetNumberField(TEXT("ActiveLines"), 1);
        TestTrue(TEXT("Manual retry uses the original request"), USugarLustRootWidget::PreparePendingSpin(Id, Pending, Desired));
        TestEqual(TEXT("Retry identity is unchanged"), Id, OriginalId);
        TestEqual(TEXT("Changed UI wager cannot change the stored fingerprint"), Pending->GetNumberField(BetKey), 100.0);
        if (!bNeon) TestEqual(TEXT("Changed UI paylines cannot change the stored fingerprint"), Pending->GetNumberField(TEXT("ActiveLines")), 20.0);
        FString WrongId = TEXT("different-request");
        TestFalse(TEXT("Mismatched cached identity fails closed"), USugarLustRootWidget::PreparePendingSpin(WrongId, Pending, Desired));
        TSharedPtr<FJsonObject> Missing;
        TestFalse(TEXT("Missing arguments cannot silently reconstruct an unresolved request"), USugarLustRootWidget::PreparePendingSpin(Id, Missing, Desired));
        Id.Reset(); Pending.Reset();
        TestTrue(TEXT("Acknowledged request allows a new wager"), USugarLustRootWidget::PreparePendingSpin(Id, Pending, Desired));
        TestTrue(TEXT("New request has a new identity"), Id != OriginalId);
        TestEqual(TEXT("New request captures the newly selected bet"), Pending->GetNumberField(BetKey), 10000.0);
    }
    return true;
}
#endif
