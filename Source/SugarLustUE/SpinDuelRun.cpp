#include "SpinDuelRun.h"
#include "SocialCasinoPrototype.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"

void USpinDuelPanel::NativeOnInitialized()
{
 Super::NativeOnInitialized();auto* C=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=C;
 auto Text=[&](int32 Size){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",Size));T->SetColorAndOpacity(FLinearColor(1,.88f,.63f));T->SetAutoWrapText(true);return T;};
 auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrushColor(FLinearColor(.035f,.008f,.055f,.94f));B->SetPadding(FMargin(20));auto* BS=C->AddChildToCanvas(B);BS->SetAnchors(FAnchors(.02f,.55f,.98f,.98f));BS->SetOffsets(FMargin(0));
 auto* V=WidgetTree->ConstructWidget<UVerticalBox>();B->SetContent(V);Heading=Text(24);V->AddChildToVerticalBox(Heading);Details=Text(16);auto* DS=V->AddChildToVerticalBox(Details);DS->SetPadding(FMargin(0,6));DS->SetPadding(FMargin(0,6));
 auto* Reels=WidgetTree->ConstructWidget<UHorizontalBox>();auto* RS=V->AddChildToVerticalBox(Reels);RS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 for(int32 I=0;I<3;++I){auto* Button=WidgetTree->ConstructWidget<UButton>();Button->SetBackgroundColor(FLinearColor(.22f,.025f,.22f));auto* T=Text(24);T->SetJustification(ETextJustify::Center);T->SetAutoWrapText(false);Button->SetContent(T);auto* ReelSlot=Reels->AddChildToHorizontalBox(Button);ReelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ReelSlot->SetPadding(FMargin(4,6));ReelButtons.Add(Button);ReelLabels.Add(T);if(I==0)Button->OnClicked.AddDynamic(this,&USpinDuelPanel::HoldOne);if(I==1)Button->OnClicked.AddDynamic(this,&USpinDuelPanel::HoldTwo);if(I==2)Button->OnClicked.AddDynamic(this,&USpinDuelPanel::HoldThree);}
 auto* Actions=WidgetTree->ConstructWidget<UHorizontalBox>();V->AddChildToVerticalBox(Actions);
 auto Button=[&](UTextBlock*& Label){auto* Q=WidgetTree->ConstructWidget<UButton>();Q->SetBackgroundColor(FLinearColor(.25f,.025f,.12f));Label=Text(18);Label->SetAutoWrapText(false);Q->SetContent(Label);auto* S=Actions->AddChildToHorizontalBox(Q);S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));S->SetPadding(FMargin(4,5));return Q;};
 UTextBlock* T=nullptr;PrimaryButton=Button(T);PrimaryLabel=T;PrimaryButton->OnClicked.AddDynamic(this,&USpinDuelPanel::Primary);SecondaryButton=Button(T);SecondaryLabel=T;SecondaryButton->OnClicked.AddDynamic(this,&USpinDuelPanel::Secondary);auto* Exit=Button(T);T->SetText(FText::FromString(TEXT("RETURN TO CASINO")));Exit->OnClicked.AddDynamic(this,&USpinDuelPanel::Leave);
 auto* Note=Text(14);Note->SetText(FText::FromString(TEXT("SPIN DUEL | LOCAL PRACTICE | No coins, gems, wagers or persistent rewards")));auto* NS=C->AddChildToCanvas(Note);NS->SetAnchors(FAnchors(.02f,.02f,.98f,.1f));NS->SetOffsets(FMargin(0));
 Refresh();
}
void USpinDuelPanel::Refresh(){if(!Run.IsValid())return;Heading->SetText(FText::FromString(Run->Title()));Details->SetText(FText::FromString(Run->Detail()));PrimaryLabel->SetText(FText::FromString(Run->PrimaryText()));SecondaryLabel->SetText(FText::FromString(Run->SecondaryText()));PrimaryButton->SetIsEnabled(!Run->IsBusy());SecondaryButton->SetIsEnabled(!Run->IsBusy());for(int32 I=0;I<3;++I){ReelLabels[I]->SetText(FText::FromString(Run->ReelText(I)));ReelButtons[I]->SetIsEnabled(Run->InCombat()&&!Run->IsBusy());}}
void USpinDuelPanel::Primary(){if(Run.IsValid())Run->Primary();}void USpinDuelPanel::Secondary(){if(Run.IsValid())Run->Secondary();}void USpinDuelPanel::Leave(){if(Run.IsValid())Run->Leave();}
void USpinDuelPanel::HoldOne(){if(Run.IsValid())Run->Hold(0);}void USpinDuelPanel::HoldTwo(){if(Run.IsValid())Run->Hold(1);}void USpinDuelPanel::HoldThree(){if(Run.IsValid())Run->Hold(2);}

