#include "EnemyPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MapGridSubsystem.h"
#include "../Projectile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "EnemyMovementComponent.h"


// Interno: aplica stats a partir de EnemyType actual
static void ApplyStatsFromType(EEnemyType Type, float& MoveSpeed, int32& HP, float& FireInterval)
{
	switch (Type)
	{
	case EEnemyType::Basic:   MoveSpeed = 900.f;  HP = 1; FireInterval = 1.4f; break;
	case EEnemyType::Fast:    MoveSpeed = 1300.f; HP = 1; FireInterval = 1.2f; break;
	case EEnemyType::Power:   MoveSpeed = 900.f;  HP = 2; FireInterval = 1.0f; break;
	case EEnemyType::Armored: MoveSpeed = 700.f;  HP = 4; FireInterval = 1.6f; break;
	}
}


AEnemyPawn::AEnemyPawn()
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
	Muzzle->SetRelativeLocation(FVector(50.f, 0.f, 40.f));

	MovementComp = CreateDefaultSubobject<UEnemyMovementComponent>(TEXT("EnemyMovement"));

}

void AEnemyPawn::ApplyHit(int32 Dmg)
{
	HitPoints -= FMath::Max(1, Dmg);
	if (HitPoints <= 0) Destroy();
}

void AEnemyPawn::BeginPlay()
{
	Super::BeginPlay();
	Grid = GetGameInstance()->GetSubsystem<UMapGridSubsystem>();

	// Si el BP deja bAcceptTypeFromSpawner=false, NO tocamos stats aquí
	if (bAcceptTypeFromSpawner)
	{
		int32 HP = HitPoints; float MS = MoveSpeed; float FI = FireInterval;
		ApplyStatsFromType(EnemyType, MS, HP, FI);
		MoveSpeed = MS; HitPoints = HP; FireInterval = FI;
	}

	if (ProjectileClass)
	{
		GetWorldTimerManager().SetTimer(FireTimer, this, &AEnemyPawn::Fire, FireInterval, true, FireInterval);
	}
}

void AEnemyPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!MovementComp) { UpdateAI(DeltaSeconds); }
	UpdateMovement(DeltaSeconds);
}

