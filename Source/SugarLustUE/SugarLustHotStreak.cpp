#include "SugarLustRootWidget.h"
#include "SugarLustSlotMachine.h"
#include "HotStreakSubsystem.h"
#include "HotStreakWidget.h"
#include "HotStreakPresenter.h"
#include "SocialCasinoPrototype.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"

UHotStreakSubsystem* USugarLustRootWidget::GetHotStreak() const
{ return GetGameInstance() ? GetGameInstance()->GetSubsystem<UHotStreakSubsystem>() : nullptr; }
bool USugarLustRootWidget::IsHotStreakPanelOpen() const
{ return IsValid(HotStreakPanel) && HotStreakPanel->GetVisibility() == ESlateVisibility::Visible; }
bool USugarLustRootWidget::IsHotStreakPresentationReady() const
{
    return !bPlayFabRequestInFlight && !bBonusPickRequestInFlight && !bNeonSpinAnimating && !bNeonPayoutAnimating &&
        !bNeonHeatBonusActive && !bNeonHeatBonusComplete && BonusTransitionSeconds <= 0 && BonusJarAnimatingCard == INDEX_NONE &&
        (!SlotGame || (!SlotGame->IsSpinning() && !SlotGame->IsBonusRoundActive() && !SlotGame->IsBonusResultReady())) &&
        !IsProgressiveJackpotVisible() && PendingSugarJackpotPrize <= 0 && PendingNeonJackpotPrize <= 0 &&
        (!BigWinCanvas || BigWinCanvas->GetVisibility() != ESlateVisibility::Visible) &&
        (!PayoutWinTuneComponent || !PayoutWinTuneComponent->IsPlaying());
}
void USugarLustRootWidget::BuildHotStreakEntry(bool bLobby)
{
    if (!WidgetTree || !DesignCanvas) return;
    if (bLobby)
    {
        auto* Frame=WidgetTree->ConstructWidget<UBorder>();Frame->SetBrushColor(FLinearColor(.075f,.004f,.035f,.97f));Frame->SetPadding(FMargin(24));
        HotStreakLobbyText=Cast<UTextBlock>(MakeText(TEXT("HOT STREAK WITH PEPPER\nOpen Events to check availability"),29,FLinearColor(1,.75f,.3f),true));
        Frame->SetContent(HotStreakLobbyText);Frame->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Frame,585,240,1128,330,6);
    }
    else
    {
        auto* Shortcut=MakeButton(TEXT(""),FLinearColor(.55f,.04f,.16f),16);
        HotStreakHud=Cast<UTextBlock>(MakeText(TEXT("HOT STREAK\nOpen Events"),19,FLinearColor(1,.8f,.4f),true));
        Shortcut->SetContent(HotStreakHud);Shortcut->OnClicked.AddDynamic(this,&USugarLustRootWidget::HandleOpenHotStreak);
        Place(Shortcut,24,315,270,160,12);
    }
    UpdateHotStreakPresentation();
}
void USugarLustRootWidget::HandleOpenHotStreak()
{
    AutoSpinsRemaining=0;NeonAutoSpinsRemaining=0;bSpinButtonHeld=false;bAutoSpinStartedFromHold=false;
    SugarFreeSpins.bPaused=true;NeonFreeSpins.bPaused=true;
    // Preserve the outstanding spin and its original parameters while finishing
    // all mandatory result presentations before opening the event modal.
    if(!IsHotStreakPresentationReady()){bOpenHotStreakWhenReady=true;return;}
    bOpenHotStreakWhenReady=false;
    if(IsHotStreakPanelOpen())return;
    if(!WidgetTree || !DesignCanvas)return;
    HotStreakPanel=CreateWidget<UHotStreakWidget>(this,UHotStreakWidget::StaticClass());
    if(!HotStreakPanel)return;
    HotStreakPanel->BindEvent(GetHotStreak());
    HotStreakPanel->OnClosed.AddUObject(this,&USugarLustRootWidget::CloseHotStreak);
    HotStreakPanel->OnPracticeRequested.AddUObject(this,&USugarLustRootWidget::OpenHotStreakPractice);
    Place(HotStreakPanel,0,0,1920,1080,65);
    if(auto* Event=GetHotStreak())Event->Refresh();
}
void USugarLustRootWidget::CloseHotStreak()
{
    if(HotStreakPanel){HotStreakPanel->BindEvent(nullptr);HotStreakPanel->RemoveFromParent();HotStreakPanel=nullptr;}
}
void USugarLustRootWidget::OpenHotStreakPractice()
{
    CloseHotStreak();HandleEnterSocialRoom();if(SocialRoom)SocialRoom->QueueSpinDuelEntry();
}
void USugarLustRootWidget::UpdateHotStreakPresentation()
{
    auto* Event=GetHotStreak();
    if(PepperPresenter && (IsHotStreakPanelOpen() || bNeonHeatBonusActive || bNeonHeatBonusComplete || bNeonPayoutAnimating ||
        (SlotGame && (SlotGame->IsBonusRoundActive() || SlotGame->IsBonusResultReady())) || IsProgressiveJackpotVisible() ||
        (BigWinCanvas && BigWinCanvas->GetVisibility()==ESlateVisibility::Visible) ||
        (PayoutWinTuneComponent && PayoutWinTuneComponent->IsPlaying())))PepperPresenter->Dismiss();
    if(Event && !bPlayFabRequestInFlight && !bNeonSpinAnimating && (!SlotGame || !SlotGame->IsSpinning())) Event->RevealSettledProgress();
    if(HotStreakHud)HotStreakHud->SetText(FText::FromString(Event ? Event->GetHudText() : TEXT("HOT STREAK\nSign in to connect")));
    if(HotStreakLobbyText && Event)
    {
        const auto& State=Event->GetSnapshot();int32 Claims=0;for(const auto& Tier:State.Tiers)if(Tier.bClaimable)++Claims;
        HotStreakLobbyText->SetText(FText::FromString(FString::Printf(TEXT("HOT STREAK WITH PEPPER\n%s\nBANKED HEAT %lld  |  %d REWARDS TO COLLECT\nOPEN EVENTS  /  SPIN DUEL PRACTICE"),*Event->GetPhaseText(),State.Heat,Claims)));
    }
    if(bOpenHotStreakWhenReady && IsHotStreakPresentationReady())HandleOpenHotStreak();
    if(!Event || !(bShowingSlot || bShowingNeonNightsPreview) || IsHotStreakPanelOpen() || !IsHotStreakPresentationReady() || (PepperPresenter && PepperPresenter->IsPresenting()))return;
    FHotStreakDelta Delta;if(!Event->PopPresentation(Delta,FPlatformTime::Seconds()))return;
    if(!PepperPresenter){PepperPresenter=CreateWidget<UHotStreakPresenter>(this,UHotStreakPresenter::StaticClass());if(PepperPresenter)Place(PepperPresenter,24,500,270,330,25);}
    if(PepperPresenter)PepperPresenter->Present(Delta);
}
