#pragma once

#include "CoreMinimal.h"
#include "MyGameInstance.h"
#include "GameFramework/Actor.h"
#include "VehicleSpawnArea.generated.h"

class UBoxComponent;

UCLASS()
class TEST_TERRAIN_API AVehicleSpawnArea : public AActor
{
	GENERATED_BODY()
	
public:
	AVehicleSpawnArea();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawn")
	TObjectPtr<UBoxComponent> Box;

	// Класс техники (Actor/Pawn — не важно)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	TArray<TSubclassOf<AActor>> VehicleClass;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void CalculatePosition();
	
	UFUNCTION()
	void TrySpawnOne();
	
	UFUNCTION()
	bool CalcRandomPoint(FVector& OutPoint) const;
	
	FRotator RandomRot;
	
	UPROPERTY()
	UMyGameInstance* GI;
	mutable TArray<FVector> SpawnedPointsWorld;
};
