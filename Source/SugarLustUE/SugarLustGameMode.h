#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SugarLustGameMode.generated.h"

UCLASS()
class SUGARLUSTUE_API ASugarLustGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
};
