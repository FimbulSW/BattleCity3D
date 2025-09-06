#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GridPathTypes.h"
#include "GridPathManager.generated.h"

// Manager sencillo para pathfinding sobre UMapGridSubsystem (cardinal).
UCLASS()
class BATTLECITY3D_API UGridPathManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override {}

	// Sin colas aún: resolución inmediata (podemos time-slicear en Fase 4).
	UFUNCTION(BlueprintCallable, Category = "GridPath")
	bool ComputePath(const FGridPathRequest& Req, FGridPathResult& OutResult) const;

private:
	// A* cardinal con heurística Manhattan; cae a Dijkstra si Manhattan=0.
	bool AStar_Internal(const FGridPathRequest& Req, FGridPathResult& OutResult) const;

	static int32 Heuristic_Manhattan(const FIntPoint& A, const FIntPoint& B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}
};
