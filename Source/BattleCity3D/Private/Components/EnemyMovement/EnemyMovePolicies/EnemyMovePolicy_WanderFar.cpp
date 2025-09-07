#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy_WanderFar.h"
#include "Components/EnemyMovement/EnemyMovementComponent.h"
#include "Map/MapGridSubsystem.h"
#include "Enemies/EnemyPawn.h"
#include "Kismet/GameplayStatics.h"

void UEnemyMovePolicy_WanderFar::Initialize(UEnemyMovementComponent* InOwner)
{
	Super::Initialize(InOwner);
	MoveOwner = MakeWeakObjectPtr(InOwner);
	if (InOwner) { 
		Pawn = MakeWeakObjectPtr(InOwner->GetPawn()); 
		Grid = MakeWeakObjectPtr(InOwner->GetGrid()); 
	}
}

bool UEnemyMovePolicy_WanderFar::IsHuntPlayer() const
{
	if (!Pawn.IsValid()) return true; // por seguridad, tratamos como player
	return (Pawn->Goal == EEnemyGoal::HuntPlayer);
}

bool UEnemyMovePolicy_WanderFar::IsPassableAhead(const FIntPoint& From, const FVector2D& Dir) const
{
	if (!Grid.IsValid()) return false;
	const FIntPoint N(From.X + Sign01(Dir.X), From.Y + Sign01(Dir.Y));
	return Grid->IsPassableCell(N, Cost);
}

int UEnemyMovePolicy_WanderFar::ScoreDir(const FIntPoint& From, const FVector2D& Dir, const FIntPoint& GoalCell) const
{
	if (!Grid.IsValid()) return -1000000;
	const FIntPoint N(From.X + Sign01(Dir.X), From.Y + Sign01(Dir.Y));
	if (!Grid->IsPassableCell(N, Cost)) return -1000000;

	// Base: aleatorio ligero
	int Score = FMath::RandRange(0, 10);

	// Sesgo: reducir distancia Manhattan al objetivo
	if (bBiasTowardTarget)
	{
		const int d0 = FMath::Abs(From.X - GoalCell.X) + FMath::Abs(From.Y - GoalCell.Y);
		const int d1 = FMath::Abs(N.X - GoalCell.X) + FMath::Abs(N.Y - GoalCell.Y);
		Score += (int)((d0 - d1) * 100 * BiasWeight); // acercar suma puntos
	}
	return Score;
}

void UEnemyMovePolicy_WanderFar::ChooseNewDir(const FMoveContext& Ctx, const FIntPoint& FromCell, const FIntPoint& GoalCell)
{
	TArray<FVector2D> Dirs = { FVector2D(1,0), FVector2D(-1,0), FVector2D(0,1), FVector2D(0,-1) };
	int BestScore = -1000000; FVector2D Best = FVector2D::ZeroVector;

	// Valora cada dirección
	for (const FVector2D& D : Dirs)
	{
		const int S = ScoreDir(FromCell, D, GoalCell);
		if (S > BestScore) { BestScore = S; Best = D; }
	}

	// Si todo inválido (encerrado), dejamos cero y policy no emitirá input
	CurrentDir = Best;
	StepsLeft = (BestScore <= -1000000) ? 0 : FMath::RandRange(MinStrideCells, MaxStrideCells);
	NextRechooseTime = UGameplayStatics::GetTimeSeconds(this) + 0.5; // cooldown ligero
}

void UEnemyMovePolicy_WanderFar::ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out)
{
	if (!Grid.IsValid() || !MoveOwner.IsValid()) return;

	// Sólo aplica para perseguir jugador
	if (!IsHuntPlayer()) return;

	// Distancia en celdas al objetivo
	int32 SX, SY, GX, GY;
	if (!Grid->WorldToGrid(Ctx.Location, SX, SY)) return;
	if (!Grid->WorldToGrid(Ctx.TargetWorld, GX, GY)) return;

	const int Manh = FMath::Abs(SX - GX) + FMath::Abs(SY - GY);
	if (Manh <= FMath::TruncToInt(ActivateBeyondCells)) return; // cerca: deja que PathFollow mande

	// Elegir/continuar tramo
	const double Now = UGameplayStatics::GetTimeSeconds(this);
	if (StepsLeft <= 0 || Now >= NextRechooseTime || !IsPassableAhead(FIntPoint(SX, SY), CurrentDir))
	{
		ChooseNewDir(Ctx, FIntPoint(SX, SY), FIntPoint(GX, GY));
	}

	if (StepsLeft > 0 && !CurrentDir.IsNearlyZero())
	{
		Out.RawMoveInput = FVector2D((float)Sign01(CurrentDir.X), (float)Sign01(CurrentDir.Y));
		Out.LockAxis = (FMath::Abs(Out.RawMoveInput.X) > 0.5f) ? EMoveLockAxis::X : EMoveLockAxis::Y;
		Out.LockTime = 0.25f; // pequeña ventana para evitar zig-zag
		Out.DebugText.Append(TEXT(" +WanderFar"));
		StepsLeft--;
	}
}
