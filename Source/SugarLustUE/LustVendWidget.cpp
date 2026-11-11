#include "LustVendWidget.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "MediaPlayer.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "MediaTexture.h"
#include "MediaSoundComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"

void ULustVendWidget::Configure(const FString& Ticket, const FString& PlayerId)
{
    Session = Ticket;
    PendingPath = FPaths::ProjectSavedDir() / TEXT("LustVend") / (FPaths::MakeValidFileName(PlayerId) + TEXT(".json"));
    FString Saved;
    TSharedPtr<FJsonObject> Pending;
    if (FFileHelper::LoadFileToString(Saved, *PendingPath) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Saved), Pending) && Pending.IsValid())
    {
        Pending->TryGetStringField(TEXT("requestId"), RequestId);
        Pending->TryGetStringField(TEXT("sku"), PendingSku);
        Pending->TryGetStringField(TEXT("version"), PendingVersion);
    }
    Refresh();
}

TSharedRef<SWidget> ULustVendWidget::RebuildWidget()
{
    Frames.Reset();
    for (int32 I=0; I<8; ++I)
    {
        Frames.Add(FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir() / TEXT("Art/LustVend") / FString::Printf(TEXT("machine_%d.png"), I)));
    }
    MachineBrush.ImageSize = FVector2D(215,360);
    SetFrame(0);
    SceneTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Art/FuturisticNeonLobby.FuturisticNeonLobby"));
    SceneBrush.SetResourceObject(SceneTexture);
    SceneBrush.ImageSize = FVector2D(1920,1080);
    ProductTextures.Reset();
    const int32 SpriteIds[] = {0,1,3,4,5,10};
    TSharedRef<SOverlay> Cabinet = SNew(SOverlay)
        + SOverlay::Slot()[SNew(SImage).Image(&MachineBrush)]
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(27,88,0,0)[
            SNew(SBox).WidthOverride(118).HeightOverride(163)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.018f,.006f,.03f,1.f))]];
    for (int32 I=0; I<6; ++I)
    {
        ProductTextures.Add(FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir()/TEXT("Art/LustVend")/FString::Printf(TEXT("product_%d.png"),SpriteIds[I])));
        ProductBrushes[I].SetResourceObject(ProductTextures.Last());
        ProductBrushes[I].ImageSize=FVector2D(32,48);
        Cabinet->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30+(I%3)*38,94+(I/3)*76,0,0)[
            SNew(SBox).WidthOverride(36).HeightOverride(72)[
            SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
            .ContentPadding(2)
            .ToolTipText_Lambda([this,I]{return FText::FromString(Names.IsValidIndex(I)?FString::Printf(TEXT("%s - %d gems"),*Names[I],Prices[I]):TEXT("Connecting..."));})
            .IsEnabled_Lambda([this,I]{return bReady && !bBusy && Receipt.IsEmpty() && RequestId.IsEmpty() && Skus.IsValidIndex(I) && !Owned.Contains(Skus[I]);})
            .OnClicked_Lambda([this,I]{SelectPrize(I);return FReply::Handled();})[
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2).BorderBackgroundColor_Lambda([this,I]{return Selected==I?FLinearColor(1.f,.7f,.1f,1.f):FLinearColor(.05f,.01f,.08f,1.f);})[
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().FillHeight(1)[SNew(SImage).Image(&ProductBrushes[I])]
                    + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this,I]{return FText::FromString(Prices.IsValidIndex(I)?Owned.Contains(Skus[I])?TEXT("OWNED"):FString::FromInt(Prices[I]):TEXT("..."));})]
                ]]
            ]
        ];
    }
    Cabinet->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30,277,0,0)[
        SNew(SBox).WidthOverride(98).HeightOverride(47).Visibility_Lambda([this]{return !Receipt.IsEmpty() || CollectAnimation>=0.f?EVisibility::Visible:EVisibility::Collapsed;})[
            SNew(SButton).ContentPadding(0).IsEnabled_Lambda([this]{return !bBusy && Animation<0.f;})
            .ToolTipText(FText::FromString(TEXT("Collect your reward")))
            .OnClicked_Lambda([this]{Collect();return FReply::Handled();})[
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding_Lambda([this]{const float Drop=Animation>=0.f ? (1.f-FMath::Clamp(Animation/.9f,0.f,1.f))*20.f : 0.f;return FMargin(30,Drop,30,0);}).BorderBackgroundColor(FLinearColor(.025f,.005f,.02f,1.f))[
                    SNew(SImage).Image(&DispensedBrush).ColorAndOpacity_Lambda([this]{const float Alpha=CollectAnimation>=0.f?1.f-FMath::Clamp(CollectAnimation/.45f,0.f,1.f):Animation>=0.f?FMath::Clamp(Animation/.9f,0.f,1.f):1.f;return FSlateColor(FLinearColor(1.f,1.f,1.f,Alpha));})]]]];
    return SNew(SOverlay)
        + SOverlay::Slot()[SNew(SImage).Image(&SceneBrush)]
        + SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.015f,.005f,.025f,.78f))]
        + SOverlay::Slot()[
        SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1000).HeightOverride(900)[
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(10)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("LUST HOUSE CASINO  /  LUSTVEND                         %lld GEMS"),Balance));})]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(8)[SNew(STextBlock).Text(FText::FromString(TEXT("TOUCH A PRIZE INSIDE THE MACHINE")))]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(SButton).ContentPadding(FMargin(20,8)).Text(FText::FromString(TEXT("MY REWARD LIBRARY"))).OnClicked_Lambda([this]{ShowRewardLibrary();return FReply::Handled();})]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(SButton).ContentPadding(FMargin(20,8)).Text(FText::FromString(TEXT("EARN GEMS / TUTORIAL"))).IsEnabled_Lambda([this]{return !bBusy;}).OnClicked_Lambda([this]{bShowingEarnings=true;Refresh();return FReply::Handled();})]
            + SVerticalBox::Slot().FillHeight(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[
                SNew(SBox).WidthOverride(430).HeightOverride(720)[
                    SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(215).HeightOverride(360)[Cabinet]]]]
            + SVerticalBox::Slot().AutoHeight().Padding(10)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]{return FText::FromString(Status);})]
            + SVerticalBox::Slot().AutoHeight().Padding(5)[SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1).Padding(5)[SNew(SButton).ContentPadding(16).IsEnabled_Lambda([this]{return !bBusy && (bConfirm || !RequestId.IsEmpty() || !Receipt.IsEmpty());})
                    .Text_Lambda([this]{return FText::FromString(!Receipt.IsEmpty()?TEXT("COLLECT REWARD"):!RequestId.IsEmpty()?TEXT("RETRY SAME PURCHASE"):TEXT("CONFIRM PURCHASE"));})
                    .OnClicked_Lambda([this]{if(!Receipt.IsEmpty()) Collect(); else Buy();return FReply::Handled();})]
                + SHorizontalBox::Slot().AutoWidth().Padding(5)[SNew(SButton).ContentPadding(16).Text(FText::FromString(TEXT("CANCEL / REFRESH")))
                    .IsEnabled_Lambda([this]{return !bBusy;}).OnClicked_Lambda([this]{bConfirm=false;Selected=INDEX_NONE;Refresh();return FReply::Handled();})]
                + SHorizontalBox::Slot().AutoWidth().Padding(5)[SNew(SButton).ContentPadding(16).Text(FText::FromString(TEXT("BACK TO LOBBY")))
                    .OnClicked_Lambda([this]{RemoveFromParent();return FReply::Handled();})]]
        ]]
    ]
    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[
        SNew(SBox).WidthOverride(440).Visibility_Lambda([this]{return bConfirm?EVisibility::Visible:EVisibility::Collapsed;})[
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(26).BorderBackgroundColor(FLinearColor(.08f,.01f,.06f,1.f))[
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text(FText::FromString(TEXT("CONFIRM YOUR REWARD")))]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]{return FText::FromString(Status);})]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SButton).ContentPadding(16).Text(FText::FromString(TEXT("SPEND GEMS"))).OnClicked_Lambda([this]{Buy();return FReply::Handled();})]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SButton).ContentPadding(16).Text(FText::FromString(TEXT("CANCEL"))).OnClicked_Lambda([this]{bConfirm=false;Status=TEXT("Touch a prize inside the machine.");return FReply::Handled();})]
            ]]
    ]
    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[
        SNew(SBox).WidthOverride(620).Visibility_Lambda([this]{return bShowingEarnings?EVisibility::Visible:EVisibility::Collapsed;})[
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(26).BorderBackgroundColor(FLinearColor(.045f,.008f,.06f,1.f))[
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text(FText::FromString(TEXT("EARN GEMS")))]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]{return FText::FromString(EarningSummary);})]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("Daily login: +10 gems\nComplete 10 / 25 / 50 regular spins: +5 each\nFinish all three: +5 extra\nFinish five daily sets in a week: +30\nAny bet size. Rewards arrive automatically. Resets use UTC.")))]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]{return FText::FromString(TutorialText());})]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SButton).ContentPadding(14).Text_Lambda([this]{return FText::FromString(TutorialStep>=3?TEXT("TUTORIAL COMPLETE"):TutorialStep==2?TEXT("FINISH TUTORIAL - EARN 30 GEMS"):TEXT("NEXT LESSON"));}).IsEnabled_Lambda([this]{return bReady && !bBusy && TutorialStep<3;}).OnClicked_Lambda([this]{CompleteTutorialLesson();return FReply::Handled();})]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SButton).ContentPadding(14).Text(FText::FromString(TEXT("RETURN TO MACHINE"))).OnClicked_Lambda([this]{bShowingEarnings=false;return FReply::Handled();})]
            ]]
    ]
    + SOverlay::Slot()[BuildRewardLibrary()]
    + SOverlay::Slot()[BuildRewardViewer()];
}

