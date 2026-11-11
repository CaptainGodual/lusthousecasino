#include "SocialCasinoPrototype.h"
#include "SpinDuelRun.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "SugarLustRootWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Input/Reply.h"

void USocialCasinoControls::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetVisibility(ESlateVisibility::Visible);
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget=Canvas;
    auto* Floor=WidgetTree->ConstructWidget<UBorder>();
    Floor->SetBrushColor(FLinearColor(0,0,0,0));
    auto* Fill=Canvas->AddChildToCanvas(Floor);
    Fill->SetAnchors(FAnchors(0,0,1,1)); Fill->SetOffsets(FMargin(0));
    auto* Menu=WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* MenuSlot=Canvas->AddChildToCanvas(Menu);
    MenuSlot->SetAnchors(FAnchors(0,0,1,0)); MenuSlot->SetOffsets(FMargin(20,20,20,60));
    const TCHAR* Names[]={TEXT("SIT: MACHINE 1"),TEXT("SIT: MACHINE 3"),TEXT("PLAY"),TEXT("STAND UP"),TEXT("LEAVE ROOM"),TEXT("SPIN DUEL")};
    for(int32 I=0;I<6;++I)
    {
        auto* Button=WidgetTree->ConstructWidget<UButton>();
        auto* Text=WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Names[I]));
        FSlateFontInfo Font=Text->GetFont(); Font.Size=16; Text->SetFont(Font);
        Button->AddChild(Text); Button->SetBackgroundColor(FLinearColor(0.12f,0.015f,0.09f,1));
        auto* ButtonSlot=Menu->AddChildToHorizontalBox(Button); ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); ButtonSlot->SetPadding(FMargin(4));
        switch(I){
        case 0:Button->OnClicked.AddDynamic(this,&USocialCasinoControls::SeatOne);break;
        case 1:Button->OnClicked.AddDynamic(this,&USocialCasinoControls::SeatThree);break;
        case 2:Button->OnClicked.AddDynamic(this,&USocialCasinoControls::Play);break;
        case 3:Button->OnClicked.AddDynamic(this,&USocialCasinoControls::Stand);break;
        case 4:Button->OnClicked.AddDynamic(this,&USocialCasinoControls::Leave);break;
        case 5:Button->OnClicked.AddDynamic(this,&USocialCasinoControls::SpinDuel);break;
        }
    }
    Status=WidgetTree->ConstructWidget<UTextBlock>();
    Status->SetText(FText::FromString(TEXT("LOCAL PROTOTYPE | Tap floor to walk. Tap a cabinet to sit. Neighbours are NPCs.")));
    Status->SetAutoWrapText(true);
    auto* StatusSlot=Canvas->AddChildToCanvas(Status);
    StatusSlot->SetAnchors(FAnchors(0,1,1,1));StatusSlot->SetOffsets(FMargin(25,-85,25,70));
}
FReply USocialCasinoControls::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
    if(E.GetEffectingButton()==EKeys::LeftMouseButton && Casino.IsValid()) {FVector2D Pixel,Viewport;USlateBlueprintLibrary::AbsoluteToViewport(this,E.GetScreenSpacePosition(),Pixel,Viewport);Casino->WalkFromScreen(Pixel);return FReply::Handled();}
    return Super::NativeOnMouseButtonDown(G,E);
}
FReply USocialCasinoControls::NativeOnTouchStarted(const FGeometry& G,const FPointerEvent& E)
{
    if(Casino.IsValid()) { FVector2D Pixel,Viewport;USlateBlueprintLibrary::AbsoluteToViewport(this,E.GetScreenSpacePosition(),Pixel,Viewport);Casino->WalkFromScreen(Pixel); }
    return FReply::Handled();
}
void USocialCasinoControls::SeatOne(){if(Casino.IsValid())Casino->RequestSeat(0);}
void USocialCasinoControls::SeatThree(){if(Casino.IsValid())Casino->RequestSeat(2);}
void USocialCasinoControls::Play(){if(Casino.IsValid())Casino->Play();}
void USocialCasinoControls::Stand(){if(Casino.IsValid())Casino->Stand();}
void USocialCasinoControls::SpinDuel(){if(Casino.IsValid())Casino->GoToSpinDuel();}
void USocialCasinoControls::Leave(){if(Casino.IsValid())Casino->Leave();}

