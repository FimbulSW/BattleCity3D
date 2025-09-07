

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpawnPointPolicy.generated.h"

class UMapGridSubsystem;

USTRUCT(BlueprintType)
struct FSpawnPointContext
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FVector PlayerLoc = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) bool    bHasBase = false;
	UPROPERTY(BlueprintReadOnly) FVector BaseLoc = FVector::ZeroVector;
};

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class BATTLECITY3D_API USpawnPointPolicy : public UObject
{
	GENERATED_BODY()
	
public:	
	virtual void BuildCandidateOrder(const TArray<FVector>& In, const FSpawnPointContext& Ctx, TArray<int32>& Out) const;

protected:
	
};
