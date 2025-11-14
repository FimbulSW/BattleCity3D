// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JsonMapUtils.generated.h"

USTRUCT(BlueprintType)
struct FSpawnPoint {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 x = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 y = 0;
};

USTRUCT(BlueprintType)
struct FBaseSpawn {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 x = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 y = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 hp = 1;
	// Futuro multi-bandos (ally/enemy). Por ahora solo "ally" se usa.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString team = TEXT("ally");
};

USTRUCT(BlueprintType)
struct FMapSpawns {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FSpawnPoint> player;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FSpawnPoint> enemies;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FBaseSpawn>  bases;
};

USTRUCT(BlueprintType)
struct FLegendEntry {
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legend") FString terrain;    // "Ground", "Ice", ...
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legend") FString obstacle;   // "Brick", "Steel"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legend") bool playerStart = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legend") FString enemySpawn; // "Basic", "Fast", ...

	// Base (águila)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool base = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 baseHP = 1;
};

USTRUCT(BlueprintType)
struct FWaveEntry {
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float   time = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString type;   // "Basic"/"Fast"/...
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString spawn;  // símbolo en el grid (A/Q/R/T)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32   count = 1; // cuántos aparecen en ese tiempo
};

USTRUCT(BlueprintType)
struct FMapConfig {
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") int32 width = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") int32 height = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float tileSize = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TArray<FString> rows;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TMap<FString, FLegendEntry> legend;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TArray<FWaveEntry> waves;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") FMapSpawns spawns;
};