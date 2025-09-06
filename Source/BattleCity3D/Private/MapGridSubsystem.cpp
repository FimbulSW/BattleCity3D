#include "MapGridSubsystem.h"
#include "MapConfigAsset.h"
#include "MapGenerator.h"

bool UMapGridSubsystem::InitializeFromAsset(UMapConfigAsset* Asset,
	bool bOverrideAssetTileSize,
	float ActorTileSize,
	const FTransform& MapTransform,
	AMapGenerator* VisualOwner,
	int32 InSubdivisionsPerTile)
{
	if (!Asset) return false;

	FMapConfig Map = Asset->Config;
	if (bOverrideAssetTileSize) Map.tileSize = ActorTileSize;

	MapXform = MapTransform;
	TileSize = Map.tileSize;
	MapWidth = Map.width;
	MapHeight = Map.height;
	Visual = VisualOwner;

	SubdivisionsPerTile = FMath::Max(1, InSubdivisionsPerTile);
	SubStep = TileSize / (float)SubdivisionsPerTile;

	return BuildFromAsset(Map, Map.legend);
}

bool UMapGridSubsystem::BuildFromAsset(const FMapConfig& Map, const TMap<FString, FLegendEntry>& Legend)
{
	if (MapWidth <= 0 || MapHeight <= 0 || TileSize <= 0.f) return false;

	const int32 N = MapWidth * MapHeight;
	TerrainGrid.SetNum(N);
	ObstacleGrid.SetNum(N);
	ObstacleHPGrid.SetNum(N);
	for (int32 I = 0; I < N; ++I) { TerrainGrid[I] = ETerrainType::Ground; ObstacleGrid[I] = EObstacleType::None; ObstacleHPGrid[I] = 0; }

	EnemySpawnGridBySymbol.Reset();
	Waves = Map.waves;

	bHasBase = false; BaseCell = FIntPoint(-1, -1); BaseWorld = FVector::ZeroVector; BaseHP = 1;

	auto TerrainFromString = [](const FString& S) {
		if (S.Equals(TEXT("Ice"), ESearchCase::IgnoreCase)) return ETerrainType::Ice;
		if (S.Equals(TEXT("Water"), ESearchCase::IgnoreCase)) return ETerrainType::Water;
		if (S.Equals(TEXT("Forest"), ESearchCase::IgnoreCase)) return ETerrainType::Forest;
		return ETerrainType::Ground;
		};
	auto ObstacleFromString = [](const FString& S) {
		if (S.Equals(TEXT("Brick"), ESearchCase::IgnoreCase)) return EObstacleType::Brick;
		if (S.Equals(TEXT("Steel"), ESearchCase::IgnoreCase)) return EObstacleType::Steel;
		return EObstacleType::None;
		};

	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		if (!Map.rows.IsValidIndex(Y)) break;
		const FString& Row = Map.rows[Y];
		for (int32 X = 0; X < MapWidth && X < Row.Len(); ++X)
		{
			const FString Sym = Row.Mid(X, 1);
			const FLegendEntry* Def = Legend.Find(Sym);
			if (!Def) continue;
			const int32 Idx = XYToIndex(X, Y);

			if (!Def->terrain.IsEmpty())
				TerrainGrid[Idx] = TerrainFromString(Def->terrain);

			if (!Def->obstacle.IsEmpty())
			{
				const EObstacleType O = ObstacleFromString(Def->obstacle);
				ObstacleGrid[Idx] = O;
				if (O == EObstacleType::Brick)      ObstacleHPGrid[Idx] = 2;
				else if (O == EObstacleType::Steel) ObstacleHPGrid[Idx] = 255;
			}

			if (Def->playerStart)
				PlayerWorldStart = GridToWorld(X, Y, TileSize * 0.5f);

			if (!Def->enemySpawn.IsEmpty())
				EnemySpawnGridBySymbol.FindOrAdd(Sym).Add(FIntPoint(X, Y));

			if (Def->base)
			{
				bHasBase = true;
				BaseCell = FIntPoint(X, Y);
				BaseWorld = GridToWorld(X, Y, TileSize * 0.5f);
				BaseHP = FMath::Max(1, Def->baseHP);
			}
		}
	}

	return true;
}

