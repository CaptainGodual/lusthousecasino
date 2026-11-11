#include "SugarLustGameMode.h"
#include "SugarLustRootWidget.h"
#include "SugarLustGameInstance.h"
#include "GameFramework/PlayerController.h"

void ASugarLustGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (USugarLustRootWidget* Lobby = CreateWidget<USugarLustRootWidget>(PC, USugarLustRootWidget::StaticClass()))
        {
            Lobby->AddToViewport(0);
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(Lobby->TakeWidget());
            InputMode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;
        }
    }

    if (USugarLustGameInstance* GameInstance = GetGameInstance<USugarLustGameInstance>())
    {
        GameInstance->StopLoadingScreen();
    }
}
