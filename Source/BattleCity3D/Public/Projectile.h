#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UMapGridSubsystem;

UENUM(BlueprintType)
enum class EProjectileTeam : uint8 { Player, Enemy };

UCLASS()
class BATTLECITY3D_API AProjectile : public AActor
{
	GENERATED_BODY()
public:
	AProjectile();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	EProjectileTeam Team = EProjectileTeam::Player;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float ManualTraceRadius = 12.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void DoManualSweep(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	USphereComponent* Collision = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UStaticMeshComponent* Visual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UProjectileMovementComponent* Movement = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage = 1;

	FVector LastLocation = FVector::ZeroVector;

	UPROPERTY() UMapGridSubsystem* Grid = nullptr;
};