bool UMapGridSubsystem::WorldToGrid(const FVector& WorldPos, int32& OutX, int32& OutY) const
{
	const FVector Local = MapXform.InverseTransformPosition(WorldPos);
	if (TileSize <= KINDA_SMALL_NUMBER) return false;

	const int32 Gx = FMath::RoundToInt(Local.X / TileSize);
	const int32 Gy = FMath::RoundToInt(Local.Y / TileSize);

	if (Gx < 0 || Gx >= MapWidth || Gy < 0 || Gy >= MapHeight) return false;
	OutX = Gx; OutY = Gy; return true;
}

FVector UMapGridSubsystem::GridToWorld(int32 X, int32 Y, float ZOffset) const
{
	const FVector Local(X * TileSize, Y * TileSize, ZOffset);
	return MapXform.TransformPosition(Local);
}

void UMapGridSubsystem::GetAllSpawnWorldLocations(TArray<FVector>& OutWorld) const
{
	OutWorld.Reset();
	for (const TPair<FString, TArray<FIntPoint>>& Pair : EnemySpawnGridBySymbol)
	{
		const FString& Sym = Pair.Key;
		if (Sym.Equals(TEXT("."))) continue; // '.' no es punto de spawn
		for (const FIntPoint& P : Pair.Value)
		{
			OutWorld.Add(GridToWorld(P.X, P.Y, TileSize * 0.5f));
		}
	}
}

ETerrainType  UMapGridSubsystem::GetTerrainAtGrid(int32 X, int32 Y) const { if (X < 0 || X >= MapWidth || Y < 0 || Y >= MapHeight || TerrainGrid.Num() == 0) return ETerrainType::Ground; return TerrainGrid[XYToIndex(X, Y)]; }
EObstacleType UMapGridSubsystem::GetObstacleAtGrid(int32 X, int32 Y) const { if (X < 0 || X >= MapWidth || Y < 0 || Y >= MapHeight || ObstacleGrid.Num() == 0) return EObstacleType::None;  return ObstacleGrid[XYToIndex(X, Y)]; }
ETerrainType  UMapGridSubsystem::GetTerrainAtWorld(const FVector& WorldPos) const { int32 X, Y; if (!WorldToGrid(WorldPos, X, Y)) return ETerrainType::Ground; return GetTerrainAtGrid(X, Y); }
EObstacleType UMapGridSubsystem::GetObstacleAtWorld(const FVector& WorldPos) const { int32 X, Y; if (!WorldToGrid(WorldPos, X, Y)) return EObstacleType::None;  return GetObstacleAtGrid(X, Y); }

bool UMapGridSubsystem::IsPassableForPawnAtWorld(const FVector& WorldPos) const
{
	int32 X, Y; if (!WorldToGrid(WorldPos, X, Y)) return true;
	const ETerrainType T = GetTerrainAtGrid(X, Y);
	if (T == ETerrainType::Water) return false;
	const EObstacleType O = GetObstacleAtGrid(X, Y);
	return !(O == EObstacleType::Brick || O == EObstacleType::Steel);
}

void UMapGridSubsystem::GetSpawnWorldLocationsForSymbol(const FString& Symbol, TArray<FVector>& OutWorld) const
{
	OutWorld.Reset();
	if (const TArray<FIntPoint>* Cells = EnemySpawnGridBySymbol.Find(Symbol))
	{
		for (const FIntPoint& P : *Cells)
		{
			OutWorld.Add(GridToWorld(P.X, P.Y, TileSize * 0.5f));
		}
	}
}

FVector UMapGridSubsystem::SnapWorldToSubgrid(const FVector& World, bool bKeepZ) const
{
	FVector Local = MapXform.InverseTransformPosition(World);
	Local.X = FMath::RoundToFloat(Local.X / SubStep) * SubStep;
	Local.Y = FMath::RoundToFloat(Local.Y / SubStep) * SubStep;
	if (!bKeepZ) Local.Z = FMath::RoundToFloat(Local.Z / SubStep) * SubStep;
	return MapXform.TransformPosition(Local);
}

