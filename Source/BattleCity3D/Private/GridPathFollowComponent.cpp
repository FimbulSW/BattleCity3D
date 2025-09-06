


#include "GridPathFollowComponent.h"
#include "MapGridSubsystem.h"

FVector UGridPathFollowComponent::GridToWorld(const FIntPoint& C, float TileSize) const
{
	if (Grid)
		return Grid->GridToWorld(C.X, C.Y); // usa tu conversión precisa
	return FVector(C.X * 200.f, C.Y * 200.f, 0.f); // fallback
}
