#pragma once
#include "CoreMinimal.h"
#include "EnemyMovePolicy.h"
#include "GridPathTypes.h"
#include "EnemyMovePolicy_WanderFar.generated.h"

class UMapGridSubsystem;
class UEnemyMovementComponent;
class AEnemyPawn;

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType)
class BATTLECITY3D_API UEnemyMovePolicy_WanderFar : public UEnemyMovePolicy
{
	GENERATED_BODY()
public:
	// Activa vagabundeo si distancia (en celdas) al objetivo > este umbral. Sólo se usa para HuntPlayer.
	UPROPERTY(EditAnywhere, Category = "Wander") float ActivateBeyondCells = 10.f;

	// Longitud de “tramo” deambulado (en celdas)
	UPROPERTY(EditAnywhere, Category = "Wander") int32 MinStrideCells = 2;
	UPROPERTY(EditAnywhere, Category = "Wander") int32 MaxStrideCells = 5;

	// Sesgo a acercarse al objetivo mientras deambula
	UPROPERTY(EditAnywhere, Category = "Wander") bool bBiasTowardTarget = true;
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0", ClampMax = "1.0")) float BiasWeight = 0.6f;

	// Coste para validar celdas “transitables” durante deambular (ladrillo como caro/impasable)
	UPROPERTY(EditAnywhere, Category = "Wander") FGridCostProfile Cost = { 1.f, 1e5f, 1e9f };

public:
	virtual void Initialize(UEnemyMovementComponent* InOwner) override;
	virtual void ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out) override;

private:
	TWeakObjectPtr<UEnemyMovementComponent> MoveOwner;
	TWeakObjectPtr<AEnemyPawn> Pawn;
	TWeakObjectPtr<UMapGridSubsystem> Grid;

	// Estado de tramo actual
	int32 StepsLeft = 0;
	FVector2D CurrentDir = FVector2D::ZeroVector;
	double NextRechooseTime = 0.0;

	bool IsHuntPlayer() const; // lee Goal del Pawn
	bool IsPassableAhead(const FIntPoint& From, const FVector2D& Dir) const;
	int  ScoreDir(const FIntPoint& From, const FVector2D& Dir, const FIntPoint& GoalCell) const;
	void ChooseNewDir(const FMoveContext& Ctx, const FIntPoint& FromCell, const FIntPoint& GoalCell);
};
