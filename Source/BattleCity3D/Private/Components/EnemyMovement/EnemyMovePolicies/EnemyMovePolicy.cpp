


#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy.h"
#include "Map/MapGridSubsystem.h"
#include "Components/EnemyMovement/EnemyMovementComponent.h"

void UEnemyMovePolicy::Initialize(UEnemyMovementComponent* InOwner)
{
	{ Owner = MakeWeakObjectPtr(InOwner); }
}

bool UEnemyMovePolicy::BuildCandidateOrder(UMapGridSubsystem* Grid, TArray<FIntPoint>& Out)
{
    if (!Grid) return false;
    Grid->GetAllEnemySpawnCells(Out);
    if (Out.Num() == 0) return false;
    // Barajar para que el primero cambie en cada llamada
    for (int32 i = 0; i < Out.Num(); ++i) {
        const int32 j = FMath::RandRange(i, Out.Num() - 1);
        Out.Swap(i, j);
    }
    return true;
}
