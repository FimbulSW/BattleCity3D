


#include "EnemySpawnPointPolicy_FarFromPlayer.h"

void UEnemySpawnPointPolicy_FarFromPlayer::BuildCandidateOrder_Implementation(const TArray<FVector>& In, const FSpawnPointContext& Ctx, TArray<int32>& Out) const
{
    TArray<TPair<float, int32>> Tmp; Tmp.Reserve(In.Num());
    for (int32 i = 0; i < In.Num(); ++i)
    {
        const float d = FVector::Dist2D(In[i], Ctx.PlayerLoc);
        Tmp.Emplace(d, i);
    }
    Tmp.Sort([](auto& A, auto& B) { return A.Key > B.Key; }); // más lejos primero

    Out.Reset(); Out.Reserve(Tmp.Num());
    for (auto& P : Tmp)
        if (P.Key >= MinDistance * MinDistance) Out.Add(P.Value);
    // fallback: si ninguno cumple, devuelve todos lejos -> cerca
    if (Out.Num() == 0) for (auto& P : Tmp) Out.Add(P.Value);
}
