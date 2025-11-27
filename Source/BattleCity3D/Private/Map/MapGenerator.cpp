#include "Map/MapGenerator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Map/MapConfigAsset.h"
#include "Map/MapGridSubsystem.h"
#include "Player/TankPawn.h"
#include "Utils/JsonMapUtils.h"
#include "DrawDebugHelpers.h"


static TAutoConsoleVariable<int32> CVarBcMapDebug(
	TEXT("bc.map.debug"),
	0,
	TEXT("1: Muestra el grid de debug. 0: Oculta."),
	ECVF_Cheat);

AMapGenerator::AMapGenerator()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	GroundISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GroundISM"));
	IceISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("IceISM"));
	WaterISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WaterISM"));
	ForestISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ForestISM"));
	BrickISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BrickISM"));
	SteelISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SteelISM"));

	GroundISM->SetupAttachment(Root);
	IceISM->SetupAttachment(Root);
	WaterISM->SetupAttachment(Root);
	ForestISM->SetupAttachment(Root);
	BrickISM->SetupAttachment(Root);
	SteelISM->SetupAttachment(Root);

	if (!BlockMesh)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (CubeMesh.Succeeded())
		{
			BlockMesh = CubeMesh.Object;
		}
	}
}

bool AMapGenerator::LoadMapAsset(FString& OutError)
{
	if (MapAsset.IsNull())
	{
		OutError = TEXT("MapAsset no asignado.");
		return false;
	}
	UMapConfigAsset* Asset = MapAsset.LoadSynchronous();
	if (!Asset)
	{
		OutError = TEXT("No se pudo cargar MapAsset.");
		return false;
	}
	LocalMap = Asset->Config;
	Legend = LocalMap.legend;
	return true;
}

void AMapGenerator::UnifyTileSize()
{
	if (bOverrideAssetTileSize)
	{
		LocalMap.tileSize = TileSize;
	}
	else
	{
		TileSize = LocalMap.tileSize;
	}
}

void AMapGenerator::SetupISMs()
{
	auto SetupOne = [&](UInstancedStaticMeshComponent* ISM, UMaterialInterface* Mat)
		{
			if (!ISM) return;
			ISM->SetMobility(EComponentMobility::Static);
			if (BlockMesh) ISM->SetStaticMesh(BlockMesh);
			if (Mat) ISM->SetMaterial(0, Mat);
			ISM->SetCullDistances(0, 0);
			// Sin colisiones físicas: gameplay por grid
			ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ISM->SetGenerateOverlapEvents(false);
		};

	SetupOne(GroundISM, M_Ground);
	SetupOne(IceISM, M_Ice);
	SetupOne(WaterISM, M_Water);
	SetupOne(ForestISM, M_Forest);
	SetupOne(BrickISM, M_Brick);
	SetupOne(SteelISM, M_Steel);

	if (BrickISM && !BrickISM->ComponentHasTag(TEXT("Brick"))) BrickISM->ComponentTags.Add(TEXT("Brick"));
	if (SteelISM && !SteelISM->ComponentHasTag(TEXT("Steel"))) SteelISM->ComponentTags.Add(TEXT("Steel"));
}

void AMapGenerator::ClearISMs()
{
	if (GroundISM) GroundISM->ClearInstances();
	if (IceISM)    IceISM->ClearInstances();
	if (WaterISM)  WaterISM->ClearInstances();
	if (ForestISM) ForestISM->ClearInstances();
	if (BrickISM)  BrickISM->ClearInstances();
	if (SteelISM)  SteelISM->ClearInstances();
}

FVector AMapGenerator::GridToWorld(int32 X, int32 Y, float ZOffset) const
{
	const FVector Local(X * TileSize, Y * TileSize, ZOffset);
	return GetActorTransform().TransformPosition(Local);
}

void AMapGenerator::BuildWorld()
{
	if (!BlockMesh || !GroundISM) return;

	const float T = TileSize;
	const FVector GroundScale(T / 100.f, T / 100.f, GroundThickness / 100.f);
	const FVector BlockScale(T / 100.f, T / 100.f, T / 100.f);

	for (int32 y = 0; y < LocalMap.height; ++y)
	{
		const FString& Row = LocalMap.rows.IsValidIndex(y) ? LocalMap.rows[y] : TEXT("");
		for (int32 x = 0; x < LocalMap.width && x < Row.Len(); ++x)
		{
			const FString Sym = Row.Mid(x, 1);
			const FLegendEntry* Def = Legend.Find(Sym);

			const FVector P = GridToWorld(x, y, 0.0f);
			GroundISM->AddInstance(FTransform(FRotator::ZeroRotator, P, GroundScale));

			if (!Def) continue;

			if (Def->terrain == "Ice")
				IceISM->AddInstance(FTransform(FRotator::ZeroRotator, GridToWorld(x, y, 0.f), GroundScale));
			if (Def->terrain == "Water")
				WaterISM->AddInstance(FTransform(FRotator::ZeroRotator, GridToWorld(x, y, 0.f), GroundScale));
			if (Def->terrain == "Forest")
				ForestISM->AddInstance(FTransform(FRotator::ZeroRotator, GridToWorld(x, y, 0.f), GroundScale));

			if (Def->obstacle == "Brick")
				BrickISM->AddInstance(FTransform(FRotator::ZeroRotator, GridToWorld(x, y, T * 0.5f), BlockScale));
			if (Def->obstacle == "Steel")
				SteelISM->AddInstance(FTransform(FRotator::ZeroRotator, GridToWorld(x, y, T * 0.5f), BlockScale));

			if (Def->playerStart)
				PlayerWorldStart = P + FVector(0, 0, T * 0.5f);
		}
	}
}

void AMapGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SetupISMs();
	ClearISMs();

	FString Err;
	if (LoadMapAsset(Err))
	{
		UnifyTileSize();
		MapWidth = LocalMap.width;
		MapHeight = LocalMap.height;

		BuildWorld();
	}
}

void AMapGenerator::BeginPlay()
{
	Super::BeginPlay();

	FString Err;
	if (!LoadMapAsset(Err))
	{
		UE_LOG(LogTemp, Error, TEXT("Map load failed: %s"), *Err);
		return;
	}
	UnifyTileSize();
	MapWidth = LocalMap.width;
	MapHeight = LocalMap.height;

	// Inicializa subsystem (estado del grid) con SUBGRID
	if (UMapGridSubsystem* Grid = GetGameInstance()->GetSubsystem<UMapGridSubsystem>())
	{
		UMapConfigAsset* Asset = MapAsset.LoadSynchronous();
		if (!Grid->InitializeFromAsset(Asset, bOverrideAssetTileSize, TileSize, GetActorTransform(), this, SubdivisionsPerTile))
		{
			UE_LOG(LogTemp, Error, TEXT("MapGridSubsystem init failed"));
		}
		TArray<FVector> PlayerSpawns;
		Grid->GetPlayerSpawnWorldLocations(PlayerSpawns);
		if (PlayerSpawns.Num() > 0)
			PlayerWorldStart = PlayerSpawns[0]; //TODO: Randomizar donde aparece el jugador.
	}

	SpawnAndPossessPlayer();
}

void AMapGenerator::SpawnAndPossessPlayer()
{
	if (!PlayerTankClass) return;
	UWorld* World = GetWorld(); if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	ATankPawn* Tank = World->SpawnActor<ATankPawn>(PlayerTankClass, PlayerWorldStart, FRotator::ZeroRotator, Params);
	if (Tank)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->bAutoManageActiveCameraTarget = false;
			PC->Possess(Tank);

			for (TActorIterator<ACameraActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(FixedCameraTag))
				{
					PC->SetViewTargetWithBlend(*It, 0.f);
					break;
				}
			}
		}
	}
}

void AMapGenerator::RemoveBrickInstanceAt(int32 X, int32 Y)
{
	if (!BrickISM) return;
	const float Zc = TileSize * 0.5f;
	const FVector Wanted = GridToWorld(X, Y, Zc);

	const int32 Count = BrickISM->GetInstanceCount();
	for (int32 i = 0; i < Count; ++i)
	{
		FTransform Tr; BrickISM->GetInstanceTransform(i, Tr, true);
		const FVector L = Tr.GetLocation();
		if (FMath::IsNearlyEqual(L.X, Wanted.X, 1.f) &&
			FMath::IsNearlyEqual(L.Y, Wanted.Y, 1.f) &&
			FMath::IsNearlyEqual(L.Z, Wanted.Z, 1.f))
		{
			BrickISM->RemoveInstance(i);
			break;
		}
	}
}

void AMapGenerator::RespawnPlayer()
{
	if (UMapGridSubsystem* Grid = GetGameInstance()->GetSubsystem<UMapGridSubsystem>())
	{
		PlayerWorldStart = Grid->GetPlayerWorldStart();
	}
	SpawnAndPossessPlayer();
}

void AMapGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Solo dibuja si la variable es > 0
	if (CVarBcMapDebug.GetValueOnGameThread() > 0)
	{
		DrawDebugGrid(false, -1.f);
	}
}

void AMapGenerator::DrawDebugGrid(bool bPersistent, float Lifetime)
{
	if (MapWidth <= 0 || MapHeight <= 0 || TileSize <= 0) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const FTransform& Tr = GetActorTransform();
	// Elevamos Z para que se vea bien sobre los ladrillos
	const float Z = GroundThickness + 55.0f;

	const FColor ColorMain = FColor::Yellow;
	const FColor ColorSub = FColor::Emerald; // Verde brillante
	const float ThicknessMain = 3.0f;
	const float ThicknessSub = 1.5f;

	const float MapW = MapWidth * TileSize;
	const float MapH = MapHeight * TileSize;
	const float SubStep = TileSize / (float)FMath::Max(1, SubdivisionsPerTile);

	// LÍNEAS VERTICALES
	int32 TotalStepsX = MapWidth * SubdivisionsPerTile;
	for (int32 i = 0; i <= TotalStepsX; ++i)
	{
		float X = i * SubStep;
		bool bIsMain = (i % SubdivisionsPerTile == 0);

		FVector Start = Tr.TransformPosition(FVector(X, 0, Z));
		FVector End = Tr.TransformPosition(FVector(X, MapH, Z));

		DrawDebugLine(World, Start, End,
			bIsMain ? ColorMain : ColorSub,
			bPersistent, Lifetime, 0,
			bIsMain ? ThicknessMain : ThicknessSub);
	}

	// LÍNEAS HORIZONTALES
	int32 TotalStepsY = MapHeight * SubdivisionsPerTile;
	for (int32 i = 0; i <= TotalStepsY; ++i)
	{
		float Y = i * SubStep;
		bool bIsMain = (i % SubdivisionsPerTile == 0);

		FVector Start = Tr.TransformPosition(FVector(0, Y, Z));
		FVector End = Tr.TransformPosition(FVector(MapW, Y, Z));

		DrawDebugLine(World, Start, End,
			bIsMain ? ColorMain : ColorSub,
			bPersistent, Lifetime, 0,
			bIsMain ? ThicknessMain : ThicknessSub);
	}
}