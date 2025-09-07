// MapConfigAsset.h
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Utils/JsonMapUtils.h"
#include "MapConfigAsset.generated.h"

// Usa aqu� tus USTRUCTs ya definidos en alg�n header visible por este .h
// (Si est�n en otro .h, incl�yelo antes de esta UCLASS)

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
    // Guarda el archivo fuente para poder reimportar (opcional, pero �til)
    UPROPERTY(VisibleAnywhere, Category = "Import Settings")
    TObjectPtr<UAssetImportData> AssetImportData;
#endif
};