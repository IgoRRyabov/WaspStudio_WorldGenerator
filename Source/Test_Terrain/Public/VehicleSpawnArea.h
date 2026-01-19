#pragma once

#include "CoreMinimal.h"
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
	TSubclassOf<AActor> VehicleClass;

	// Сколько техники хочет эта зона (самый простой контроль)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 LocalCount = 5;

	// Минимальная дистанция между точками спавна (uu)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	float MinDistance = 500.f;

	// Попыток подобрать точку на один объект
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 MaxTriesPerVehicle = 60;

	// Стабильный ID зоны для воспроизводимого рандома (задай руками: 1..N)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 StableAreaId = 1;

	// Если true — во время спавна рисуем debug-точки/боксы
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	bool bDebugDraw = false;

protected:
	virtual void BeginPlay() override;

private:
	// --- Core ---
	FRandomStream MakeStreamForThisArea() const;
	ECollisionChannel GetLandscapeTraceChannel() const;

	bool FindSpawnTransform(FRandomStream& Stream, FTransform& OutXf) const;

	// --- Helpers ---
	bool IsInsideBoxLocal(const FVector& LocalPoint, const FVector& Extent) const;

	bool IsFarEnough(const FVector& CandidateWorld) const;

	bool TraceToLandscapeOnly(const FVector& WorldPoint, FVector& OutGroundPoint) const;

	// Получить "безопасный" Extent (уменьшенный на bounds техники по XY), если включено
	FVector GetSafeBoxExtent() const;

	// Запоминаем уже использованные точки для MinDistance
	mutable TArray<FVector> SpawnedPointsWorld;
};