FString ULustVendWidget::DescribeEarnings(const TSharedPtr<FJsonObject>& Json)
{
    const TSharedPtr<FJsonObject>* E=nullptr;
    if(!Json.IsValid() || !Json->TryGetObjectField(TEXT("earning"),E) || !E || !(*E).IsValid())return TEXT("Earning progress is not available yet. Please refresh.");
    double Spins=0,Days=0;(*E)->TryGetNumberField(TEXT("spins"),Spins);(*E)->TryGetNumberField(TEXT("weeklyDays"),Days);
    return FString::Printf(TEXT("GEM MISSIONS TODAY: %d/10 | %d/25 | %d/50 spins\nWEEKLY: %d/5 completed daily sets (+30 gems)"),FMath::Min(10,(int32)Spins),FMath::Min(25,(int32)Spins),FMath::Min(50,(int32)Spins),(int32)Days);
}

FString ULustVendWidget::TutorialText() const
{
    if(TutorialStep==0)return TEXT("LESSON 1 OF 3: Gems are loyalty points for fixed digital rewards. Daily login and regular-play milestones earn gems. They have no cash value.");
    if(TutorialStep==1)return TEXT("LESSON 2 OF 3: Touch a prize inside the glass to highlight it. Check its gem price before confirming. You can always cancel.");
    if(TutorialStep==2)return TEXT("LESSON 3 OF 3: The server approves the purchase and saves your reward. Tap the chute to collect and open your package. Reopen owned rewards any time from My Reward Library. If disconnected, reopen the machine to recover it.");
    return TEXT("TUTORIAL COMPLETE: Your once-per-account 30-gem tutorial reward is saved.");
}

