#include "Enemies/EnemyGoalPolicies/EnemyGoalPolicy.h"
#include "Spawner/EnemySpawner.h"

void UEnemyGoalPolicy::Initialize_Implementation(AEnemySpawner* Owner)
{
    OwnerSpawner = Owner;
    TimeAcc = 0.f;
}

EEnemyGoal UEnemyGoalPolicy::DecideGoalOnSpawn_Implementation(const FEnemySpawnContext& /*Ctx*/) const
{
    return EEnemyGoal::HuntBase;
}

void UEnemyGoalPolicy::OnEnemySpawned_Implementation(AEnemyPawn* /*Enemy*/) {}
void UEnemyGoalPolicy::OnEnemyDestroyed_Implementation(AEnemyPawn* /*Enemy*/) {}
void UEnemyGoalPolicy::OnAliveCountChanged_Implementation(int32 /*AliveCount*/) {}
void UEnemyGoalPolicy::TickGoal(float /*DeltaSeconds*/) {}
