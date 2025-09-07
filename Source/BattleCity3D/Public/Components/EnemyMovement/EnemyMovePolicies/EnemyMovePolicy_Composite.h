#pragma once
#include "CoreMinimal.h"
#include "EnemyMovePolicy.h"
#include "EnemyMovePolicy_Composite.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType)
class BATTLECITY3D_API UEnemyMovePolicy_Composite : public UEnemyMovePolicy
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Instanced, Category = "Policies")
	TArray<TObjectPtr<UEnemyMovePolicy>> Policies;

	UPROPERTY(EditAnywhere, Category = "Policies")
	bool bStartFromEmpty = true;

	virtual void Initialize(UEnemyMovementComponent* InOwner) override;
	virtual void ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out) override;
};
