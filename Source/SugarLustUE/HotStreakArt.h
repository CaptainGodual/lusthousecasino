#pragma once

#include "CoreMinimal.h"

class UTexture2D;

namespace HotStreakArt
{
    UTexture2D* LoadTierTexture(const FString& TierId);
    FString TierLabel(const FString& TierId);
}