void ULustVendWidget::CompleteTutorialLesson()
{
    if(bBusy || TutorialStep>=3)return;
    bBusy=true;auto Body=MakeShared<FJsonObject>();Body->SetNumberField(TEXT("step"),TutorialStep+1);
    Send(TEXT("tutorial"),Body,[this](TSharedPtr<FJsonObject> Json,int32 Code){bBusy=false;if(Code==200)Refresh();else {EarningSummary=TEXT("Lesson could not be saved. Try again; rewards cannot be duplicated.");}});
}

void ULustVendWidget::SelectPrize(int32 Index)
{
    if(bBusy || !Receipt.IsEmpty() || !RequestId.IsEmpty() || !Skus.IsValidIndex(Index))return;
    Selected=Index;bConfirm=true;SetFrame(2);
    Status=FString::Printf(TEXT("Spend %d gems on %s?\n\nDEMO PREVIEW: Reward photos, videos and cosmetic content are not included in v0.0.2. This spends your gems to unlock a preview package with no media inside. You can cancel without spending gems."),Prices[Index],*Names[Index]);
}

void ULustVendWidget::SetFrame(int32 Frame)
{
    if(Frames.IsValidIndex(Frame) && Frames[Frame]) MachineBrush.SetResourceObject(Frames[Frame]);
}

void ULustVendWidget::NativeTick(const FGeometry& Geometry, float Delta)
{
    Super::NativeTick(Geometry, Delta);
    if(RewardOpeningSeconds>=0.f)
    {
        RewardOpeningSeconds+=Delta;
        if(RewardOpeningSeconds>=0.8f)FinishRewardOpening();
    }
    if(CollectAnimation>=0.f)
    {
        CollectAnimation+=Delta;
        if(CollectAnimation>=.5f){CollectAnimation=-1.f;bBusy=false;SetFrame(7);Refresh();}
    }
    if(Animation>=0.f)
    {
        Animation+=Delta;
        SetFrame(Animation<.45f?4:Animation<.9f?5:6);
        if(Animation>=1.35f) Animation=-1.f;
    }
}

void ULustVendWidget::Send(const FString& Route, const TSharedPtr<FJsonObject>& Body,
    TFunction<void(TSharedPtr<FJsonObject>,int32)> Done)
{
    const TWeakObjectPtr<ULustVendWidget> Weak(this);
    SendAccount(Session,Route,Body,[Weak,Done](TSharedPtr<FJsonObject> Json,int32 Code){if(Weak.IsValid())Done(Json,Code);});
}

