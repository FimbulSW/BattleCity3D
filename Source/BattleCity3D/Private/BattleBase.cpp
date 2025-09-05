#include "BattleBase.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "../BattleGameMode.h"

ABattleBase::ABattleBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);

	// Visual placeholder
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Body->SetStaticMesh(Cube.Object);
		Body->SetRelativeScale3D(FVector(1.f, 1.f, 0.5f));
	}

	// QueryOnly: detectable por trazas, no bloquea
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
}

void ABattleBase::BeginPlay()
{
	Super::BeginPlay();
}

float ABattleBase::TakeDamage(float DamageAmount, const FDamageEvent& /*DamageEvent*/,
	AController* /*EventInstigator*/, AActor* /*DamageCauser*/)
{
	const int32 Dmg = FMath::Max(1, FMath::RoundToInt(DamageAmount));
	HitPoints -= Dmg;
	if (HitPoints <= 0)
	{
		NotifyDefeat();
		Destroy();
	}
	return DamageAmount;
}

void ABattleBase::NotifyDefeat()
{
	if (ABattleGameMode* GM = Cast<ABattleGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->HandleBaseDestroyed();
	}
}