static void PoseDuelPerson(UPoseableMeshComponent* Mesh)
{
    const bool bSeated=false;const float Stride=0;
    auto* Asset=Cast<USkeletalMesh>(Mesh->GetSkinnedAsset());if(!Asset)return;
    const FReferenceSkeleton& Ref=Asset->GetRefSkeleton();
    TArray<FTransform> Pose=Ref.GetRefBonePose();
    for(int32 I=0;I<Pose.Num();++I){const int32 Parent=Ref.GetParentIndex(I);if(Parent>=0)Pose[I]=Pose[I]*Pose[Parent];}
    auto RotateChain=[&](const TCHAR* Name,FVector Axis,float Degrees)
    {
        const int32 Bone=Ref.FindBoneIndex(FName(Name));if(Bone==INDEX_NONE)return;
        const FVector Pivot=Pose[Bone].GetLocation();const FQuat Rotation(Axis,FMath::DegreesToRadians(Degrees));
        for(int32 I=Bone;I<Pose.Num();++I)
        {
            int32 Ancestor=I;while(Ancestor>=0 && Ancestor!=Bone)Ancestor=Ref.GetParentIndex(Ancestor);
            if(Ancestor!=Bone)continue;
            Pose[I].SetLocation(Pivot+Rotation.RotateVector(Pose[I].GetLocation()-Pivot));
            Pose[I].SetRotation((Rotation*Pose[I].GetRotation()).GetNormalized());
        }
    };
    auto AimChain=[&](const TCHAR* Name,const TCHAR* Child,FVector Direction)
    {
        const int32 Bone=Ref.FindBoneIndex(FName(Name)),Tip=Ref.FindBoneIndex(FName(Child));
        if(Bone==INDEX_NONE||Tip==INDEX_NONE)return;
        const FQuat Delta=FQuat::FindBetweenNormals((Pose[Tip].GetLocation()-Pose[Bone].GetLocation()).GetSafeNormal(),Direction.GetSafeNormal());
        FVector Axis;float Angle;Delta.ToAxisAndAngle(Axis,Angle);RotateChain(Name,Axis,FMath::RadiansToDegrees(Angle));
    };
    AimChain(TEXT("upperarm_l"),TEXT("lowerarm_l"),FVector(0,.12f,-1));
    AimChain(TEXT("upperarm_r"),TEXT("lowerarm_r"),FVector(0,-.12f,-1));
    AimChain(TEXT("lowerarm_l"),TEXT("hand_l"),FVector(0,0,-1));
    AimChain(TEXT("lowerarm_r"),TEXT("hand_r"),FVector(0,0,-1));
    if(bSeated)
    {
        AimChain(TEXT("thigh_l"),TEXT("calf_l"),FVector::RightVector);
        AimChain(TEXT("thigh_r"),TEXT("calf_r"),FVector::RightVector);
        AimChain(TEXT("calf_l"),TEXT("foot_l"),FVector(0,0,-1));
        AimChain(TEXT("calf_r"),TEXT("foot_r"),FVector(0,0,-1));
    }
    else
    {
        RotateChain(TEXT("thigh_l"),FVector::ForwardVector,Stride);
        RotateChain(TEXT("thigh_r"),FVector::ForwardVector,-Stride);
    }
    for(int32 I=0;I<Pose.Num();++I)Mesh->SetBoneTransformByName(Ref.GetBoneName(I),Pose[I],EBoneSpaces::ComponentSpace);
    Mesh->SetRelativeLocation(FVector(0,0,bSeated?-35.f:0.f));
    Mesh->RefreshBoneTransforms();
}

