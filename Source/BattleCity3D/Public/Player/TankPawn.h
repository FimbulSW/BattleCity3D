#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "TankPawn.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UInputAction;
class UEnhancedInputComponent;
class UMapGridSubsystem;
class AProjectile;

UCLASS()
class BATTLECITY3D_API ATankPawn : public APawn
{
	GENERATED_BODY()
public:
	ATankPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank") USceneComponent* Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank") UStaticMeshComponent* Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank") USceneComponent* Muzzle;

	// Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float MoveSpeed = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float AccelRate = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float AccelRateIce = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float DecelRate = 16.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float DecelRateIce = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|GridLock") bool  bGridLock = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|GridLock") float AlignTolerance = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|GridLock") float AlignRate = 5000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|GridLock") float StopMargin = 1.f;
	// Tiempo (en segundos) que el tanque permanece inmóvil tras un giro brusco.
	// Evita el desplazamiento accidental al cambiar de dirección.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|GridLock")
	float TurnDelay = 0.06f; // Un valor bajo (0.05 a 0.1) suele bastar

	// Firing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") TSubclassOf<AProjectile> ProjectileClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") float FireCooldown = 0.2f;

	// Input actions (Enhanced Input)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input") UInputAction* IA_Move = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input") UInputAction* IA_Fire = nullptr;

	// Salud básica del jugador
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Health")
	int32 HitPoints = 1;

	// Daño genérico
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// Input handlers
	void OnMove(const FInputActionValue& Value);
	void OnFire(const FInputActionValue& Value);
	void TryFire();

	// Movement internals
	void UpdateMovement(float DT);
	float GetAccelRateCurrent() const;
	float GetDecelRateCurrent() const;
	bool GetCurrentCell(int32& OutX, int32& OutY) const;
	FVector GetCellCenterWorld(int32 X, int32 Y) const;

private:
	UPROPERTY() UMapGridSubsystem* Grid = nullptr;
	FVector2D RawMoveInput = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	FVector2D FacingDir = FVector2D(1, 0);
	double LastFireTime = -1.0;
	// Temporizador interno para el bloqueo de giro
	float CurrentTurnDelay = 0.0f;
};