void UMapGridSubsystem::GetNeighbors4(const FIntPoint& Cell, TArray<FIntPoint>& OutNeighbors) const
{
	OutNeighbors.Reset();
	const FIntPoint C = Cell;

	const FIntPoint Nbs[4] = {
		FIntPoint(C.X, C.Y - 1),
		FIntPoint(C.X + 1, C.Y),
		FIntPoint(C.X, C.Y + 1),
		FIntPoint(C.X - 1, C.Y)
	};
	for (const FIntPoint& N : Nbs)
	{
		if (IsInside(N)) OutNeighbors.Add(N);
	}
}

float UMapGridSubsystem::GetTileCost(const FIntPoint& Cell, const FGridCostProfile& Profile) const
{
	if (!IsInside(Cell)) return Profile.ImpassableCost;

	const int32 Idx = XYToIndex(Cell.X, Cell.Y);

	// Terreno base: terreno (ice/water/forest/ground)
	const ETerrainType T = (TerrainGrid.IsValidIndex(Idx)) ? TerrainGrid[Idx] : ETerrainType::Ground;
	// Obstáculo en la celda
	const EObstacleType O = (ObstacleGrid.IsValidIndex(Idx)) ? ObstacleGrid[Idx] : EObstacleType::None;

	// Agua: impasable (en tu juego actual)
	if (T == ETerrainType::Water) return Profile.ImpassableCost;

	// Acero: impasable
	if (O == EObstacleType::Steel) return Profile.ImpassableCost;

	// Ladrillo: pasable "caro" (atravesable si compensa romper)
	if (O == EObstacleType::Brick) return Profile.BrickCost;

	// Resto: libre
	return Profile.FreeCost;
}

void UMapGridSubsystem::GetAllEnemySpawnCells(TArray<FIntPoint>& Out) const
{
	Out.Reset();

	// Reunir la unión de todas las celdas de spawn de todos los símbolos (excepto ".")
	TSet<FIntPoint> Unique;
	for (const TPair<FString, TArray<FIntPoint>>& Pair : EnemySpawnGridBySymbol)
	{
		// Ignorar símbolo "." (no es spawn)
		if (Pair.Key.Equals(TEXT("."))) continue;

		for (const FIntPoint& P : Pair.Value)
		{
			if (IsInside(P))
			{
				Unique.Add(P);
			}
		}
	}

	Out.Reserve(Unique.Num());
	for (const FIntPoint& P : Unique)
	{
		Out.Add(P);
	}
}


// === HOOK: marcar cambio de grid al destruir ladrillo ===
bool UMapGridSubsystem::TryHitObstacleAtWorld(const FVector& WorldPos, bool& bWasBrick)
{
	bWasBrick = false;
	int32 X, Y; 
	if (!WorldToGrid(WorldPos, X, Y)) 
		return false;

	const int32 Idx = XYToIndex(X, Y);
	if (!ObstacleGrid.IsValidIndex(Idx) || !ObstacleHPGrid.IsValidIndex(Idx)) return false;

	const EObstacleType O = ObstacleGrid[Idx];
	if (O == EObstacleType::Brick)
	{
		bWasBrick = true;
		uint8& Hp = ObstacleHPGrid[Idx];
		if (Hp == 0) return true;

		if (Hp > 1) { Hp -= 1; return true; }

		Hp = 0;
		ObstacleGrid[Idx] = EObstacleType::None;

		if (AMapGenerator* Viz = Visual.Get())
		{
			Viz->RemoveBrickInstanceAt(X, Y);
		}

		// NUEVO: notificar cambio de celda para invalidar rutas
		OnGridCellChanged.Broadcast(FIntPoint(X, Y));
		return true;
	}
	else if (O == EObstacleType::Steel)
	{
		return true; // consume proyectil
	}
	return false;
}