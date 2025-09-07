#include "Player/TankPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "Map/MapGridSubsystem.h"
#include "Projectiles/Projectile.h"

ATankPawn::ATankPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);

	// QueryOnly: no bloquea movimiento, pero las trazas lo “ven”
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);


	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Body);
	Muzzle->SetRelativeLocation(FVector(50, 0, 0));
}

void ATankPawn::BeginPlay()
{
	Super::BeginPlay();
	Grid = GetGameInstance()->GetSubsystem<UMapGridSubsystem>();
}

void ATankPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMovement(DeltaSeconds);
}

void ATankPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Move)
		{
			EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ATankPawn::OnMove);
			EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &ATankPawn::OnMove);
		}
		if (IA_Fire)
		{
			EIC->BindAction(IA_Fire, ETriggerEvent::Triggered, this, &ATankPawn::OnFire);
		}
	}
}

void ATankPawn::OnMove(const FInputActionValue& Value)
{
	FVector2D V = Value.Get<FVector2D>();
	// 4 direcciones (eje dominante)
	if (FMath::Abs(V.X) >= FMath::Abs(V.Y))
	{
		if (FMath::Abs(V.X) > 0.f) V = FVector2D(FMath::Sign(V.X), 0.f);
		else                        V = FVector2D::ZeroVector;
	}
	else
	{
		if (FMath::Abs(V.Y) > 0.f) V = FVector2D(0.f, FMath::Sign(V.Y));
		else                        V = FVector2D::ZeroVector;
	}
	RawMoveInput = V;
}

void ATankPawn::OnFire(const FInputActionValue& Value)
{
	TryFire();
}

void ATankPawn::TryFire()
{
	if (!ProjectileClass) return;
	const double Now = UGameplayStatics::GetTimeSeconds(this);
	if (Now - LastFireTime < FireCooldown) return;
	LastFireTime = Now;

	const FVector SpawnLoc = Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation() + FVector(50, 0, 0);
	const FRotator SpawnRot = GetActorRotation();

	FActorSpawnParameters P; P.Owner = this; P.Instigator = this;
	if (AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLoc, SpawnRot, P))
	{
		projectile->Team = EProjectileTeam::Player;
	}
}

float ATankPawn::GetAccelRateCurrent() const
{
	const bool bIce = (Grid && Grid->GetTerrainAtWorld(GetActorLocation()) == ETerrainType::Ice);
	return bIce ? AccelRateIce : AccelRate;
}
float ATankPawn::GetDecelRateCurrent() const
{
	const bool bIce = (Grid && Grid->GetTerrainAtWorld(GetActorLocation()) == ETerrainType::Ice);
	return bIce ? DecelRateIce : DecelRate;
}

bool ATankPawn::GetCurrentCell(int32& OutX, int32& OutY) const
{
	if (!Grid) return false;
	return Grid->WorldToGrid(GetActorLocation(), OutX, OutY);
}
FVector ATankPawn::GetCellCenterWorld(int32 X, int32 Y) const
{
	if (!Grid) return GetActorLocation();
	const float T = Grid->GetTileSize();
	return Grid->GridToWorld(X, Y, T * 0.5f);
}

