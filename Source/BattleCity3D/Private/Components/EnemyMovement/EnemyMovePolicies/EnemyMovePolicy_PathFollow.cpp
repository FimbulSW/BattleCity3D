#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy_PathFollow.h"
#include "Components/GridPathFollow/GridPathFollowComponent.h"
#include "Components/GridPathFollow/GridPathManager.h"
#include "Map/MapGridSubsystem.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/EnemyMovement/EnemyMovementComponent.h"

void UEnemyMovePolicy_PathFollow::Initialize(UEnemyMovementComponent* InOwner)
{
	Super::Initialize(InOwner);
	EnemyMoveOwner = MakeWeakObjectPtr(InOwner);
}

void UEnemyMovePolicy_PathFollow::EnsureDeps(const FMoveContext& Ctx)
{
	// Grid desde el MovementComponent
	if (!Grid && EnemyMoveOwner.IsValid())
	{
		Grid = EnemyMoveOwner->GetGrid();
	}

	// PathMgr
	if (!PathMgr && EnemyMoveOwner.IsValid())
	{
		if (UWorld* W = EnemyMoveOwner->GetWorld())
		{
			if (UGameInstance* GI = W->GetGameInstance())
			{
				PathMgr = GI->GetSubsystem<UGridPathManager>();
			}
		}
	}

	// Follower en el Owner actor
	if (!Follower && EnemyMoveOwner.IsValid())
	{
		if (AActor* OwnerActor = EnemyMoveOwner->GetOwner())
		{
			Follower = OwnerActor->FindComponentByClass<UGridPathFollowComponent>();
			if (!Follower)
			{
				Follower = NewObject<UGridPathFollowComponent>(OwnerActor, UGridPathFollowComponent::StaticClass(), NAME_None, RF_Transactional);
				Follower->RegisterComponent();
			}
		}
	}
}


bool UEnemyMovePolicy_PathFollow::TryWorldToGrid(const FVector& World, FIntPoint& OutCell) const
{
	if (!Grid) return false;
	int32 X = 0, Y = 0;
	if (!Grid->WorldToGrid(World, X, Y)) return false;
	OutCell = FIntPoint(X, Y);
	return true;
}

void UEnemyMovePolicy_PathFollow::MaybeReplan(const FMoveContext& Ctx, const FIntPoint& StartCell, const FIntPoint& GoalCell)
{
	if (!PathMgr || !Grid) return;

	const float Now = (EnemyMoveOwner.IsValid() && EnemyMoveOwner->GetWorld())
		? EnemyMoveOwner->GetWorld()->GetTimeSeconds() : 0.f;

	const bool bGoalMoved = (FMath::Abs(GoalCell.X - LastGoalCell.X) + FMath::Abs(GoalCell.Y - LastGoalCell.Y)) >= ReplanDistCells;
	const bool bTime = (Now - LastReplanTime) >= ReplanInterval;

	if (!Follower || !Follower->HasPath() || bGoalMoved || bTime)
	{
		FGridPathRequest Req;
		Req.Grid = Grid;
		Req.Start = StartCell;
		Req.Goal = GoalCell;
		Req.Cost = Cost;
		Req.MaxSteps = bTargetIsPlayer ? FMath::Max(0, HorizonSteps) : 0;
		Req.bAllowPartial = true;

		FGridPathResult Res;
		if (PathMgr->ComputePath(Req, Res) && Res.bValid)
		{
			Follower->SetPath(Grid, Res);
			LastGoalCell = GoalCell;
			LastReplanTime = Now;
		}
	}
}

static int32 Sign01(float V) { return (V > 0.f) ? 1 : (V < 0.f) ? -1 : 0; }

FVector2D UEnemyMovePolicy_PathFollow::ToCardinalInput(const FVector& DirWorld)
{
	// DirWorld cardinal (1,0,0) o (0,1,0) según eje dominante
	const float ax = FMath::Abs(DirWorld.X);
	const float ay = FMath::Abs(DirWorld.Y);
	if (ax >= ay) return FVector2D((float)Sign01(DirWorld.X), 0.f);
	return FVector2D(0.f, (float)Sign01(DirWorld.Y));
}

void UEnemyMovePolicy_PathFollow::ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out)
{
	Out.DebugText = TEXT("PathFollow");
	EnsureDeps(Ctx);

	if (!Grid || !PathMgr || !Follower) return;

	// Celdas start/goal desde world
	FIntPoint StartCell, GoalCell;
	if (!TryWorldToGrid(Ctx.Location, StartCell)) return;
	if (!TryWorldToGrid(Ctx.TargetWorld, GoalCell)) return;

	// Replanificación (parcial si objetivo móvil)
	MaybeReplan(Ctx, StartCell, GoalCell);

	// Avance y dirección cardinal
	Follower->AdvanceIfReached(Ctx.Location, Ctx.TileSize, Ctx.AlignEpsilon);
	const FVector DesiredDirWorld = Follower->GetDesiredDirWorld(Ctx.Location, Ctx.TileSize);

	Out.RawMoveInput = ToCardinalInput(DesiredDirWorld);
	// LockAxis opcional (podemos dejar None; lo ajustaremos si quieres conservar axis-lock)
	Out.LockAxis = EMoveLockAxis::None;
	Out.LockTime = 0.f;
	// Disparo lo evalúa otra policy
}
