#include "Enemies/EnemyGoalPolicies/EnemyGoalPolicy_AdvantageBias.h"
#include "Enemies/EnemyPawn.h"
#include "Spawner/EnemySpawner.h"


void UEnemyGoalPolicy_AdvantageBias::Initialize_Implementation(AEnemySpawner* Owner)
{
    Super::Initialize_Implementation(Owner);
    CurrentGoal = DefaultGoal;
}

void UEnemyGoalPolicy_AdvantageBias::OnAliveCountChanged_Implementation(int32 AliveCount)
{
    if (!OwnerSpawner.IsValid()) return;
    EEnemyGoal NewGoal = CurrentGoal;

    if (AliveCount >= AdvantageHighThreshold)      NewGoal = EEnemyGoal::HuntBase;
    else if (AliveCount <= AdvantageLowThreshold)  NewGoal = EEnemyGoal::HuntPlayer;

    ApplyIfChanged(NewGoal);
}

void UEnemyGoalPolicy_AdvantageBias::ApplyIfChanged(EEnemyGoal NewGoal)
{
    if (!OwnerSpawner.IsValid() || NewGoal == CurrentGoal) return;
    CurrentGoal = NewGoal;
    OwnerSpawner->ApplyGoalToAll(CurrentGoal);
}
