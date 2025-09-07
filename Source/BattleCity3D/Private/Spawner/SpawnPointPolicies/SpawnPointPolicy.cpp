


#include "Spawner/SpawnPointPolicies/SpawnPointPolicy.h"

void USpawnPointPolicy::BuildCandidateOrder(const TArray<FVector>& In, const FSpawnPointContext& Ctx, TArray<int32>& Out) const
{
    Out.Reset(); Out.Reserve(In.Num());
    for (int32 i = 0; i < In.Num(); ++i) Out.Add(i);
    for (int32 i = 0; i < Out.Num(); ++i) { const int32 j = FMath::RandRange(i, Out.Num() - 1); Out.Swap(i, j); }
}



