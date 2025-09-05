#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BattleGameMode.generated.h"

class AMapGenerator;
class AEnemySpawner;
class AEnemyPawn;
class ABattleBase;

UCLASS()
class BATTLECITY3D_API ABattleGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ABattleGameMode();

	// --- Reglas ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules")
	int32 PlayerLives = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules")
	TSubclassOf<ABattleBase> BaseClass;

	// Estado consulta
	UFUNCTION(BlueprintCallable, Category = "Rules") int32 GetEnemiesPlanned() const { return EnemiesPlanned; }
	UFUNCTION(BlueprintCallable, Category = "Rules") int32 GetEnemiesSpawned() const { return EnemiesSpawned; }
	UFUNCTION(BlueprintCallable, Category = "Rules") int32 GetEnemiesAlive()   const { return EnemiesAlive; }

	// Eventos de fin (para UI)
	UFUNCTION(BlueprintImplementableEvent, Category = "Rules") void OnVictory();
	UFUNCTION(BlueprintImplementableEvent, Category = "Rules") void OnDefeat();

	// Llamado por la base
	void HandleBaseDestroyed();

protected:
	virtual void BeginPlay() override;

private:
	// Refs runtime
	UPROPERTY() AMapGenerator* MapGen = nullptr;
	UPROPERTY() AEnemySpawner* Spawner = nullptr;
	UPROPERTY() ABattleBase* BaseActor = nullptr;

	// Conteos
	int32 EnemiesPlanned = 0;
	int32 EnemiesSpawned = 0;
	int32 EnemiesAlive = 0;
	bool bGameOver = false;

	FTimerHandle PlayerSearchTimer;

	// Setup
	void FindActors();
	void SpawnBaseIfAny();
	void BindSpawner();
	void WatchPlayerPawn();

	// Callbacks
	UFUNCTION() void OnEnemySpawned(AEnemyPawn* E);
	UFUNCTION() void OnEnemyDestroyed(AEnemyPawn* E);
	UFUNCTION() void OnPlayerPawnDestroyed(AActor* Dead);

	// Flow
	void CheckVictory();
	void RespawnPlayer();
	void Defeat();
	void Victory();
};