void ULustVendWidget::SendAccount(const FString& Ticket,const FString& Route,const TSharedPtr<FJsonObject>& Body,
    TFunction<void(TSharedPtr<FJsonObject>,int32)> Done)
{
    FString TitleId;
    GConfig->GetString(TEXT("SugarLust.PlayFab"),TEXT("TitleId"),TitleId,GGameIni);
    if(Ticket.IsEmpty() || TitleId.IsEmpty()){Done(nullptr,401);return;}
    FString Function;
    if(Route==TEXT("state"))Function=TEXT("LustVendState");
    else if(Route==TEXT("purchase"))Function=TEXT("LustVendPurchase");
    else if(Route==TEXT("collect"))Function=TEXT("LustVendCollect");
    else if(Route==TEXT("tutorial"))Function=TEXT("LustVendTutorial");
    else{Done(nullptr,400);return;}
    auto Envelope=MakeShared<FJsonObject>();
    Envelope->SetStringField(TEXT("FunctionName"),Function);
    Envelope->SetStringField(TEXT("RevisionSelection"),TEXT("Live"));
    Envelope->SetObjectField(TEXT("FunctionParameter"),Body.IsValid()?Body.ToSharedRef():MakeShared<FJsonObject>());
    Envelope->SetBoolField(TEXT("GeneratePlayStreamEvent"),false);
    FString Payload;FJsonSerializer::Serialize(Envelope,TJsonWriterFactory<>::Create(&Payload));
    const auto Request=FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("https://%s.playfabapi.com/Client/ExecuteCloudScript"),*TitleId));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("X-Authorization"),Ticket);
    Request->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
    Request->SetTimeout(20);
    Request->SetContentAsString(Payload);
    Request->OnProcessRequestComplete().BindLambda([Done](FHttpRequestPtr Req,FHttpResponsePtr Response,bool Ok){
        if(!Ok || !Response.IsValid()){Done(nullptr,0);return;}
        TSharedPtr<FJsonObject> Json;
        if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()),Json) || !Json.IsValid()){Done(nullptr,503);return;}
        const TSharedPtr<FJsonObject>* Data=nullptr;
        const TSharedPtr<FJsonObject>* Result=nullptr;
        const TSharedPtr<FJsonObject>* Error=nullptr;
        if(Response->GetResponseCode()!=200 || !Json->TryGetObjectField(TEXT("data"),Data) || !Data || !(*Data).IsValid())
        {Done(nullptr,503);return;}
        if((*Data)->TryGetObjectField(TEXT("Error"),Error) && Error && (*Error).IsValid())
        {UE_LOG(LogTemp,Warning,TEXT("LUSTVEND CloudScript failed. Verify the Live PlayFab revision and Nakama runtime deployment."));Done(nullptr,503);return;}
        if(!(*Data)->TryGetObjectField(TEXT("FunctionResult"),Result) || !Result || !(*Result).IsValid()){Done(nullptr,503);return;}
        FString BusinessError;
        if((*Result)->TryGetStringField(TEXT("error"),BusinessError)){Done(*Result,409);return;}
        Done(*Result,200);
    });
    if(!Request->ProcessRequest())Done(nullptr,0);
}

void ULustVendWidget::SavePending()
{
    if(RequestId.IsEmpty()){IFileManager::Get().Delete(*PendingPath);return;}
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("requestId"),RequestId);Body->SetStringField(TEXT("sku"),PendingSku);Body->SetStringField(TEXT("version"),PendingVersion);
    FString Json;FJsonSerializer::Serialize(Body,TJsonWriterFactory<>::Create(&Json));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(PendingPath),true);
    if(!FFileHelper::SaveStringToFile(Json,*PendingPath)) Status=TEXT("Could not save purchase recovery record.");
}

