#include "EnemyMovePolicy_Composite.h"

void UEnemyMovePolicy_Composite::Initialize(UEnemyMovementComponent* InOwner)
{
	for (UEnemyMovePolicy* P : Policies)
	{
		if (!P) continue;
		P->Initialize(InOwner);
	}
}

void UEnemyMovePolicy_Composite::ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out)
{
	FMoveDecision Accum = bStartFromEmpty ? FMoveDecision{} : Out;

	for (UEnemyMovePolicy* P : Policies)
	{
		if (!P) continue;
		FMoveDecision Tmp = Accum;
		P->ComputeMove(Ctx, Tmp);

		// Mezcla: priorizamos campos no triviales del último que habló
		if (!Tmp.RawMoveInput.IsNearlyZero()) Accum.RawMoveInput = Tmp.RawMoveInput;
		if (Tmp.LockAxis != EMoveLockAxis::None) { Accum.LockAxis = Tmp.LockAxis; Accum.LockTime = Tmp.LockTime; }
		Accum.bRequestFrontShot |= Tmp.bRequestFrontShot;

		if (!Tmp.DebugText.IsEmpty())
		{
			if (!Accum.DebugText.IsEmpty()) Accum.DebugText.Append(TEXT(" -> "));
			Accum.DebugText.Append(Tmp.DebugText);
		}
	}
	Out = Accum;
}
