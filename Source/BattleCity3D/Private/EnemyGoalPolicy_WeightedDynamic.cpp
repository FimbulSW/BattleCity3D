#include "EnemyGoalPolicy_WeightedDynamic.h"
#include "EnemySpawner.h"
#include "EnemyPawn.h"
#include "Kismet/GameplayStatics.h"

float UEnemyGoalPolicy_WeightedDynamic::ComputePBase(int32 AliveCount) const
{
    float p = WeightedHuntPlayerChance;
    if (OwnerSpawner.IsValid())
    {
        const int32 High = OwnerSpawner->AdvantageHighThreshold;
        const int32 Low = OwnerSpawner->AdvantageLowThreshold;
        if (AliveCount >= High)      p -= WeightedBiasByAdvantage;
        else if (AliveCount <= Low)  p += WeightedBiasByAdvantage;
    }
    return FMath::Clamp(p, 0.f, 1.f);
}

EEnemyGoal UEnemyGoalPolicy_WeightedDynamic::DecideGoalOnSpawn_Implementation(const FEnemySpawnContext& Ctx) const
{
    float p = ComputePBase(Ctx.AliveCount);
    p += FMath::FRandRange(-WeightedRandomJitter, WeightedRandomJitter);
    p = FMath::Clamp(p, 0.f, 1.f);
    return (FMath::FRand() < p) ? EEnemyGoal::HuntPlayer : EEnemyGoal::HuntBase;
}

void UEnemyGoalPolicy_WeightedDynamic::TickGoal(float DeltaSeconds)
{
    if (!OwnerSpawner.IsValid()) return;
    TimeAcc += DeltaSeconds;
    if (TimeAcc < WeightedReevaluationPeriod) return;
    TimeAcc = 0.f;

    // calcula prob base con sesgo
    const int32 Alive = OwnerSpawner->GetAliveCount();
    float pBase = ComputePBase(Alive);

    // aplica a enemigos vivos
    auto& Living = OwnerSpawner->GetLivingEnemies();
    if (bPerEnemy)
    {
        for (const TWeakObjectPtr<AEnemyPawn>& W : Living)
        {
            if (AEnemyPawn* E = W.Get())
            {
                float p = FMath::Clamp(pBase + FMath::FRandRange(-WeightedRandomJitter, WeightedRandomJitter), 0.f, 1.f);
                E->Goal = (FMath::FRand() < p) ? EEnemyGoal::HuntPlayer : EEnemyGoal::HuntBase;
            }
        }
    }
    else
    {
        float p = FMath::Clamp(pBase + FMath::FRandRange(-WeightedRandomJitter, WeightedRandomJitter), 0.f, 1.f);
        const EEnemyGoal NewGlobal = (FMath::FRand() < p) ? EEnemyGoal::HuntPlayer : EEnemyGoal::HuntBase;
        OwnerSpawner->ApplyGoalToAll(NewGlobal);
    }
}
