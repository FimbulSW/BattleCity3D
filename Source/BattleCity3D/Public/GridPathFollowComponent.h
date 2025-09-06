#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridPathTypes.h"
#include "GridPathFollowComponent.generated.h"

class UMapGridSubsystem;

UCLASS(ClassGroup = (BattleCity), meta = (BlueprintSpawnableComponent))
class BATTLECITY3D_API UGridPathFollowComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "GridPath")
	void SetPath(UMapGridSubsystem* InGrid, const FGridPathResult& InPath)
	{
		Grid = InGrid; Path = InPath; Index = (Path.Cells.Num() > 1) ? 1 : 0;
	}

	UFUNCTION(BlueprintCallable, Category = "GridPath")
	bool HasPath() const { return Path.bValid && Path.Cells.Num() > 1 && Grid != nullptr; }

	// Celda objetivo actual (waypoint)
	bool GetCurrentTargetCell(FIntPoint& OutCell) const
	{
		if (!HasPath()) return false;
		OutCell = Path.Cells[Index];
		return true;
	}

	// Avanza si ya alcanzó la celda actual (en espacio mundo)
	void AdvanceIfReached(const FVector& WorldPos, float TileSize, float SnapTolWorld = 5.f)
	{
		if (!HasPath()) return;
		FIntPoint Target;
		if (!GetCurrentTargetCell(Target)) return;

		const FVector TargetW = GridToWorld(Target, TileSize);
		if (FVector::Dist2D(WorldPos, TargetW) <= SnapTolWorld)
		{
			if (Index + 1 < Path.Cells.Num()) ++Index;
		}
	}

	// Dirección cardinal deseada hacia la celda objetivo (unidad en X/Y mundo)
	FVector GetDesiredDirWorld(const FVector& WorldPos, float TileSize) const
	{
		if (!HasPath()) return FVector::ZeroVector;
		FIntPoint Target;
		if (!GetCurrentTargetCell(Target)) return FVector::ZeroVector;

		const FVector TargetW = GridToWorld(Target, TileSize);
		const FVector Delta = TargetW - WorldPos;
		// Proyecta a eje dominante para cardinal puro
		if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
			return FVector(FMath::Sign(Delta.X), 0, 0);
		else
			return FVector(0, FMath::Sign(Delta.Y), 0);
	}

	// ¿Llegó al final?
	bool IsAtLastCell() const { return HasPath() && Index >= Path.Cells.Num() - 1; }

private:
	UPROPERTY() TObjectPtr<UMapGridSubsystem> Grid = nullptr;
	FGridPathResult Path;
	int32 Index = 0;

	FVector GridToWorld(const FIntPoint& C, float TileSize) const;
};
