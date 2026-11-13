#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotStreakTypes.h"
#include "HotStreakPresenter.generated.h"
class UTextBlock;
class UImage;
class USizeBox;
UCLASS()
class SUGARLUSTUE_API UHotStreakPresenter : public UUserWidget
{
    GENERATED_BODY()
public:
    void Present(const FHotStreakDelta& Delta);
    void Dismiss() { Remaining = 0; SetVisibility(ESlateVisibility::Collapsed); }
    bool IsPresenting() const { return Remaining > 0; }
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
private:
    friend class FHotStreakClientTest;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Dialogue;
    UPROPERTY(Transient) TObjectPtr<UImage> TierIcon;
    UPROPERTY(Transient) TObjectPtr<USizeBox> TierIconBox;
    float Remaining = 0;
};
