#pragma once
#include "CoreMinimal.h"
#include "Common/BattleTankPawn.h" // IMPORTANTE
#include "InputActionValue.h"
#include "TankPawn.generated.h"

class UInputAction;
class AProjectile;

UCLASS()
class BATTLECITY3D_API ATankPawn : public ABattleTankPawn
{
	GENERATED_BODY()
public:
	ATankPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") TSubclassOf<AProjectile> ProjectileClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") float FireCooldown = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input") UInputAction* IA_Move = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input") UInputAction* IA_Fire = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Health") int32 HitPoints = 1;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	void OnMove(const FInputActionValue& Value);
	void OnFire(const FInputActionValue& Value);
	void TryFire();

private:
	double LastFireTime = -1.0;
};