void ATankPawn::UpdateMovement(float DT)
{
	// 1) Direccionalidad y rotación
	const bool bWantsMove = !RawMoveInput.IsNearlyZero();
	if (bWantsMove)
	{
		FacingDir = RawMoveInput;
		const float Yaw = (FacingDir.X > 0 ? 0.f :
			(FacingDir.X < 0 ? 180.f :
				(FacingDir.Y > 0 ? 90.f : -90.f)));
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}

	// 2) Velocidad (hielo/normal)
	const float A = GetAccelRateCurrent();
	const float D = GetDecelRateCurrent();
	if (bWantsMove)  Velocity = FMath::Vector2DInterpTo(Velocity, RawMoveInput * MoveSpeed, DT, A);
	else             Velocity = FMath::Vector2DInterpTo(Velocity, FVector2D::ZeroVector, DT, D);

	if (!Grid)
	{
		AddActorWorldOffset(FVector(Velocity.X, Velocity.Y, 0.f) * DT, true);
		return;
	}

	// 3) Centro de tile actual
	int32 CX, CY;
	const bool bHaveCell = Grid->WorldToGrid(GetActorLocation(), CX, CY);
	const float T = Grid->GetTileSize();
	const FVector Center = bHaveCell ? Grid->GridToWorld(CX, CY, T * 0.5f) : GetActorLocation();

	// 4) Avance con corte en borde (usa center del tile actual)
	auto AdvanceAxis = [&](bool bAxisX, float VComp, float& OutDX, float& OutDY)
		{
			OutDX = 0.f; OutDY = 0.f;
			if (FMath::IsNearlyZero(VComp)) return;

			const int Dir = (VComp > 0.f) ? +1 : -1;
			const float Delta = VComp * DT;

			if (bAxisX)
			{
				const bool BlockedFront = !Grid->IsPassableForPawnAtWorld(FVector(Center.X + Dir * (T * 0.51f), Center.Y, Center.Z));
				const float Edge = Center.X + Dir * (T * 0.5f - StopMargin);
				const float NextX = GetActorLocation().X + Delta;
				OutDX = Delta;
				if (BlockedFront && ((Dir > 0 && NextX > Edge) || (Dir < 0 && NextX < Edge)))
					OutDX = Edge - GetActorLocation().X;
			}
			else
			{
				const bool BlockedFront = !Grid->IsPassableForPawnAtWorld(FVector(Center.X, Center.Y + Dir * (T * 0.51f), Center.Z));
				const float Edge = Center.Y + Dir * (T * 0.5f - StopMargin);
				const float NextY = GetActorLocation().Y + Delta;
				OutDY = Delta;
				if (BlockedFront && ((Dir > 0 && NextY > Edge) || (Dir < 0 && NextY < Edge)))
					OutDY = Edge - GetActorLocation().Y;
			}
		};

	const bool bXDominant = (FMath::Abs(Velocity.X) >= FMath::Abs(Velocity.Y));
	float dX = 0.f, dY = 0.f;
	if (bXDominant)  AdvanceAxis(true, Velocity.X, dX, dY);
	else             AdvanceAxis(false, Velocity.Y, dX, dY);

	FHitResult Hit;
	AddActorWorldOffset(FVector(dX, dY, 0.f), true, &Hit);

	// 5) Recalcula tile NUEVO tras mover
	int32 NCX, NCY;
	const bool bHaveNewCell = Grid->WorldToGrid(GetActorLocation(), NCX, NCY);
	const FVector NewCenter = bHaveNewCell ? Grid->GridToWorld(NCX, NCY, T * 0.5f) : GetActorLocation();

	// 6) Snap SIEMPRE al subgrid en X e Y (no al centro del tile)
	FVector P = GetActorLocation();
	P = Grid->SnapWorldToSubgrid(P, /*bKeepZ*/true);

	// 7) Clampeo dentro del tile NUEVO para no cruzar paredes al snappear
	const float SubStep = Grid->GetSubStep();
	const float LocalStopMargin = FMath::Max(StopMargin, SubStep * 0.25f); // margen acorde al subgrid
	const float Half = T * 0.5f - LocalStopMargin;

	P.X = FMath::Clamp(P.X, NewCenter.X - Half, NewCenter.X + Half);
	P.Y = FMath::Clamp(P.Y, NewCenter.Y - Half, NewCenter.Y + Half);

	// 8) Fija Z al centro del tile
	SetActorLocation(FVector(P.X, P.Y, NewCenter.Z));
}

float ATankPawn::TakeDamage(float DamageAmount, const FDamageEvent& /*DamageEvent*/,
	AController* /*EventInstigator*/, AActor* /*DamageCauser*/)
{
	const int32 Dmg = FMath::Max(1, FMath::RoundToInt(DamageAmount));
	HitPoints -= Dmg;
	if (HitPoints <= 0)
	{
		Destroy(); // luego podemos hacer respawn/vidas
	}
	return DamageAmount;
}