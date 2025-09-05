#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyEnums.h"
#include "EnemySpawner.generated.h"

class UMapGridSubsystem;
class USpawnPointPolicy;
class AEnemyPawn;
class UEnemyGoalPolicy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyEvent, AEnemyPawn*, Enemy);

USTRUCT()
struct FPendingWave
{
	GENERATED_BODY()
	float   Time = 0.f;
	FString Type;    // del JSON
	FString Symbol;  // A/Q/R/T
	int32   Count = 1;
};

UCLASS()
class BATTLECITY3D_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

	// Eventos para GameMode/HUD
	UPROPERTY(BlueprintAssignable, Category = "Spawn|Events")
	FOnEnemyEvent OnEnemySpawned;
	UPROPERTY(BlueprintAssignable, Category = "Spawn|Events")
	FOnEnemyEvent OnEnemyDestroyedEvt;

	// Consulta de conteos
	UFUNCTION(BlueprintCallable, Category = "Spawn") int32 GetPlannedCount() const { return EnemiesPlanned; }
	UFUNCTION(BlueprintCallable, Category = "Spawn") int32 GetSpawnedCount() const { return EnemiesSpawned; }
	UFUNCTION(BlueprintCallable, Category = "Spawn") int32 GetAliveCount() const { return AliveCount; }

	// === AI|Goals: configuración común y acceso desde políticas ===
	UPROPERTY(EditAnywhere, Category = "AI|Goals")
	int32 AdvantageHighThreshold = 4;

	UPROPERTY(EditAnywhere, Category = "AI|Goals")
	int32 AdvantageLowThreshold = 2;

	// Selector de clase (dropdown)
	UPROPERTY(EditAnywhere, Category = "AI|Goals")
	TSubclassOf<UEnemyGoalPolicy> GoalPolicyClass;

	// Selector + instancia de SpawnPointPolicy
	UPROPERTY(EditAnywhere, Category = "Spawns|Policy")
	TSubclassOf<USpawnPointPolicy> SpawnPointPolicyClass;

	UPROPERTY(EditAnywhere, Instanced, Category = "Spawns|Policy")
	USpawnPointPolicy* SpawnPointPolicy = nullptr;

	// Overrides de clase
	UPROPERTY(EditAnywhere, Category = "Spawn|Clases") TSubclassOf<AEnemyPawn> EnemyClassDefault;
	UPROPERTY(EditAnywhere, Category = "Spawn|Clases") TSubclassOf<AEnemyPawn> EnemyClass_Basic;
	UPROPERTY(EditAnywhere, Category = "Spawn|Clases") TSubclassOf<AEnemyPawn> EnemyClass_Fast;
	UPROPERTY(EditAnywhere, Category = "Spawn|Clases") TSubclassOf<AEnemyPawn> EnemyClass_Power;
	UPROPERTY(EditAnywhere, Category = "Spawn|Clases") TSubclassOf<AEnemyPawn> EnemyClass_Armored;
	UPROPERTY(EditAnywhere, Category = "Spawn|Clases", meta = (DisplayName = "Overrides por Tipo (JSON Type)"))
	TMap<FName, TSubclassOf<AEnemyPawn>> CustomTypeToClass;
	UPROPERTY(EditAnywhere, Category = "Spawn|Clases", meta = (DisplayName = "Overrides por Símbolo"))
	TMap<FName, TSubclassOf<AEnemyPawn>> SymbolToClass;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "1"))
	int32 MaxAlive = 4;

	// En public:
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bAIDebug = false;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	const TArray<TWeakObjectPtr<AEnemyPawn>>& GetLivingEnemies() const { return LivingEnemies; }
	void ApplyGoalToAll(EEnemyGoal NewGoal);

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() UMapGridSubsystem* Grid = nullptr;
	TArray<FPendingWave> Pending;

	// Conteos
	int32 EnemiesPlanned = 0;
	int32 EnemiesSpawned = 0;
	int32 AliveCount = 0;

	// Cache de puntos por símbolo
	TMap<FString, TArray<FVector>> SpawnLocs;
	// Unión de todos los puntos de spawn
	TArray<FVector> AllSpawnLocs;
	// Lista de vivos (para políticas)
	TArray<TWeakObjectPtr<AEnemyPawn>> LivingEnemies;

	void ScheduleWaves();
	void OnWaveDue(FPendingWave Wave);
	void SpawnOne(const FString& Type, const FString& Symbol);
	bool IsSpawnPointFree(const FVector& Location) const;

	TSubclassOf<AEnemyPawn> ResolveClassFor(const FString& Type, const FString& Symbol) const;
	static FName ToKey(const FString& S);
	static class ABattleGameMode* GetBattleGM(UWorld* World);

	UFUNCTION() void HandleActorDestroyed(AActor* Dead);
	UEnemyGoalPolicy* GoalPolicy = nullptr;

	UFUNCTION(Exec)
	void bc_ai_debug(int32 v);
};
