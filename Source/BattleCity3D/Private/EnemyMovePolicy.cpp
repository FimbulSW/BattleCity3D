


#include "EnemyMovePolicy.h"
#include "EnemyMovementComponent.h"

void UEnemyMovePolicy::Initialize(UEnemyMovementComponent* InOwner)
{
	{ Owner = MakeWeakObjectPtr(InOwner); }
}
