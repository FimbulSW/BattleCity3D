#pragma once
#include "CoreMinimal.h"
#include "EnemyMovePolicy.h" // tu base con FMoveContext/FMoveDecision
#include "GridPathTypes.h"
#include "EnemyMovePolicy_PathFollow.generated.h"

class UGridPathManager;
class UGridPathFollowComponent;
class UMapGridSubsystem;
class UEnemyMovementComponent;

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType)
class BATTLECITY3D_API UEnemyMovePolicy_PathFollow : public UEnemyMovePolicy
{
	GENERATED_BODY()
public:
	// === Tuning ===
	UPROPERTY(EditAnywhere, Category = "Path") float ReplanInterval = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Path") int32 HorizonSteps = 6;
	UPROPERTY(EditAnywhere, Category = "Path") float ReplanDistCells = 2.f;
	UPROPERTY(EditAnywhere, Category = "Cost") FGridCostProfile Cost = { 1.f, 10.f, 1e9f };

	// Objetivo: true=jugador (parcial), false=base (completa). Usa Ctx.TargetWorld.
	UPROPERTY(EditAnywhere, Category = "Goal") bool bTargetIsPlayer = true;

public:
	virtual void Initialize(UEnemyMovementComponent* InOwner) override;
	virtual void ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out) override;

private:
	UPROPERTY(Transient) TWeakObjectPtr<UEnemyMovementComponent> EnemyMoveOwner;
	UPROPERTY(Transient) TObjectPtr<UGridPathFollowComponent> Follower = nullptr;
	UPROPERTY(Transient) TObjectPtr<UGridPathManager> PathMgr = nullptr;
	UPROPERTY(Transient) TObjectPtr<UMapGridSubsystem> Grid = nullptr;

	float LastReplanTime = -1000.f;
	FIntPoint LastGoalCell = FIntPoint(-999, -999);

	void EnsureDeps(const FMoveContext& Ctx);
	bool TryWorldToGrid(const FVector& World, FIntPoint& OutCell) const;
	void MaybeReplan(const FMoveContext& Ctx, const FIntPoint& StartCell, const FIntPoint& GoalCell);
	static FVector2D ToCardinalInput(const FVector& DirWorld);
};