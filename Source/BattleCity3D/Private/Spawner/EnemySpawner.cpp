#include "Spawner/EnemySpawner.h"
#include "Enemies/EnemyPawn.h"
#include "Map/MapGridSubsystem.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameClasses/BattleGameMode.h"
#include "GameFramework/Pawn.h"
#include "Utils/JsonMapUtils.h"
#include "Engine/OverlapResult.h"
#include "Enemies/EnemyGoalPolicies/EnemyGoalPolicy.h"
#include "Enemies/EnemyGoalPolicies/EnemyGoalPolicy_RandomFixed.h"
#include "Enemies/EnemyGoalPolicies/EnemyGoalPolicy_AdvantageBias.h"

#include "Engine/Engine.h"
#include "Enemies/EnemyEnums.h"
#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy.h"
#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy_PathFollow.h"
#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy_Composite.h"
#include "Components/EnemyMovement/EnemyMovementComponent.h"

#include "Spawner/SpawnPointPolicies/EnemySpawnPointPolicy_RandomAny.h"
#include "Spawner/SpawnPointPolicies/EnemySpawnPointPolicy_FarFromPlayer.h"

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

ABattleGameMode* AEnemySpawner::GetBattleGM(UWorld* World)
{
	return World ? Cast<ABattleGameMode>(UGameplayStatics::GetGameMode(World)) : nullptr;
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	Grid = GetGameInstance()->GetSubsystem<UMapGridSubsystem>();
	if (!Grid)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemySpawner: MapGridSubsystem no disponible."));
		return;
	}

	ScheduleWaves();

	if (!GoalPolicy && GoalPolicyClass)
	{
		GoalPolicy = NewObject<UEnemyGoalPolicy>(this, GoalPolicyClass, NAME_None, RF_Transactional);
	}
	if (!IsValid(GoalPolicy))
	{
		UClass* C = GoalPolicyClass ? *GoalPolicyClass : UEnemyGoalPolicy_AdvantageBias::StaticClass(); // o la que prefieras
		GoalPolicy = NewObject<UEnemyGoalPolicy>(this, C, NAME_None, RF_Transactional);
	}
	if (!GoalPolicy) // fallback
	{
		GoalPolicy = NewObject<UEnemyGoalPolicy_RandomFixed>(this, UEnemyGoalPolicy_RandomFixed::StaticClass(), NAME_None, RF_Transactional);
	}

	if (GoalPolicy) GoalPolicy->Initialize(this);

	if (!SpawnPointPolicy && SpawnPointPolicyClass)
		SpawnPointPolicy = NewObject<USpawnPointPolicy>(this, SpawnPointPolicyClass, NAME_None, RF_Transactional);
	if (!SpawnPointPolicy)
		SpawnPointPolicy = NewObject<UEnemySpawnPointPolicy_RandomAny>(this, UEnemySpawnPointPolicy_RandomAny::StaticClass(), NAME_None, RF_Transactional);
}

#if WITH_EDITOR
void AEnemySpawner::PostEditChangeProperty(FPropertyChangedEvent& E)
{
	Super::PostEditChangeProperty(E);
	static const FName NAME_GoalPolicyClass(TEXT("GoalPolicyClass"));
	if (E.Property && E.Property->GetFName() == NAME_GoalPolicyClass)
	{
		// recrear instancia cuando cambia la clase
		if (GoalPolicyClass)
		{
			GoalPolicy = NewObject<UEnemyGoalPolicy>(this, GoalPolicyClass, NAME_None, RF_Transactional);
			if (GoalPolicy) GoalPolicy->Initialize(this);
		}
		else
		{
			GoalPolicy = nullptr;
		}
	}
}
#endif

FName AEnemySpawner::ToKey(const FString& S) { return FName(*S.ToLower()); }

