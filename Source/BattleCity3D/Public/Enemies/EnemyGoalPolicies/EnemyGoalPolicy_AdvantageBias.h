#pragma once
#include "EnemyGoalPolicy.h"
#include "EnemyGoalPolicy_AdvantageBias.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class BATTLECITY3D_API UEnemyGoalPolicy_AdvantageBias : public UEnemyGoalPolicy
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals")
    int32 AdvantageHighThreshold = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals")
    int32 AdvantageLowThreshold = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Goals")
    EEnemyGoal DefaultGoal = EEnemyGoal::HuntBase;

    virtual void Initialize_Implementation(AEnemySpawner* Owner) override;
    virtual void OnAliveCountChanged_Implementation(int32 AliveCount) override;

private:
    EEnemyGoal CurrentGoal = EEnemyGoal::HuntBase;
    void ApplyIfChanged(EEnemyGoal NewGoal);
};
