#include "Enemies/EnemyPawn.h"
#include "Components/EnemyMovement/EnemyMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BattleBases/BattleBase.h"
#include "Projectiles/Projectile.h"
#include "Map/MapGridSubsystem.h"
#include "GameClasses/BattleGameMode.h"
#include "Spawner/EnemySpawner.h"

// Nota: No necesitamos DrawDebugHelpers aquí para el movimiento físico, 
// eso ya lo hace ABattleTankPawn.

AEnemyPawn::AEnemyPawn()
{
    // 1. ELIMINADO: Creación de Root, Body y Muzzle.
    // La clase padre (ABattleTankPawn) ya los crea por nosotros.

    // Componente de cerebro (Opcional si usas tu lógica UpdateAI interna)
    MovementComp = CreateDefaultSubobject<UEnemyMovementComponent>(TEXT("EnemyMovement"));
}

void AEnemyPawn::BeginPlay()
{
    // Llama a la base para inicializar Grid y hacer el Snap inicial
    Super::BeginPlay();

    // Configuración inicial de Stats según el tipo
    if (bAcceptTypeFromSpawner)
    {
        // Si el spawner no lo configuró, forzamos actualización visual/stats por defecto
        SetTypeFromSpawner(EnemyType);
    }

    // Iniciar disparo automático si hay clase de proyectil
    if (ProjectileClass)
    {
        GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &AEnemyPawn::Fire, FireInterval, true, FMath::RandRange(0.5f, 1.5f));
    }
}

void AEnemyPawn::Tick(float DeltaSeconds)
{
    // 1. PENSAR (Cerebro)
    // Decidimos la dirección (RawMoveInput)
    if (!MovementComp)
    {
        UpdateAI(DeltaSeconds);
    }

    // 2. MOVERSE (Cuerpo)
    // Llamamos a Super::Tick. 
    // ABattleTankPawn::Tick se encargará de leer RawMoveInput, 
    // aplicar bigotes, colisiones, delay de giro y mover el actor.
    Super::Tick(DeltaSeconds);
}

void AEnemyPawn::UpdateAI(float DT)
{
    const double Now = UGameplayStatics::GetTimeSeconds(this);

    // --- 1. CHEQUEO DE EMERGENCIA (Bloqueo Inmediato) ---
    // Verificamos si la dirección actual nos va a estrellar AHORA MISMO.

    const float Tile = Grid ? Grid->GetTileSize() : 100.f;
    int32 CX, CY;
    bool bHaveCell = (Grid && Grid->WorldToGrid(GetActorLocation(), CX, CY));
    const FVector Center = bHaveCell ? Grid->GridToWorld(CX, CY, Tile * 0.5f) : GetActorLocation();

    bool bIsBlockedNow = false;
    if (Grid && !RawMoveInput.IsNearlyZero())
    {
        const bool bAxisX = FMath::Abs(RawMoveInput.X) > 0.f;
        const int Dir = bAxisX ? (RawMoveInput.X > 0 ? +1 : -1) : (RawMoveInput.Y > 0 ? +1 : -1);

        // Miramos 1 tile adelante para reaccionar antes de chocar físicamente
        bIsBlockedNow = IsBlockedAhead(Center, bAxisX, Dir, Tile * LookAheadTiles);
    }

    // --- 2. CONTROL DE TIEMPO (Throttle) ---
    // Solo recalculamos ruta si:
    // A) Ya pasó el tiempo de espera (Timer vencido)
    // B) O estamos bloqueados AHORA MISMO (Emergencia)
    if (Now < NextAIDecisionTime && !bIsBlockedNow)
    {
        return; // Mantenemos la decisión anterior y seguimos moviéndonos
    }

    // --- 3. TOMA DE DECISIÓN ---

    // A. Evasión de Choque
    if (bIsBlockedNow)
    {
        ChooseTurn();
        // ChooseTurn establece un tiempo de "Compromiso" largo para salir del atolladero
        return;
    }

    // B. Persecución del Objetivo

    // Reseteamos el timer para pensamiento normal
    NextAIDecisionTime = Now + AIReplanInterval;

    const FVector Target = GetAITargetWorld();
    const FVector To = Target - GetActorLocation();
    const float eps = AlignEpsilonFactor * Tile;
    const float db = TieDeadbandFactor * Tile;

    const bool AlignedX = FMath::Abs(To.X) <= eps;
    const bool AlignedY = FMath::Abs(To.Y) <= eps;

    // Lambda para verificar seguridad
    auto IsDirSafe = [&](bool bAxisX, int Dir)->bool
        {
            const FVector TestCenter = Grid ? Grid->SnapWorldToSubgrid(GetActorLocation(), true) : GetActorLocation();
            return !IsBlockedAhead(TestCenter, bAxisX, Dir, Tile * LookAheadTiles);
        };

    FVector2D NewMoveDir = FacingDir;

    // Lógica simple de persecución por ejes
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
        // Diagonal / Cerca
        const float dmag = FMath::Abs(FMath::Abs(To.X) - FMath::Abs(To.Y));
        if (dmag > db)
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

    if (AxisLock != EAxisLock::None) LockUntilTime = Now + MinLockTime;

    // Asignamos a la variable que leerá la clase padre
    RawMoveInput = NewMoveDir;
}

