#pragma once
#include "CoreMinimal.h"

// Update metadata is untrusted network input. Only stable numeric versions and
// HTTPS distribution links are accepted; it never installs downloaded code.
namespace SugarLustUpdatePolicy
{
    bool ParseVersion(const FString& Text, TArray<int32>& Parts);
    bool IsNewerVersion(const FString& Candidate, const FString& Current);
    bool IsAllowedDownloadUrl(const FString& Url);
}
