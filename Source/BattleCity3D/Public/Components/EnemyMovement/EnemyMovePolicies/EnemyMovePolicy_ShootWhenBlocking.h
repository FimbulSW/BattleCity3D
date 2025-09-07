#pragma once
#include "CoreMinimal.h"
#include "EnemyMovePolicy.h"
#include "Components/GridPathFollow/GridPathTypes.h"
#include "EnemyMovePolicy_ShootWhenBlocking.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType)
class BATTLECITY3D_API UEnemyMovePolicy_ShootWhenBlocking : public UEnemyMovePolicy
{
	GENERATED_BODY()
public:
	// Heurística simple: disparamos si hay ladrillo justo al frente.
	// (En fase 4 podemos evaluar desvío vs. brick-cost con GridPathManager).
	UPROPERTY(EditAnywhere, Category = "Brick")
	FGridCostProfile Cost = { 1.f, 10.f, 1e9f };

	virtual void ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out) override;

private:
	bool IsFrontBrick(const FMoveContext& Ctx) const;
};