void AEnemySpawner::ScheduleWaves()
{
	Pending.Reset();
	EnemiesPlanned = 0;
	EnemiesSpawned = 0;
	AliveCount = 0;

	if (!Grid) return;
	const TArray<FWaveEntry>& Waves = Grid->GetWaves();
	if (Waves.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: sin oleadas."));
		return;
	}

	for (const FWaveEntry& W : Waves)
	{
		FPendingWave PW;
		PW.Time = W.time;
		PW.Type = W.type;
		PW.Symbol = W.spawn;
		PW.Count = FMath::Max(1, W.count);
		Pending.Add(PW);
		EnemiesPlanned += PW.Count;

		FTimerHandle H;
		FTimerDelegate D = FTimerDelegate::CreateUObject(this, &AEnemySpawner::OnWaveDue, PW);
		GetWorldTimerManager().SetTimer(H, D, PW.Time, false);
	}

	// Precache spawn points por símbolo (mundo)
	TSet<FString> Symbols;
	for (const FPendingWave& W : Pending) Symbols.Add(W.Symbol);
	for (const FString& S : Symbols)
	{
		TArray<FVector> Locs;
		Grid->GetSpawnWorldLocationsForSymbol(S, Locs);
		SpawnLocs.Add(S, Locs);
	}

	// === NUEVO: precargar UNIÓN deduplicada desde celdas y derivar a mundo ===
	AllSpawnCells.Reset();
	Grid->GetAllEnemySpawnCells(AllSpawnCells); // ya ignora '.'

	// Derivar AllSpawnLocs desde celdas (garantiza 1:1, sin duplicados)
	AllSpawnLocs.Reset();
	AllSpawnLocs.Reserve(AllSpawnCells.Num());
	for (const FIntPoint& C : AllSpawnCells)
	{
		AllSpawnLocs.Add(Grid->GridToWorld(C.X, C.Y, Grid->GetTileSize() * 0.5f));
	}

	UE_LOG(LogTemp, Log, TEXT("EnemySpawner: %d spawn cells unificadas."), AllSpawnCells.Num());
}

void AEnemySpawner::OnWaveDue(FPendingWave Wave)
{
	if (!Grid) return;

	// Respeta MaxAlive repartiendo la wave en tandas
	const int32 FreeSlots = MaxAlive - AliveCount;
	if (FreeSlots <= 0)
	{
		FTimerHandle H;
		FTimerDelegate D = FTimerDelegate::CreateUObject(this, &AEnemySpawner::OnWaveDue, Wave);
		GetWorldTimerManager().SetTimer(H, D, 0.5f, false);
		return;
	}

	const int32 ToSpawnNow = FMath::Clamp(Wave.Count, 0, FreeSlots);
	for (int32 i = 0; i < ToSpawnNow; ++i)
	{
		SpawnOne(Wave.Type, Wave.Symbol);
	}

	const int32 Remaining = Wave.Count - ToSpawnNow;
	if (Remaining > 0)
	{
		FPendingWave Rest = Wave;
		Rest.Count = Remaining;

		FTimerHandle H;
		FTimerDelegate D = FTimerDelegate::CreateUObject(this, &AEnemySpawner::OnWaveDue, Rest);
		GetWorldTimerManager().SetTimer(H, D, 0.5f, false);
	}

	UE_LOG(LogTemp, Log, TEXT("Wave '%s' at '%s': spawned %d, remaining %d (Alive=%d/%d)"),
		*Wave.Type, *Wave.Symbol, ToSpawnNow, FMath::Max(0, Remaining), AliveCount, MaxAlive);
}

TSubclassOf<AEnemyPawn> AEnemySpawner::ResolveClassFor(const FString& Type, const FString& Symbol) const
{
	// 1) Símbolo
	if (const TSubclassOf<AEnemyPawn>* BySym = SymbolToClass.Find(ToKey(Symbol)))
		if (BySym->Get()) return *BySym;

	// 2) Tipo custom (JSON Type)
	if (const TSubclassOf<AEnemyPawn>* ByType = CustomTypeToClass.Find(ToKey(Type)))
		if (ByType->Get()) return *ByType;

	// 3) Tipos rápidos
	const FString LT = Type.ToLower();
	if (LT == TEXT("basic") && EnemyClass_Basic)   return EnemyClass_Basic;
	if (LT == TEXT("fast") && EnemyClass_Fast)    return EnemyClass_Fast;
	if (LT == TEXT("power") && EnemyClass_Power)   return EnemyClass_Power;
	if (LT == TEXT("armored") && EnemyClass_Armored) return EnemyClass_Armored;

	// 4) Fallback
	return EnemyClassDefault;
}

