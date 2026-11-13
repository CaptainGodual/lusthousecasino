#include "CasinoAccountApi.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool FCasinoAccountApi::ShouldRetry(const FString& Function, const FString& RequestId, int32 Attempt,
    int32 HttpCode, const FString& CloudError, bool bConnected)
{
    return Attempt < 2 && !RequestId.IsEmpty() &&
        (Function == TEXT("Spin") || Function == TEXT("NeonNightsSpin") || Function == TEXT("MegaLustPotSpin") ||
            Function == TEXT("ClaimHotStreakReward") || Function == TEXT("EquipCasinoCosmetic")) &&
        (!bConnected || HttpCode == 408 || HttpCode == 429 || (HttpCode >= 500 && HttpCode <= 599) ||
            CloudError == TEXT("CloudScriptHTTPRequestError"));
}

void FCasinoAccountApi::Execute(UObject* Lifetime, const FString& Ticket, const FString& Function,
    const TSharedRef<FJsonObject>& Parameters, FSuccess Success, FFailure Failure,
    TFunction<bool()> IsCurrentSession, TFunction<void(bool)> Retrying, int32 Attempt)
{
    if (!Lifetime || !IsCurrentSession || !IsCurrentSession()) return;
    if (Ticket.IsEmpty()) { Failure(TEXT("Sign in to the casino to connect.")); return; }
    FString TitleId;
    GConfig->GetString(TEXT("SugarLust.PlayFab"), TEXT("TitleId"), TitleId, GGameIni);
    TitleId.TrimStartAndEndInline();
    if (TitleId.IsEmpty()) { Failure(TEXT("The casino account service is not configured.")); return; }
    TSharedRef<FJsonObject> BodyObject = MakeShared<FJsonObject>();
    BodyObject->SetStringField(TEXT("FunctionName"), Function);
    BodyObject->SetObjectField(TEXT("FunctionParameter"), Parameters);
    BodyObject->SetBoolField(TEXT("GeneratePlayStreamEvent"), true);
    BodyObject->SetStringField(TEXT("RevisionSelection"), TEXT("Live"));
    FString Body;
    if (!FJsonSerializer::Serialize(BodyObject, TJsonWriterFactory<>::Create(&Body)))
    { Failure(TEXT("Could not create the account request.")); return; }
    const TWeakObjectPtr<UObject> WeakLifetime(Lifetime);
    auto SuccessCallback = MakeShared<FSuccess>(MoveTemp(Success));
    auto FailureCallback = MakeShared<FFailure>(MoveTemp(Failure));
    auto Guard = MakeShared<TFunction<bool()>>(MoveTemp(IsCurrentSession));
    auto RetryCallback = MakeShared<TFunction<void(bool)>>(MoveTemp(Retrying));
    auto Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("https://%s.playfabapi.com/Client/ExecuteCloudScript"), *TitleId));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("X-Authorization"), Ticket);
    Request->SetContentAsString(Body);
    Request->SetTimeout(20.0f);
    Request->OnProcessRequestComplete().BindLambda(
        [WeakLifetime, Ticket, Function, Parameters, SuccessCallback, FailureCallback, Guard, RetryCallback, Attempt]
        (FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
        {
            if (!WeakLifetime.IsValid() || !(*Guard)()) return;
            if (*RetryCallback) (*RetryCallback)(false);
            auto Retry = [&](int32 HttpCode, const FString& CloudError, bool bTransport)
            {
                FString Id;
                Parameters->TryGetStringField(Function == TEXT("ClaimHotStreakReward") ? TEXT("ClaimId") :
                    Function == TEXT("EquipCasinoCosmetic") ? TEXT("EquipId") : TEXT("SpinId"), Id);
                if (!ShouldRetry(Function, Id, Attempt, HttpCode, CloudError, bTransport)) return false;
                if (*RetryCallback) (*RetryCallback)(true);
                FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
                    [WeakLifetime, Ticket, Function, Parameters, SuccessCallback, FailureCallback, Guard, RetryCallback, Attempt](float)
                    {
                        if (WeakLifetime.IsValid() && (*Guard)())
                            Execute(WeakLifetime.Get(), Ticket, Function, Parameters, *SuccessCallback, *FailureCallback, *Guard, *RetryCallback, Attempt + 1);
                        return false;
                    }), Attempt == 0 ? 0.75f : 1.5f);
                return true;
            };
            if (!bConnected || !Response.IsValid())
            { if (!Retry(0, TEXT(""), false)) (*FailureCallback)(TEXT("Connection interrupted. Retry to recover the original request.")); return; }
            if (Retry(Response->GetResponseCode(), TEXT(""), true)) return;
            TSharedPtr<FJsonObject> Envelope;
            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Envelope) || !Envelope.IsValid())
            { (*FailureCallback)(TEXT("The account service returned unreadable data. Retry the original request.")); return; }
            const TSharedPtr<FJsonObject>* Data = nullptr;
            if (!Envelope->TryGetObjectField(TEXT("data"), Data) || !Data || !Data->IsValid())
            {
                FString Message;
                Envelope->TryGetStringField(TEXT("errorMessage"), Message);
                (*FailureCallback)(Message.IsEmpty() ? TEXT("The account request failed. Please retry.") : Message);
                return;
            }
            const TSharedPtr<FJsonObject>* Error = nullptr;
            if ((*Data)->TryGetObjectField(TEXT("Error"), Error) && Error && Error->IsValid())
            {
                FString Name, Message;
                (*Error)->TryGetStringField(TEXT("Error"), Name);
                (*Error)->TryGetStringField(TEXT("Message"), Message);
                if (!Retry(Response->GetResponseCode(), Name, true)) (*FailureCallback)(FString::Printf(TEXT("%s: %s"), *Name, *Message));
                return;
            }
            const TSharedPtr<FJsonObject>* Result = nullptr;
            if (!(*Data)->TryGetObjectField(TEXT("FunctionResult"), Result) || !Result || !Result->IsValid())
            { (*FailureCallback)(TEXT("The account service returned no result.")); return; }
            (*SuccessCallback)(*Result);
        });
    if (!Request->ProcessRequest()) FailureCallback->operator()(TEXT("The account request could not be started."));
}