void ULustVendWidget::Refresh()
{
    bBusy=true;
    Send(TEXT("state"),nullptr,[this](TSharedPtr<FJsonObject> Json,int32 Code){
        bBusy=false;
        if(Code!=200 || !Json.IsValid()){Status=TEXT("Rewards are unavailable. Please try Refresh. Your gem balance is safe.");return;}
        double Value=0;Json->TryGetNumberField(TEXT("balance"),Value);Balance=static_cast<int64>(Value);
        if(OnWalletChanged)OnWalletChanged(Balance);
        EarningSummary=DescribeEarnings(Json);
        const TSharedPtr<FJsonObject>* E=nullptr;double Step=0;
        if(Json->TryGetObjectField(TEXT("earning"),E) && E && (*E).IsValid() && (*E)->TryGetNumberField(TEXT("tutorialStep"),Step))TutorialStep=FMath::Clamp((int32)Step,0,3);
        Json->TryGetStringField(TEXT("version"),Version);
        Skus.Reset();Names.Reset();Prices.Reset();Owned.Reset();
        const TArray<TSharedPtr<FJsonValue>>* List=nullptr;
        if(Json->TryGetArrayField(TEXT("catalog"),List))for(const auto& V:*List){const auto P=V->AsObject();if(!P.IsValid())continue;Skus.Add(P->GetStringField(TEXT("sku")));Names.Add(P->GetStringField(TEXT("name")));Prices.Add(P->GetIntegerField(TEXT("price")));}
        if(Json->TryGetArrayField(TEXT("owned"),List))for(const auto& V:*List)Owned.Add(V->AsString());
        if(RequestId.IsEmpty())Receipt.Reset();
        if(RequestId.IsEmpty() && Json->TryGetArrayField(TEXT("pending"),List) && List->Num()>0)
        {
            const auto Pending=(*List)[0]->AsObject();Receipt=Pending->GetStringField(TEXT("receipt"));
            const int32 Product=Skus.IndexOfByKey(Pending->GetStringField(TEXT("sku")));
            if(Product>=0 && Product<6)DispensedBrush=ProductBrushes[Product];
            SetFrame(6);
        }
        bReady=true;
        if(!AutoOpenSku.IsEmpty())
        {
            const int32 Product=Skus.IndexOfByKey(AutoOpenSku);
            if(Product!=INDEX_NONE && Owned.Contains(AutoOpenSku)){AutoOpenSku.Reset();OpenReward(Product);}
        }
        Status=!Receipt.IsEmpty()?TEXT("Your reward is saved. Tap COLLECT REWARD."):!RequestId.IsEmpty()?TEXT("An unfinished purchase is saved. Retry the same request."):TEXT("Touch a prize to select and highlight it.");
    });
}

void ULustVendWidget::Buy()
{
    if(bBusy)return;
    if(RequestId.IsEmpty())
    {
        if(!bConfirm || !Skus.IsValidIndex(Selected))return;
        RequestId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);PendingSku=Skus[Selected];PendingVersion=Version;SavePending();
        if(!IFileManager::Get().FileExists(*PendingPath)){RequestId.Reset();return;}
    }
    bBusy=true;bConfirm=false;SetFrame(3);Status=TEXT("Waiting for server authorization...");
    auto Body=MakeShared<FJsonObject>();Body->SetStringField(TEXT("requestId"),RequestId);Body->SetStringField(TEXT("sku"),PendingSku);Body->SetStringField(TEXT("version"),PendingVersion);
    Send(TEXT("purchase"),Body,[this](TSharedPtr<FJsonObject> Json,int32 Code){
        bBusy=false;
        if(Code==200 && Json.IsValid() && Json->TryGetStringField(TEXT("receipt"),Receipt))
        {
            const int32 Product=Skus.IndexOfByKey(PendingSku);if(Product>=0 && Product<6)DispensedBrush=ProductBrushes[Product];
            RequestId.Reset();SavePending();double V=0;Json->TryGetNumberField(TEXT("balance"),V);Balance=static_cast<int64>(V);
            if(OnWalletChanged)OnWalletChanged(Balance);
            Animation=0;Status=TEXT("APPROVED. Dispensing your saved reward. Tap the chute to collect.");
        }
        else if(Code>=400 && Code<500){RequestId.Reset();SavePending();Status=TEXT("Purchase declined. Refresh wallet and select again.");SetFrame(0);}
        else Status=TEXT("Connection uncertain. Retry the same purchase safely.");
    });
}

void ULustVendWidget::Collect()
{
    if(bBusy || Receipt.IsEmpty())return;
    bBusy=true;auto Body=MakeShared<FJsonObject>();Body->SetStringField(TEXT("receipt"),Receipt);
    Send(TEXT("collect"),Body,[this](TSharedPtr<FJsonObject> Json,int32 Code){
        bBusy=false;
        if(Code==200 && Json.IsValid())
        {
            Json->TryGetStringField(TEXT("sku"),AutoOpenSku);
            Receipt.Reset();Animation=-1;CollectAnimation=0.f;bBusy=true;
            Status=TEXT("Reward collected. Opening your package in the reward library...");
        }
        else Status=TEXT("Collect unconfirmed. Tap COLLECT REWARD to retry; ownership is safe.");
    });
}

namespace
{
const FButtonStyle& RewardLibraryButtonStyle()
{
    static const FButtonStyle Style=[]
    {
        FButtonStyle Result;
        const FLinearColor Gold(.8f,.42f,.08f,1.f);
        Result.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.025f,.006f,.045f,1),10.f,Gold,1.f));
        Result.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.13f,.015f,.12f,1),10.f,FLinearColor(1.f,.2f,.6f,1),2.f));
        Result.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.18f,.015f,.12f,1),10.f,Gold,2.f));
        Result.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(.012f,.008f,.02f,1),10.f,FLinearColor(.12f,.08f,.14f,1),1.f));
        Result.SetNormalPadding(FMargin(0));Result.SetPressedPadding(FMargin(0));
        return Result;
    }();
    return Style;
}
}

