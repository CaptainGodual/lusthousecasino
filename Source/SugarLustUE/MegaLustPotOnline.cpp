#include "MegaLustPotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "SugarLustRootWidget.h"
#include "SugarLustSlotMachine.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
namespace {
 bool MegaNumber(const TSharedPtr<FJsonObject>& O,const TCHAR* K,double Min,double Max,int64& Out){double V=0;if(!O||!O->TryGetNumberField(K,V)||!FMath::IsFinite(V)||V!=FMath::FloorToDouble(V)||V<Min||V>Max)return false;Out=static_cast<int64>(V);return true;}
 bool MegaArrayNumber(const TSharedPtr<FJsonValue>& V,int64 Max,int64& Out){if(!V||V->Type!=EJson::Number)return false;double N=V->AsNumber();if(!FMath::IsFinite(N)||N!=FMath::FloorToDouble(N)||N<0||N>Max)return false;Out=static_cast<int64>(N);return true;}
}
FString UMegaLustPotWidget::JournalPath() const {return Lobby&&!Lobby->PlayFabPlayerId.IsEmpty()?FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("MegaPotPending_")+FPaths::MakeValidFileName(Lobby->PlayFabPlayerId)+TEXT(".json")):FString();}
bool UMegaLustPotWidget::SavePending()
{
 const FString Path=JournalPath();if(Path.IsEmpty())return false;auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("SpinId"),PendingId);O->SetNumberField(TEXT("Bet"),Game.Bet);O->SetNumberField(TEXT("ExpectedRevision"),ServerRevision);FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(O,Writer);
 return FFileHelper::SaveStringToFile(Json,*(Path+TEXT(".tmp")))&&IFileManager::Get().Move(*Path,*(Path+TEXT(".tmp")),true,true);
}
bool UMegaLustPotWidget::ClearPending(){const FString Path=JournalPath();if(Path.IsEmpty())return false;if(IFileManager::Get().FileExists(*Path)&&!IFileManager::Get().Delete(*Path))return false;PendingId.Reset();return true;}
void UMegaLustPotWidget::ConfigureLive(USugarLustRootWidget* Owner)
{
 Lobby=Owner;bLive=true;bReady=false;Auto=false;Animation=0;Game.Credits=0;
 ModeLabel->SetText(FText::FromString(TEXT("ONLINE • Server-owned wallet • Royal Lust rules v1")));
 for(auto& W:PracticeOnly)W->SetVisibility(ESlateVisibility::Collapsed);
 FString Rules=Help->GetText().ToString();Rules=Rules.Replace(TEXT("Pot counters are real practice-session progress, shared across bet levels."),TEXT("Pot progress is saved by the server separately for each bet level."));Rules=Rules.Replace(TEXT("Prototype credits reset when the application restarts. No live coins, gems or rewards."),TEXT("Online mode uses your casino coins. Practice previews and reset are disabled."));Rules=Rules.Replace(TEXT("Frequencies and payouts are provisional; no RTP target has been approved."),TEXT("Rules v1 targets 96% long-run RTP. Individual sessions can vary substantially."));Help->SetText(FText::FromString(Rules));
 if(!GetWidgetFromName(TEXT("MegaSync"))){auto* Sync=Button(TEXT("SYNC / RECOVER"),65,392,235,65);Sync->Rename(TEXT("MegaSync"),WidgetTree);Sync->OnClicked.AddDynamic(this,&UMegaLustPotWidget::Synchronize);}
 if(IFileManager::Get().FileExists(*JournalPath()))PendingId=TEXT("recovery-required");Synchronize();
}
void UMegaLustPotWidget::UpdateLivePots()
{
 Game.CrimsonCharge=Game.RoyalCharge=Game.MegaCharge=0;if(!LatestState)return;const TSharedPtr<FJsonObject>* PotJson=nullptr;if(!LatestState->TryGetObjectField(TEXT("Pots"),PotJson))return;const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;if((*PotJson)->TryGetArrayField(FString::FromInt(Game.Bet),Values)&&Values->Num()==3){Game.CrimsonCharge=static_cast<int32>((*Values)[0]->AsNumber());Game.RoyalCharge=static_cast<int32>((*Values)[1]->AsNumber());Game.MegaCharge=static_cast<int32>((*Values)[2]->AsNumber());}
}
bool UMegaLustPotWidget::ApplyServer(const TSharedPtr<FJsonObject>& Result,bool Sync)
{
 int64 Version=0;if(!MegaNumber(Result,TEXT("RulesVersion"),1,1,Version))return false;
 const TSharedPtr<FJsonObject>* Profile=nullptr;const TSharedPtr<FJsonObject>* State=nullptr;
 if(!Result->TryGetObjectField(TEXT("Profile"),Profile)||!(*Profile)->TryGetObjectField(TEXT("MegaPot"),State))return false;
 int64 Coins=0,XP=0,Rev=0,Bet=0,Free=0,Royal=0,Respins=0,Collector=0,Win=0;
 if(!MegaNumber(*Profile,TEXT("Coins"),0,9007199254740991.,Coins)||!MegaNumber(*Profile,TEXT("XP"),0,9007199254740991.,XP)||
 !MegaNumber(*State,TEXT("Version"),1,1,Version)||!MegaNumber(*State,TEXT("Revision"),0,9007199254740990.,Rev)||!MegaNumber(*State,TEXT("Bet"),1,100,Bet)||
 !MegaNumber(*State,TEXT("Free"),0,100,Free)||!MegaNumber(*State,TEXT("Royal"),0,100,Royal)||!MegaNumber(*State,TEXT("Respins"),0,3,Respins)||!MegaNumber(*State,TEXT("Collector"),1,10,Collector)||!MegaNumber(*State,TEXT("Win"),0,5000000,Win))return false;
 FString Phase;if(!(*State)->TryGetStringField(TEXT("Phase"),Phase)||(Phase!=TEXT("Base")&&Phase!=TEXT("Hold")&&Phase!=TEXT("Result")))return false;
 const TArray<TSharedPtr<FJsonValue>> *Grid=nullptr,*Locked=nullptr;if(!(*State)->TryGetArrayField(TEXT("Grid"),Grid)||Grid->Num()!=15||!(*State)->TryGetArrayField(TEXT("Locked"),Locked)||Locked->Num()!=15)return false;
 TArray<int32> NewGrid;TArray<int64> NewLocked;int32 Count=0;for(int32 I=0;I<15;++I){int64 V=0;if(!MegaArrayNumber((*Grid)[I],16,V))return false;NewGrid.Add(V);if(!MegaArrayNumber((*Locked)[I],Bet*200,V))return false;NewLocked.Add(V);if(V>0)++Count;}
 if(Phase==TEXT("Hold")&&(Count<6||Count>=15||Respins<=0))return false;
 const TSharedPtr<FJsonObject>* PotJson=nullptr;if(!(*State)->TryGetObjectField(TEXT("Pots"),PotJson))return false;
 for(const auto& Pair:(*PotJson)->Values){if(!FString(*Pair.Key).IsNumeric()||FCString::Atoi(*Pair.Key)<1||FCString::Atoi(*Pair.Key)>100||Pair.Value->Type!=EJson::Array||Pair.Value->AsArray().Num()!=3)return false;const auto& P=Pair.Value->AsArray();int64 V;const int32 Goals[]={11,15,23};for(int32 I=0;I<3;++I)if(!MegaArrayNumber(P[I],Goals[I],V))return false;}
 if(Sync){bool Synchronized=false;if(!Result->TryGetBoolField(TEXT("Synchronized"),Synchronized)||!Synchronized)return false;}
 if(!ClearPending())return false;
 Game.Grid=MoveTemp(NewGrid);Game.Locked=MoveTemp(NewLocked);Game.Credits=Coins;Game.Bet=Bet;Game.FreeSpins=Free;Game.RoyalSpins=Royal;Game.Respins=Respins;Game.CollectorMultiplier=Collector;Game.LastWin=Win;Game.Mode=Phase==TEXT("Hold")?FMegaLustPotGame::Phase::Hold:Phase==TEXT("Result")?FMegaLustPotGame::Phase::Result:FMegaLustPotGame::Phase::Base;
 LatestState=*State;ServerRevision=Rev;Game.WinningCells.Reset();Game.WinningLines.Reset();
 Game.Message=Sync?TEXT("Server synchronized. Your wallet and pot features are restored."):Phase==TEXT("Hold")?TEXT("MEGA POT • New hearts reset your respins."):Phase==TEXT("Result")?TEXT("Feature complete. Award already credited by the server."):TEXT("Spin settled by server.");
 if(Lobby&&Lobby->SlotGame){FString Date;double Streak=0;(*Profile)->TryGetStringField(TEXT("DailyBonusLastClaimDate"),Date);(*Profile)->TryGetNumberField(TEXT("DailyBonusStreakDay"),Streak);Lobby->SlotGame->ApplyCloudProfile(Coins,XP,Date,static_cast<int32>(Streak));Lobby->ApplyCloudMissionData(*Profile);Lobby->UpdatePlayerProfileUI();Lobby->UpdateMissionsUI();}
 bReady=true;if(Game.Mode==FMegaLustPotGame::Phase::Result)Auto=false;AutoDelay=1.8f;Refresh();return true;
}
void UMegaLustPotWidget::Synchronize()
{
 if(!bLive||bBusy||Animation>0)return;Auto=false;bReady=false;
 if(!Lobby||!Lobby->bPlayFabProfileReady||Lobby->bPlayFabRequestInFlight){Game.Message=TEXT("Wait for casino login, then choose SYNC / RECOVER.");Refresh();return;}
 bBusy=true;Lobby->bPlayFabRequestInFlight=true;const int32 Token=++Generation;Game.Message=TEXT("Synchronizing server wallet and features...");Refresh();TWeakObjectPtr<UMegaLustPotWidget> Weak(this);
 Lobby->ExecutePlayFabCloudScript(TEXT("MegaLustPotState"),MakeShared<FJsonObject>(),[Weak,Token](TSharedPtr<FJsonObject> R){if(!Weak.IsValid()||Weak->Generation!=Token)return;Weak->bBusy=false;if(Weak->Lobby)Weak->Lobby->bPlayFabRequestInFlight=false;if(!Weak->ApplyServer(R,true)){Weak->bReady=false;Weak->Game.Message=TEXT("Server rules/state mismatch. Online play remains locked; SYNC to recover.");Weak->Refresh();}},[Weak,Token](const FString& Error){if(!Weak.IsValid()||Weak->Generation!=Token)return;Weak->bBusy=false;if(Weak->Lobby)Weak->Lobby->bPlayFabRequestInFlight=false;UE_LOG(LogTemp,Warning,TEXT("MegaLustPotState failed: %s"),*Error);Weak->Game.Message=FString::Printf(TEXT("Mega Pot sync failed: %s. Choose SYNC / RECOVER to retry."),*Error);Weak->Refresh();});
}
void UMegaLustPotWidget::SpinLive()
{
 if(!bReady||bBusy||Animation>0||ShowingHelp||!Lobby||Lobby->bPlayFabRequestInFlight)return;
 if(Game.Mode==FMegaLustPotGame::Phase::Result){Game.Collect();Refresh();return;}
 if(Game.Mode==FMegaLustPotGame::Phase::Base&&Game.FreeSpins==0&&Game.Credits<Game.Bet*20){Auto=false;Game.Message=TEXT("Not enough casino coins for this bet.");Refresh();return;}
 if(!PendingId.IsEmpty()){bReady=false;Auto=false;Game.Message=TEXT("An earlier spin needs recovery. Choose SYNC / RECOVER.");Refresh();return;}
 PendingId=FGuid::NewGuid().ToString(EGuidFormats::Digits);if(!SavePending()){bReady=false;Auto=false;Game.Message=TEXT("Could not save the spin recovery receipt. No spin sent.");Refresh();return;}
 auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("SpinId"),PendingId);P->SetNumberField(TEXT("Bet"),Game.Bet);P->SetNumberField(TEXT("ExpectedRevision"),ServerRevision);bBusy=true;Lobby->bPlayFabRequestInFlight=true;const int32 Token=++Generation;Game.Message=TEXT("Waiting for server result...");Refresh();TWeakObjectPtr<UMegaLustPotWidget> Weak(this);
 Animation=Game.Mode==FMegaLustPotGame::Phase::Hold?2.5f:3.5f;PendingPresentation.Reset();
 Lobby->ExecutePlayFabCloudScript(TEXT("MegaLustPotSpin"),P,[Weak,Token](TSharedPtr<FJsonObject> R){if(!Weak.IsValid()||Weak->Generation!=Token)return;Weak->PendingPresentation=R.IsValid()?R:MakeShared<FJsonObject>();if(Weak->Animation<=0)Weak->FinishLiveSpin();},[Weak,Token](const FString& Error){if(!Weak.IsValid()||Weak->Generation!=Token)return;Weak->Animation=0;Weak->PendingPresentation.Reset();Weak->bBusy=false;Weak->bReady=false;Weak->Auto=false;if(Weak->Lobby)Weak->Lobby->bPlayFabRequestInFlight=false;UE_LOG(LogTemp,Warning,TEXT("MegaLustPotSpin failed: %s"),*Error);Weak->Game.Message=TEXT("Spin connection lost. Choose SYNC / RECOVER; no local reroll or payout.");Weak->Refresh();});
}



void UMegaLustPotWidget::FinishLiveSpin()
{
 if(!PendingPresentation||Animation>0)return;
 auto R=PendingPresentation;PendingPresentation.Reset();Animation=0;bBusy=false;
 if(Lobby)Lobby->bPlayFabRequestInFlight=false;
 FString ReplyId;
 if(!R->TryGetStringField(TEXT("SpinId"),ReplyId)||ReplyId!=PendingId||!ApplyServer(R,false)){
  bReady=false;Auto=false;Game.Message=TEXT("Unconfirmed response. Choose SYNC / RECOVER before another spin.");Refresh();
 }
}
