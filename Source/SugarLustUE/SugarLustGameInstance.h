#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Styling/SlateBrush.h"
#include "SugarLustGameInstance.generated.h"

class UTexture2D;
class IHttpRequest;

UCLASS()
class SUGARLUSTUE_API USugarLustGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	void ShowLoadingScreen();
	void StopLoadingScreen();
	bool IsUpdateAvailable() const { return bUpdateAvailable; }

private:
	void StartLoadingScreen();
	void CheckForUpdates();

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LoadingBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LoadingLogo;

	FSlateBrush BackdropBrush;
	FSlateBrush LogoBrush;
	TSharedPtr<FString, ESPMode::ThreadSafe> LoadingUpdateStatus;
	FString UpdateDownloadUrl;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UpdateRequest;
	bool bUpdateAvailable = false;
	bool bLoadingScreenStarted = false;
};
