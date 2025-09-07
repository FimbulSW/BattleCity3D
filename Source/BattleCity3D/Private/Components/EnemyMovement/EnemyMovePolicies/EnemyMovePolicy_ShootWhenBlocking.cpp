#include "Components/EnemyMovement/EnemyMovePolicies/EnemyMovePolicy_ShootWhenBlocking.h"

void UEnemyMovePolicy_ShootWhenBlocking::ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out)
{
	Out.DebugText += TEXT(" +ShootCheck");

	if (!Ctx.bFireReady) return;
	if (!IsFrontBrick(Ctx)) return;

	Out.bRequestFrontShot = true;
	Out.RawMoveInput = FVector2D::ZeroVector;
}

bool UEnemyMovePolicy_ShootWhenBlocking::IsFrontBrick(const FMoveContext& Ctx) const
{
	if (!Ctx.FrontObstacle) return false;

	FVector Hit;
	const uint8 Type = Ctx.FrontObstacle(Ctx.TileSize * 0.51f, &Hit);
	// Convención dada: 0=None, 1=Brick, 2=Steel
	if (Type == 1) return true;

	// Si tu FrontObstacle no usa distancias, puedes ajustar a:
	// const uint8 Type = Ctx.FrontObstacle(0.f, &Hit);

	return false;
}
