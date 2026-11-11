#include "LustHouseLounge.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALustHouseLounge::ALustHouseLounge()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("LoungeRoot"));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("LoungeCamera"));
    Camera->SetupAttachment(RootComponent);
    Camera->FieldOfView = 68.f;
    Camera->bConstrainAspectRatio = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cabinet(TEXT("/Game/Art/Lounge/ModernPaintedCabinet/SM_ModernCabinetSingle.SM_ModernCabinetSingle"));
    SugarCabinetMesh = Cabinet.Object;
    Cube = Mesh.Object;
    BaseMaterial = Mat.Object;
}

void ALustHouseLounge::Box(FVector Position, FVector Size, FLinearColor Color)
{
    auto* Mesh = NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Mesh);
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetStaticMesh(Cube);
    Mesh->SetRelativeLocation(Position);
    Mesh->SetRelativeScale3D(Size / 100.f);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCastShadow(false);
    if (BaseMaterial)
    {
        auto* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        Material->SetVectorParameterValue(TEXT("Color"), Color);
        Mesh->SetMaterial(0, Material);
    }
    Mesh->RegisterComponent();
}

void ALustHouseLounge::Sign(const FString& Text, FVector Position, float Size, FColor Color)
{
    auto* Label = NewObject<UTextRenderComponent>(this);
    AddInstanceComponent(Label);
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(Position);
    Label->SetRelativeRotation(FRotator(0, 180, 0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(Size);
    Label->SetTextRenderColor(Color);
    Label->SetText(FText::FromString(Text));
    Label->RegisterComponent();
}

void ALustHouseLounge::BeginPlay()
{
    Super::BeginPlay();
    const FLinearColor Dark(.035f,.018f,.065f), Pink(1,.025f,.32f), Cyan(.02f,.65f,1), Gold(.85f,.48f,.08f);
    Box({400,0,-15}, {1500,1500,30}, Dark);
    Box({1100,0,240}, {30,1500,510}, Dark);
    Box({400,-750,240}, {1500,30,510}, Dark);
    Box({400,750,240}, {1500,30,510}, Dark);
    Box({960,0,25}, {270,610,50}, FLinearColor(.12f,.025f,.16f));
    Box({810,0,48}, {20,610,8}, Pink);
    for (int32 Side : {-1,1})
    {
        Box({1080,Side*335.f,230}, {16,14,450}, Pink);
        Box({400,Side*720.f,8}, {1400,10,8}, Cyan);
        for (int32 I=0; I<4; ++I)
        {
            Box({float(I*300),Side*690.f,230}, {40,40,460}, FLinearColor(.12f,.04f,.17f));
            Box({float(I*300)-23,Side*690.f,230}, {5,44,420}, Pink);
        }
    }
    Box({1080,0,395}, {10,670,8}, Gold);
    Sign(TEXT("LUST HOUSE"), {1060,0,335}, 60, FColor(255,100,180));
    Sign(TEXT("OPENING NIGHT"), {1055,0,265}, 23, FColor(100,220,255));
    Sign(TEXT("THE STAGE"), {1045,0,140}, 22, FColor(255,200,100));
    for (int32 I=0; I<2; ++I)
    {
        const float Y = -430.f + I*160.f;
        if (SugarCabinetMesh)
        {
            auto* Cabinet = NewObject<UStaticMeshComponent>(this,
                *FString::Printf(TEXT("SugarLustCabinet%d"), I + 1));
            AddInstanceComponent(Cabinet);
            Cabinet->SetupAttachment(RootComponent);
            Cabinet->SetStaticMesh(SugarCabinetMesh);
            // Fit the supplied cabinet to 220 cm and seat its lowest vertex on the floor.
            const FBox Bounds = SugarCabinetMesh->GetBoundingBox();
            const float Scale = 220.f / FMath::Max(Bounds.GetSize().Z, 1.f);
            Cabinet->SetRelativeScale3D(FVector(Scale));
            Cabinet->SetRelativeRotation(FRotator(0, 90, 0));
            const FVector Center = Bounds.GetCenter();
            Cabinet->SetRelativeLocation(FVector(570,Y,0) - FRotator(0,90,0).RotateVector(FVector(Center.X,Center.Y,Bounds.Min.Z)*Scale));
            Cabinet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Cabinet->SetCastShadow(false);
            Cabinet->RegisterComponent();
            // Preserve the supplied material; the lounge sign identifies the game.
            Sign(TEXT("SUGAR LUST"), {570,Y,245}, 18, FColor(255,175,220));
        }
        else
        {
            // Keep the lounge usable if the imported asset is unavailable.
            Box({570,Y,110}, {100,130,220}, Dark);
            Sign(TEXT("SUGAR LUST"), {510,Y,150}, 18, FColor(255,175,220));
        }
    }
    Box({565,400,135}, {110,180,270}, Dark);
    Box({505,400,150}, {10,160,160}, Cyan);
    Box({497,400,150}, {5,145,140}, FLinearColor(.035f,.04f,.1f));
    Sign(TEXT("LUSTVEND"), {490,400,245}, 22, FColor(100,230,255));
    Sign(TEXT("REWARDS"), {490,400,155}, 18, FColor(255,180,220));
    Box({500,400,45}, {12,115,30}, FLinearColor::Black);
    for (int32 I=0; I<3; ++I)
    {
        auto* Light = NewObject<UPointLightComponent>(this);
        AddInstanceComponent(Light);
        Light->SetupAttachment(RootComponent);
        Light->SetRelativeLocation(FVector(250+I*300,0,380));
        Light->SetLightColor(I==1 ? FLinearColor(1,.24f,.5f) : FLinearColor(.4f,.55f,1));
        Light->SetIntensity(22000.f);
        Light->SetAttenuationRadius(1200.f);
        Light->SetCastShadows(false);
        Light->RegisterComponent();
    }
    SetView(0);
    Camera->SetRelativeLocation(TargetLocation);
    Camera->SetRelativeRotation(TargetRotation);
}

void ALustHouseLounge::SetView(int32 View)
{
    FVector Focus(850,0,190);
    TargetLocation = FVector(-260,0,240);
    if (View==1) { TargetLocation=FVector(80,-470,220); Focus=FVector(550,-350,145); }
    if (View==2) { TargetLocation=FVector(80,470,220); Focus=FVector(550,400,145); }
    if (View==3) { TargetLocation=FVector(350,0,220); Focus=FVector(1050,0,240); }
    TargetRotation=(Focus-TargetLocation).Rotation();
}

void ALustHouseLounge::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Camera->SetRelativeLocation(FMath::VInterpTo(Camera->GetRelativeLocation(),TargetLocation,DeltaSeconds,4.f));
    Camera->SetRelativeRotation(FMath::RInterpTo(Camera->GetRelativeRotation(),TargetRotation,DeltaSeconds,4.f));
}
