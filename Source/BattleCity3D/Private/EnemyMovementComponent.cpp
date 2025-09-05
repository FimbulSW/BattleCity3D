#include "EnemyMovementComponent.h"
#include "EnemyMovePolicy.h"
#include "EnemyPawn.h"
#include "MapGridSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EnemyMovePolicy_GridAxisLock.h"

UEnemyMovementComponent::UEnemyMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UEnemyMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    CachedPawn = Cast<AEnemyPawn>(GetOwner());
    CachedGrid = (GetWorld() && GetWorld()->GetGameInstance())
        ? GetWorld()->GetGameInstance()->GetSubsystem<UMapGridSubsystem>()
        : nullptr;
    EnsurePolicy();
}

void UEnemyMovementComponent::EnsurePolicy()
{
    if (!MovePolicy && MovePolicyClass)
        MovePolicy = NewObject<UEnemyMovePolicy>(this, MovePolicyClass, NAME_None, RF_Transactional);

    if (!MovePolicy)
    {
        TSubclassOf<UEnemyMovePolicy> Cls =
            MovePolicyClass ? MovePolicyClass : (TSubclassOf<UEnemyMovePolicy>)(UEnemyMovePolicy_GridAxisLock::StaticClass());
        MovePolicy = NewObject<UEnemyMovePolicy>(this, Cls, NAME_None, RF_Transactional);
    }

    if (MovePolicy) MovePolicy->Initialize(this);
}

float UEnemyMovementComponent::GetTileSizeSafe() const
{
    if (CachedGrid.IsValid()) return CachedGrid->GetTileSize();
    return 200.f;
}

UMapGridSubsystem* UEnemyMovementComponent::GetGrid() const
{
    return CachedGrid.Get();
}

bool UEnemyMovementComponent::IsAheadBlocked(bool bAxisX, int Dir, float DistWorld) const
{
    if (!CachedGrid.IsValid() || !CachedPawn.IsValid()) return false;
    const float Tile = GetTileSizeSafe();
    const FVector Start = CachedGrid->SnapWorldToSubgrid(CachedPawn->GetActorLocation(), true);
    const int Steps = FMath::Max(1, FMath::FloorToInt(DistWorld / Tile));

    int32 SX, SY;
    if (!CachedGrid->WorldToGrid(Start, SX, SY)) return false;

    for (int s = 1; s <= Steps; ++s)
    {
        const int X = bAxisX ? (SX + Dir * s) : SX;
        const int Y = bAxisX ? SY : (SY + Dir * s);
        const EObstacleType O = CachedGrid->GetObstacleAtGrid(X, Y);
        if (O == EObstacleType::Brick || O == EObstacleType::Steel) return true;
    }
    return false;
}

uint8 UEnemyMovementComponent::QueryFrontObstacle(float MaxDistanceWorld, FVector* OutHitWorld) const
{
    if (!CachedGrid.IsValid() || !CachedPawn.IsValid()) return 0;
    const float Tile = GetTileSizeSafe();

    // Facing del Pawn
    const FVector2D Facing = CachedPawn->GetFacingDir();
    const bool bAxisX = (FMath::Abs(Facing.X) >= FMath::Abs(Facing.Y));
    const int  Dir = bAxisX ? ((Facing.X >= 0.f) ? +1 : -1)
        : ((Facing.Y >= 0.f) ? +1 : -1);

    const FVector Start = CachedGrid->SnapWorldToSubgrid(CachedPawn->GetActorLocation(), true);
    int32 SX, SY; if (!CachedGrid->WorldToGrid(Start, SX, SY)) return 0;

    const int Steps = FMath::Max(1, FMath::FloorToInt(MaxDistanceWorld / Tile));
    for (int s = 1; s <= Steps; ++s)
    {
        const int X = bAxisX ? (SX + Dir * s) : SX;
        const int Y = bAxisX ? SY : (SY + Dir * s);
        const EObstacleType O = CachedGrid->GetObstacleAtGrid(X, Y);
        if (O == EObstacleType::Brick || O == EObstacleType::Steel)
        {
            if (OutHitWorld) *OutHitWorld = CachedGrid->GridToWorld(X, Y, Tile * 0.5f);
            return (O == EObstacleType::Brick) ? 1 : 2;
        }
    }
    return 0;
}

