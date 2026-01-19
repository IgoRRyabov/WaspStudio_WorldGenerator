#include "VehicleSpawnBudgetSubsystem.h"

void UVehicleSpawnBudgetSubsystem::ResetBudget(int32 NewMaxTotal)
{
	MaxTotal = FMath::Max(0, NewMaxTotal);
	SpawnedTotal = 0;
}

bool UVehicleSpawnBudgetSubsystem::TryConsumeOne()
{
	if (SpawnedTotal + 1 > MaxTotal) return false;
	SpawnedTotal += 1;
	return true;
}

void UVehicleSpawnBudgetSubsystem::EnsureInitialized(int32 InMaxTotal)
{
	if (bInitialized) return;
	ResetBudget(InMaxTotal);
}
