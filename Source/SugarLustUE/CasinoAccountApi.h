#pragma once
#include "CoreMinimal.h"

class FJsonObject;

/** Shared authenticated CloudScript transport. Owns no login or player balance. */
struct FCasinoAccountApi
{
    using FSuccess = TFunction<void(TSharedPtr<FJsonObject>)>;
    using FFailure = TFunction<void(const FString&)>;
    static bool ShouldRetry(const FString& Function, const FString& RequestId, int32 Attempt,
        int32 HttpCode, const FString& CloudError, bool bConnected);
    static void Execute(UObject* Lifetime, const FString& Ticket, const FString& Function,
        const TSharedRef<FJsonObject>& Parameters, FSuccess Success, FFailure Failure,
        TFunction<bool()> IsCurrentSession, TFunction<void(bool)> Retrying = {}, int32 Attempt = 0);
};