ASpinDuelRun::ASpinDuelRun()
{
 PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("RunRoot"));Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("RunCamera"));Camera->SetupAttachment(RootComponent);Camera->FieldOfView=65;
 static ConstructorHelpers::FObjectFinder<UStaticMesh> C(TEXT("/Engine/BasicShapes/Cube.Cube"));Cube=C.Object;static ConstructorHelpers::FObjectFinder<UStaticMesh> S(TEXT("/Engine/BasicShapes/Sphere.Sphere"));Sphere=S.Object;static ConstructorHelpers::FObjectFinder<UMaterialInterface> M(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));BaseMaterial=M.Object;
}
void ASpinDuelRun::BeginPlay(){Super::BeginPlay();Random.GenerateNewSeed();}
void ASpinDuelRun::Configure(ASocialCasinoPrototype* InCasino){Casino=InCasino;auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;Panel=CreateWidget<USpinDuelPanel>(PC);Panel->Run=this;Panel->AddToViewport(100);Refresh();}
UStaticMeshComponent* ASpinDuelRun::Part(USceneComponent* Parent,FVector P,FVector Size,FLinearColor Color,bool Round){auto* C=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(Parent);C->SetStaticMesh(Round?Sphere:Cube);C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100.f);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);auto* M=UMaterialInstanceDynamic::Create(BaseMaterial,this);M->SetVectorParameterValue(TEXT("Color"),Color);C->SetMaterial(0,M);C->RegisterComponent();return C;}
USceneComponent* ASpinDuelRun::Person(FVector P,bool Rival)
{
 auto* Group=NewObject<USceneComponent>(this);AddInstanceComponent(Group);Group->SetupAttachment(RootComponent);Group->SetRelativeLocation(P);Group->RegisterComponent();
 const TCHAR* Asset=Rival?TEXT("/Game/RP_Character/rp_eric_rigged_001_ue4/rp_eric_rigged_001_ue4.rp_eric_rigged_001_ue4"):TEXT("/Game/RP_Character/rp_carla_rigged_001_ue4/rp_carla_rigged_001_ue4.rp_carla_rigged_001_ue4");
 if(auto* Mesh=LoadObject<USkeletalMesh>(nullptr,Asset)){auto* Body=NewObject<UPoseableMeshComponent>(this);AddInstanceComponent(Body);Body->SetupAttachment(Group);Body->SetSkinnedAssetAndUpdate(Mesh);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Body->RegisterComponent();PoseDuelPerson(Body);}
 else{Part(Group,{0,0,90},{45,30,65},Rival?FLinearColor(.6f,.1f,.3f):FLinearColor(.2f,.3f,.7f));Part(Group,{0,0,145},{30,30,30},FLinearColor(.7f,.5f,.4f),true);}
 return Group;
}
void ASpinDuelRun::Label(const FString& Text,FVector P,float Size){auto* T=NewObject<UTextRenderComponent>(this);AddInstanceComponent(T);T->SetupAttachment(RootComponent);T->SetRelativeLocation(P);T->SetRelativeRotation(FRotator(0,90,0));T->SetWorldSize(Size);T->SetText(FText::FromString(Text));T->SetTextRenderColor(FColor(220,100,240));T->RegisterComponent();}
FVector ASpinDuelRun::Ground(FVector P) const
{
 TArray<FHitResult> Hits;FCollisionQueryParams Q;Q.bTraceComplex=true;GetWorld()->LineTraceMultiByChannel(Hits,GetActorLocation()+FVector(P.X,P.Y,8000),GetActorLocation()+FVector(P.X,P.Y,-4000),ECC_Visibility,Q);
 for(const auto& H:Hits)if(H.GetActor()&&H.GetActor()->GetClass()->GetName().Contains(TEXT("Landscape"))){P.Z=H.ImpactPoint.Z-GetActorLocation().Z+8;return P;}return P;
}
void ASpinDuelRun::StartForest()
{
 bool Loaded=false;Forest=ULevelStreamingDynamic::LoadLevelInstance(GetWorld(),TEXT("/Game/Maps/LustHouseSpinDuelGrove"),GetActorLocation(),FRotator::ZeroRotator,Loaded);Phase=TEXT("Loading");LoadTime=0;Message=TEXT("Opening the Enchanted Grove...");if(!Loaded||!Forest){Phase=TEXT("Error");Message=TEXT("The grove could not load. Return safely to the casino and try again.");}Refresh();
}
void ASpinDuelRun::ReadyForest()
{
 bReady=true;Stops={Ground({-22255,-4015,200}),Ground({-21200,-3600,200}),Ground({-20100,-2900,200}),Ground({-19000,-2600,200})};Walker=Person(Stops[0],false);Challenger=Person(Stops[0]+FVector(160,100,0),true);Challenger->SetRelativeRotation(FRotator(0,122,0));Walker->SetRelativeRotation(FRotator(0,-58,0));
 for(int32 I=0;I<Stops.Num();++I){Part(RootComponent,Stops[I]+FVector(0,0,-3),{250,250,5},FLinearColor(.25f,.03f,.4f));Label(I==3?TEXT("GROVE CHAMPION"):I==0?TEXT("PRACTICE CLEARING"):TEXT("DUEL CLEARING"),Stops[I]+FVector(-90,120,185));}
 for(int32 I=0;I<Stops.Num()-1;++I)for(int32 J=1;J<10;++J){FVector P=Ground(FMath::Lerp(Stops[I],Stops[I+1],J/10.f));Part(RootComponent,P+FVector(0,0,20),{16,16,16},FLinearColor(.7f,.1f,.7f),true);}
 if(Casino.IsValid())Casino->SetVenueVisible(false);if(auto* PC=GetWorld()->GetFirstPlayerController())PC->SetViewTarget(this);Camera->SetRelativeLocation(Stops[0]+FVector(-450,550,400));Camera->SetRelativeRotation((Stops[0]+FVector(80,0,90)-Camera->GetRelativeLocation()).Rotation());Phase=TEXT("Tutorial");Message=TEXT("Welcome! Swords attack; shields block; hearts heal; stars and wilds combine effects. Three matching symbols add 15 attack. Tap one reel to hold it for your next spin. Your rival's next move is shown before you spin. This guided practice trail has three key duels and a champion; return whenever you like.");Refresh();
}
void ASpinDuelRun::StartTravel(bool Hard){Combat.bHard=Hard;const int32 Next=Combat.Encounter+1;if(!Stops.IsValidIndex(Next))return;TravelFrom=Walker->GetRelativeLocation();TravelTo=Stops[Next];TravelAlpha=0;Phase=TEXT("Travel");Message=TEXT("Following the glowing trail to the next clearing...");Challenger->SetVisibility(false,true);Refresh();}
void ASpinDuelRun::Arrive(){Combat.BeginEncounter(Combat.Encounter+1);Phase=TEXT("Duel");Challenger->SetRelativeLocation(TravelTo+FVector(160,100,0));Challenger->SetVisibility(true,true);Message=TEXT("Choose a reel to hold, or spin all three. Your rival acts after your spin.");Refresh();}
void ASpinDuelRun::Tick(float Dt)
{
 Super::Tick(Dt);if(Phase==TEXT("Loading")){LoadTime+=Dt;if(Forest&&Forest->IsLevelVisible())ReadyForest();else if(LoadTime>45){Phase=TEXT("Error");Message=TEXT("Forest loading timed out. Return to the casino and try again.");Refresh();}return;}
 if(SpinDelay>0){SpinDelay-=Dt;ReelRefresh-=Dt;if(SpinDelay<=0)ResolveSpin();else if(ReelRefresh<=0){ReelRefresh=.08f;Refresh();}return;}
 if(!bReady||!Walker)return;
 if(Phase==TEXT("Travel")){TravelAlpha=FMath::Min(1.f,TravelAlpha+Dt*240.f/FMath::Max(FVector::Dist2D(TravelFrom,TravelTo),1.f));FVector P=Ground(FMath::Lerp(TravelFrom,TravelTo,TravelAlpha));Walker->SetRelativeLocation(P);Walker->SetRelativeRotation((TravelTo-TravelFrom).Rotation());if(TravelAlpha>=1){Walker->SetRelativeRotation(FRotator(0,-58,0));Arrive();}}
 const FVector P=Walker->GetRelativeLocation();const FVector Eye=P+FVector(-450,550,400);Camera->SetRelativeLocation(FMath::VInterpTo(Camera->GetRelativeLocation(),Eye,Dt,3));Camera->SetRelativeRotation((P+FVector(80,0,100)-Camera->GetRelativeLocation()).Rotation());
}
void ASpinDuelRun::Primary()
{
 if(IsBusy())return;if(Phase==TEXT("Confirm")){StartForest();return;}if(Phase==TEXT("Tutorial")){Combat.BeginEncounter(0);Phase=TEXT("Duel");Message=TEXT("Training rival: watch the next move, then SPIN. Holding is optional.");}
 else if(Phase==TEXT("Duel")&&Combat.CanSpin()){Pending.Reset();for(int32 I=0;I<3;++I)Pending.Add(Combat.Held==I?Combat.Reels[I]:Random.RandRange(0,4));SpinDelay=1.6f;Message=TEXT("Spinning...");}
 else if(Phase==TEXT("Cleared")){if(Combat.Encounter==0)StartTravel(true);else StartTravel(false);}
 else if(Phase==TEXT("Complete")||Phase==TEXT("Defeat")||Phase==TEXT("Error")){Leave();return;}Refresh();
}
void ASpinDuelRun::Secondary(){if(IsBusy())return;if(Phase==TEXT("Confirm")){Leave();return;}if(Phase==TEXT("Cleared")&&Combat.Encounter==0){Combat.HP=FMath::Min(100,Combat.HP+25);StartTravel(false);}else if(Phase==TEXT("Duel")){if(Combat.UseTonic())Message=TEXT("Recovery tonic restored up to 25 health. Your one run tonic is used.");else Message=TEXT("Tonic unavailable, or health is already full.");}Refresh();}
void ASpinDuelRun::Hold(int32 I){if(!InCombat()||IsBusy()||I<0||I>=3)return;Combat.Held=Combat.Held==I?-1:I;Refresh();}
void ASpinDuelRun::ResolveSpin()
{
 Message=Combat.Resolve(Pending);Pending.Reset();if(Combat.EnemyHP==0&&Combat.HP>0){Combat.AwardEncounter();if(Combat.Encounter==3){Phase=TEXT("Complete");Combat.Score+=250;Message=FString::Printf(TEXT("Champion defeated! Chest opened: %d practice score. This local reward does not change your casino balances. Return through the portal to finish."),Combat.Score);Part(RootComponent,Stops[3]+FVector(220,-90,35),{65,45,50},FLinearColor(.9f,.6f,.1f));}else{Phase=TEXT("Cleared");Message+=TEXT("\nKey collected! +12 health. Choose your next route.");}}else if(Combat.HP<=0){Phase=TEXT("Defeat");Message=TEXT("Run ended. No casino coins or gems were lost. Return to the casino and try a fresh run.");}Refresh();
}
void ASpinDuelRun::Refresh(){if(Panel)Panel->Refresh();}
FString ASpinDuelRun::Title() const{return FString::Printf(TEXT("SPIN DUEL | ENCHANTED GROVE | %s | HP %d | Shield %d | Keys %d/3"),*Phase,Combat.HP,Combat.Shield,FMath::Min(Combat.Keys,3));}
FString ASpinDuelRun::Detail() const{if(Phase==TEXT("Confirm"))return TEXT("Enter Spin Duel? Explore a guided forest trail, battle rivals with three reels, and unlock the champion's chest. Local practice only: no entry fee, casino currency, or persistent rewards. Your casino seat will remain empty while you play.");if(Phase==TEXT("Duel")){const TCHAR* Intent=Combat.Intent()==1?TEXT("GUARD: reduces your attack by 10"):Combat.Intent()==2?TEXT("HEAVY STRIKE"):TEXT("STRIKE");return FString::Printf(TEXT("%s | Rival HP %d/%d | NEXT: %s (%d damage)\n%s"),Combat.Encounter==3?TEXT("Grove Champion"):Combat.Encounter==0?TEXT("Training Rival"):TEXT("Grove Challenger"),Combat.EnemyHP,Combat.EnemyMax+(Combat.bHard&&Combat.Encounter==1?15:0),Intent,Combat.EnemyDamage(),*Message);}return Message;}
FString ASpinDuelRun::PrimaryText() const{if(SpinDelay>0)return TEXT("SPINNING...");if(Phase==TEXT("Confirm"))return TEXT("ENTER SPIN DUEL");if(Phase==TEXT("Loading"))return TEXT("LOADING...");if(Phase==TEXT("Tutorial"))return TEXT("START TRAINING DUEL");if(Phase==TEXT("Duel"))return TEXT("SPIN");if(Phase==TEXT("Cleared"))return Combat.Encounter==0?TEXT("CHALLENGER ROUTE +75 SCORE"):Combat.Encounter==2?TEXT("UNLOCK CHAMPION"):TEXT("NEXT CLEARING");if(Phase==TEXT("Travel"))return TEXT("FOLLOWING TRAIL...");return TEXT("FINISH / RETURN");}
FString ASpinDuelRun::SecondaryText() const{if(Phase==TEXT("Confirm"))return TEXT("NOT NOW");if(Phase==TEXT("Cleared")&&Combat.Encounter==0)return TEXT("RECOVERY ROUTE +25 HP");if(Phase==TEXT("Duel"))return FString::Printf(TEXT("TONIC (%d LEFT)"),Combat.Tonic);return TEXT("GUIDED PRACTICE RUN");}
FString ASpinDuelRun::ReelText(int32 I) const{static const TCHAR* Names[]={TEXT("SWORD"),TEXT("SHIELD"),TEXT("HEART"),TEXT("STAR"),TEXT("WILD")};return FString::Printf(TEXT("%s%s"),Names[SpinDelay>0&&Combat.Held!=I?(FMath::FloorToInt(SpinDelay*18)+I)%5:FMath::Clamp(Combat.Reels[I],0,4)],Combat.Held==I?TEXT(" [HELD]"):TEXT(""));}
void ASpinDuelRun::Leave(){if(bClosing||SpinDelay>0)return;bClosing=true;if(Casino.IsValid())Casino->ResumeFromSpinDuel();Destroy();}
void ASpinDuelRun::EndPlay(const EEndPlayReason::Type Reason){if(Panel)Panel->RemoveFromParent();if(Forest){Forest->SetShouldBeLoaded(false);Forest->SetShouldBeVisible(false);Forest->SetIsRequestingUnloadAndRemoval(true);}Super::EndPlay(Reason);}