void ULustVendWidget::ShowRewardLibrary()
{
    bShowingLibrary=true;bShowingEarnings=false;bConfirm=false;
    if(bReady && !bBusy)Refresh();
}

TSharedRef<SWidget> ULustVendWidget::BuildRewardLibrary()
{
    TSharedRef<SUniformGridPanel> Cards=SNew(SUniformGridPanel).SlotPadding(FMargin(10));
    for(int32 I=0;I<6;++I)
    {
        Cards->AddSlot(I%3,I/3)[SNew(SBox).WidthOverride(280).HeightOverride(215)[
            SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(14)
            .IsEnabled_Lambda([this,I]{return bReady && !bBusy && Skus.IsValidIndex(I) && Owned.Contains(Skus[I]);})
            .OnClicked_Lambda([this,I]{OpenReward(I);return FReply::Handled();})[
                SNew(SVerticalBox)
                + SVerticalBox::Slot().FillHeight(1).HAlign(HAlign_Center)[SNew(SBox).WidthOverride(80).HeightOverride(110)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(&ProductBrushes[I])]]]
                + SVerticalBox::Slot().AutoHeight().Padding(6)[SNew(STextBlock).Justification(ETextJustify::Center).Text_Lambda([this,I]{return FText::FromString(Names.IsValidIndex(I)?Names[I]:TEXT("Loading reward..."));})]
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Justification(ETextJustify::Center).Text_Lambda([this,I]{return FText::FromString(Skus.IsValidIndex(I) && Owned.Contains(Skus[I])?TEXT("OWNED - OPEN"):TEXT("LOCKED"));})]
            ]]];
    }
    return SNew(SOverlay).Visibility_Lambda([this]{return bShowingLibrary?EVisibility::Visible:EVisibility::Collapsed;})
        + SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0,0,0,.95f))]
        + SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1050).HeightOverride(760)[
            SNew(SBorder).Padding(28).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.055f,.008f,.075f,1))[
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",26)).Text(FText::FromString(TEXT("MY REWARD LIBRARY")))]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("Open owned packages to view your collection. You can return to them any time.")))]
                + SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Center)[Cards]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]{return FText::FromString(!bReady?TEXT("Connecting to your reward inventory..."):bBusy?TEXT("Refreshing your collection..."):Owned.Num()==0?TEXT("Your collection is empty. Collect a reward from the machine to get started."):FString::Printf(TEXT("%d OWNED PACKAGES"),Owned.Num()));})]
                + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1).Padding(4)[SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(14).Text(FText::FromString(TEXT("RETURN TO MACHINE"))).OnClicked_Lambda([this]{bShowingLibrary=false;return FReply::Handled();})]
                    + SHorizontalBox::Slot().FillWidth(1).Padding(4)[SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(14).Text(FText::FromString(TEXT("REFRESH COLLECTION"))).IsEnabled_Lambda([this]{return !bBusy;}).OnClicked_Lambda([this]{Refresh();return FReply::Handled();})]
                    + SHorizontalBox::Slot().FillWidth(1).Padding(4)[SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(14).Text(FText::FromString(TEXT("BACK TO LOBBY"))).OnClicked_Lambda([this]{RemoveFromParent();return FReply::Handled();})]]
            ]]]];
}

