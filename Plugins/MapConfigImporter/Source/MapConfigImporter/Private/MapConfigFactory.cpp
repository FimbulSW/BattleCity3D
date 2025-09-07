// MapConfigFactory.cpp
#include "MapConfigFactory.h"
#include "Map/MapConfigAsset.h"                 // Nuestro asset (vive en BattleCity3D)
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"

#if WITH_EDITOR
#include "EditorFramework/AssetImportData.h"
#endif


UMapConfigFactory::UMapConfigFactory()
{
    bCreateNew = false;
    bEditAfterNew = true;
    bEditorImport = true;
    SupportedClass = UMapConfigAsset::StaticClass();
    Formats.Add(TEXT("json;Map Config JSON"));
}

bool UMapConfigFactory::FactoryCanImport(const FString& Filename)
{
    return FPaths::GetExtension(Filename).Equals(TEXT("json"), ESearchCase::IgnoreCase);
}

UObject* UMapConfigFactory::FactoryCreateFile(
    UClass* InClass,
    UObject* InParent,
    FName InName,
    EObjectFlags Flags,
    const FString& Filename,
    const TCHAR* Parms,
    FFeedbackContext* Warn,
    bool& bOutOperationCanceled)
{
    bOutOperationCanceled = false;

    // 1) Lee el archivo
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *Filename))
    {
        Warn->Logf(ELogVerbosity::Error, TEXT("No se pudo leer: %s"), *Filename);
        return nullptr;
    }

    // 2) Parseo base (valida que sea JSON)
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        Warn->Logf(ELogVerbosity::Error, TEXT("JSON invalido"));
        return nullptr;
    }
    // 3) Crea el asset
    UMapConfigAsset* Asset = NewObject<UMapConfigAsset>(InParent, InClass, InName, Flags);
    if (!Asset)
    {
        Warn->Logf(ELogVerbosity::Error, TEXT("No se pudo crear UMapConfigAsset"));
        return nullptr;
    }
    //4) Convierte JSON -> FMapConfig (usa nombres de campos 1:1)
    if (!FJsonObjectConverter::JsonObjectToUStruct(
        Root.ToSharedRef(),
        FMapConfig::StaticStruct(),
        &Asset->Config,
        /*CheckFlags*/0, /*SkipFlags*/0))
    {
        Warn->Logf(ELogVerbosity::Error, TEXT("No se pudo convertir JSON a FMapConfig"));
        return nullptr;
    }
#if WITH_EDITORONLY_DATA
    // 5) Guarda la ruta de origen para Reimport
    if (!Asset->AssetImportData)
    {
        Asset->AssetImportData = NewObject<UAssetImportData>(Asset, TEXT("AssetImportData"));
    }
    Asset->AssetImportData->Update(Filename);
#endif

    return Asset;
}

UMapConfigFactory::~UMapConfigFactory()
{
}
