// MapConfigAsset.h
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Utils/JsonMapUtils.h"
#include "MapConfigAsset.generated.h"

// Usa aquí tus USTRUCTs ya definidos en algún header visible por este .h
// (Si están en otro .h, inclúyelo antes de esta UCLASS)

class UAssetImportData; // Forward-declare para editor (puntero)

// Asset que guarda tu FMapConfig
UCLASS(BlueprintType)
class BATTLECITY3D_API UMapConfigAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
    FMapConfig Config;

#if WITH_EDITORONLY_DATA
    // Guarda el archivo fuente para poder reimportar (opcional, pero útil)
    UPROPERTY(VisibleAnywhere, Category = "Import Settings")
    TObjectPtr<UAssetImportData> AssetImportData;
#endif
};