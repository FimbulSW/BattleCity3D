#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utils/JsonMapUtils.h"
#include "MapGenerator.generated.h"

class UMapConfigAsset;
class UStaticMesh;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class USceneComponent;
class ATankPawn;

/**
 * Genera SOLO la vista (ISM). El estado vive en UMapGridSubsystem.
 */
UCLASS(Blueprintable)
class BATTLECITY3D_API AMapGenerator : public AActor
{
	GENERATED_BODY()
public:
	AMapGenerator();

	// --- Config ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Core")
	TSoftObjectPtr<UMapConfigAsset> MapAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Core")
	bool bOverrideAssetTileSize = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Core", meta = (ClampMin = "10.0"))
	float TileSize = 200.f;

	// Subgrid (snap fino configurable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Core", meta = (ClampMin = "1", ClampMax = "128"))
	int32 SubdivisionsPerTile = 20;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Core")
	int32 MapWidth = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Core")
	int32 MapHeight = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Core")
	float GroundThickness = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Assets")
	UStaticMesh* BlockMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Assets")
	UMaterialInterface* M_Ground = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Assets")
	UMaterialInterface* M_Ice = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Assets")
	UMaterialInterface* M_Water = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Assets")
	UMaterialInterface* M_Forest = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Assets")
	UMaterialInterface* M_Brick = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Assets")
	UMaterialInterface* M_Steel = nullptr;

	// Player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Tanks")
	TSubclassOf<ATankPawn> PlayerTankClass = nullptr;

	// Cámara fija
	UPROPERTY(EditAnywhere, Category = "Map|Camera")
	FName FixedCameraTag = "FixedCamera";

	// --- Visual components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|ISM") USceneComponent* Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|ISM") UInstancedStaticMeshComponent* GroundISM;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|ISM") UInstancedStaticMeshComponent* IceISM;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|ISM") UInstancedStaticMeshComponent* WaterISM;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|ISM") UInstancedStaticMeshComponent* ForestISM;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|ISM") UInstancedStaticMeshComponent* BrickISM;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|ISM") UInstancedStaticMeshComponent* SteelISM;

	// Quitar visual de ladrillo cuando el grid lo indique
	UFUNCTION(BlueprintCallable, Category = "Map|Visual")
	void RemoveBrickInstanceAt(int32 X, int32 Y);

	UFUNCTION(BlueprintCallable, Category = "Map|Player")
	void RespawnPlayer();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	// Asset data
	FMapConfig LocalMap;
	TMap<FString, FLegendEntry> Legend;

	// Vista
	void SetupISMs();
	void ClearISMs();
	void BuildWorld();
	bool LoadMapAsset(FString& OutError);
	void UnifyTileSize();

	// Helpers
	FVector GridToWorld(int32 X, int32 Y, float ZOffset = 0.f) const;

	// Player
	FVector PlayerWorldStart = FVector::ZeroVector;
	void SpawnAndPossessPlayer();
};