void AEnemySpawner::SpawnOne(const FString& Type, const FString& Symbol)
{
	// Usar la unión de spawns (deduplicada) sin importar el símbolo
	const TArray<FVector>& Candidates = AllSpawnLocs;
	if (Candidates.Num() == 0 || !Grid) return;

	// Pedimos orden a la policy; si no hay, baraja fallback
	TArray<int32> Indices;
	if (SpawnPointPolicy)
	{
		FSpawnPointContext Ctx;
		if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) Ctx.PlayerLoc = P->GetActorLocation();
		Ctx.bHasBase = Grid->HasBase();
		if (Ctx.bHasBase) Ctx.BaseLoc = Grid->GetBaseWorldLocation();

		SpawnPointPolicy->BuildCandidateOrder(Candidates, Ctx, Indices);
	}
	if (Indices.Num() == 0)
	{
		Indices.Reserve(Candidates.Num());
		for (int32 i = 0; i < Candidates.Num(); ++i) Indices.Add(i);
		for (int32 i = 0; i < Indices.Num(); ++i) { const int32 j = FMath::RandRange(i, Indices.Num() - 1); Indices.Swap(i, j); }
	}

	// === NUEVO: evitar duplicar CELDA en el mismo tick
	FVector SpawnLoc = Candidates[Indices[0]];
	FIntPoint SpawnCell(-999, -999);
	bool bFound = false;

	for (int32 idx : Indices)
	{
		const FVector& L = Candidates[idx];

		int32 gx, gy;
		if (!Grid->WorldToGrid(L, gx, gy)) continue; // debería entrar siempre

		const FIntPoint Cell(gx, gy);
		if (ClaimedCellsThisTick.Contains(Cell)) continue; // YA tomada este tick

		if (!IsSpawnPointFree(L)) continue; // hay algo ocupando (pawn/jugador)

		SpawnLoc = L;
		SpawnCell = Cell;
		bFound = true;
		break;
	}

	if (!bFound)
	{
		// Reintenta pronto (tal vez por colisiones transitorias)
		FTimerHandle H;
		FTimerDelegate D;
		D.BindWeakLambda(this, [this, Type, Symbol]()
			{
				SpawnOne(Type, Symbol);
			});
		GetWorldTimerManager().SetTimer(H, D, 0.25f, false);
		return;
	}

	// Marca celda como "reclamada" este tick
	ClaimedCellsThisTick.Add(SpawnCell); // === NUEVO

	const FRotator SpawnRot = FRotator::ZeroRotator;

	TSubclassOf<AEnemyPawn> Chosen = ResolveClassFor(Type, Symbol);
	if (!Chosen) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	AEnemyPawn* E = GetWorld()->SpawnActor<AEnemyPawn>(Chosen, SpawnLoc, SpawnRot, Params);
	if (!E) return;

	// ---- Asignación de meta por política ----
	FEnemySpawnContext Ctx;
	Ctx.Type = Type; Ctx.Symbol = Symbol;
	Ctx.AliveCount = AliveCount;
	Ctx.EnemiesSpawned = EnemiesSpawned;
	Ctx.MaxAlive = MaxAlive;
	Ctx.bHasBase = Grid->HasBase();
	Ctx.bHasPlayer = (UGameplayStatics::GetPlayerPawn(this, 0) != nullptr);

	if (GoalPolicy)
		E->Goal = GoalPolicy->DecideGoalOnSpawn(Ctx);
	else
		E->Goal = EEnemyGoal::HuntBase;

	E->SpawnerRef = this;
	LivingEnemies.Add(E);

	if (GoalPolicy)
		GoalPolicy->OnEnemySpawned(E);

	EnemiesSpawned++;
	AliveCount++;
	OnEnemySpawned.Broadcast(E);

	if (GoalPolicy) GoalPolicy->OnAliveCountChanged(AliveCount);

	if (UEnemyMovementComponent* Move = E->FindComponentByClass<UEnemyMovementComponent>())
	{
		auto ApplyDefaultsTo = [this](UEnemyMovePolicy* Policy)
			{
				if (auto* PF = Cast<UEnemyMovePolicy_PathFollow>(Policy))
				{
					PF->Cost = PathDefaults.Cost;
					PF->ReplanInterval = PathDefaults.ReplanInterval;
					PF->HorizonSteps = PathDefaults.HorizonSteps;
					PF->bTargetIsPlayer = PathDefaults.bTargetIsPlayer;
				}
			};

		if (auto* Comp = Cast<UEnemyMovePolicy_Composite>(Move->MovePolicy)) // si tienes getter
		{
			for (UEnemyMovePolicy* Sub : Comp->Policies) ApplyDefaultsTo(Sub);
		}
		else if (UEnemyMovePolicy* Single = Move->MovePolicy)
		{
			ApplyDefaultsTo(Single);
		}
	}

	// Cuando muera:
	E->OnDestroyed.AddDynamic(this, &AEnemySpawner::HandleActorDestroyed);
}