void AEnemyPawn::UpdateAI(float DT)
{
	// === META -> vector deseado con Axis-Lock + histéresis ===
	const FVector Target = GetAITargetWorld();
	const FVector To = Target - GetActorLocation();

	const float Tile = Grid ? Grid->GetTileSize() : 200.f;
	const float eps = AlignEpsilonFactor * Tile;
	const float db = TieDeadbandFactor * Tile;

	const bool AlignedX = FMath::Abs(To.X) <= eps;
	const bool AlignedY = FMath::Abs(To.Y) <= eps;

	auto IsAheadBlocked = [&](bool bAxisX, int Dir)->bool
		{
			const FVector Center = Grid ? Grid->SnapWorldToSubgrid(GetActorLocation(), true) : GetActorLocation();
			return IsBlockedAhead(Center, bAxisX, Dir, Tile * LookAheadTiles);
		};

	// 1) Mantener eje si lock activo y sin bloqueo real
	const double Now = UGameplayStatics::GetTimeSeconds(this);
	if (AxisLock != EAxisLock::None && Now < LockUntilTime)
	{
		const bool bAxisX = (AxisLock == EAxisLock::X);
		const int  Dir = bAxisX ? FMath::Sign(To.X == 0 ? FacingDir.X : To.X)
			: FMath::Sign(To.Y == 0 ? FacingDir.Y : To.Y);
		if (!IsAheadBlocked(bAxisX, Dir))
		{
			RawMoveInput = bAxisX ? FVector2D(Dir, 0.f) : FVector2D(0.f, Dir);
			return;
		}
		// si está bloqueado, liberamos el lock y reevaluamos
		AxisLock = EAxisLock::None;
	}

	// 2) Elegir eje
	FVector2D Desired = FacingDir;

	// Priorizamos el eje no alineado; si ambos no alineados, aplicamos deadband
	if (!AlignedX)
	{
		const int DirX = FMath::Sign(To.X);
		if (!IsAheadBlocked(true, DirX))
		{
			Desired = FVector2D(DirX, 0.f);
			AxisLock = EAxisLock::X;
		}
		else if (!AlignedY)
		{
			const int DirY = FMath::Sign(To.Y);
			if (!IsAheadBlocked(false, DirY))
			{
				Desired = FVector2D(0.f, DirY);
				AxisLock = EAxisLock::Y;
			}
		}
	}
	else if (!AlignedY)
	{
		const int DirY = FMath::Sign(To.Y);
		if (!IsAheadBlocked(false, DirY))
		{
			Desired = FVector2D(0.f, DirY);
			AxisLock = EAxisLock::Y;
		}
		else
		{
			const int DirX = FMath::Sign(To.X);
			if (!IsAheadBlocked(true, DirX))
			{
				Desired = FVector2D(DirX, 0.f);
				AxisLock = EAxisLock::X;
			}
		}
	}
	else
	{
		// ambos casi alineados: evita ping-pong con deadband
		const float dmag = FMath::Abs(FMath::Abs(To.X) - FMath::Abs(To.Y));
		if (dmag <= db)
		{
			// mantener facing actual
			Desired = FacingDir;
			AxisLock = (FMath::Abs(FacingDir.X) > 0.5f) ? EAxisLock::X : (FMath::Abs(FacingDir.Y) > 0.5f ? EAxisLock::Y : EAxisLock::None);
		}
		else if (FMath::Abs(To.X) > FMath::Abs(To.Y))
		{
			const int DirX = FMath::Sign(To.X);
			if (!IsAheadBlocked(true, DirX)) { Desired = FVector2D(DirX, 0.f); AxisLock = EAxisLock::X; }
		}
		else
		{
			const int DirY = FMath::Sign(To.Y);
			if (!IsAheadBlocked(false, DirY)) { Desired = FVector2D(0.f, DirY); AxisLock = EAxisLock::Y; }
		}
	}

	// 3) Activar lock por ventana mínima
	if (AxisLock != EAxisLock::None)
	{
		LockUntilTime = Now + MinLockTime;
	}


	// Centro/tile actual
	int32 CX, CY; const float T = Grid ? Grid->GetTileSize() : 200.f;
	const bool bHaveCell = (Grid && Grid->WorldToGrid(GetActorLocation(), CX, CY));
	const FVector Center = bHaveCell ? Grid->GridToWorld(CX, CY, T * 0.5f) : GetActorLocation();

	bool bBlocked = false;
	if (Grid && !Desired.IsNearlyZero())
	{
		const bool bAxisX = FMath::Abs(Desired.X) > 0.f;
		const int Dir = bAxisX ? (Desired.X > 0 ? +1 : -1) : (Desired.Y > 0 ? +1 : -1);
		bBlocked = IsBlockedAhead(Center, bAxisX, Dir, T);
	}

	if (bBlocked && Now - LastDecisionTime >= DecisionCooldown)
	{
		LastDecisionTime = Now;
		ChooseTurn(); // elige izquierda/derecha o vertical/horizontal alternos
	}
	else
	{
		RawMoveInput = Desired;
	}
}

bool AEnemyPawn::IsBlockedAhead(const FVector& Center, bool bAxisX, int Dir, float T) const
{
	if (!Grid) return false;
	const FVector Probe = bAxisX
		? FVector(Center.X + Dir * (T * 0.51f), Center.Y, Center.Z)
		: FVector(Center.X, Center.Y + Dir * (T * 0.51f), Center.Z);
	return !Grid->IsPassableForPawnAtWorld(Probe);
}

bool AEnemyPawn::CanMoveTowards(const FVector2D& Axis) const
{
	if (!Grid) return true;
	const float T = Grid->GetTileSize();
	const FVector Pos = GetActorLocation() + FVector(Axis.X * T * 0.5f, Axis.Y * T * 0.5f, 0.f);
	return Grid->IsPassableForPawnAtWorld(Pos);
}

