

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
	UFUNCTION(BlueprintNativeEvent)
	void BuildCandidateOrder(const TArray<FVector>& In, const FSpawnPointContext& Ctx, TArray<int32>& OutIndices) const;
	virtual void BuildCandidateOrder_Implementation(const TArray<FVector>& In, const FSpawnPointContext& Ctx, TArray<int32>& OutIndices) const;

protected:
	
};
