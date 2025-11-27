#include "Enemies/EnemyPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Map/MapGridSubsystem.h"
#include "Projectiles/Projectile.h"
#include "DrawDebugHelpers.h"
#include "Components/EnemyMovement/EnemyMovementComponent.h"


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

	// SNAP AL NACER PARA EVITAR ATASCOS INICIALES
	if (Grid)
	{
		FVector P = GetActorLocation();
		P = Grid->SnapWorldToSubgrid(P, true);
		SetActorLocation(P);
	}
}

void AEnemyPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!MovementComp) { UpdateAI(DeltaSeconds); }
	UpdateMovement(DeltaSeconds);

	// --- DEBUG CAJA DE COLISIÓN ---
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("bc.collision.debug"));
	if (Grid && CVar && CVar->GetInt() > 0)
	{
		const float TileSize = Grid->GetTileSize();
		const float Extent = TileSize * 0.96f;
		FVector Pos = GetActorLocation();

		bool bOverlapping = false;
		FVector Corners[4] = {
			Pos + FVector(Extent, Extent, 0), Pos + FVector(Extent, -Extent, 0),
			Pos + FVector(-Extent, Extent, 0), Pos + FVector(-Extent, -Extent, 0)
		};
		for (const FVector& P : Corners) if (Grid->IsPointBlocked(P)) { bOverlapping = true; break; }

		FColor Color = bOverlapping ? FColor::Red : FColor::Green;
		DrawDebugBox(GetWorld(), Pos + FVector(0, 0, 10), FVector(Extent, Extent, 55.f), Color, false, -1.f, 0, 2.0f);
	}
}

void AEnemyPawn::UpdateAI(float DT)
{
	const double Now = UGameplayStatics::GetTimeSeconds(this);

	// 1. CHEQUEO DE BLOQUEO CONSTANTE (Prioridad Alta)
	// Necesitamos saber SIEMPRE si estamos bloqueados para frenar, aunque no cambiemos de decisión aún.
	// (Esto evita atravesar paredes mientras esperamos el timer)

	const float Tile = Grid ? Grid->GetTileSize() : 200.f;
	int32 CX, CY;
	bool bHaveCell = (Grid && Grid->WorldToGrid(GetActorLocation(), CX, CY));
	const FVector Center = bHaveCell ? Grid->GridToWorld(CX, CY, Tile * 0.5f) : GetActorLocation();

	// Verificar si la dirección ACTUAL nos lleva a un choque inmediato
	bool bIsBlockedNow = false;
	if (Grid && !RawMoveInput.IsNearlyZero())
	{
		const bool bAxisX = FMath::Abs(RawMoveInput.X) > 0.f;
		const int Dir = bAxisX ? (RawMoveInput.X > 0 ? +1 : -1) : (RawMoveInput.Y > 0 ? +1 : -1);

		// Usamos LookAheadTiles para anticipar
		bIsBlockedNow = IsBlockedAhead(Center, bAxisX, Dir, Tile * LookAheadTiles);
	}

	// 2. DECISIÓN DE IA (Throttled / Con Freno de mano)
	// Solo recalculamos si:
	// A) Ya pasó el tiempo de espera (Timer vencido)
	// B) O estamos bloqueados AHORA MISMO (Emergencia, hay que girar ya)
	if (Now < NextAIDecisionTime && !bIsBlockedNow)
	{
		return; // Mantener decisión anterior
	}

	// --- FASE DE PENSAMIENTO ---

	// A. Si estamos bloqueados, forzamos un giro evasivo
	if (bIsBlockedNow)
	{
		ChooseTurn();
		// ChooseTurn ya actualiza RawMoveInput y establece un tiempo de compromiso largo
		return;
	}

	// B. Si estamos libres, perseguimos el objetivo (Lógica de Ejes)

	// Reseteamos el timer para pensamiento normal (más rápido que cuando choca)
	NextAIDecisionTime = Now + AIReplanInterval;

	const FVector Target = GetAITargetWorld();
	const FVector To = Target - GetActorLocation();
	const float eps = AlignEpsilonFactor * Tile;
	const float db = TieDeadbandFactor * Tile;

	const bool AlignedX = FMath::Abs(To.X) <= eps;
	const bool AlignedY = FMath::Abs(To.Y) <= eps;

	// Lambda local para verificar si una dirección propuesta está libre
	auto IsDirSafe = [&](bool bAxisX, int Dir)->bool
		{
			const FVector TestCenter = Grid ? Grid->SnapWorldToSubgrid(GetActorLocation(), true) : GetActorLocation();
			return !IsBlockedAhead(TestCenter, bAxisX, Dir, Tile * LookAheadTiles);
		};

	FVector2D NewMoveDir = FacingDir; // Por defecto mantener

	// Lógica de selección de eje (idéntica a tu anterior, pero ahora solo ocurre cada X segundos)
	if (!AlignedX)
	{
		const int DirX = FMath::Sign(To.X);
		if (IsDirSafe(true, DirX))
		{
			NewMoveDir = FVector2D(DirX, 0.f);
			AxisLock = EAxisLock::X;
		}
		else if (!AlignedY)
		{
			const int DirY = FMath::Sign(To.Y);
			if (IsDirSafe(false, DirY))
			{
				NewMoveDir = FVector2D(0.f, DirY);
				AxisLock = EAxisLock::Y;
			}
		}
	}
	else if (!AlignedY)
	{
		const int DirY = FMath::Sign(To.Y);
		if (IsDirSafe(false, DirY))
		{
			NewMoveDir = FVector2D(0.f, DirY);
			AxisLock = EAxisLock::Y;
		}
		else
		{
			const int DirX = FMath::Sign(To.X);
			if (IsDirSafe(true, DirX))
			{
				NewMoveDir = FVector2D(DirX, 0.f);
				AxisLock = EAxisLock::X;
			}
		}
	}
	else
	{
		// Ambos alineados (cerca de la meta o en diagonal perfecta)
		const float dmag = FMath::Abs(FMath::Abs(To.X) - FMath::Abs(To.Y));
		if (dmag > db) // Solo cambiar si hay clara diferencia
		{
			if (FMath::Abs(To.X) > FMath::Abs(To.Y))
			{
				const int DirX = FMath::Sign(To.X);
				if (IsDirSafe(true, DirX)) { NewMoveDir = FVector2D(DirX, 0.f); AxisLock = EAxisLock::X; }
			}
			else
			{
				const int DirY = FMath::Sign(To.Y);
				if (IsDirSafe(false, DirY)) { NewMoveDir = FVector2D(0.f, DirY); AxisLock = EAxisLock::Y; }
			}
		}
	}

	// Aplicar cambios
	if (AxisLock != EAxisLock::None) LockUntilTime = Now + MinLockTime;
	RawMoveInput = NewMoveDir;
}

