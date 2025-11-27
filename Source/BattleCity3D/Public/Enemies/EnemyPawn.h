#pragma once
#include "CoreMinimal.h"
#include "Common/BattleTankPawn.h" // Heredar de la base
#include "EnemyEnums.h"
#include "EnemyPawn.generated.h"

class UEnemyMovementComponent;

// Heredamos de ABattleTankPawn
UCLASS()
class BATTLECITY3D_API AEnemyPawn : public ABattleTankPawn
{
	GENERATED_BODY()

public:
	AEnemyPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void ApplyHit(int32 Dmg);

	// Configuración específica de Enemigo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") int32 HitPoints = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") float FireInterval = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") EEnemyType EnemyType = EEnemyType::Basic;

	UPROPERTY() TWeakObjectPtr<class AEnemySpawner> SpawnerRef;

	// AI Brain
	FVector GetAITargetWorld() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI|Goal")
	EEnemyGoal Goal = EEnemyGoal::HuntBase;

	UPROPERTY(EditAnywhere, Category = "Enemy|AI|Brain") float AIReplanInterval = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Enemy|AI|Brain") float AICommitmentTimeAfterTurn = 1.0f;

	// Funciones auxiliares para la AI
	bool IsFireReady() const;
	void Fire();

	// Métodos para que el MovementComponent controle este Pawn
	// Nota: ApplyRawMoveInput es redundante si accedemos directo a RawMoveInput de la base, 
	// pero lo mantenemos por compatibilidad con tu componente.
	void ApplyRawMoveInput(const FVector2D& In) { RawMoveInput = In; }

	// tuning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Tuning") bool bAcceptTypeFromSpawner = true;
	UFUNCTION(BlueprintCallable, Category = "Enemy|Tuning") void SetTypeFromSpawner(EEnemyType InType);
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Componente de cerebro
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components")
	UEnemyMovementComponent* MovementComp = nullptr;

private:
	double NextAIDecisionTime = 0.0;
	FTimerHandle FireTimer;

	// Lógica de cerebro (decidir A DONDE ir)
	void UpdateAI(float DT);

	// Helpers de AI
	bool IsBlockedAhead(const FVector& Center, bool bAxisX, int Dir, float T) const;
	bool CanMoveTowards(const FVector2D& Axis) const;
	void ChooseTurn();

	// Variables de GridAxisLock necesarias para UpdateAI
	enum class EAxisLock : uint8 { None, X, Y };
	EAxisLock AxisLock = EAxisLock::None;
	double    LockUntilTime = 0.0;
	// LookAheadTiles, AlignEpsilonFactor, TieDeadbandFactor...
	// Si estas las usas SOLO para decidir (no para mover), mantenlas aquí. 
	// Si se usan en movimiento físico, muévelas a la Base. 
	// Asumo que son para la IA de decisión, así que las dejo:
	UPROPERTY(EditAnywhere, Category = "Enemy|AI") float MinLockTime = 0.30f;
	UPROPERTY(EditAnywhere, Category = "Enemy|AI") float AlignEpsilonFactor = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Enemy|AI") float TieDeadbandFactor = 0.10f;
	UPROPERTY(EditAnywhere, Category = "Enemy|AI") int32 LookAheadTiles = 1;
};