void AEnemyPawn::ChooseTurn()
{
    // Intentar girar a una dirección perpendicular o contraria
    TArray<FVector2D> Options;

    if (FMath::Abs(RawMoveInput.X) > 0.f)
    {
        // Si íbamos en X, probar Y (Arriba/Abajo) y luego reversa X
        Options = { FVector2D(0, +1), FVector2D(0, -1), FVector2D(-RawMoveInput.X, 0) };
    }
    else
    {
        // Si íbamos en Y, probar X (Der/Izq) y luego reversa Y
        Options = { FVector2D(+1, 0), FVector2D(-1, 0), FVector2D(0, -RawMoveInput.Y) };
    }

    const float Tile = Grid ? Grid->GetTileSize() : 100.f;

    for (const FVector2D& O : Options)
    {
        // Verificar si es seguro moverse ahí
        const bool bAxisX = FMath::Abs(O.X) > 0.f;
        const int Dir = bAxisX ? (O.X > 0 ? 1 : -1) : (O.Y > 0 ? 1 : -1);

        // Chequeo rápido
        const FVector TestCenter = Grid ? Grid->SnapWorldToSubgrid(GetActorLocation(), true) : GetActorLocation();
        if (!IsBlockedAhead(TestCenter, bAxisX, Dir, Tile * 0.5f))
        {
            RawMoveInput = O;

            // COMPROMISO: Forzamos a mantener esta dirección por un tiempo
            // para evitar que la IA intente volver inmediatamente a la ruta bloqueada.
            NextAIDecisionTime = UGameplayStatics::GetTimeSeconds(this) + AICommitmentTimeAfterTurn;

            return;
        }
    }

    // Si todo está bloqueado, no cambiamos input (se detendrá por física en la clase base)
}

// --- Helpers de AI y Combate ---

FVector AEnemyPawn::GetAITargetWorld() const
{
    // 1. Si tenemos objetivo prioritario (Base)
    if (Goal == EEnemyGoal::HuntBase)
    {
        // Buscar actor base
        TArray<AActor*> Bases;
        UGameplayStatics::GetAllActorsOfClass(this, ABattleBase::StaticClass(), Bases); // Optimizar caché en prod

        // Filtrar bases aliadas (Bando del jugador = azul, nosotros = rojo/enemigo)
        // Buscamos la base del jugador para destruirla
        for (AActor* B : Bases)
        {
            if (ABattleBase* BaseParams = Cast<ABattleBase>(B))
            {
                if (!BaseParams->bIsEnemyBase) // Es base aliada (objetivo)
                    return B->GetActorLocation();
            }
        }
    }

    // 2. Fallback: Ir al jugador
    if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        return Player->GetActorLocation();
    }

    return FVector::ZeroVector;
}

bool AEnemyPawn::IsBlockedAhead(const FVector& Center, bool bAxisX, int Dir, float Dist) const
{
    if (!Grid) return false;
    // Check simple de un punto adelante
    FVector CheckPos = Center;
    if (bAxisX) CheckPos.X += Dir * Dist;
    else        CheckPos.Y += Dir * Dist;

    return Grid->IsPointBlocked(CheckPos);
}

bool AEnemyPawn::CanMoveTowards(const FVector2D& Axis) const
{
    // Wrapper simple
    const bool bX = FMath::Abs(Axis.X) > 0;
    const int D = bX ? (Axis.X > 0 ? 1 : -1) : (Axis.Y > 0 ? 1 : -1);
    const float T = Grid ? Grid->GetTileSize() : 100.f;
    return !IsBlockedAhead(GetActorLocation(), bX, D, T * 0.6f);
}

void AEnemyPawn::Fire()
{
    if (!IsFireReady()) return;
    if (!ProjectileClass) return;

    // Spawn
    FVector Loc = GetActorLocation() + GetActorForwardVector() * 60.f; // Offset manual simple
    FRotator Rot = GetActorRotation();

    FActorSpawnParameters P;
    P.Owner = this;
    P.Instigator = this;
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AProjectile* Proj = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, Loc, Rot, P))
    {
        Proj->Team = EProjectileTeam::Enemy;
    }
}

bool AEnemyPawn::IsFireReady() const
{
    // Aquí podrías chequear si hay línea de visión al jugador o base
    return true;
}

void AEnemyPawn::ApplyHit(int32 Dmg)
{
    // ... Lógica de hit visual ...
}

float AEnemyPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    HitPoints--;
    if (HitPoints <= 0)
    {
        // Notificar muerte / Spawner
        if (SpawnerRef.IsValid()) SpawnerRef->NotifyEnemyDeath();

        // FX y Destrucción
        Destroy();
    }
    return DamageAmount;
}

void AEnemyPawn::SetTypeFromSpawner(EEnemyType InType)
{
    EnemyType = InType;
    // Aquí configurarías Mesh, Color, Velocidad y HP según el Enum
    // Ej:
    // if (EnemyType == EEnemyType::Fast) { MoveSpeed = 600; HitPoints = 1; }
    // else if (EnemyType == EEnemyType::Armored) { MoveSpeed = 200; HitPoints = 4; }
    // IMPORTANTE: MoveSpeed ahora es miembro de la clase padre ABattleTankPawn
}