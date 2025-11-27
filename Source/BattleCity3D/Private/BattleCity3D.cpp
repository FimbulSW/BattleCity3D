// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleCity3D.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, BattleCity3D, "BattleCity3D" );
// Uso en consola: bc.collision.debug 1
TAutoConsoleVariable<int32> CVarBcCollisionDebug(
    TEXT("bc.collision.debug"),
    0,
    TEXT("Muestra cajas de colision: Tanques (Verde/Rojo) y Bases (Azul/Violeta)"),
    ECVF_Cheat);