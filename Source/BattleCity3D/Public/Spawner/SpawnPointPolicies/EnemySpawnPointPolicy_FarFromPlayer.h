#pragma once
#include "SpawnPointPolicy.h"
#include "EnemySpawnPointPolicy_FarFromPlayer.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class BATTLECITY3D_API UEnemySpawnPointPolicy_FarFromPlayer : public USpawnPointPolicy
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Spawn|Policy")
    float MinDistance = 600.f;

    virtual void BuildCandidateOrder(const TArray<FVector>& In, const FSpawnPointContext& Ctx, TArray<int32>& Out) const override;
};