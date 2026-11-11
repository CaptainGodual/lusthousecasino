#if WITH_DEV_AUTOMATION_TESTS
#include "../SugarLustUpdatePolicy.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSugarLustUpdatePolicyTest,"SugarLust.Security.UpdatePolicy",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSugarLustUpdatePolicyTest::RunTest(const FString& Parameters)
{
    using namespace SugarLustUpdatePolicy;
    TestTrue(TEXT("Numeric comparison handles double digit components"),IsNewerVersion(TEXT("0.0.10"),TEXT("0.0.2")));
    TestFalse(TEXT("Equal versions are not updates"),IsNewerVersion(TEXT("0.0.2"),TEXT("0.0.2")));
    TestFalse(TEXT("Older versions are not updates"),IsNewerVersion(TEXT("0.0.1"),TEXT("0.0.2")));
    for (const FString& Invalid : {TEXT("0..2"),TEXT("0.0.2evil"),TEXT("0.0.-1"),TEXT("0.0.999999999999"),TEXT("0.2"),TEXT(" 0.0.2"),TEXT("0.0.2.1")})
    {
        TArray<int32> Parts;
        TestFalse(FString::Printf(TEXT("Reject malformed version %s"),*Invalid),ParseVersion(Invalid,Parts));
    }
    TestTrue(TEXT("Approved GitHub release link"),IsAllowedDownloadUrl(TEXT("https://github.com/CaptainGodual/lusthousecasino/releases/latest")));
    TestTrue(TEXT("Itch creator page"),IsAllowedDownloadUrl(TEXT("https://captain.itch.io/lust-house")));
    for (const FString& Invalid : {TEXT("http://github.com/CaptainGodual/lusthousecasino/releases/latest"),TEXT("https://github.com.evil.test/CaptainGodual/lusthousecasino/releases/latest"),TEXT("https://github.com@evil.test/"),TEXT("https://github.com/other/project/releases/latest"),TEXT("https://itch.io.evil.test/"),TEXT("https://itch.io:443/"),TEXT("file:///private.exe"),TEXT("javascript:alert(1)"),TEXT("https://itch.io\\@evil.test/"),TEXT("https://itch.io/ bad")})
        TestFalse(FString::Printf(TEXT("Reject unsafe update link %s"),*Invalid),IsAllowedDownloadUrl(Invalid));
    return true;
}
#endif
