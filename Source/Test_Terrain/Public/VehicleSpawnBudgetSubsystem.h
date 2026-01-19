#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VehicleSpawnBudgetSubsystem.generated.h"

UCLASS()
class TEST_TERRAIN_API UVehicleSpawnBudgetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	void ResetBudget(int32 NewMaxTotal);
	
	bool TryConsumeOne();
	void EnsureInitialized(int32 InMaxTotal);
	int32 GetMaxTotal() const { return MaxTotal; }
	int32 GetSpawnedTotal() const { return SpawnedTotal; }
	int32 GetRemaining() const { return FMath::Max(0, MaxTotal - SpawnedTotal); }

private:
	bool bInitialized = false;
	int32 MaxTotal = 0;
	int32 SpawnedTotal = 0;
};
