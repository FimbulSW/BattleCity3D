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
#include "DrawDebugHelpers.h"

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

    // Si aparecemos desplazados 0.1 unidades, el sistema de bordes podría 
    // detectar colisión inmediata. Forzamos alineación al nacer.
    if (Grid)
    {
        FVector P = GetActorLocation();
        // Snap al subgrid más cercano
        P = Grid->SnapWorldToSubgrid(P, true);
        SetActorLocation(P);

        // Debug Log
        UE_LOG(LogTemp, Warning, TEXT("TankPawn: Snapped start location to %s"), *P.ToString());
    }
}

void ATankPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateMovement(DeltaSeconds);

    // --- DEBUG CAJA DE COLISIÓN ---
    static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("bc.collision.debug"));
    if (CVar && CVar->GetInt() > 0 && Grid)
    {
        const float TileSize = Grid->GetTileSize();
        const float Extent = TileSize * 0.96f;
        FVector Pos = GetActorLocation();

        bool bOverlappingWall = false;
        // Verificar las 4 esquinas para saber si estamos "empotrados"
        FVector Corners[4] = {
            Pos + FVector(Extent, Extent, 0), Pos + FVector(Extent, -Extent, 0),
            Pos + FVector(-Extent, Extent, 0), Pos + FVector(-Extent, -Extent, 0)
        };
        for (const FVector& P : Corners) if (Grid->IsPointBlocked(P)) { bOverlappingWall = true; break; }

        FColor BoxColor = bOverlappingWall ? FColor::Red : FColor::Green;
        // Caja elevada
        DrawDebugBox(GetWorld(), Pos + FVector(0, 0, 10), FVector(Extent, Extent, 50.f), BoxColor, false, -1.f, 0, 2.0f);
    }
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
    // DEBUG VISUAL EN PANTALLA
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, TEXT("INPUT FIRE PRESSED"));
    TryFire();
}

void ATankPawn::TryFire()
{
    if (!ProjectileClass)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("ERROR: No ProjectileClass set in BP!"));
        return;
    }

    const double Now = UGameplayStatics::GetTimeSeconds(this);
    if (Now - LastFireTime < FireCooldown)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Orange, TEXT("Fire Cooldown..."));
        return;
    }

    LastFireTime = Now;

    // Calcular spawn location
    // Usamos un offset fijo si no hay socket, pero asegurándonos que esté ALINEADO al grid
    FVector SpawnLoc = GetActorLocation();
    if (Muzzle)
    {
        SpawnLoc = Muzzle->GetComponentLocation();
    }
    else
    {
        // Fallback manual basado en FacingDir
        SpawnLoc += FVector(FacingDir.X * 60.f, FacingDir.Y * 60.f, 0.f);
    }

    const FRotator SpawnRot = GetActorRotation();

    FActorSpawnParameters P;
    P.Owner = this;
    P.Instigator = this;
    // IMPORTANTE: Siempre spawnear, incluso si colisiona un poco al nacer
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AProjectile* Proj = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLoc, SpawnRot, P);
    if (Proj)
    {
        Proj->Team = EProjectileTeam::Player;
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Projectile Spawned!"));
    }
    else
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Projectile Spawn FAILED!"));
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
	if (!RawMoveInput.IsNearlyZero())
	{
		FacingDir = RawMoveInput;
		const float Yaw = (FacingDir.X > 0 ? 0.f : (FacingDir.X < 0 ? 180.f : (FacingDir.Y > 0 ? 90.f : -90.f)));
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}

	float TargetSpeed = RawMoveInput.IsNearlyZero() ? 0.f : MoveSpeed;
	float CurrentRate = RawMoveInput.IsNearlyZero() ? DecelRate : AccelRate;
	Velocity = FMath::Vector2DInterpTo(Velocity, RawMoveInput * TargetSpeed, DT, CurrentRate);

	if (Velocity.IsNearlyZero() || !Grid) return;

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
			float IdealY = FMath::RoundToFloat(FuturePos.Y / TileSize) * TileSize;
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
			float IdealX = FMath::RoundToFloat(FuturePos.X / TileSize) * TileSize;
			FuturePos.X = FMath::FInterpConstantTo(FuturePos.X, IdealX, DT, AlignRate);
		}
	}

	// --- DEBUG VISUAL (BIGOTES) CONDICIONAL ---
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("bc.collision.debug"));
	if (CVar && CVar->GetInt() > 0)
	{
		if (UWorld* W = GetWorld())
		{
			bool bP1Hit = Grid->IsPointBlocked(DebugP1);
			bool bP2Hit = Grid->IsPointBlocked(DebugP2);
			DrawDebugPoint(W, DebugP1, 12.f, bP1Hit ? FColor::Red : FColor::Green, false, 0.03f);
			DrawDebugPoint(W, DebugP2, 12.f, bP2Hit ? FColor::Red : FColor::Green, false, 0.03f);
		}
	}

	if (!bBlocked) SetActorLocation(FuturePos);
	else Velocity = FVector2D::ZeroVector;
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