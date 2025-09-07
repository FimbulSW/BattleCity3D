#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Enemies/EnemyEnums.h"
#include "EnemyGoalPolicy.generated.h"

class AEnemySpawner;
class AEnemyPawn;

USTRUCT(BlueprintType)
struct FEnemySpawnContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString Type;
    UPROPERTY(BlueprintReadOnly) FString Symbol;
    UPROPERTY(BlueprintReadOnly) int32   AliveCount = 0;
    UPROPERTY(BlueprintReadOnly) int32   EnemiesSpawned = 0;
    UPROPERTY(BlueprintReadOnly) int32   MaxAlive = 0;
    UPROPERTY(BlueprintReadOnly) bool    bHasBase = false;
    UPROPERTY(BlueprintReadOnly) bool    bHasPlayer = false;
};

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class BATTLECITY3D_API UEnemyGoalPolicy : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent) void Initialize(AEnemySpawner* Owner);
    virtual void Initialize_Implementation(AEnemySpawner* Owner);

    UFUNCTION(BlueprintNativeEvent) EEnemyGoal DecideGoalOnSpawn(const FEnemySpawnContext& Ctx) const;
    virtual EEnemyGoal DecideGoalOnSpawn_Implementation(const FEnemySpawnContext& Ctx) const;

    UFUNCTION(BlueprintNativeEvent) void OnEnemySpawned(AEnemyPawn* Enemy);
    virtual void OnEnemySpawned_Implementation(AEnemyPawn* Enemy);

    UFUNCTION(BlueprintNativeEvent) void OnEnemyDestroyed(AEnemyPawn* Enemy);
    virtual void OnEnemyDestroyed_Implementation(AEnemyPawn* Enemy);

    UFUNCTION(BlueprintNativeEvent) void OnAliveCountChanged(int32 AliveCount);
    virtual void OnAliveCountChanged_Implementation(int32 AliveCount);

    virtual void TickGoal(float DeltaSeconds);

protected:
    UPROPERTY(Transient) TWeakObjectPtr<AEnemySpawner> OwnerSpawner;
    float TimeAcc = 0.f;
};