TSharedRef<SWidget> ULustVendWidget::BuildRewardViewer()
{
    return SNew(SOverlay).Visibility_Lambda([this]{return bViewingReward?EVisibility::Visible:EVisibility::Collapsed;})
        + SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.015f,.003f,.025f,1))]
        + SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1050).HeightOverride(820)[
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(16)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",26)).Justification(ETextJustify::Center).Text_Lambda([this]{return FText::FromString(Names.IsValidIndex(ViewedReward)?Names[ViewedReward]:TEXT("YOUR REWARD"));})]
            + SVerticalBox::Slot().FillHeight(1).Padding(16)[SNew(SOverlay)
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).WidthOverride(160).HeightOverride(230).Visibility_Lambda([this]{return RewardOpeningSeconds>=0.f?EVisibility::HitTestInvisible:EVisibility::Collapsed;})[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(&OpeningPackageBrush).ColorAndOpacity_Lambda([this]{return FSlateColor(FLinearColor(1,1,1,1.f-FMath::Clamp(RewardOpeningSeconds/.8f,0.f,1.f)));})]]]
                + SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).Visibility_Lambda([this]{return RewardOpeningSeconds<0.f && RewardPreviewBrush.GetResourceObject()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})[SNew(SImage).Image(&RewardPreviewBrush)]]
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).WidthOverride(780).Visibility_Lambda([this]{return RewardOpeningSeconds>=0.f || RewardPreviewBrush.GetResourceObject()?EVisibility::Collapsed:EVisibility::HitTestInvisible;})[SNew(STextBlock).Justification(ETextJustify::Center).WrapTextAt(720.f).AutoWrapText(false).Font(FCoreStyle::GetDefaultFontStyle("Regular",23)).Text_Lambda([this]{return FText::FromString(RewardOpeningSeconds>=0.f?TEXT("OPENING YOUR PACKAGE..."):RewardMessage);})]]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Justification(ETextJustify::Center).AutoWrapText(true).Text_Lambda([this]{const int32 Pages=RewardImages.Num()+RewardVideos.Num();return FText::FromString(RewardOpeningSeconds>=0.f?TEXT("OPENING YOUR PACKAGE..."):Pages>0?FString::Printf(TEXT("ITEM %d / %d  |  %s"),RewardPage+1,Pages,*RewardMessage):TEXT("This package remains in your collection. No additional gems are charged."));})]
            + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1).Padding(4)[SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(14).Text(FText::FromString(TEXT("PREVIOUS"))).IsEnabled_Lambda([this]{return RewardOpeningSeconds<0.f && RewardPage>0;}).OnClicked_Lambda([this]{--RewardPage;LoadRewardPage();return FReply::Handled();})]
                + SHorizontalBox::Slot().FillWidth(1).Padding(4)[SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(14).Text(FText::FromString(TEXT("PLAY / REPLAY"))).IsEnabled_Lambda([this]{return IsValid(RewardMediaPlayer) && RewardOpeningSeconds<0.f;}).OnClicked_Lambda([this]{if(RewardMediaPlayer){RewardMediaPlayer->Rewind();RewardMediaPlayer->Play();}return FReply::Handled();})]
                + SHorizontalBox::Slot().FillWidth(1).Padding(4)[SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(14).Text_Lambda([this]{return FText::FromString(bRewardAudioMuted?TEXT("UNMUTE VIDEO"):TEXT("MUTE VIDEO"));}).IsEnabled_Lambda([this]{return IsValid(RewardMediaSound);}).OnClicked_Lambda([this]{bRewardAudioMuted=!bRewardAudioMuted;if(RewardMediaSound)RewardMediaSound->SetVolumeMultiplier(bRewardAudioMuted?0.f:1.f);return FReply::Handled();})]
                + SHorizontalBox::Slot().FillWidth(1).Padding(4)[SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(14).Text(FText::FromString(TEXT("NEXT"))).IsEnabled_Lambda([this]{return RewardOpeningSeconds<0.f && RewardPage+1<RewardImages.Num()+RewardVideos.Num();}).OnClicked_Lambda([this]{++RewardPage;LoadRewardPage();return FReply::Handled();})]]
            + SVerticalBox::Slot().AutoHeight().Padding(12)[SNew(SButton).ButtonStyle(&RewardLibraryButtonStyle()).ForegroundColor(FLinearColor(1.f,.88f,.65f,1.f)).ContentPadding(16).Text(FText::FromString(TEXT("BACK TO MY COLLECTION"))).OnClicked_Lambda([this]{CloseRewardViewer();return FReply::Handled();})]
        ]]];
}

bool ULustVendWidget::OpenReward(int32 Index)
{
    if(!bReady || bBusy || !Skus.IsValidIndex(Index) || !Owned.Contains(Skus[Index]))return false;
    StopRewardMedia();RewardPreviewTexture=nullptr;RewardPreviewBrush.SetResourceObject(nullptr);
    ViewedReward=Index;RewardPage=0;RewardImages.Reset();RewardVideos.Reset();
    const FString Section=TEXT("SugarLust.RewardContent.")+Skus[Index];
    GConfig->GetArray(*Section,TEXT("Images"),RewardImages,GGameIni);
    GConfig->GetArray(*Section,TEXT("Videos"),RewardVideos,GGameIni);
    OpeningPackageBrush=ProductBrushes[FMath::Clamp(Index,0,5)];
    bShowingLibrary=true;bViewingReward=true;RewardOpeningSeconds=0.f;RewardMessage.Reset();
    return true;
}

void ULustVendWidget::FinishRewardOpening()
{
    RewardOpeningSeconds=-1.f;
    LoadRewardPage();
}

FString ULustVendWidget::ResolveRewardVideoPath(const FString& RelativePath)
{
    if(RelativePath.IsEmpty() || !FPaths::IsRelative(RelativePath) || RelativePath.Contains(TEXT("..")) || RelativePath.Contains(TEXT(":")))return {};
    const FString Root=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("Movies"));
    const FString Full=FPaths::ConvertRelativePathToFull(Root,RelativePath);
    return Full.StartsWith(Root+TEXT("/"))?Full:FString();
}

