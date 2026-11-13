#include "HotStreakArt.h"
#include "Engine/Texture2D.h"

namespace
{
    struct FTierArt
    {
        const TCHAR* Id;
        const TCHAR* Label;
        const TCHAR* TexturePath;
    };

    const FTierArt TierArt[] = {
        {TEXT("Mild"), TEXT("MILD"), TEXT("/Game/Art/HotStreak/pepper-tier-mild.pepper-tier-mild")},
        {TEXT("Hot"), TEXT("HOT"), TEXT("/Game/Art/HotStreak/pepper-tier-hot.pepper-tier-hot")},
        {TEXT("ExtraHot"), TEXT("EXTRA HOT"), TEXT("/Game/Art/HotStreak/pepper-tier-extra-hot.pepper-tier-extra-hot")},
        {TEXT("Inferno"), TEXT("INFERNO"), TEXT("/Game/Art/HotStreak/pepper-tier-inferno.pepper-tier-inferno")}
    };

    const FTierArt* FindTier(const FString& TierId)
    {
        for (const auto& Art : TierArt)
            if (TierId == Art.Id) return &Art;
        return nullptr;
    }
}

UTexture2D* HotStreakArt::LoadTierTexture(const FString& TierId)
{
    const auto* Art = FindTier(TierId);
    return Art ? LoadObject<UTexture2D>(nullptr, Art->TexturePath) : nullptr;
}

FString HotStreakArt::TierLabel(const FString& TierId)
{
    const auto* Art = FindTier(TierId);
    return Art ? FString(Art->Label) : TEXT("HEAT TIER");
}