uint8 UEnemyMovementComponent::CheckCardinalLineToTarget(const FVector& From, const FVector& To, FVector* OutFirstHitWorld) const
{
    if (!CachedGrid.IsValid()) return 1; // Clear by default
    const float Tile = GetTileSizeSafe();
    const float eps = AlignEpsilonFactor * Tile;

    const bool CardinalX = FMath::Abs(From.Y - To.Y) <= eps;
    const bool CardinalY = FMath::Abs(From.X - To.X) <= eps;
    if (!CardinalX && !CardinalY) return 0; // NotCardinal

    int32 SX, SY, EX, EY;
    if (!CachedGrid->WorldToGrid(From, SX, SY) || !CachedGrid->WorldToGrid(To, EX, EY))
        return 0;

    if (CardinalX)
    {
        const int Dir = (EX >= SX) ? 1 : -1;
        for (int x = SX + Dir; x != EX + Dir; x += Dir)
        {
            const EObstacleType O = CachedGrid->GetObstacleAtGrid(x, SY);
            if (O == EObstacleType::Brick || O == EObstacleType::Steel)
            {
                if (OutFirstHitWorld) *OutFirstHitWorld = CachedGrid->GridToWorld(x, SY, Tile * 0.5f);
                return (O == EObstacleType::Brick) ? 2 : 3; // Brick / Steel
            }
        }
    }
    else
    {
        const int Dir = (EY >= SY) ? 1 : -1;
        for (int y = SY + Dir; y != EY + Dir; y += Dir)
        {
            const EObstacleType O = CachedGrid->GetObstacleAtGrid(SX, y);
            if (O == EObstacleType::Brick || O == EObstacleType::Steel)
            {
                if (OutFirstHitWorld) *OutFirstHitWorld = CachedGrid->GridToWorld(SX, y, Tile * 0.5f);
                return (O == EObstacleType::Brick) ? 2 : 3;
            }
        }
    }
    return 1; // Clear
}

bool UEnemyMovementComponent::IsFireReady() const
{
    if (!CachedPawn.IsValid()) return false;
    // Usa el timer del Pawn; si no existe en tu Pawn, expón un método
    const UWorld* W = GetWorld(); if (!W) return false;
    // Permitimos que el Pawn implemente el check si ya lo tenías
    return CachedPawn->IsFireReady();
}

void UEnemyMovementComponent::ApplyDecision(const FMoveDecision& D)
{
    if (!CachedPawn.IsValid()) return;

    // Lock
    if (D.LockAxis != EMoveLockAxis::None && D.LockTime > 0.f)
    {
        AxisLock = D.LockAxis;
        LockUntilTime = UGameplayStatics::GetTimeSeconds(this) + D.LockTime;
    }

    // Movimiento (el Pawn sigue moviendo como antes con RawMoveInput)
    CachedPawn->ApplyRawMoveInput(D.RawMoveInput);

    // Disparo frontal si procede
    if (D.bRequestFrontShot && IsFireReady())
    {
        CachedPawn->Fire();

        // Si quieres re-fasear el timer, hazlo en el Pawn (o añade un Exec aquí)
    }
}

void UEnemyMovementComponent::TickComponent(float DT, ELevelTick levelTick, FActorComponentTickFunction* tickFunction)
{
    Super::TickComponent(DT, levelTick, tickFunction);

    if (!MovePolicy || !CachedPawn.IsValid()) return;

    // Construir contexto
    FMoveContext Ctx;
    Ctx.Location = CachedPawn->GetActorLocation();
    Ctx.FacingDir = CachedPawn->GetFacingDir();
    Ctx.TargetWorld = CachedPawn->GetAITargetWorld();

    const float Tile = GetTileSizeSafe();
    Ctx.TileSize = Tile;
    Ctx.AlignEpsilon = AlignEpsilonFactor * Tile;
    Ctx.TieDeadband = TieDeadbandFactor * Tile;
    Ctx.LookAheadTiles = LookAheadTiles;

    Ctx.bFireReady = IsFireReady();

    Ctx.IsAheadBlocked = [this, Tile](bool bAxisX, int Dir, float DistWorld) { return IsAheadBlocked(bAxisX, Dir, DistWorld); };
    Ctx.FrontObstacle = [this](float D, FVector* Out) { return QueryFrontObstacle(D, Out); };
    Ctx.CardinalLineToTarget = [this](const FVector& A, const FVector& B, FVector* Out) { return CheckCardinalLineToTarget(A, B, Out); };

    // Pedir decisión a la policy
    FMoveDecision Dec;
    MovePolicy->ComputeMove(Ctx, Dec);

    // Si hay lock activo que no expiró y no hay bloqueo real, mantener axis
    const double Now = UGameplayStatics::GetTimeSeconds(this);
    if (AxisLock != EMoveLockAxis::None && Now < LockUntilTime)
    {
        const bool bAxisX = (AxisLock == EMoveLockAxis::X);
        const int Dir = bAxisX ? ((Ctx.FacingDir.X >= 0) ? +1 : -1)
            : ((Ctx.FacingDir.Y >= 0) ? +1 : -1);
        if (!IsAheadBlocked(bAxisX, Dir, Tile * LookAheadTiles))
        {
            // forzar mantener
            Dec.RawMoveInput = bAxisX ? FVector2D(Dir, 0) : FVector2D(0, Dir);
            Dec.LockAxis = AxisLock;
            Dec.LockTime = LockUntilTime - Now;
        }
        else
        {
            AxisLock = EMoveLockAxis::None;
        }
    }

    ApplyDecision(Dec);
}

#if WITH_EDITOR
void UEnemyMovementComponent::PostEditChangeProperty(FPropertyChangedEvent& E)
{
    Super::PostEditChangeProperty(E);
    static const FName NAME_MovePolicyClass(TEXT("MovePolicyClass"));
    if (E.Property && E.Property->GetFName() == NAME_MovePolicyClass)
    {
        EnsurePolicy();
    }
}
#endif