void AEnemyPawn::ChooseTurn()
{
	// prioridades: derecha, izquierda, atrás (en el eje ortogonal primero)
	TArray<FVector2D> Options;

	if (FMath::Abs(RawMoveInput.X) > 0.f)
	{
		Options = { FVector2D(0, +1), FVector2D(0, -1), FVector2D(-RawMoveInput.X, 0) };
	}
	else
	{
		Options = { FVector2D(+1, 0), FVector2D(-1, 0), FVector2D(0, -RawMoveInput.Y) };
	}

	for (const FVector2D& O : Options)
	{
		if (CanMoveTowards(O))
		{
			RawMoveInput = O;
			return;
		}
	}
	// Si nada es posible, nos quedamos y el movimiento cortará en borde
}

void AEnemyPawn::UpdateMovement(float DT)
{
	// Rotación por dirección
	if (!RawMoveInput.IsNearlyZero())
	{
		FacingDir = RawMoveInput;
		const float Yaw = (FacingDir.X > 0 ? 0.f :
			(FacingDir.X < 0 ? 180.f :
				(FacingDir.Y > 0 ? 90.f : -90.f)));
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}

	// Suavizado de velocidad
	if (!RawMoveInput.IsNearlyZero())
		Velocity = FMath::Vector2DInterpTo(Velocity, RawMoveInput * MoveSpeed, DT, AccelRate);
	else
		Velocity = FMath::Vector2DInterpTo(Velocity, FVector2D::ZeroVector, DT, DecelRate);

	if (!Grid)
	{
		AddActorWorldOffset(FVector(Velocity.X, Velocity.Y, 0.f) * DT, true);
		return;
	}

	// Centro tile actual
	int32 CX, CY; const float T = Grid->GetTileSize();
	const bool bHaveCell = Grid->WorldToGrid(GetActorLocation(), CX, CY);
	const FVector Center = bHaveCell ? Grid->GridToWorld(CX, CY, T * 0.5f) : GetActorLocation();

	// Corte en borde según eje dominante
	const bool bXDominant = (FMath::Abs(Velocity.X) >= FMath::Abs(Velocity.Y));
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

	float dX = 0, dY = 0;
	if (bXDominant)  AdvanceAxis(true, Velocity.X, dX, dY);
	else             AdvanceAxis(false, Velocity.Y, dX, dY);

	FHitResult Hit;
	AddActorWorldOffset(FVector(dX, dY, 0.f), true, &Hit);

	// Recalcular tile nuevo
	int32 NCX, NCY;
	const bool bHaveNewCell = Grid->WorldToGrid(GetActorLocation(), NCX, NCY);
	const FVector NewCenter = bHaveNewCell ? Grid->GridToWorld(NCX, NCY, T * 0.5f) : GetActorLocation();

	// Snap SIEMPRE al subgrid (ambos ejes) + clamp dentro del tile nuevo
	FVector P = GetActorLocation();
	P = Grid->SnapWorldToSubgrid(P, /*bKeepZ*/true);

	const float SubStep = Grid->GetSubStep();
	const float LocalStopMargin = FMath::Max(StopMargin, SubStep * 0.25f);
	const float Half = T * 0.5f - LocalStopMargin;
	P.X = FMath::Clamp(P.X, NewCenter.X - Half, NewCenter.X + Half);
	P.Y = FMath::Clamp(P.Y, NewCenter.Y - Half, NewCenter.Y + Half);

	SetActorLocation(FVector(P.X, P.Y, NewCenter.Z));
}

void AEnemyPawn::Fire()
{
	if (!ProjectileClass) return;
	// Solo disparamos si hay alineación cardinal y LOS limpio hacia la meta actual
	const FVector Target = GetAITargetWorld();
	FVector FirstHit;
	const ECardinalLOS LOS = CheckCardinalLineToTarget(GetActorLocation(), Target, &FirstHit);

	if (LOS == ECardinalLOS::NotCardinal)
	{
		return; // no estamos alineados, evita “spam”
	}
	if (LOS == ECardinalLOS::Steel)
	{
		return; // acero: no sirve disparar
	}

	const FVector SpawnLoc = Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation() + FVector(50, 0, 40);
	const FRotator SpawnRot = GetActorRotation();
	FActorSpawnParameters P; P.Owner = this; P.Instigator = this;
	if (AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLoc, SpawnRot, P))
	{
		projectile->Team = EProjectileTeam::Enemy;
	}
}