void AEnemyPawn::ChooseTurn()
{
	// Prioridades: derecha, izquierda, atrás (en el eje ortogonal primero)
	TArray<FVector2D> Options;

	// Si nos movíamos en X, probar Y. Si nos movíamos en Y, probar X.
	if (FMath::Abs(RawMoveInput.X) > 0.f)
	{
		Options = { FVector2D(0, +1), FVector2D(0, -1), FVector2D(-RawMoveInput.X, 0) };
	}
	else
	{
		Options = { FVector2D(+1, 0), FVector2D(-1, 0), FVector2D(0, -RawMoveInput.Y) };
	}

	// Barajar un poco las opciones ortogonales para que no siempre gire al mismo lado (opcional)
	// Pero para Battle City clásico, el orden determinista suele estar bien.

	for (const FVector2D& O : Options)
	{
		if (CanMoveTowards(O))
		{
			RawMoveInput = O;

			// CLAVE: Compromiso.
			// Si acabamos de chocar y girar, OBLIGAMOS a la IA a mantener esta nueva dirección
			// por un tiempo (ej. 1 segundo) para que salga del atolladero y no intente
			// volver a la ruta "óptima" bloqueada inmediatamente.
			NextAIDecisionTime = UGameplayStatics::GetTimeSeconds(this) + AICommitmentTimeAfterTurn;

			return;
		}
	}

	// Si nada es posible (callejón sin salida total), reversa total o quedarse quieto
	// RawMoveInput se queda como estaba y el sistema de física lo detendrá.
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

void AEnemyPawn::UpdateMovement(float DT)
{
	// 0. Delay de Giro
	if (CurrentTurnDelay > 0.f)
	{
		CurrentTurnDelay -= DT;
		Velocity = FVector2D::ZeroVector;
		// Permitir actualizar rotación si la IA cambia de decisión rápido
		if (!RawMoveInput.IsNearlyZero())
		{
			FacingDir = RawMoveInput;
			const float Yaw = (FacingDir.X > 0 ? 0.f : (FacingDir.X < 0 ? 180.f : (FacingDir.Y > 0 ? 90.f : -90.f)));
			SetActorRotation(FRotator(0.f, Yaw, 0.f));
		}
		return;
	}

	// 1. Lógica de Giro
	if (!RawMoveInput.IsNearlyZero())
	{
		FacingDir = RawMoveInput;
		const float Yaw = (FacingDir.X > 0 ? 0.f : (FacingDir.X < 0 ? 180.f : (FacingDir.Y > 0 ? 90.f : -90.f)));
		FRotator TargetRot(0.f, Yaw, 0.f);

		FVector CurrentFwd = GetActorForwardVector();
		FVector2D CurrentFwd2D(CurrentFwd.X, CurrentFwd.Y);

		// Detectar cambio de dirección
		if (FVector2D::DotProduct(CurrentFwd2D, RawMoveInput) < 0.95f)
		{
			SetActorRotation(TargetRot);
			Velocity = FVector2D::ZeroVector;
			CurrentTurnDelay = TurnDelay; // Activar delay
			return;
		}
		SetActorRotation(TargetRot);
	}

	// 2. Velocidad
	if (!RawMoveInput.IsNearlyZero())
		Velocity = FMath::Vector2DInterpTo(Velocity, RawMoveInput * MoveSpeed, DT, AccelRate);
	else
		Velocity = FMath::Vector2DInterpTo(Velocity, FVector2D::ZeroVector, DT, DecelRate);

	if (!Grid) return;

	// ... (Resto de la función: Bigotes, Colisión y Snap exactamente igual que antes) ...
	const float TileSize = Grid->GetTileSize();
	const float TankExtent = TileSize * 0.96f;
	FVector CurrentPos = GetActorLocation();
	FVector MoveDelta = FVector(Velocity.X, Velocity.Y, 0.f) * DT;
	FVector FuturePos = CurrentPos + MoveDelta;

	bool bBlocked = false;
	bool bMovingX = FMath::Abs(Velocity.X) > FMath::Abs(Velocity.Y);

	FVector DebugP1, DebugP2;
	const float SideMargin = 2.0f;

	if (bMovingX)
	{
		float FrontX = (Velocity.X > 0) ? (FuturePos.X + TankExtent) : (FuturePos.X - TankExtent);
		FVector P1(FrontX, FuturePos.Y + TankExtent - SideMargin, CurrentPos.Z);
		FVector P2(FrontX, FuturePos.Y - TankExtent + SideMargin, CurrentPos.Z);
		DebugP1 = P1; DebugP2 = P2;

		if (Grid->IsPointBlocked(P1) || Grid->IsPointBlocked(P2)) bBlocked = true;
		else
		{
			float Step = Grid->GetSubStep();
			if (Step <= 1.f) Step = TileSize;
			float IdealY = FMath::RoundToFloat(FuturePos.Y / Step) * Step;
			FuturePos.Y = FMath::FInterpConstantTo(FuturePos.Y, IdealY, DT, AlignRate);
		}
	}
	else
	{
		float FrontY = (Velocity.Y > 0) ? (FuturePos.Y + TankExtent) : (FuturePos.Y - TankExtent);
		FVector P1(FuturePos.X + TankExtent - SideMargin, FrontY, CurrentPos.Z);
		FVector P2(FuturePos.X - TankExtent + SideMargin, FrontY, CurrentPos.Z);
		DebugP1 = P1; DebugP2 = P2;

		if (Grid->IsPointBlocked(P1) || Grid->IsPointBlocked(P2)) bBlocked = true;
		else
		{
			float Step = Grid->GetSubStep();
			if (Step <= 1.f) Step = TileSize;
			float IdealX = FMath::RoundToFloat(FuturePos.X / Step) * Step;
			FuturePos.X = FMath::FInterpConstantTo(FuturePos.X, IdealX, DT, AlignRate);
		}
	}

	// Debug
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("bc.collision.debug"));
	if (CVar && CVar->GetInt() > 0 && GetWorld())
	{
		bool bP1Hit = Grid->IsPointBlocked(DebugP1);
		bool bP2Hit = Grid->IsPointBlocked(DebugP2);
		DrawDebugPoint(GetWorld(), DebugP1, 12.f, bP1Hit ? FColor::Red : FColor::Green, false, 0.03f);
		DrawDebugPoint(GetWorld(), DebugP2, 12.f, bP2Hit ? FColor::Red : FColor::Green, false, 0.03f);
	}

	if (!bBlocked) SetActorLocation(FuturePos);
	else Velocity = FVector2D::ZeroVector;
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
