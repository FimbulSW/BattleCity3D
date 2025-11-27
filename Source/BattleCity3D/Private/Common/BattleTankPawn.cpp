#include "Common/BattleTankPawn.h"
#include "Map/MapGridSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ABattleTankPawn::ABattleTankPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Root);
    // Configuración de colisión común
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionObjectType(ECC_WorldDynamic);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

    Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
    Muzzle->SetupAttachment(Body);
    Muzzle->SetRelativeLocation(FVector(50.f, 0.f, 0.f));
}

void ABattleTankPawn::BeginPlay()
{
    Super::BeginPlay();
    Grid = GetGameInstance()->GetSubsystem<UMapGridSubsystem>();

    // SNAP INICIAL COMÚN
    if (Grid)
    {
        FVector P = GetActorLocation();
        P = Grid->SnapWorldToSubgrid(P, true);
        SetActorLocation(P);
    }
}

void ABattleTankPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Por defecto ejecutamos movimiento. Las clases hijas pueden llamar a Super::Tick()
    UpdateTankMovement(DeltaTime);
}

void ABattleTankPawn::UpdateTankMovement(float DT)
{
    // 0. Delay de Giro
    if (CurrentTurnDelay > 0.f)
    {
        CurrentTurnDelay -= DT;
        Velocity = FVector2D::ZeroVector;
        if (!RawMoveInput.IsNearlyZero())
        {
            FacingDir = RawMoveInput;
            const float Yaw = (FacingDir.X > 0 ? 0.f : (FacingDir.X < 0 ? 180.f : (FacingDir.Y > 0 ? 90.f : -90.f)));
            SetActorRotation(FRotator(0.f, Yaw, 0.f));
        }
        return;
    }

    // 1. Lógica de Giro
    if (!RawMoveInput.IsNearlyZero())
    {
        FacingDir = RawMoveInput;
        const float Yaw = (FacingDir.X > 0 ? 0.f : (FacingDir.X < 0 ? 180.f : (FacingDir.Y > 0 ? 90.f : -90.f)));
        FRotator TargetRot(0.f, Yaw, 0.f);

        FVector CurrentFwd = GetActorForwardVector();
        FVector2D CurrentFwd2D(CurrentFwd.X, CurrentFwd.Y);

        if (FVector2D::DotProduct(CurrentFwd2D, RawMoveInput) < 0.95f)
        {
            SetActorRotation(TargetRot);
            Velocity = FVector2D::ZeroVector;
            CurrentTurnDelay = TurnDelay;
            return;
        }
        SetActorRotation(TargetRot);
    }

    // 2. Velocidad
    float TargetSpeed = RawMoveInput.IsNearlyZero() ? 0.f : MoveSpeed;
    float CurrentRate = RawMoveInput.IsNearlyZero() ? DecelRate : AccelRate;
    Velocity = FMath::Vector2DInterpTo(Velocity, RawMoveInput * TargetSpeed, DT, CurrentRate);

    if (Velocity.IsNearlyZero() || !Grid) return;

    // 3. Colisión y Grid (Bigotes)
    const float TileSize = Grid->GetTileSize();
    const float TankExtent = TileSize * 0.96f;
    FVector CurrentPos = GetActorLocation();
    FVector MoveDelta = FVector(Velocity.X, Velocity.Y, 0.f) * DT;
    FVector FuturePos = CurrentPos + MoveDelta;

    bool bBlocked = false;
    bool bMovingX = FMath::Abs(Velocity.X) > FMath::Abs(Velocity.Y);

    FVector DebugP1, DebugP2;
    const float SideMargin = 2.0f;

    if (bMovingX)
    {
        float FrontX = (Velocity.X > 0) ? (FuturePos.X + TankExtent) : (FuturePos.X - TankExtent);
        FVector P1(FrontX, FuturePos.Y + TankExtent - SideMargin, CurrentPos.Z);
        FVector P2(FrontX, FuturePos.Y - TankExtent + SideMargin, CurrentPos.Z);
        DebugP1 = P1; DebugP2 = P2;

        if (Grid->IsPointBlocked(P1) || Grid->IsPointBlocked(P2)) bBlocked = true;
        else
        {
            float Step = Grid->GetSubStep();
            if (Step <= 1.f) Step = TileSize;
            float IdealY = FMath::RoundToFloat(FuturePos.Y / Step) * Step;
            FuturePos.Y = FMath::FInterpConstantTo(FuturePos.Y, IdealY, DT, AlignRate);
        }
    }
    else
    {
        float FrontY = (Velocity.Y > 0) ? (FuturePos.Y + TankExtent) : (FuturePos.Y - TankExtent);
        FVector P1(FuturePos.X + TankExtent - SideMargin, FrontY, CurrentPos.Z);
        FVector P2(FuturePos.X - TankExtent + SideMargin, FrontY, CurrentPos.Z);
        DebugP1 = P1; DebugP2 = P2;

        if (Grid->IsPointBlocked(P1) || Grid->IsPointBlocked(P2)) bBlocked = true;
        else
        {
            float Step = Grid->GetSubStep();
            if (Step <= 1.f) Step = TileSize;
            float IdealX = FMath::RoundToFloat(FuturePos.X / Step) * Step;
            FuturePos.X = FMath::FInterpConstantTo(FuturePos.X, IdealX, DT, AlignRate);
        }
    }

    // Debug Visual
    static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("bc.collision.debug"));
    if (CVar && CVar->GetInt() > 0 && GetWorld())
    {
        bool bP1Hit = Grid->IsPointBlocked(DebugP1);
        bool bP2Hit = Grid->IsPointBlocked(DebugP2);
        DrawDebugPoint(GetWorld(), DebugP1, 12.f, bP1Hit ? FColor::Red : FColor::Green, false, 0.03f);
        DrawDebugPoint(GetWorld(), DebugP2, 12.f, bP2Hit ? FColor::Red : FColor::Green, false, 0.03f);
    }

    if (!bBlocked) SetActorLocation(FuturePos);
    else Velocity = FVector2D::ZeroVector;
}