void AEnemyPawn::SetTypeFromSpawner(EEnemyType InType)
{
	if (!bAcceptTypeFromSpawner) return;
	EnemyType = InType;
	// Reconfigura stats SOLO si queremos aceptar del spawner
	int32 HP = HitPoints; float MS = MoveSpeed; float FI = FireInterval;
	ApplyStatsFromType(EnemyType, MS, HP, FI);
	MoveSpeed = MS; HitPoints = HP; FireInterval = FI;

	// Si ya había timer de disparo, lo reprogramamos
	if (ProjectileClass)
	{
		GetWorldTimerManager().ClearTimer(FireTimer);
		GetWorldTimerManager().SetTimer(FireTimer, this, &AEnemyPawn::Fire, FireInterval, true, FireInterval);
	}
}

float AEnemyPawn::TakeDamage(float DamageAmount, const FDamageEvent& /*DamageEvent*/,
	AController* /*EventInstigator*/, AActor* /*DamageCauser*/)
{
	const int32 Dmg = FMath::Max(1, FMath::RoundToInt(DamageAmount));
	ApplyHit(Dmg);
	return DamageAmount;
}

FVector AEnemyPawn::GetAITargetWorld() const
{
	// 1) Base si la meta es base y existe
	if (Goal == EEnemyGoal::HuntBase && Grid && Grid->HasBase())
		return Grid->GetBaseWorldLocation();

	// 2) Jugador si existe (meta HuntPlayer o fallback)
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		return Player->GetActorLocation();

	// 3) Fallback inverso: si no hay jugador pero hay base
	if (Grid && Grid->HasBase())
		return Grid->GetBaseWorldLocation();

	// 4) Último recurso
	return GetActorLocation();
}

AEnemyPawn::ECardinalLOS AEnemyPawn::CheckCardinalLineToTarget(const FVector& From, const FVector& To, FVector* OutFirstHitWorld) const
{
	if (!Grid) 
		return ECardinalLOS::Clear;

	const float Tile = Grid->GetTileSize();
	const float eps = AlignEpsilonFactor * Tile;

	// ¿Cardinal “suficiente”?
	const bool CardinalX = FMath::Abs(From.Y - To.Y) <= eps;
	const bool CardinalY = FMath::Abs(From.X - To.X) <= eps;
	if (!CardinalX && !CardinalY) return ECardinalLOS::NotCardinal;

	// Trabajamos en coordenadas de GRID
	int32 SX, SY, EX, EY;
	if (!Grid->WorldToGrid(From, SX, SY) || !Grid->WorldToGrid(To, EX, EY))
		return ECardinalLOS::NotCardinal;

	// Elegir eje y dirección
	if (CardinalX)
	{
		const int Dir = (EX >= SX) ? 1 : -1;
		for (int x = SX + Dir; x != EX + Dir; x += Dir)
		{
			const EObstacleType O = Grid->GetObstacleAtGrid(x, SY);
			if (O == EObstacleType::Brick || O == EObstacleType::Steel)
			{
				if (OutFirstHitWorld) *OutFirstHitWorld = Grid->GridToWorld(x, SY, Tile * 0.5f);
				return (O == EObstacleType::Brick) ? ECardinalLOS::Brick : ECardinalLOS::Steel;
			}
		}
	}
	else // CardinalY
	{
		const int Dir = (EY >= SY) ? 1 : -1;
		for (int y = SY + Dir; y != EY + Dir; y += Dir)
		{
			const EObstacleType O = Grid->GetObstacleAtGrid(SX, y);
			if (O == EObstacleType::Brick || O == EObstacleType::Steel)
			{
				if (OutFirstHitWorld) *OutFirstHitWorld = Grid->GridToWorld(SX, y, Tile * 0.5f);
				return (O == EObstacleType::Brick) ? ECardinalLOS::Brick : ECardinalLOS::Steel;
			}
		}
	}
	return ECardinalLOS::Clear;
}

bool AEnemyPawn::IsFireReady() const
{
	const UWorld* W = GetWorld();
	if (!W) return false;
	const FTimerManager& TM = W->GetTimerManager();
	// Si no hay timer activo, o le queda ~nada, consideramos listo
	if (!TM.IsTimerActive(FireTimer)) return true;
	return (TM.GetTimerRemaining(FireTimer) <= 0.01f);
}
