#include "GameClasses/BattleGameMode.h"
#include "BattleBases/BattleBase.h"
#include "Enemies/EnemyPawn.h"
#include "GameClasses/BattlePlayerController.h"
#include "Map/MapGenerator.h"
#include "Map/MapGridSubsystem.h"
#include "Spawner/EnemySpawner.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ABattleGameMode::ABattleGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ABattlePlayerController::StaticClass();
}

void ABattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	FindActors();
	SpawnBaseIfAny();
	BindSpawner();

	// Esperar a que el MapGenerator haya spawneado al jugador; enlazar destrucción
	WatchPlayerPawn();
}

void ABattleGameMode::FindActors()
{
	// MapGenerator
	for (TActorIterator<AMapGenerator> It(GetWorld()); It; ++It) { MapGen = *It; break; }
	// Spawner
	for (TActorIterator<AEnemySpawner> It(GetWorld()); It; ++It) { Spawner = *It; break; }
}

void ABattleGameMode::SpawnBaseIfAny()
{
	if (!MapGen) return;
	if (UMapGridSubsystem* Grid = GetGameInstance()->GetSubsystem<UMapGridSubsystem>())
	{
		if (Grid->HasBase())
		{
			const FVector Loc = Grid->GetBaseWorldLocation();
			int32 Hp = Grid->GetBaseDefaultHP();

			if (!BaseClass)
			{
				BaseClass = ABattleBase::StaticClass();
			}
			FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			BaseActor = GetWorld()->SpawnActor<ABattleBase>(BaseClass, Loc, FRotator::ZeroRotator, P);
			if (BaseActor) BaseActor->HitPoints = Hp;
		}
	}
}

void ABattleGameMode::BindSpawner()
{
	if (!Spawner) return;
	EnemiesPlanned = Spawner->GetPlannedCount();
	EnemiesSpawned = Spawner->GetSpawnedCount();
	EnemiesAlive = Spawner->GetAliveCount();

	Spawner->OnEnemySpawned.AddDynamic(this, &ABattleGameMode::OnEnemySpawned);
	Spawner->OnEnemyDestroyedEvt.AddDynamic(this, &ABattleGameMode::OnEnemyDestroyed);
}

void ABattleGameMode::WatchPlayerPawn()
{
	APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
	if (P)
	{
		P->OnDestroyed.AddDynamic(this, &ABattleGameMode::OnPlayerPawnDestroyed);
	}
	else
	{
		// Reintenta pronto
		FTimerDelegate D = FTimerDelegate::CreateUObject(this, &ABattleGameMode::WatchPlayerPawn);
		GetWorldTimerManager().SetTimer(PlayerSearchTimer, D, 0.2f, false);
	}
}

void ABattleGameMode::OnEnemySpawned(AEnemyPawn* /*E*/)
{
	EnemiesSpawned++; EnemiesAlive++;
	UE_LOG(LogTemp, Log, TEXT("Spawned=%d / Planned=%d  Alive=%d"),
		EnemiesSpawned, EnemiesPlanned, EnemiesAlive);
}

void ABattleGameMode::OnEnemyDestroyed(AEnemyPawn* /*E*/)
{
	EnemiesAlive = FMath::Max(0, EnemiesAlive - 1);
	UE_LOG(LogTemp, Log, TEXT("Destroyed. Alive=%d | Spawned=%d / Planned=%d"),
		EnemiesAlive, EnemiesSpawned, EnemiesPlanned);
	CheckVictory();
}

void ABattleGameMode::OnPlayerPawnDestroyed(AActor* /*Dead*/)
{
	PlayerLives--;
	UE_LOG(LogTemp, Warning, TEXT("Jugador destruido. Vidas restantes: %d"), PlayerLives);
	if (PlayerLives > 0)
	{
		RespawnPlayer();
	}
	else
	{
		Defeat();
	}
}

void ABattleGameMode::HandleBaseDestroyed()
{
	UE_LOG(LogTemp, Warning, TEXT("Base destruida -> DERROTA"));
	Defeat();
}

void ABattleGameMode::RespawnPlayer()
{
	if (!MapGen) return;
	MapGen->RespawnPlayer();
	// Volver a enganchar el callback
	WatchPlayerPawn();
}

void ABattleGameMode::CheckVictory()
{
	if (Spawner)
	{
		EnemiesPlanned = Spawner->GetPlannedCount();
	}
	if (EnemiesAlive == 0 && EnemiesSpawned >= EnemiesPlanned)
	{
		Victory();
	}
}

void ABattleGameMode::Defeat()
{
	if (bGameOver) return;
	bGameOver = true;

	UE_LOG(LogTemp, Warning, TEXT("DERROTA: sin vidas o base destruida."));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("DERROTA"));

	UGameplayStatics::SetGamePaused(this, true);
	OnDefeat(); // BP/UI
}

void ABattleGameMode::Victory()
{
	if (bGameOver) return;
	bGameOver = true;

	UE_LOG(LogTemp, Log, TEXT("VICTORIA: todos los enemigos planificados eliminados."));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("VICTORIA"));

	UGameplayStatics::SetGamePaused(this, true);
	OnVictory(); // BP/UI
}