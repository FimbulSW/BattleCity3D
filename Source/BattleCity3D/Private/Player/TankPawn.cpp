#include "Player/TankPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "Projectiles/Projectile.h"
// Nota: Ya no necesitamos headers de Grid o Debug aquí, la base se encarga.

ATankPawn::ATankPawn()
{
    // Constructor simplificado: La base ya crea Root, Body y Muzzle.
    // Solo ajustamos lo específico del jugador si hace falta.
}

void ATankPawn::BeginPlay()
{
    Super::BeginPlay();
    // El Snap inicial ya lo hace Super::BeginPlay
}

void ATankPawn::Tick(float DeltaSeconds)
{
    // Super::Tick llama a UpdateTankMovement automáticamente
    Super::Tick(DeltaSeconds);

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
    // Eje dominante
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

    // Establecer variable en la clase padre
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