#include "HotStreakWidget.h"
#include "HotStreakArt.h"
#include "HotStreakSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UHotStreakWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Canvas;
    auto Place = [Canvas](UWidget* Widget, float X, float Y, float W, float H)
    { auto* Slot = Canvas->AddChildToCanvas(Widget); Slot->SetPosition({X,Y}); Slot->SetSize({W,H}); };
    const FLinearColor Gold(1,.7f,.2f), Pink(1,.12f,.35f), White(1,.94f,.88f);
    auto Panel = [&](float X,float Y,float W,float H,FLinearColor Color)
    { auto* Border=WidgetTree->ConstructWidget<UBorder>();Border->SetBrushColor(Color);Place(Border,X,Y,W,H); };
    auto Text = [&](const FString& Value,float X,float Y,float W,float H,int32 Size,FLinearColor Color)
    {
        auto* Label=WidgetTree->ConstructWidget<UTextBlock>();Label->SetText(FText::FromString(Value));
        Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",Size));Label->SetColorAndOpacity(Color);
        Label->SetJustification(ETextJustify::Center);Label->SetWrapTextAt(W);Label->SetAutoWrapText(false);Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Label,X,Y,W,H);return Label;
    };
    auto Button = [&](const FString& Value,float X,float Y,float W,float H,UTextBlock** Out=nullptr)
    {
        auto* Control=WidgetTree->ConstructWidget<UButton>();Control->SetBackgroundColor(FLinearColor(.5f,.05f,.16f));
        auto* Label=WidgetTree->ConstructWidget<UTextBlock>();Label->SetText(FText::FromString(Value));Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",22));
        Label->SetColorAndOpacity(White);Label->SetJustification(ETextJustify::Center);Control->SetContent(Label);Place(Control,X,Y,W,H);if(Out)*Out=Label;return Control;
    };
    Panel(0,0,1920,1080,FLinearColor(.008f,.002f,.018f,.96f));
    Panel(100,76,1720,916,FLinearColor(.075f,.008f,.035f,1));
    Text(TEXT("HOT STREAK WITH PEPPER"),180,110,1560,70,42,Gold);
    Text(TEXT("PEPPER  /  HOT STREAK QUEEN"),200,184,1520,38,20,Pink);
    PhaseText=Text(TEXT("Checking event availability..."),180,230,1560,45,22,White);
    ProgressText=Text(TEXT(""),180,288,1560,78,27,Gold);
    const TCHAR* Ids[]={TEXT("Mild"),TEXT("Hot"),TEXT("ExtraHot"),TEXT("Inferno")};
    for(int32 I=0;I<4;++I)
    {
        const float X=180+I*395;
        Panel(X,385,375,370,FLinearColor(.36f,.025f,.14f,1));
        // The supplied badges have black backgrounds; match them without altering the art.
        Panel(X+2,387,371,366,FLinearColor::Black);
        if(auto* Texture=HotStreakArt::LoadTierTexture(Ids[I]))
        {
            auto* Badge=WidgetTree->ConstructWidget<UImage>();Badge->SetBrushFromTexture(Texture);
            auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);Scale->SetContent(Badge);
            Scale->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Scale,X+85,395,205,205);
        }
        else Text(HotStreakArt::TierLabel(Ids[I]),X+10,480,355,45,27,Gold);
        TierText.Add(Text(TEXT("Preview"),X+18,608,339,90,18,White));
        UTextBlock* Label=nullptr;ClaimButtons.Add(Button(TEXT("Preview"),X+28,703,319,40,&Label));ClaimText.Add(Label);
    }
    ClaimButtons[0]->OnClicked.AddDynamic(this,&UHotStreakWidget::ClaimMild);
    ClaimButtons[1]->OnClicked.AddDynamic(this,&UHotStreakWidget::ClaimHot);
    ClaimButtons[2]->OnClicked.AddDynamic(this,&UHotStreakWidget::ClaimExtraHot);
    ClaimButtons[3]->OnClicked.AddDynamic(this,&UHotStreakWidget::ClaimInferno);
    Text(TEXT("Any base-game payout counts, even below your bet. Three consecutive paid wins start Heat. Each further paid win adds 1 Heat. A paid loss resets your streak; Heat stays banked. Free spins and bonus rounds pause progress."),180,778,1560,65,20,White);
    StatusText=Text(TEXT(""),180,850,1560,40,19,Pink);
    Button(TEXT("CLOSE"),180,904,250,58)->OnClicked.AddDynamic(this,&UHotStreakWidget::Close);
    Button(TEXT("REFRESH"),455,904,250,58)->OnClicked.AddDynamic(this,&UHotStreakWidget::Retry);
    UTextBlock* EquipLabel=nullptr;EquipButton=Button(TEXT("EQUIP MILD TITLE"),730,904,450,58,&EquipLabel);EquipText=EquipLabel;
    EquipButton->OnClicked.AddDynamic(this,&UHotStreakWidget::EquipMild);
    Button(TEXT("SPIN DUEL PRACTICE"),1205,904,535,58)->OnClicked.AddDynamic(this,&UHotStreakWidget::Practice);
    RefreshView();
}
void UHotStreakWidget::BindEvent(UHotStreakSubsystem* Event)
{
    if(Subsystem)Subsystem->OnChanged.Remove(ChangedHandle);
    Subsystem=Event;
    if(Subsystem)ChangedHandle=Subsystem->OnChanged.AddUObject(this,&UHotStreakWidget::RefreshView);
    RefreshView();
}
void UHotStreakWidget::NativeDestruct()
{
    if(Subsystem)Subsystem->OnChanged.Remove(ChangedHandle);
    Subsystem=nullptr;Super::NativeDestruct();
}
void UHotStreakWidget::RefreshView()
{
    if(!PhaseText)return;
    const FHotStreakSnapshot Empty;const auto& State=Subsystem ? Subsystem->GetSnapshot() : Empty;
    PhaseText->SetText(FText::FromString(Subsystem ? Subsystem->GetPhaseText() : TEXT("Sign in to see saved event progress.")));
    TArray<FString> GameNames;for(const auto& Game:State.EligibleGames)GameNames.Add(Game==TEXT("SugarLust")?TEXT("Sugar Lust"):Game==TEXT("NeonNights")?TEXT("Neon Nights"):TEXT("Supported slots"));
    ProgressText->SetText(FText::FromString(State.bAvailable ? FString::Printf(TEXT("STREAK %lld  |  BEST %lld  |  BANKED HEAT %lld\nPaid base spins in %s"),State.CurrentStreak,State.BestStreak,State.Heat,*FString::Join(GameNames,TEXT(" and "))) : TEXT("Your event progress will appear here when an event is available.")));
    const TCHAR* Ids[]={TEXT("Mild"),TEXT("Hot"),TEXT("ExtraHot"),TEXT("Inferno")};
    const int32 Thresholds[]={1,5,15,30};
    for(int32 I=0;I<4;++I)
    {
        const auto* Tier=State.FindTier(Ids[I]);
        const bool Pending=Subsystem && Subsystem->HasPendingClaim(Ids[I]);
        TierText[I]->SetText(FText::FromString(Tier ? FString::Printf(TEXT("%lld TOTAL HEAT\n%s\n%s"),Tier->HeatRequired,*Tier->RewardName,Tier->bReady ? TEXT("Permanent collection reward") : TEXT("Collection preview")) : FString::Printf(TEXT("%d TOTAL HEAT\nCollection preview"),Thresholds[I])));
        const bool CanClaim=Tier && State.PresentationVersion==1 && (Tier->bClaimable || Pending);
        ClaimText[I]->SetText(FText::FromString(Pending ? TEXT("RECOVER CLAIM") : !Tier || !Tier->bReady ? TEXT("PREVIEW") : Tier->bClaimed ? TEXT("COLLECTED") : Tier->bClaimable ? TEXT("COLLECT") : TEXT("LOCKED")));
        ClaimButtons[I]->SetIsEnabled(CanClaim && Subsystem && !Subsystem->IsBusy());
    }
    const auto* Mild=State.FindTier(TEXT("Mild"));
    const bool Equipped=Mild && State.EquippedRewardId==Mild->RewardId;
    EquipButton->SetIsEnabled(Subsystem && !Subsystem->IsBusy() && State.PresentationVersion==1 && State.OwnedRewardIds.Contains(TEXT("pepper_internal_mild_badge_v1")) && !Equipped);
    EquipText->SetText(FText::FromString(Equipped ? TEXT("MILD TITLE EQUIPPED") : TEXT("EQUIP MILD TITLE")));
    const FString Status=Subsystem && Subsystem->IsBusy() ? TEXT("Connecting...") : Subsystem && !Subsystem->GetError().IsEmpty() ? Subsystem->GetError() :
        State.PresentationVersion>1 ? TEXT("This event needs a newer presentation update to collect rewards.") : TEXT("Heat changes collection progress. Slot payouts and reel odds stay the same.");
    StatusText->SetText(FText::FromString(Status));
}
void UHotStreakWidget::Close(){OnClosed.Broadcast();}
void UHotStreakWidget::Practice(){OnPracticeRequested.Broadcast();}
void UHotStreakWidget::Retry(){if(Subsystem)Subsystem->Refresh();}
void UHotStreakWidget::ClaimMild(){if(Subsystem)Subsystem->RequestClaim(TEXT("Mild"));}
void UHotStreakWidget::ClaimHot(){if(Subsystem)Subsystem->RequestClaim(TEXT("Hot"));}
void UHotStreakWidget::ClaimExtraHot(){if(Subsystem)Subsystem->RequestClaim(TEXT("ExtraHot"));}
void UHotStreakWidget::ClaimInferno(){if(Subsystem)Subsystem->RequestClaim(TEXT("Inferno"));}
void UHotStreakWidget::EquipMild(){if(Subsystem)Subsystem->RequestEquipMild();}
