#pragma once
#include "CoreMinimal.h"
#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy.h"
#include "Components/ActorComponent.h"
#include "EnemyMovementComponent.generated.h"

class AEnemyPawn;
class UMapGridSubsystem;
class UEnemyMovePolicy;

static int32 Sign01(float V) { return (V > 0.f) ? +1 : (V < 0.f) ? -1 : 0; }

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BATTLECITY3D_API UEnemyMovementComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UEnemyMovementComponent();

    // === Policy Class + Instance (patrón robusto para editor)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Policy")
    TSubclassOf<UEnemyMovePolicy> MovePolicyClass;

    UPROPERTY(EditAnywhere, Instanced, Category = "Move|Policy", meta = (ShowOnlyInnerProperties))
    UEnemyMovePolicy* MovePolicy = nullptr;

    // === Tuning
    UPROPERTY(EditAnywhere, Category = "Move|Tuning") float MinLockTime = 0.30f;
    UPROPERTY(EditAnywhere, Category = "Move|Tuning", meta = (ClampMin = "0.01", ClampMax = "1.0")) float AlignEpsilonFactor = 0.25f;
    UPROPERTY(EditAnywhere, Category = "Move|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0")) float TieDeadbandFactor = 0.10f;
    UPROPERTY(EditAnywhere, Category = "Move|Tuning", meta = (ClampMin = "1", ClampMax = "3"))   int32 LookAheadTiles = 1;

    // Stop & Shoot frontal a Brick
    UPROPERTY(EditAnywhere, Category = "Move|Combat") bool  bPreferShootWhenFrontBrick = true;
    UPROPERTY(EditAnywhere, Category = "Move|Combat", meta = (ClampMin = "0.5", ClampMax = "3.0")) float FrontCheckTiles = 1.0f;

    // === Accesos para policies
    UFUNCTION(BlueprintCallable, Category = "Move") UMapGridSubsystem* GetGrid() const;
    UFUNCTION(BlueprintCallable, Category = "Move") AEnemyPawn* GetPawn() const { return CachedPawn.Get(); }

    // Helpers consultables por policies
    bool IsAheadBlocked(bool bAxisX, int Dir, float DistWorld) const;
    uint8 QueryFrontObstacle(float MaxDistanceWorld, FVector* OutHitWorld = nullptr) const; // 0=None,1=Brick,2=Steel
    uint8 CheckCardinalLineToTarget(const FVector& From, const FVector& To, FVector* OutFirstHitWorld = nullptr) const; // 0=NotCardinal,1=Clear,2=Brick,3=Steel
    bool  IsFireReady() const;

    // Aplicación de decisión
    void  ApplyDecision(const FMoveDecision& D);

    // Tick
    virtual void BeginPlay() override;
    virtual void TickComponent(float /*DT*/, enum ELevelTick /*TickType*/, FActorComponentTickFunction* /*ThisTickFunction*/) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& E) override;
#endif

private:
    TWeakObjectPtr<AEnemyPawn>        CachedPawn;
    TWeakObjectPtr<UMapGridSubsystem> CachedGrid;

    FVector2D LastFacingDir = FVector2D(1.f, 0.f);

    // Estado de lock
    EMoveLockAxis AxisLock = EMoveLockAxis::None;
    double        LockUntilTime = 0.0;

    float GetTileSizeSafe() const;
    void  EnsurePolicy();
};
