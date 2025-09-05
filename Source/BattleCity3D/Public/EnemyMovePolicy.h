#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EnemyMovePolicy.generated.h"

class UEnemyMovementComponent;

UENUM()
enum class EMoveLockAxis : uint8 { None, X, Y };

USTRUCT(BlueprintType)
struct FMoveContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FVector  Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector2D FacingDir = FVector2D(1, 0);
    UPROPERTY(BlueprintReadOnly) FVector  TargetWorld = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly) float TileSize = 200.f;
    UPROPERTY(BlueprintReadOnly) float AlignEpsilon = 50.f;
    UPROPERTY(BlueprintReadOnly) float TieDeadband = 20.f;
    UPROPERTY(BlueprintReadOnly) int32 LookAheadTiles = 1;

    // Estado
    UPROPERTY(BlueprintReadOnly) bool bFireReady = false;

    // Consultas (implementadas por el MovementComponent)
    TFunction<bool(bool /*bAxisX*/, int /*Dir*/, float /*DistWorld*/)> IsAheadBlocked;
    TFunction<uint8(float /*DistWorld*/, FVector* /*OutHitWorld*/)>     FrontObstacle; // 0=None,1=Brick,2=Steel
    TFunction<uint8(const FVector& /*From*/, const FVector& /*To*/, FVector* /*OutFirstHit*/)> CardinalLineToTarget; // 0=NotCardinal,1=Clear,2=Brick,3=Steel
};

USTRUCT(BlueprintType)
struct FMoveDecision
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FVector2D RawMoveInput = FVector2D::ZeroVector; // {-1,0,1} en ejes
    UPROPERTY(BlueprintReadWrite) EMoveLockAxis LockAxis = EMoveLockAxis::None;
    UPROPERTY(BlueprintReadWrite) float LockTime = 0.f;
    UPROPERTY(BlueprintReadWrite) bool bRequestFrontShot = false; // pedir disparo frontal este tick
    UPROPERTY(BlueprintReadWrite) FString DebugText;
};

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class BATTLECITY3D_API UEnemyMovePolicy : public UObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(UEnemyMovementComponent* InOwner);
    virtual void ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out) PURE_VIRTUAL(UEnemyMovePolicy::ComputeMove, );

protected:
    UPROPERTY(Transient) TWeakObjectPtr<UEnemyMovementComponent> Owner;
};
