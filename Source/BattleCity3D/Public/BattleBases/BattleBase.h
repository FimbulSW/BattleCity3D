#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleBase.generated.h"

class UStaticMeshComponent;

UCLASS()
class BATTLECITY3D_API ABattleBase : public AActor
{
	GENERATED_BODY()

public:
	ABattleBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	int32 HitPoints = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	bool bIsEnemyBase = false;

	// Daño genérico
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) UStaticMeshComponent* Body;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
private:
	void NotifyDefeat();
	void DrawDebugBounds();
};
