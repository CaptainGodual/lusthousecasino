#include "HotStreakPresenter.h"
#include "HotStreakArt.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UHotStreakPresenter::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    auto* Frame=WidgetTree->ConstructWidget<UBorder>();Frame->SetBrushColor(FLinearColor::Black);Frame->SetPadding(FMargin(14));
    auto* Content=WidgetTree->ConstructWidget<UVerticalBox>();Frame->SetContent(Content);
    TierIcon=WidgetTree->ConstructWidget<UImage>();
    TierIconBox=WidgetTree->ConstructWidget<USizeBox>();TierIconBox->SetWidthOverride(126);TierIconBox->SetHeightOverride(126);TierIconBox->SetContent(TierIcon);
    auto* IconSlot=Content->AddChildToVerticalBox(TierIconBox);IconSlot->SetHorizontalAlignment(HAlign_Center);IconSlot->SetPadding(FMargin(0,0,0,8));
    TierIconBox->SetVisibility(ESlateVisibility::Collapsed);
    Dialogue=WidgetTree->ConstructWidget<UTextBlock>();Dialogue->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",21));
    Dialogue->SetColorAndOpacity(FLinearColor(1,.8f,.4f));Dialogue->SetWrapTextAt(242);Dialogue->SetAutoWrapText(false);Dialogue->SetJustification(ETextJustify::Center);
    Content->AddChildToVerticalBox(Dialogue);
    WidgetTree->RootWidget=Frame;SetVisibility(ESlateVisibility::Collapsed);
}
void UHotStreakPresenter::Present(const FHotStreakDelta& Delta)
{
    FString Line=Delta.TriggerKind==TEXT("Entrance") ? TEXT("PEPPER\nThree wins! You're heating up!") :
        Delta.TriggerKind==TEXT("StreakBreak") ? TEXT("PEPPER\nFresh streak, same banked Heat. Let's go again.") : TEXT("PEPPER\nKeep that fire going!");
    const FString TierId=Delta.NewlyUnlockedTierIds.IsEmpty() ? FString() : Delta.NewlyUnlockedTierIds.Last();
    if(!TierId.IsEmpty())Line+=TEXT("\n")+HotStreakArt::TierLabel(TierId)+TEXT(" UNLOCKED!");
    if(TierIcon && TierIconBox)
    {
        auto* Texture=HotStreakArt::LoadTierTexture(TierId);TierIcon->SetBrushFromTexture(Texture);
        TierIconBox->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if(Dialogue)Dialogue->SetText(FText::FromString(Line));
    Remaining=4.0f;SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UHotStreakPresenter::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);Remaining=FMath::Max(0.0f,Remaining-DeltaSeconds);
    if(Remaining<=0)SetVisibility(ESlateVisibility::Collapsed);
}
