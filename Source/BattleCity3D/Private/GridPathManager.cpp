#include "GridPathManager.h"
#include "Containers/Map.h"
#include "Containers/Queue.h"
#include "Algo/Reverse.h"
#include "MapGridSubsystem.h"

struct FAStarNode
{
	FIntPoint Cell;
	float G = 0.f; // coste desde Start
	float F = 0.f; // G + H
};

static float TileCost(const UMapGridSubsystem* Grid, const FIntPoint& C, const FGridCostProfile& Cost)
{
	return Grid ? Grid->GetTileCost(C, Cost) : Cost.ImpassableCost;
}

bool UGridPathManager::ComputePath(const FGridPathRequest& Req, FGridPathResult& OutResult) const
{
	OutResult = FGridPathResult{};
	if (!Req.Grid) return false;
	return AStar_Internal(Req, OutResult);
}

bool UGridPathManager::AStar_Internal(const FGridPathRequest& Req, FGridPathResult& OutResult) const
{
	UMapGridSubsystem* Grid = Req.Grid;
	if (!Grid) return false;

	const float StartCost = TileCost(Grid, Req.Start, Req.Cost);
	const float GoalCost = TileCost(Grid, Req.Goal, Req.Cost);
	const bool bGoalBlocked = GoalCost >= Req.Cost.ImpassableCost;

	TMap<FIntPoint, float> GScore;
	TMap<FIntPoint, float> FScore;
	TMap<FIntPoint, FIntPoint> CameFrom;
	TSet<FIntPoint> Closed;

	TArray<FIntPoint> Open;
	auto PushOpen = [&](const FIntPoint& C) { Open.Add(C); };
	auto PopBest = [&]() -> FIntPoint {
		int32 BestIdx = 0; float BestF = TNumericLimits<float>::Max();
		for (int32 i = 0; i < Open.Num(); ++i)
		{
			const float* FPtr = FScore.Find(Open[i]);
			const float Fv = FPtr ? *FPtr : TNumericLimits<float>::Max();
			if (Fv < BestF) { BestF = Fv; BestIdx = i; }
		}
		const FIntPoint Best = Open[BestIdx];
		Open.RemoveAtSwap(BestIdx, 1, false);
		return Best;
		};

	auto H = [&](const FIntPoint& C) { return (float)Heuristic_Manhattan(C, Req.Goal); };

	GScore.Add(Req.Start, 0.f);
	FScore.Add(Req.Start, H(Req.Start));
	PushOpen(Req.Start);

	int32 StepsEmitted = 0;
	bool bReached = false;
	FIntPoint ReachedCell = Req.Start;

	while (Open.Num() > 0)
	{
		const FIntPoint Current = PopBest();
		if (Current == Req.Goal && !bGoalBlocked)
		{
			bReached = true;
			ReachedCell = Current;
			break;
		}

		Closed.Add(Current);

		TArray<FIntPoint> Neigh;
		Grid->GetNeighbors4(Current, Neigh);

		for (const FIntPoint& N : Neigh)
		{
			if (Closed.Contains(N)) continue;

			const float MoveCost = TileCost(Grid, N, Req.Cost);
			if (MoveCost >= Req.Cost.ImpassableCost) continue;

			const float* GCurPtr = GScore.Find(Current);
			if (!GCurPtr) continue; // seguridad
			const float TentG = *GCurPtr + MoveCost;

			const float* PrevG = GScore.Find(N);
			if (!PrevG || TentG < *PrevG)
			{
				CameFrom.Add(N, Current);
				GScore.Add(N, TentG);
				FScore.Add(N, TentG + H(N));
				if (!Open.Contains(N)) PushOpen(N);
			}
		}

		// Horizonte parcial
		if (Req.MaxSteps > 0)
		{
			StepsEmitted++;
			if (StepsEmitted >= Req.MaxSteps)
			{
				float BestF = TNumericLimits<float>::Max();
				FIntPoint BestCell = Current;
				for (const auto& KVP : FScore)
				{
					const FIntPoint& Cell = KVP.Key;
					if (!Closed.Contains(Cell)) continue;
					// Asegura que hay cadena hacia Start
					if (Cell != Req.Start && !CameFrom.Contains(Cell)) continue;
					if (KVP.Value < BestF) { BestF = KVP.Value; BestCell = Cell; }
				}
				ReachedCell = BestCell;
				bReached = (BestCell == Req.Goal);
				break;
			}
		}
	}

	// === NUEVO: fallback parcial cuando NO se alcanzó Goal y NO hubo corte por horizonte ===
	if (!bReached)
	{
		if (!Req.bAllowPartial)
		{
			OutResult = FGridPathResult{};
			return false;
		}

		float BestF = TNumericLimits<float>::Max();
		FIntPoint BestCell = Req.Start;

		for (const auto& KVP : FScore)
		{
			const FIntPoint& Cell = KVP.Key;
			if (!Closed.Contains(Cell)) continue;
			// necesita cadena válida hasta Start
			if (Cell != Req.Start && !CameFrom.Contains(Cell)) continue;
			if (KVP.Value < BestF) { BestF = KVP.Value; BestCell = Cell; }
		}
		ReachedCell = BestCell;
	}

	// Reconstrucción
	auto Reconstruct = [&](const FIntPoint& End, TArray<FIntPoint>& OutPath, float& OutCost)
		{
			OutPath.Reset();
			OutPath.Add(End);
			float CostAcc = 0.f;

			FIntPoint Cur = End;
			while (Cur != Req.Start)
			{
				const FIntPoint* Prev = CameFrom.Find(Cur);
				if (!Prev) break;
				CostAcc += TileCost(Grid, Cur, Req.Cost);
				Cur = *Prev;
				OutPath.Add(Cur);
			}
			Algo::Reverse(OutPath);
			OutCost = CostAcc;
		};

	TArray<FIntPoint> Path;
	float CostSum = 0.f;
	Reconstruct(ReachedCell, Path, CostSum);

	// Válida sólo si hay al menos un paso real, salvo que Start==Goal
	if (Path.Num() <= 1 && Req.Start != ReachedCell)
	{
		OutResult = FGridPathResult{};
		return false;
	}

	OutResult.Cells = MoveTemp(Path);
	OutResult.TotalCost = CostSum;
	OutResult.bReachedGoal = (ReachedCell == Req.Goal);
	OutResult.bValid = true;
	return true;
}
