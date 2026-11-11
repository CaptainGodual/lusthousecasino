#include "SugarLustUpdatePolicy.h"

bool SugarLustUpdatePolicy::ParseVersion(const FString& Text, TArray<int32>& Parts)
{
    Parts.Reset();
    TArray<FString> Segments;
    Text.ParseIntoArray(Segments, TEXT("."), false);
    if (Segments.Num() != 3) return false;
    for (const FString& Segment : Segments)
    {
        if (Segment.IsEmpty() || Segment.Len() > 9) { Parts.Reset(); return false; }
        for (const TCHAR Ch : Segment)
            if (Ch < TEXT('0') || Ch > TEXT('9')) { Parts.Reset(); return false; }
        Parts.Add(FCString::Atoi(*Segment));
    }
    return true;
}

bool SugarLustUpdatePolicy::IsNewerVersion(const FString& Candidate, const FString& Current)
{
    TArray<int32> A, B;
    if (!ParseVersion(Candidate, A) || !ParseVersion(Current, B)) return false;
    for (int32 I=0; I<3; ++I) if (A[I] != B[I]) return A[I] > B[I];
    return false;
}

bool SugarLustUpdatePolicy::IsAllowedDownloadUrl(const FString& Url)
{
    if (!Url.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase)) return false;
    for (const TCHAR Ch : Url)
        if (Ch <= TEXT(' ') || Ch == TEXT('\\')) return false;
    const FString Remainder = Url.Mid(8);
    int32 End = Remainder.Len();
    for (int32 I=0; I<Remainder.Len(); ++I)
        if (Remainder[I] == TEXT('/') || Remainder[I] == TEXT('?') || Remainder[I] == TEXT('#')) { End=I; break; }
    const FString Host = Remainder.Left(End).ToLower();
    // No credentials, nonstandard ports, encoded authority or host suffix tricks.
    for (const TCHAR Ch : Host)
        if (!((Ch >= TEXT('a') && Ch <= TEXT('z')) || (Ch >= TEXT('0') && Ch <= TEXT('9')) || Ch == TEXT('-') || Ch == TEXT('.'))) return false;
    if (Host == TEXT("github.com"))
    {
        const FString Path = Remainder.Mid(End);
        return Path.StartsWith(TEXT("/CaptainGodual/lusthousecasino/releases/"));
    }
    return Host == TEXT("itch.io") || (Host.EndsWith(TEXT(".itch.io")) && Host.Len() > 8);
}
