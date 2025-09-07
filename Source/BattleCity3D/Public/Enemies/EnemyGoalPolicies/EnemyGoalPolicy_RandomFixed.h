#pragma once
#include "EnemyGoalPolicy.h"
#include "EnemyGoalPolicy_RandomFixed.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class BATTLECITY3D_API UEnemyGoalPolicy_RandomFixed : public UEnemyGoalPolicy
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals")
    float RandomHuntPlayerChance = 0.5f;

    virtual EEnemyGoal DecideGoalOnSpawn_Implementation(const FEnemySpawnContext& Ctx) const override
    {
        return (FMath::FRand() < RandomHuntPlayerChance) ? EEnemyGoal::HuntPlayer : EEnemyGoal::HuntBase;
    }
};
