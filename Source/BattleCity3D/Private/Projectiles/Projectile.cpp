#include "Projectiles/Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Map/MapGridSubsystem.h"

#include "Player/TankPawn.h"
#include "Enemies/EnemyPawn.h"
#include "BattleBases/BattleBase.h"
#include "Kismet/GameplayStatics.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(12.f);

	// QueryOnly: no bloquea físicamente, sólo sirve para trazas
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);

	// Respuestas: bloquea consultas en WorldStatic/Dynamic (para que los sweeps detecten),
	// pero puedes ignorar Pawns si quieres evitar stops por físico (ya no hay físico)
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	Collision->SetNotifyRigidBodyCollision(false);
	RootComponent = Collision;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(RootComponent);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		if (SphereMesh.Succeeded())
		{
			Visual->SetStaticMesh(SphereMesh.Object);
			Visual->SetRelativeScale3D(FVector(0.5f));
		}
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 3000.f;
	Movement->MaxSpeed = 3000.f;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;

	Movement->bSweepCollision = false;
	Movement->bForceSubStepping = true;
	Movement->MaxSimulationTimeStep = 1.f / 120.f;
	Movement->MaxSimulationIterations = 8;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	LastLocation = GetActorLocation();
	if (AActor* Inst = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Inst, true);
	}
	Grid = GetGameInstance()->GetSubsystem<UMapGridSubsystem>();
}

void AProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Grid) return;

	// 1. Detección Volumétrica contra el Grid
	// Un radio de 15.0f garantiza que si pasa entre dos tiles (unión), toque ambos.
	float ProjRadius = 15.0f;

	// Usamos la posición actual.
	if (Grid->ProcessProjectileHit(GetActorLocation(), ProjRadius, GetInstigator()))
	{
		// Aquí podrías spawnear una explosión (Emitter)
		Destroy();
		return;
	}

	// 2. Colisión con Actores Dinámicos (Tanques, Base, otras Balas)
	// Mantenemos tu barrido manual original para esto
	DoManualSweep(DeltaSeconds);

	LastLocation = GetActorLocation();
}

void AProjectile::DoManualSweep(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f) return;

	const FVector Start = LastLocation.IsNearlyZero() ? GetActorLocation() : LastLocation;
	const FVector End = GetActorLocation();
	if ((End - Start).IsNearlyZero()) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileManualSweep), false, this);
	Params.bTraceComplex = false;
	Params.bReturnPhysicalMaterial = false;
	Params.AddIgnoredActor(this);
	if (AActor* Inst = GetInstigator()) Params.AddIgnoredActor(Inst);

	// Dinámicos: proyectiles, jugador, enemigos, base
	TArray<FHitResult> Hits;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(ManualTraceRadius);
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_WorldDynamic, Shape, Params);

	for (const FHitResult& Hit : Hits)
	{
		AActor* Other = Hit.GetActor();
		if (!Other) continue;

		// Proyectil vs Proyectil -> destrucción mutua
		if (Other->IsA(AProjectile::StaticClass()))
		{
			if (AProjectile* OtherProj = Cast<AProjectile>(Other))
			{
				OtherProj->Destroy();
			}
			Destroy();
			return;
		}

		// Base (águila): cualquier facción la puede dañar (ajustable si lo deseas)
		if (Other->IsA(ABattleBase::StaticClass()))
		{
			UGameplayStatics::ApplyDamage(Other, Damage, GetInstigatorController(), this, nullptr);
			Destroy();
			return;
		}

		// Jugador / Enemigo según facción
		const bool HitEnemy = (Team == EProjectileTeam::Player && Other->IsA(AEnemyPawn::StaticClass()));
		const bool HitPlayer = (Team == EProjectileTeam::Enemy && Other->IsA(ATankPawn::StaticClass()));
		if (HitEnemy || HitPlayer)
		{
			UGameplayStatics::ApplyDamage(Other, Damage, GetInstigatorController(), this, nullptr);
			Destroy();
			return;
		}

		// Otro dinámico: destruye bala
		if (Other != GetInstigator())
		{
			Destroy();
			return;
		}
	}

	// Estáticos: si pega algo inusual, destruye
	FHitResult HitStatic;
	if (GetWorld()->SweepSingleByChannel(HitStatic, Start, End, FQuat::Identity, ECC_WorldStatic, Shape, Params))
	{
		Destroy();
		return;
	}
}



