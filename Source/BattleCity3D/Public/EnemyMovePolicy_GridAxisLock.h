#pragma once
#include "EnemyMovePolicy.h"
#include "EnemyMovePolicy_GridAxisLock.generated.h"

// Heurística de eje + stop&shoot si Brick frontal
UCLASS(Blueprintable, EditInlineNew)
class BATTLECITY3D_API UEnemyMovePolicy_GridAxisLock : public UEnemyMovePolicy
{
    GENERATED_BODY()
public:
    virtual void ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out) override;
};