ASocialCasinoPrototype::ASocialCasinoPrototype()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("SocialRoot"));
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("SocialCamera"));Camera->SetupAttachment(RootComponent);Camera->FieldOfView=60;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> C(TEXT("/Engine/BasicShapes/Cube.Cube"));Cube=C.Object;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> S(TEXT("/Engine/BasicShapes/Sphere.Sphere"));Sphere=S.Object;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> M(TEXT("/Game/Art/Lounge/ModernPaintedCabinet/SM_ModernCabinetSingle.SM_ModernCabinetSingle"));CabinetMesh=M.Object;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Eric(TEXT("/Game/RP_Character/rp_eric_rigged_001_ue4/rp_eric_rigged_001_ue4.rp_eric_rigged_001_ue4"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Carla(TEXT("/Game/RP_Character/rp_carla_rigged_001_ue4/rp_carla_rigged_001_ue4.rp_carla_rigged_001_ue4"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Claudia(TEXT("/Game/RP_Character/rp_claudia_rigged_002_ue4/rp_claudia_rigged_002_ue4.rp_claudia_rigged_002_ue4"));
    for (auto* Person : {Eric.Object.Get(), Carla.Object.Get(), Claudia.Object.Get()}) if(Person) People.Add(Person);
    static ConstructorHelpers::FObjectFinder<UAnimSequence> FemaleSit(TEXT("/Game/Art/Lounge/Animations/Seated/RP_FemaleSit_Anim_mixamo_com.RP_FemaleSit_Anim_mixamo_com"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> MaleSit(TEXT("/Game/Art/Lounge/Animations/Seated/RP_MaleSit_Anim_mixamo_com.RP_MaleSit_Anim_mixamo_com"));
    FemaleSeatedAnimation=FemaleSit.Object;MaleSeatedAnimation=MaleSit.Object;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> B(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));BaseMaterial=B.Object;
}
USceneComponent* ASocialCasinoPrototype::MakeGroup(USceneComponent* Parent)
{
    auto* Group=NewObject<USceneComponent>(this);AddInstanceComponent(Group);Group->SetupAttachment(Parent);Group->RegisterComponent();return Group;
}
void ASocialCasinoPrototype::Part(USceneComponent* Parent,FVector Pos,FVector Size,FLinearColor Color,bool bSphere)
{
    auto* Mesh=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Mesh);Mesh->SetupAttachment(Parent);
    Mesh->SetStaticMesh(bSphere?Sphere:Cube);Mesh->SetRelativeLocation(Pos);Mesh->SetRelativeScale3D(Size/100.f);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCastShadow(false);
    if(BaseMaterial){auto* Mat=UMaterialInstanceDynamic::Create(BaseMaterial,this);Mat->SetVectorParameterValue(TEXT("Color"),Color);Mesh->SetMaterial(0,Mat);}
    Mesh->RegisterComponent();
}
USceneComponent* ASocialCasinoPrototype::MakeAvatar(FVector Pos,FLinearColor Color,bool bSeated)
{
    auto* Body=MakeGroup(RootComponent);Body->SetRelativeLocation(Pos);
    if(People.Num()>0)
    {
        auto* Person=NewObject<UPoseableMeshComponent>(this);AddInstanceComponent(Person);Person->SetupAttachment(Body);
        const int32 PersonIndex=PeopleCursor++ % People.Num();
        Person->SetSkinnedAssetAndUpdate(People[PersonIndex]);
        Person->SetRelativeRotation(FRotator(0,180,0));Person->SetCollisionEnabled(ECollisionEnabled::NoCollision);Person->SetCastShadow(false);
        Person->RegisterComponent();ApplyPersonPose(Person,bSeated,0);
        UAnimSequence* Clip=PersonIndex==0?MaleSeatedAnimation:FemaleSeatedAnimation;
        if(Clip && Clip->GetSkeleton()==People[PersonIndex]->GetSkeleton())
        {
            auto* Animated=NewObject<USkeletalMeshComponent>(this);AddInstanceComponent(Animated);Animated->SetupAttachment(Body);
            Animated->SetSkeletalMesh(People[PersonIndex]);Animated->SetRelativeRotation(FRotator(0,180,0));
            // The male clip needs extra thigh clearance above the 62 cm cushion top.
            Animated->SetRelativeLocation(FVector(0,PersonIndex==0?-20.f:0.f,PersonIndex==0?16.f:0.f));
            Animated->SetCollisionEnabled(ECollisionEnabled::NoCollision);Animated->SetCastShadow(false);
            Animated->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
            Animated->RegisterComponent();Animated->PlayAnimation(Clip,true);Animated->SetPosition(.5f,false);
            Animated->TickAnimation(0.f,false);Animated->RefreshBoneTransforms();
            Animated->SetVisibility(bSeated,true);Person->SetVisibility(!bSeated,true);
            SeatedVisuals.Add(Animated);if(!bSeated)PlayerSeatedVisual=Animated;
        }
        if(!bSeated) PlayerPose=Person;
        return Body;
    }
    Part(Body,{0,0,108},{38,25,55},Color,true);Part(Body,{0,0,151},{30,30,34},FLinearColor(0.65f,0.4f,0.25f),true);
    Part(Body,{-25,0,108},{12,15,52},Color);Part(Body,{25,0,108},{12,15,52},Color);
    auto* Legs=MakeGroup(Body);Part(Legs,{-11,0,42},{14,20,82},FLinearColor(0.04f,0.04f,0.08f));Part(Legs,{11,0,42},{14,20,82},FLinearColor(0.04f,0.04f,0.08f));
    auto* Bent=MakeGroup(Body);
    for(float X:{-11.f,11.f}){Part(Bent,{X,-20,80},{14,50,16},FLinearColor(0.04f,0.04f,0.08f));Part(Bent,{X,-40,49},{14,16,55},FLinearColor(0.04f,0.04f,0.08f));}
    Legs->SetVisibility(!bSeated,true);Bent->SetVisibility(bSeated,true);
    if(!bSeated){StandingLegs=Legs;SittingLegs=Bent;}
    return Body;
}
void ASocialCasinoPrototype::Label(const FString& Text,FVector Pos,float Size)
{
    auto* T=NewObject<UTextRenderComponent>(this);AddInstanceComponent(T);T->SetupAttachment(RootComponent);
    T->SetRelativeLocation(Pos);T->SetRelativeRotation(FRotator(0,90,0));T->SetHorizontalAlignment(EHTA_Center);T->SetWorldSize(Size);T->SetText(FText::FromString(Text));
    if(auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Lounge/CasinoScene/LHC_TextTwoSided.LHC_TextTwoSided")))T->SetTextMaterial(Mat);
    T->RegisterComponent();
}
FVector ASocialCasinoPrototype::SeatPosition(int32 I) const {return bUseVenue && VenueSeatTransforms.IsValidIndex(I) ? VenueSeatTransforms[I].GetLocation() : FVector(-210+140*I,205,0);}
void ASocialCasinoPrototype::BeginPlay()
{
    Super::BeginPlay();
    BuildPreview();
}
void ASocialCasinoPrototype::BuildPreview()
{
    if(Avatar) return;
    if(bUseVenue)
    {
        if(!bVenueLoading){bool Success=false;VenueStream=ULevelStreamingDynamic::LoadLevelInstance(GetWorld(),TEXT("/Game/Maps/LustHouseVelvetCasinoFloor"),GetActorLocation(),FRotator::ZeroRotator,Success);bVenueLoading=Success;Status(Success?TEXT("Loading event venue..."):TEXT("Venue could not load. Leave the room and try again."));}
        return;
    }
    Part(RootComponent,{0,380,-10},{1050,1100,20},FLinearColor(0.08f,0.025f,0.045f));
    Part(RootComponent,{0,-50,190},{1050,25,380},FLinearColor(0.2f,0.01f,0.035f));
    for(int32 I=0;I<4;++I)
    {
        const float X=SeatPosition(I).X;
        auto* Cabinet=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("Machine%d"),I));AddInstanceComponent(Cabinet);Cabinet->SetupAttachment(RootComponent);
        Cabinet->SetStaticMesh(CabinetMesh?CabinetMesh:Cube);
        const FBox Bounds=Cabinet->GetStaticMesh()->GetBoundingBox();
        const float CabinetScale=220.f/FMath::Max(Bounds.GetSize().Z,1.f);
        const FVector Center=Bounds.GetCenter();
        Cabinet->SetRelativeScale3D(FVector(CabinetScale));
        Cabinet->SetRelativeRotation(FRotator::ZeroRotator);
        Cabinet->SetRelativeLocation(FVector(X,100,0)-FVector(Center.X,Center.Y,Bounds.Min.Z)*CabinetScale);
        Cabinet->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Cabinet->SetCollisionResponseToAllChannels(ECR_Ignore);Cabinet->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Cabinet->SetCollisionObjectType(ECC_WorldStatic);
        Cabinet->SetCastShadow(false);Cabinet->RegisterComponent();
        Part(RootComponent,{X,SeatPosition(I).Y,55},{55,55,14},FLinearColor(0.4f,0.01f,0.09f));Part(RootComponent,{X,SeatPosition(I).Y,25},{8,8,50},FLinearColor(0.7f,0.4f,0.1f));
        Label(FString::Printf(TEXT("%d%s"),I+1,(I==1||I==3)?TEXT(" | NPC"):TEXT(" | AVAILABLE")),{X,100,250},17);
    }
    MakeAvatar(SeatPosition(1)+FVector(0,0,-8),FLinearColor(0.1f,0.35f,0.7f),true);
    MakeAvatar(SeatPosition(3)+FVector(0,0,-8),FLinearColor(0.6f,0.15f,0.3f),true);
    Avatar=MakeAvatar({0,700,0},FLinearColor(0.1f,0.7f,0.55f),false);Destination=Avatar->GetRelativeLocation();
    auto* Light=NewObject<UPointLightComponent>(this);AddInstanceComponent(Light);Light->SetupAttachment(RootComponent);Light->SetRelativeLocation({0,400,500});Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(1800);Light->SetAttenuationRadius(1800);Light->SetCastShadows(false);Light->RegisterComponent();
    auto* RoomExposure=NewObject<UPostProcessComponent>(this);AddInstanceComponent(RoomExposure);RoomExposure->SetupAttachment(RootComponent);
    RoomExposure->bUnbound=true;RoomExposure->Settings.bOverride_AutoExposureBias=true;RoomExposure->Settings.AutoExposureBias=-0.7f;RoomExposure->RegisterComponent();
    Camera->SetRelativeLocation({-850,1050,1050});Camera->SetRelativeRotation((FVector(0,300,70)-Camera->GetRelativeLocation()).Rotation());
}
void ASocialCasinoPrototype::Configure(USugarLustRootWidget* InLobby)
{
    Lobby=InLobby;
    if(auto* PC=GetWorld()->GetFirstPlayerController())
    {
        PC->SetViewTarget(this);PC->bShowMouseCursor=true;
        Controls=CreateWidget<USocialCasinoControls>(PC);Controls->Casino=this;Controls->AddToViewport(10);
        FInputModeGameAndUI Input;Input.SetHideCursorDuringCapture(false);PC->SetInputMode(Input);
    }
}
void ASocialCasinoPrototype::Status(const FString& Text){if(Controls&&Controls->Status)Controls->Status->SetText(FText::FromString(Text));}
void ASocialCasinoPrototype::WalkFromScreen(FVector2D Screen)
{
    if(Seat>=0||bPlaying||!Avatar)return;
    auto* PC=GetWorld()->GetFirstPlayerController();FVector Origin,Direction;
    if(!PC||!PC->DeprojectScreenPositionToWorld(Screen.X,Screen.Y,Origin,Direction))return;
    FCollisionQueryParams Query; Query.bTraceComplex=true;
    FHitResult Hit;
    if(GetWorld()->LineTraceSingleByChannel(Hit,Origin,Origin+Direction*10000,ECC_Visibility,Query))
    {
        if(bUseVenue)
        {
            const int32 Index=VenueMachines.IndexOfByPredicate([&](const TObjectPtr<AActor>& Machine){return Machine.Get()==Hit.GetActor();});
            if(Index!=INDEX_NONE){RequestSeat(Index);return;}
        }
        const FString Name=Hit.Component.IsValid()?Hit.Component->GetName():FString();
        if(Name.StartsWith(TEXT("Machine"))){RequestSeat(FCString::Atoi(*Name.Mid(7)));return;}
    }
    if(Direction.Z>=-.001f)return;
    if(bUseVenue)
    {
        const FVector O=GetActorTransform().InverseTransformPosition(Origin);
        const FVector D=GetActorTransform().InverseTransformVectorNoScale(Direction);
        float T=-O.Z/D.Z;FVector P=O+D*T;
        if(P.Y<150){T=(41.f-O.Z)/D.Z;P=O+D*T;}
        else if(P.Y<330){const float Denom=D.Z+(41.f/180.f)*D.Y;if(FMath::Abs(Denom)<.001f)return;T=(41.f*330.f/180.f-O.Z-(41.f/180.f)*O.Y)/Denom;P=O+D*T;}
        if(T>0)RequestWalkTo(P);return;
    }
    const FVector Point=GetActorTransform().InverseTransformPosition(Origin+Direction*((GetActorLocation().Z-Origin.Z)/Direction.Z));
    // This first room has an unobstructed rectangular aisle. Keep walking behind all seats.
    Destination={FMath::Clamp(Point.X,-420.f,420.f),FMath::Clamp(Point.Y,340.f,850.f),0};PendingSeat=-1;
    Status(TEXT("Walking. Select machine 1 or 3 to take a seat."));
}
void ASocialCasinoPrototype::RequestSeat(int32 I)
{
    if(bPlaying||Seat>=0)return;
    if(!Avatar || I<0 || I >= (bUseVenue?VenueSeatTransforms.Num():4))return;
    if(Occupied(I)){Status(TEXT("That seat is occupied by a local NPC. Choose a different cabinet."));return;}
    if(bUseVenue && !RequestWalkTo(Approach(I)))return;
    PendingSeat=I;Destination=bUseVenue?(Waypoints.IsValidIndex(0)?Waypoints[0]:Approach(I)):Approach(I);Status(TEXT("Walking to your machine..."));
}
void ASocialCasinoPrototype::Play()
{
    if(Seat<0){Status(TEXT("Sit at machine 1 or 3 first."));return;}
    if(bPlaying||!Lobby.IsValid())return;
    bPlaying=true;Controls->SetVisibility(ESlateVisibility::Collapsed);Lobby->OpenSocialSlot(this);
}
void ASocialCasinoPrototype::ReturnFromGame()
{
    bPlaying=false;if(Controls)Controls->SetVisibility(ESlateVisibility::Visible);
    Status(TEXT("Still seated. Play again, or stand up to explore."));
}
void ASocialCasinoPrototype::Stand()
{
    if(bPlaying)return;
    bGroupBonusFocus=false;
    if(Seat<0){PendingSeat=-1;Waypoints.Reset();WaypointIndex=0;if(Avatar)Destination=Avatar->GetRelativeLocation();return;}
    Avatar->SetRelativeLocation(Approach(Seat));Destination=Avatar->GetRelativeLocation();Seat=-1;PendingSeat=-1;
    if(PlayerSeatedVisual){PlayerSeatedVisual->SetVisibility(false,true);if(PlayerPose)PlayerPose->SetVisibility(true,true);}
    if(StandingLegs) StandingLegs->SetVisibility(true,true);if(SittingLegs) SittingLegs->SetVisibility(false,true);Status(TEXT("Tap the floor to walk, or choose another machine."));
}
void ASocialCasinoPrototype::Leave(){if(bPlaying)return;if(Lobby.IsValid())Lobby->CloseSocialRoom();Destroy();}
void ASocialCasinoPrototype::EndPlay(const EEndPlayReason::Type Reason){if(SpinDuelRun)SpinDuelRun->Destroy();if(Controls)Controls->RemoveFromParent();if(VenueStream){VenueStream->SetShouldBeLoaded(false);VenueStream->SetShouldBeVisible(false);VenueStream->SetIsRequestingUnloadAndRemoval(true);}Super::EndPlay(Reason);}
void ASocialCasinoPrototype::Tick(float Dt)
{
    Super::Tick(Dt);
    if(SpinDuelRun)return;
    if(!Avatar)
    {
        if(bUseVenue && VenueStream)
        {
            VenueLoadSeconds+=Dt;
            if(VenueStream->IsLevelVisible())InitializeVenue(VenueStream->GetLoadedLevel());
            else if(VenueLoadSeconds>30)Status(TEXT("Venue loading is taking longer than expected. You can leave and try again."));
        }
        return;
    }
    if(bMenuEventEntry) { bMenuEventEntry=false; GoToSpinDuel(); }
    if(Seat<0)
    {
        FVector Pos=Avatar->GetRelativeLocation();const FVector Delta=Destination-Pos;
        if(Delta.Size()>3)
        {
            const FVector Step=Delta.GetClampedToMaxSize(220.f*Dt);Avatar->SetRelativeLocation(Pos+Step);
            Avatar->SetRelativeRotation(FRotator(0,Delta.Rotation().Yaw+90,0));WalkPhase+=Dt*9;
            if(StandingLegs) StandingLegs->SetRelativeRotation(FRotator(0,0,FMath::Sin(WalkPhase)*8));
        }
        else if(bUseVenue && WaypointIndex+1<Waypoints.Num())
        {
            ++WaypointIndex;Destination=Waypoints[WaypointIndex];
        }
        else if(PendingSeat>=0)
        {
            Seat=PendingSeat;PendingSeat=-1;if(StandingLegs) StandingLegs->SetRelativeRotation(FRotator::ZeroRotator);
            Avatar->SetRelativeLocation(SeatPosition(Seat)+FVector(0,0,-8));Avatar->SetRelativeRotation(FRotator(0,SeatYaw(Seat),0));
            Waypoints.Reset();WaypointIndex=0;
            if(PlayerSeatedVisual){PlayerSeatedVisual->SetVisibility(true,true);PlayerSeatedVisual->SetPosition(.5f,false);if(PlayerPose)PlayerPose->SetVisibility(false,true);}

            if(StandingLegs) StandingLegs->SetVisibility(false,true);if(SittingLegs) SittingLegs->SetVisibility(true,true);Status(TEXT("Seated. Press PLAY to open Sugar Lust. Neighbours are local NPCs."));
            if(bUseVenue && Lobby.IsValid())Play();
        }
    }
    if(PlayerPose && (Seat<0 || !PlayerSeatedVisual)) ApplyPersonPose(PlayerPose,Seat>=0,Seat<0 && FVector::Dist(Avatar->GetRelativeLocation(),Destination)>3 ? FMath::Sin(WalkPhase)*22.f : 0.f);
    // Editor-only visual review has no game-world component tick; evaluate explicitly there.
    if(!GetWorld()->IsGameWorld()) for(USkeletalMeshComponent* Visual : SeatedVisuals)
    {
        Visual->TickAnimation(Dt,false);Visual->RefreshBoneTransforms();
    }
    FVector Desired=Seat<0?FVector(-850,1050,1050):FVector(-450,650,390);
    FVector Focus=Seat<0?FVector(0,300,70):FVector(40,180,115);
    if(bUseVenue)
    {
        if(bPlaying && bGroupBonusFocus){Desired=FVector(0,850,470);Focus=FVector(0,-280,50);}
        else if(bPlaying && Seat>=0){BankCamera(Seat,Desired,Focus);}
        else if(Seat>=0){const FVector P=SeatPosition(Seat);Desired=P+FRotator(0,SeatYaw(Seat),0).RotateVector(FVector(-140,360,260));Focus=P+FRotator(0,SeatYaw(Seat),0).RotateVector(FVector(0,-100,100));}
        else{const FVector P=Avatar->GetRelativeLocation();Desired=FVector(P.X*.35f,FMath::Clamp(P.Y+600.f,650.f,1750.f),470);Focus=FVector(P.X*.4f,P.Y-250,100);}
    }
    if(bUseVenue){Desired.X=FMath::Clamp(Desired.X,-1050.f,1050.f);Desired.Y=FMath::Clamp(Desired.Y,-440.f,1720.f);Desired.Z=FMath::Min(Desired.Z,420.f);}
    const float DesiredFOV=bUseVenue && bPlaying && !bGroupBonusFocus ? 80.f : (bUseVenue?65.f:60.f);
    Camera->FieldOfView=FMath::FInterpTo(Camera->FieldOfView,DesiredFOV,Dt,4.f);
    Camera->SetRelativeLocation(FMath::VInterpTo(Camera->GetRelativeLocation(),Desired,Dt,3));Camera->SetRelativeRotation(FMath::RInterpTo(Camera->GetRelativeRotation(),(Focus-Camera->GetRelativeLocation()).Rotation(),Dt,3));
    if(bUseVenue && Seat<0 && !bPlaying){const float D=FVector::Dist2D(Avatar->GetRelativeLocation(),FVector(0,1550,0));if(D>120)bDuelPadArmed=true;if(bDuelPadArmed && D<65){bDuelPadArmed=false;OpenSpinDuel();}}
}

void ASocialCasinoPrototype::ApplyPersonPose(UPoseableMeshComponent* Mesh,bool bSeated,float Stride)
{
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

FVector ASocialCasinoPrototype::AvatarPosition() const { return Avatar->GetRelativeLocation(); }
FVector ASocialCasinoPrototype::Approach(int32 I) const
{
    FVector P=SeatPosition(I)+FRotator(0,SeatYaw(I),0).RotateVector(FVector(0,90,0));
    if(bUseVenue)P.Z=FCasinoFloorRoute::Height(P.Y);
    return P;
}
bool ASocialCasinoPrototype::RequestWalkTo(FVector Point)
{
    if(!bUseVenue || !Avatar || Seat>=0 || bPlaying)return false;
    TArray<FVector> Path;
    if(!VenueRoute.Plan(Avatar->GetRelativeLocation(),Point,Path)){Status(TEXT("That spot is blocked or outside the walking area. Choose a clear aisle."));return false;}
    Waypoints=MoveTemp(Path);WaypointIndex=0;Destination=Waypoints[0];PendingSeat=-1;Status(TEXT("Walking. Tap a free cabinet to sit and play Sugar Lust."));return true;
}
void ASocialCasinoPrototype::BuildVenuePreview()
{
    bUseVenue=true;InitializeVenue(GetWorld()->PersistentLevel);
}
bool ASocialCasinoPrototype::InitializeVenue(ULevel* Level)
{
    if(Avatar)return true;if(!Level)return false;
    TArray<AActor*> Machines;
    for(AActor* A:Level->Actors)if(IsValid(A)&&A->ActorHasTag(TEXT("LHC_VenueMachine")))Machines.Add(A);
    Machines.Sort([](const AActor& A,const AActor& B){return A.Tags.Last().ToString()<B.Tags.Last().ToString();});
    if(Machines.IsEmpty()){Status(TEXT("This venue has no configured machines."));return false;}
    VenueRoute.Blockers.Reset();VenueMachines.Reset();VenueSeatTransforms.Reset();
    for(AActor* A:Machines)
    {
        const FVector Center=GetActorTransform().InverseTransformPosition(A->GetActorLocation());
        USceneComponent* SeatMarker=nullptr;
        for(AActor* Marker:Level->Actors)
            if(IsValid(Marker)&&Marker->ActorHasTag(TEXT("LHC_VenueSeat"))&&Marker->Tags.Last()==A->Tags.Last()){SeatMarker=Marker->GetRootComponent();break;}
        if(!SeatMarker)continue;
        const FTransform Local=SeatMarker->GetComponentTransform().GetRelativeTransform(GetActorTransform());
        VenueMachines.Add(A);VenueSeatTransforms.Add(Local);
    }
    for(AActor* A:Level->Actors)
    {
        if(!IsValid(A)||!A->ActorHasTag(TEXT("LHC_WalkBlocker")))continue;
        FVector Origin,Extent;A->GetActorBounds(false,Origin,Extent);
        Origin=GetActorTransform().InverseTransformPosition(Origin);
        // World AABBs deliberately give angled furniture conservative pedestrian clearance.
        VenueRoute.Blockers.Add({FVector2D(Origin.X,Origin.Y),FVector2D(Extent.X+25,Extent.Y+25),0});
    }
    if(VenueMachines.IsEmpty())return false;
    for(int32 I:{1,4})if(VenueSeatTransforms.IsValidIndex(I))
    {
        auto* Body=MakeAvatar(SeatPosition(I)+FVector(0,0,-8),FLinearColor::White,true);Body->SetRelativeRotation(FRotator(0,SeatYaw(I),0));
    }
    Part(RootComponent,{0,1550,2},{140,140,4},FLinearColor(.35f,.02f,.5f));Label(TEXT("SPIN DUEL PORTAL"),FVector(-115,1550,145),24);
    Avatar=MakeAvatar(FVector(0,1450,0),FLinearColor::White,false);Destination=Avatar->GetRelativeLocation();
    Camera->SetRelativeLocation(FVector(0,1750,470));Camera->SetRelativeRotation((FVector(0,850,100)-Camera->GetRelativeLocation()).Rotation());Camera->FieldOfView=65;
    Status(TEXT("Tap a clear aisle to walk. Tap a cabinet to sit and open Sugar Lust. Stage access is through the center."));return true;
}

TArray<int32> ASocialCasinoPrototype::BankMembers(int32 Index) const
{
    TArray<int32> Result;
    if(!VenueSeatTransforms.IsValidIndex(Index))return Result;
    const FVector P=SeatPosition(Index);const float Yaw=SeatYaw(Index);
    for(int32 I=0;I<VenueSeatTransforms.Num();++I)
    {
        const FVector Q=SeatPosition(I);
        if(FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,SeatYaw(I)))>1.f)continue;
        if(FVector::Dist2D(P,Q)>320.f)continue;
        if(FMath::Abs(Yaw)<1.f && (FMath::Abs(P.Y-Q.Y)>20.f || (P.X<0)!=(Q.X<0)))continue;
        Result.Add(I);
    }
    return Result;
}
void ASocialCasinoPrototype::BankCamera(int32 Index,FVector& Position,FVector& Target) const
{
    const TArray<int32> Members=BankMembers(Index);FVector Center=FVector::ZeroVector;
    for(int32 I:Members)Center+=SeatPosition(I);
    Center=Members.IsEmpty()?SeatPosition(Index):Center/Members.Num();
    const float Side=-1.f;const FRotator Facing(0,SeatYaw(Index),0);
    Position=Center+Facing.RotateVector(FVector(Side*350.f,350.f,420.f));
    Position.X=FMath::Clamp(Position.X,-1050.f,1050.f);Position.Y=FMath::Clamp(Position.Y,-440.f,1720.f);
    const FVector Interest=Center+Facing.RotateVector(FVector(0,-40.f,100.f));
    FRotator Look=(Interest-Position).Rotation();
    // Place the bank in the left 37% of the viewport, beside the game panel.
    Look.Yaw+=FMath::RadiansToDegrees(FMath::Atan(.50f*FMath::Tan(FMath::DegreesToRadians(80.f*.5f))));
    // Keep the chosen player clear of the panel even at either end of a bank.
    const FVector PlayerInterest=SeatPosition(Index)+FVector(0,0,110);
    const float TanHalfFOV=FMath::Tan(FMath::DegreesToRadians(40.f));
    for(int32 Pass=0;Pass<4;++Pass)
    {
        const FVector View=Look.UnrotateVector(PlayerInterest-Position);
        const float U=.5f+View.Y/(2.f*FMath::Max(View.X,1.f)*TanHalfFOV);
        const float Wanted=FMath::Clamp(U,.16f,.34f);
        Look.Yaw+=FMath::RadiansToDegrees(FMath::Atan2(View.Y,View.X)-FMath::Atan((Wanted-.5f)*2.f*TanHalfFOV));
    }
    Target=Position+Look.Vector()*1000.f;
}





bool ASocialCasinoPrototype::IsSelectedNeonNights() const
{
    return VenueMachines.IsValidIndex(Seat) && IsValid(VenueMachines[Seat]) && VenueMachines[Seat]->ActorHasTag(TEXT("LHC_Cabinet_NeonNights"));
}

 bool ASocialCasinoPrototype::IsSelectedMegaPot() const { return VenueMachines.IsValidIndex(Seat) && IsValid(VenueMachines[Seat]) && VenueMachines[Seat]->ActorHasTag(TEXT("LHC_Cabinet_MegaPot")); }

UStaticMesh* ASocialCasinoPrototype::GetSelectedCabinetMesh() const
{
 if(!VenueMachines.IsValidIndex(Seat) || !IsValid(VenueMachines[Seat]))return nullptr;
 auto* Mesh=VenueMachines[Seat]->FindComponentByClass<UStaticMeshComponent>();return Mesh?Mesh->GetStaticMesh():nullptr;
}

void ASocialCasinoPrototype::GoToSpinDuel(){if(SpinDuelRun||bPlaying)return;if(Seat>=0)Stand();bDuelPadArmed=true;RequestWalkTo(FVector(0,1550,0));}
void ASocialCasinoPrototype::OpenSpinDuel(){if(SpinDuelRun||bPlaying||Seat>=0)return;PendingSeat=-1;Waypoints.Reset();if(Avatar)Destination=Avatar->GetRelativeLocation();SpinDuelRun=GetWorld()->SpawnActor<ASpinDuelRun>(FVector(60000,0,0),FRotator::ZeroRotator);if(SpinDuelRun){if(Controls)Controls->SetVisibility(ESlateVisibility::Collapsed);SpinDuelRun->Configure(this);}}
void ASocialCasinoPrototype::SetVenueVisible(bool Visible){SetActorHiddenInGame(!Visible);if(VenueStream)VenueStream->SetShouldBeVisible(Visible);}
void ASocialCasinoPrototype::ResumeFromSpinDuel(){SpinDuelRun=nullptr;bDuelPadArmed=false;SetVenueVisible(true);if(Avatar){Avatar->SetRelativeLocation(FVector(0,1450,0));Destination=Avatar->GetRelativeLocation();}if(Controls)Controls->SetVisibility(ESlateVisibility::Visible);if(auto* PC=GetWorld()->GetFirstPlayerController())PC->SetViewTarget(this);Status(TEXT("Welcome back. Spin Duel practice does not change casino balances."));}
