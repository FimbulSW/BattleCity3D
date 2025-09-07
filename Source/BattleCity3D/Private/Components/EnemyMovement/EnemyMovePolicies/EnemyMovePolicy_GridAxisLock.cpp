#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy_GridAxisLock.h"
#include "Components/EnemyMovement/EnemyMovementComponent.h"

void UEnemyMovePolicy_GridAxisLock::ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out)
{
    Out = FMoveDecision{};
    const float Tile = Ctx.TileSize;

    // 0) Stop & Shoot si hay Brick inmediato al frente
    if (Owner.IsValid() && Owner->bPreferShootWhenFrontBrick)
    {
        const uint8 Front = Ctx.FrontObstacle(Tile * FMath::Max(1, Ctx.LookAheadTiles), nullptr);
        if (Front == 1 /*Brick*/)
        {
            Out.RawMoveInput = FVector2D::ZeroVector;
            Out.LockAxis = (FMath::Abs(Ctx.FacingDir.X) >= FMath::Abs(Ctx.FacingDir.Y)) ? EMoveLockAxis::X : EMoveLockAxis::Y;
            Out.LockTime = Owner->MinLockTime;
            Out.bRequestFrontShot = Ctx.bFireReady; // disparamos en este tick si listo
            Out.DebugText = TEXT("Stop&Shoot Front Brick");
            return;
        }
    }

    // 1) Vector hacia la meta (cardinal preferido)
    const FVector To = Ctx.TargetWorld - Ctx.Location;
    const bool AlignedX = FMath::Abs(To.X) <= Ctx.AlignEpsilon;
    const bool AlignedY = FMath::Abs(To.Y) <= Ctx.AlignEpsilon;

    auto TryAxis = [&](bool bAxisX, int Dir)->bool
        {
            if (!Ctx.IsAheadBlocked) return false;
            if (!Ctx.IsAheadBlocked(bAxisX, Dir, Tile * Ctx.LookAheadTiles))
            {
                Out.RawMoveInput = bAxisX ? FVector2D(Dir, 0) : FVector2D(0, Dir);
                Out.LockAxis = bAxisX ? EMoveLockAxis::X : EMoveLockAxis::Y;
                Out.LockTime = Owner.IsValid() ? Owner->MinLockTime : 0.25f;
                return true;
            }
            return false;
        };

    // 2) Priorizar eje no alineado
    if (!AlignedX)
    {
        const int DirX = FMath::Sign(To.X);
        if (TryAxis(true, DirX)) { Out.DebugText = TEXT("Go X"); return; }
        if (!AlignedY)
        {
            const int DirY = FMath::Sign(To.Y);
            if (TryAxis(false, DirY)) { Out.DebugText = TEXT("Fallback Y"); return; }
        }
    }
    else if (!AlignedY)
    {
        const int DirY = FMath::Sign(To.Y);
        if (TryAxis(false, DirY)) { Out.DebugText = TEXT("Go Y"); return; }
        const int DirX = FMath::Sign(To.X);
        if (TryAxis(true, DirX)) { Out.DebugText = TEXT("Fallback X"); return; }
    }
    else
    {
        // Ambos casi alineados: evita ping-pong con deadband
        const float dmag = FMath::Abs(FMath::Abs(To.X) - FMath::Abs(To.Y));
        if (dmag <= Ctx.TieDeadband)
        {
            // Mantener facing actual
            if (FMath::Abs(Ctx.FacingDir.X) >= FMath::Abs(Ctx.FacingDir.Y))
            {
                const int DirX = (Ctx.FacingDir.X >= 0.f) ? +1 : -1;
                if (TryAxis(true, DirX)) { Out.DebugText = TEXT("Keep X"); return; }
            }
            else
            {
                const int DirY = (Ctx.FacingDir.Y >= 0.f) ? +1 : -1;
                if (TryAxis(false, DirY)) { Out.DebugText = TEXT("Keep Y"); return; }
            }
        }
        else if (FMath::Abs(To.X) > FMath::Abs(To.Y))
        {
            const int DirX = FMath::Sign(To.X);
            if (TryAxis(true, DirX)) { Out.DebugText = TEXT("Major X"); return; }
        }
        else
        {
            const int DirY = FMath::Sign(To.Y);
            if (TryAxis(false, DirY)) { Out.DebugText = TEXT("Major Y"); return; }
        }
    }

    // 3) Si todo bloqueado, detenerse (ChooseTurn lo puedes seguir usando en el Pawn)
    Out.RawMoveInput = FVector2D::ZeroVector;
    Out.LockAxis = EMoveLockAxis::None;
    Out.LockTime = 0.f;

    // 4) Si está cardinal con la meta y claro/brick, sugiere disparo (policy no fuerza, solo sugiere)
    if (Owner.IsValid())
    {
        const uint8 LOS = Ctx.CardinalLineToTarget(Ctx.Location, Ctx.TargetWorld, nullptr);
        if (LOS == 1 /*Clear*/ || LOS == 2 /*Brick*/)
            Out.bRequestFrontShot = Ctx.bFireReady;
    }

    Out.DebugText = TEXT("Blocked/Wait");
}
