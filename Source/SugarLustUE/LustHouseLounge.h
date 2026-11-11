#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LustHouseLounge.generated.h"

class UCameraComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class SUGARLUSTUE_API ALustHouseLounge : public AActor
{
    GENERATED_BODY()
public:
    ALustHouseLounge();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void SetView(int32 View);
private:
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UStaticMesh> SugarCabinetMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
    FVector TargetLocation;
    FRotator TargetRotation;
    void Box(FVector Position, FVector Size, FLinearColor Color);
    void Sign(const FString& Text, FVector Position, float Size, FColor Color);
};
