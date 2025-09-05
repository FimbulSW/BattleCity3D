#pragma once
#include "EnemyGoalPolicy.h"
#include "EnemyGoalPolicy_WeightedDynamic.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class BATTLECITY3D_API UEnemyGoalPolicy_WeightedDynamic : public UEnemyGoalPolicy
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals (Weighted)")
    float WeightedHuntPlayerChance = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals (Weighted)")
    float WeightedBiasByAdvantage = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals (Weighted)")
    float WeightedRandomJitter = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals (Weighted)", meta = (ClampMin = "0.2"))
    float WeightedReevaluationPeriod = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals (Weighted)")
    bool bPerEnemy = true;

    virtual EEnemyGoal DecideGoalOnSpawn_Implementation(const FEnemySpawnContext& Ctx) const override;
    virtual void TickGoal(float DeltaSeconds) override;

private:
    float ComputePBase(int32 AliveCount) const;
};