void ULustVendWidget::LoadRewardPage()
{
    StopRewardMedia();RewardPreviewTexture=nullptr;RewardPreviewBrush.SetResourceObject(nullptr);
    if(!Skus.IsValidIndex(ViewedReward) || !Owned.Contains(Skus[ViewedReward])){CloseRewardViewer();return;}
    RewardMessage=TEXT("CONTENT COMING SOON\nYour package is unlocked. Its photos, videos or cosmetic previews have not been added yet.");
    if(RewardImages.IsValidIndex(RewardPage))
    {
        const FString Asset=RewardImages[RewardPage];
        if(!Asset.StartsWith(TEXT("/Game/Art/Rewards/")) || Asset.Contains(TEXT("..")))return;
        const FString Package=FPackageName::ObjectPathToPackageName(Asset);
        if(!FPackageName::DoesPackageExist(Package))return;
        RewardPreviewTexture=LoadObject<UTexture2D>(nullptr,*Asset);
        if(RewardPreviewTexture){RewardPreviewBrush.SetResourceObject(RewardPreviewTexture);RewardPreviewBrush.ImageSize=FVector2D(RewardPreviewTexture->GetSizeX(),RewardPreviewTexture->GetSizeY());RewardMessage=TEXT("UNLOCKED - SAVED IN YOUR COLLECTION");}
        return;
    }
    const int32 VideoIndex=RewardPage-RewardImages.Num();
    if(!RewardVideos.IsValidIndex(VideoIndex))return;
    const FString File=ResolveRewardVideoPath(RewardVideos[VideoIndex]);
    if(File.IsEmpty() || !IFileManager::Get().FileExists(*File))return;
    RewardMediaPlayer=NewObject<UMediaPlayer>(this);
    RewardMediaPlayer->PlayOnOpen=true;RewardMediaPlayer->SetLooping(false);
    RewardMediaPlayer->OnMediaOpenFailed.AddDynamic(this,&ULustVendWidget::HandleRewardMediaFailed);
    RewardMediaPlayer->OnMediaOpened.AddDynamic(this,&ULustVendWidget::HandleRewardMediaOpened);
    RewardMediaTexture=NewObject<UMediaTexture>(this);
    RewardMediaTexture->SetMediaPlayer(RewardMediaPlayer);RewardMediaTexture->UpdateResource();
    RewardPreviewBrush.SetResourceObject(RewardMediaTexture);RewardPreviewBrush.ImageSize=FVector2D(1280,720);
    if(GetOwningPlayer())
    {
        RewardMediaSound=NewObject<UMediaSoundComponent>(GetOwningPlayer());
        RewardMediaSound->SetMediaPlayer(RewardMediaPlayer);RewardMediaSound->bIsUISound=true;
        RewardMediaSound->RegisterComponent();RewardMediaSound->SetVolumeMultiplier(bRewardAudioMuted?0.f:1.f);RewardMediaSound->Start();
    }
    RewardMessage=TEXT("YOUR UNLOCKED VIDEO");
    if(!RewardMediaPlayer->OpenFile(File))HandleRewardMediaFailed(TEXT(""));
}

void ULustVendWidget::HandleRewardMediaOpened(FString OpenedUrl)
{
    if(!RewardMediaPlayer)return;
    const FIntPoint Size=RewardMediaPlayer->GetVideoTrackDimensions(INDEX_NONE,INDEX_NONE);
    if(Size.X>0 && Size.Y>0)RewardPreviewBrush.ImageSize=FVector2D(Size.X,Size.Y);
}

void ULustVendWidget::HandleRewardMediaFailed(FString FailedUrl)
{
    StopRewardMedia();RewardPreviewBrush.SetResourceObject(nullptr);
    RewardMessage=TEXT("This video could not be played. Your package remains owned. Try reopening it.");
}

void ULustVendWidget::StopRewardMedia()
{
    if(RewardMediaSound){RewardMediaSound->Stop();RewardMediaSound->DestroyComponent();RewardMediaSound=nullptr;}
    if(RewardMediaPlayer){RewardMediaPlayer->OnMediaOpenFailed.RemoveAll(this);RewardMediaPlayer->OnMediaOpened.RemoveAll(this);RewardMediaPlayer->Close();RewardMediaPlayer=nullptr;}
    if(RewardMediaTexture){RewardMediaTexture->SetMediaPlayer(nullptr);RewardMediaTexture=nullptr;}
    RewardPreviewBrush.SetResourceObject(nullptr);
}

void ULustVendWidget::CloseRewardViewer()
{
    bViewingReward=false;RewardOpeningSeconds=-1.f;StopRewardMedia();RewardPreviewTexture=nullptr;
}

void ULustVendWidget::NativeDestruct()
{
    CloseRewardViewer();Super::NativeDestruct();
}
