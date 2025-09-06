#pragma once
#include "CoreMinimal.h"
#include "GridPathTypes.generated.h"

// Costos por tipo de celda (tuning en runtime).
USTRUCT(BlueprintType)
struct FGridCostProfile
{
	GENERATED_BODY()

	// Costo para celdas libres/pasables (ground/ice/forest, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FreeCost = 1.f;

	// Ladrillo "rompible": lo tratamos como pasable caro (si conviene romper)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BrickCost = 10.f;

	// Costo para acero/agua: usamos un "infinito" efectivo = bloqueado
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ImpassableCost = 1e9f;
};

USTRUCT(BlueprintType)
struct FGridPathRequest
{
	GENERATED_BODY()

	// Grid lógico a consultar
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UMapGridSubsystem> Grid = nullptr;

	// Inicio/Meta en coordenadas de CELDA (X,Y)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Start = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Goal = FIntPoint::ZeroValue;

	// Perfil de costos
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGridCostProfile Cost;

	// Limitar horizonte (0 = ruta completa)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxSteps = 0;

	// Permitir devolver ruta parcial si no se alcanza la meta en el horizonte
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowPartial = true;
};

USTRUCT(BlueprintType)
struct FGridPathResult
{
	GENERATED_BODY()

	// Secuencia de celdas (incluye Start y, si se alcanzó, Goal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntPoint> Cells;

	// Costo acumulado estimado
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TotalCost = 0.f;

	// ¿Alcanzó la meta?
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bReachedGoal = false;

	// ¿Ruta válida?
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bValid = false;
};
