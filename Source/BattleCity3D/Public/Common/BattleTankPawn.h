#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BattleTankPawn.generated.h"

class UMapGridSubsystem;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class BATTLECITY3D_API ABattleTankPawn : public APawn
{
	GENERATED_BODY()

public:
	ABattleTankPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// --- Componentes Base (Comunes) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank") USceneComponent* Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank") UStaticMeshComponent* Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank") USceneComponent* Muzzle;

	// --- Configuración de Movimiento ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float MoveSpeed = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float AccelRate = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float DecelRate = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move") float AlignRate = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|GridLock")
	float TurnDelay = 0.06f;

	// Input crudo (El Player lo llena con Teclado, el Enemigo con IA)
	FVector2D RawMoveInput = FVector2D::ZeroVector;
	FVector2D GetFacingDir() const { return FacingDir; }

protected:
	// Lógica central de física (Bigotes + Rieles)
	void UpdateTankMovement(float DT);

	UPROPERTY() UMapGridSubsystem* Grid = nullptr;
	FVector2D Velocity = FVector2D::ZeroVector;
	FVector2D FacingDir = FVector2D(1, 0);
	float CurrentTurnDelay = 0.0f;
};