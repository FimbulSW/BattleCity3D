#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Utils/JsonMapUtils.h"
#include "Components/GridPathFollow/GridPathTypes.h"
#include "MapGridSubsystem.generated.h"

class UMapConfigAsset;
class AMapGenerator;

UENUM(BlueprintType)
enum class ETerrainType : uint8 { Ground, Ice, Water, Forest };
UENUM(BlueprintType)
enum class EObstacleType : uint8 { None, Brick, Steel };

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGridCellChanged, FIntPoint);

UCLASS()
class BATTLECITY3D_API UMapGridSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MapGrid")
	bool InitializeFromAsset(UMapConfigAsset* Asset,
		bool bOverrideAssetTileSize,
		float ActorTileSize,
		const FTransform& MapTransform,
		AMapGenerator* VisualOwner,
		int32 InSubdivisionsPerTile);

	// Grid <-> Mundo
	bool WorldToGrid(const FVector& WorldPos, int32& OutX, int32& OutY) const;
	FVector GridToWorld(int32 X, int32 Y, float ZOffset = 0.f) const;

	// Obtener TODOS los puntos de spawn (unión de símbolos), ignorando "."
	void GetAllSpawnWorldLocations(TArray<FVector>& OutWorld) const;

	// Consultas
	UFUNCTION(BlueprintCallable, Category = "MapGrid") ETerrainType  GetTerrainAtGrid(int32 X, int32 Y) const;
	UFUNCTION(BlueprintCallable, Category = "MapGrid") EObstacleType GetObstacleAtGrid(int32 X, int32 Y) const;
	UFUNCTION(BlueprintCallable, Category = "MapGrid") ETerrainType  GetTerrainAtWorld(const FVector& WorldPos) const;
	UFUNCTION(BlueprintCallable, Category = "MapGrid") EObstacleType GetObstacleAtWorld(const FVector& WorldPos) const;
	UFUNCTION(BlueprintCallable, Category = "MapGrid") bool          IsPassableForPawnAtWorld(const FVector& WorldPos) const;

	// Impacto de proyectil vs obstáculos
	UFUNCTION(BlueprintCallable, Category = "MapGrid")
	bool TryHitObstacleAtWorld(const FVector& WorldPos, /*out*/ bool& bWasBrick);

	// Spawns / Waves
	UFUNCTION(BlueprintCallable, Category = "MapGrid|Spawns")
	void GetSpawnWorldLocationsForSymbol(const FString& Symbol, TArray<FVector>& OutWorld) const;
	UFUNCTION(BlueprintCallable, Category = "MapGrid|Spawns")
	const TArray<FWaveEntry>& GetWaves() const { return Waves; }

	// Base (águila)
	UFUNCTION(BlueprintCallable, Category = "MapGrid|Base") bool   HasBase() const { return bHasBase; }
	UFUNCTION(BlueprintCallable, Category = "MapGrid|Base") FVector GetBaseWorldLocation() const { return BaseWorld; }
	UFUNCTION(BlueprintCallable, Category = "MapGrid|Base") int32  GetBaseDefaultHP() const { return BaseHP; }

	// Subgrid
	UFUNCTION(BlueprintCallable, Category = "MapGrid|Subgrid") int32  GetSubdivisionsPerTile() const { return SubdivisionsPerTile; }
	UFUNCTION(BlueprintCallable, Category = "MapGrid|Subgrid") float  GetSubStep() const { return SubStep; }
	UFUNCTION(BlueprintCallable, Category = "MapGrid|Subgrid") FVector SnapWorldToSubgrid(const FVector& World, bool bKeepZ = true) const;

	// Accesores
	int32  GetWidth() const { return MapWidth; }
	int32  GetHeight() const { return MapHeight; }
	float  GetTileSize() const { return TileSize; }
	FVector GetPlayerWorldStart() const { return PlayerWorldStart; }

	// ==== Pathfinding utils ====
	void GetNeighbors4(const FIntPoint& Cell, TArray<FIntPoint>& OutNeighbors) const;
	float GetTileCost(const FIntPoint& Cell, const FGridCostProfile& Profile) const;
	bool IsPassableCell(const FIntPoint& Cell, const FGridCostProfile& Profile) const
	{
		return GetTileCost(Cell, Profile) < Profile.ImpassableCost;
	}

	// NUEVO: unión de celdas de spawn de todos los símbolos (excepto ".")
	void GetAllEnemySpawnCells(TArray<FIntPoint>& Out) const;

	// Evento: ladrillo destruido, etc.
	FOnGridCellChanged OnGridCellChanged;

private:
	// Datos mapa
	int32 MapWidth = 0, MapHeight = 0;
	float TileSize = 200.f;
	FTransform MapXform = FTransform::Identity;

	// Subgrid
	int32 SubdivisionsPerTile = 20;
	float SubStep = 10.f;

	TArray<ETerrainType>  TerrainGrid;
	TArray<EObstacleType> ObstacleGrid;
	TArray<uint8>         ObstacleHPGrid;

	// Base
	bool     bHasBase = false;
	FIntPoint BaseCell = FIntPoint(-1, -1);
	FVector  BaseWorld = FVector::ZeroVector;
	int32    BaseHP = 1;

	// Spawns
	TMap<FString, TArray<FIntPoint>> EnemySpawnGridBySymbol;
	TArray<FWaveEntry>               Waves;

	// Vista para quitar ladrillos
	TWeakObjectPtr<AMapGenerator> Visual;

	bool BuildFromAsset(const FMapConfig& Map, const TMap<FString, FLegendEntry>& Legend);

	// Player start
	FVector PlayerWorldStart = FVector::ZeroVector;

	FORCEINLINE int32 XYToIndex(int32 X, int32 Y) const { return X + Y * MapWidth; }

	// bounds
	bool IsInside(const FIntPoint& Cell) const
	{
		return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < MapWidth && Cell.Y < MapHeight;
	}
};