bool AEnemySpawner::IsSpawnPointFree(const FVector& Location) const
{
	UWorld* World = GetWorld();
	if (!World) return true;

	const float Tile = Grid ? Grid->GetTileSize() : 200.f;
	const float Radius = FMath::Max(50.f, Tile * 0.45f);

	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	TArray<FOverlapResult> Hits;

	FCollisionObjectQueryParams Obj;
	Obj.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemySpawnOverlap), false, nullptr);

	const bool bAny = World->OverlapMultiByObjectType(Hits, Location, FQuat::Identity, Obj, Shape, Params);
	if (!bAny) return true;

	for (const FOverlapResult& R : Hits)
	{
		AActor* A = R.GetActor();
		if (!A) continue;
		if (A->IsA(AEnemyPawn::StaticClass())) return false; // otro enemigo
		if (A->IsA(APawn::StaticClass()))      return false; // jugador u otro pawn
	}
	return true;
}

void AEnemySpawner::HandleActorDestroyed(AActor* Dead)
{
	AliveCount = FMath::Max(0, AliveCount - 1);
	LivingEnemies.RemoveAll([Dead](const TWeakObjectPtr<AEnemyPawn>& W) { return !W.IsValid() || W.Get() == Dead; });
	if (GoalPolicy)
	{
		if (AEnemyPawn* EP = Cast<AEnemyPawn>(Dead)) GoalPolicy->OnEnemyDestroyed(EP);
		GoalPolicy->OnAliveCountChanged(AliveCount);
	}

	if (AEnemyPawn* EP = Cast<AEnemyPawn>(Dead))
	{
		OnEnemyDestroyedEvt.Broadcast(EP);
	}
}

void AEnemySpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ClaimedCellsThisTick.Reset();

	if (GoalPolicy && IsValid(GoalPolicy)) GoalPolicy->TickGoal(DeltaSeconds);

	if (!bAIDebug) return;

	static double Acc = 0.0; Acc += DeltaSeconds;
	if (Acc < 0.5) return; Acc = 0.0;

	int32 HuntBase = 0, HuntPlayer = 0;
	LivingEnemies.RemoveAll([](const TWeakObjectPtr<AEnemyPawn>& W) { return !W.IsValid(); });
	for (const auto& W : LivingEnemies)
		if (const AEnemyPawn* E = W.Get())
			(E->Goal == EEnemyGoal::HuntBase ? HuntBase : HuntPlayer)++;

	const FString PolicyName = GoalPolicy ? GoalPolicy->GetClass()->GetName() : TEXT("None");
	const FString Msg = FString::Printf(
		TEXT("[AI] Alive=%d  Spawned=%d/%d  HB=%d  HP=%d  Policy=%s"),
		AliveCount, EnemiesSpawned, EnemiesPlanned, HuntBase, HuntPlayer, *PolicyName);

	if (GEngine) GEngine->AddOnScreenDebugMessage(0xBADC0DE, 0.55f, FColor::Yellow, Msg);
}

void AEnemySpawner::ApplyGoalToAll(EEnemyGoal NewGoal)
{
	LivingEnemies.RemoveAll([](const TWeakObjectPtr<AEnemyPawn>& W) { return !W.IsValid(); });
	for (const TWeakObjectPtr<AEnemyPawn>& W : LivingEnemies)
		if (AEnemyPawn* E = W.Get()) E->Goal = NewGoal;
}

void AEnemySpawner::bc_ai_debug(int32 v)
{
	bAIDebug = (v != 0);
	UE_LOG(LogTemp, Log, TEXT("bc.ai.debug = %d"), bAIDebug ? 1 : 0);
}
