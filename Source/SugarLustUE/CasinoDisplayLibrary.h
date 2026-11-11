#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CasinoDisplayLibrary.generated.h"
class UUserWidget;
class UWidgetComponent;
class UTextureRenderTarget2D;
UCLASS()
class SUGARLUSTUE_API UCasinoDisplayLibrary : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Casino|Display")
 static bool SaveDisplayPreview(UTextureRenderTarget2D* Target, const FString& Filename);
 UFUNCTION(BlueprintCallable, Category="Casino|Display")
 static FString DescribeDisplay(UUserWidget* Widget, UWidgetComponent* Display);
 UFUNCTION(BlueprintCallable, Category="Casino|Display")
 static UTextureRenderTarget2D* RenderWidgetPreview(UUserWidget* Widget);
};
