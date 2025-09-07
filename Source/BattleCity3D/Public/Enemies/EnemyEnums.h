

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EEnemyType : uint8 { Basic, Fast, Power, Armored };

UENUM(BlueprintType)
enum class EEnemyGoal : uint8 { HuntBase, HuntPlayer };