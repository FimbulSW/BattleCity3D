#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EnemyEnums.h"
#include "EnemyPawn.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UMapGridSubsystem;
class AProjectile;

class UEnemyMovementComponent;
class UMapGridSubsystem;


UCLASS()
class BATTLECITY3D_API AEnemyPawn : public APawn
{
	GENERATED_BODY()

public:
	AEnemyPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override {}

	void ApplyHit(int32 Dmg);

	// --- Stats base ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") float MoveSpeed = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") float AccelRate = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") float DecelRate = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") float AlignRate = 2200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") float StopMargin = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") int32 HitPoints = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") float FireInterval = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy") EEnemyType EnemyType = EEnemyType::Basic;

	UPROPERTY() TWeakObjectPtr<class AEnemySpawner> SpawnerRef;

	// Objetivo según la meta actual (con fallbacks)
	FVector GetAITargetWorld() const;


	// Permite que el BP decida si tomar el tipo/estadísticas del spawner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Tuning")
	bool bAcceptTypeFromSpawner = true;

	// Setter que respeta la bandera anterior
	UFUNCTION(BlueprintCallable, Category = "Enemy|Tuning")
	void SetTypeFromSpawner(EEnemyType InType);

	// Daño genérico de Unreal
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	// === AI Goal ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI|Goal")
	EEnemyGoal Goal = EEnemyGoal::HuntBase;

	// === AI|Move Lock (config) ===
	UPROPERTY(EditAnywhere, Category = "Enemy|AI|Move")
	float MinLockTime = 0.30f;

	UPROPERTY(EditAnywhere, Category = "Enemy|AI|Move", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float AlignEpsilonFactor = 0.25f; // del tamaño del tile

	UPROPERTY(EditAnywhere, Category = "Enemy|AI|Move", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TieDeadbandFactor = 0.10f;  // compara |dx|-|dy|

	UPROPERTY(EditAnywhere, Category = "Enemy|AI|Move", meta = (ClampMin = "1", ClampMax = "3"))
	int32 LookAheadTiles = 1; // 1..2 recomendable

	// Expuesto para el MovementComponent
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI") FVector2D GetFacingDir() const { return FacingDir; }
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI") void      ApplyRawMoveInput(const FVector2D& In) { RawMoveInput = In; }
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI") bool      IsFireReady() const; // si ya lo tenías, omite
	bool bPreferShootWhenFrontBrick = true;

	void Fire();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy") USceneComponent* Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy") UStaticMeshComponent* Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy") USceneComponent* Muzzle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components")
	UEnemyMovementComponent* MovementComp = nullptr;


private:
	UPROPERTY() UMapGridSubsystem* Grid = nullptr;

	// Movimiento tipo tanque con subgrid
	FVector2D RawMoveInput = FVector2D(1, 0);
	FVector2D Velocity = FVector2D::ZeroVector;
	FVector2D FacingDir = FVector2D(1, 0);

	// AI
	double LastDecisionTime = -1e9;
	double DecisionCooldown = 0.20; // s entre decisiones de giro

	FTimerHandle FireTimer;

	// --- Lógica ---
	void UpdateAI(float DT);
	void UpdateMovement(float DT);

	// utilidades
	bool IsBlockedAhead(const FVector& Center, bool bAxisX, int Dir, float T) const;
	bool CanMoveTowards(const FVector2D& Axis) const;
	void ChooseTurn();

	enum class EAxisLock : uint8 { None, X, Y };
	EAxisLock AxisLock = EAxisLock::None;
	double    LockUntilTime = 0.0;

	enum class ECardinalLOS : uint8 { NotCardinal, Clear, Brick, Steel };

	// Devuelve el estado de la línea cardinal entre From y To.
	// Usa AlignEpsilonFactor * Tile como tolerancia para “estar alineado”.
	ECardinalLOS CheckCardinalLineToTarget(const FVector& From, const FVector& To, FVector* OutFirstHitWorld = nullptr) const;

};
