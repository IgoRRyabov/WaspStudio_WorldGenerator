#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScreenBoundsComponent.generated.h"

USTRUCT(BlueprintType)
struct FScreenBox
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FVector2D Min;

	UPROPERTY(BlueprintReadOnly)
	FVector2D Max;

	FScreenBox()
		: Min(FLT_MAX, FLT_MAX)
		, Max(-FLT_MAX, -FLT_MAX)
	{}

	bool IsValid() const
	{
		return Min.X <= Max.X && Min.Y <= Max.Y;
	}

	void IncludePoint(const FVector2D& P)
	{
		Min.X = FMath::Min(Min.X, P.X);
		Min.Y = FMath::Min(Min.Y, P.Y);
		Max.X = FMath::Max(Max.X, P.X);
		Max.Y = FMath::Max(Max.Y, P.Y);
	}
};

USTRUCT()
struct FCachedMeshData
{
	GENERATED_BODY()

	UPROPERTY()
	UStaticMeshComponent* MeshComp = nullptr;

	// Локальные вершины LOD0
	TArray<FVector> LocalVertices;

	// AABB локального меша (прямо из кеша)
	FBox LocalBounds;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEST_TERRAIN_API UScreenBoundsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UScreenBoundsComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ScreenBounds")
	int32 VertexSampleStep = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ScreenBounds")
	bool bUseAsyncTask = true; // пока не используем, оставим флаг для будущего

	UFUNCTION(BlueprintCallable)
	bool IsVisible(FScreenBox& OutBounds);

	UFUNCTION(BlueprintCallable)
	bool ComputeScreenBounds_Async(FScreenBox& OutBounds);

	UFUNCTION(BlueprintCallable, Category="ScreenBounds")
	bool ComputeScreenBounds(FScreenBox& OutBounds);

	bool ComputeScreenBounds_Sync(APlayerController* PC, FScreenBox& OutBounds) const;

	bool FastAABBTest(APlayerController* PC) const;
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FCachedMeshData> CachedMeshes;
};


class FWorldPointsTask : public FNonAbandonableTask
{
public:
	const TArray<FCachedMeshData>* Meshes;
	const TArray<FTransform>* Transforms;
	TArray<TArray<FVector>>* OutPoints;
	int32 Step;

	FWorldPointsTask(
		const TArray<FCachedMeshData>* InMeshes,
		const TArray<FTransform>* InTransforms,
		TArray<TArray<FVector>>* InOutPoints,
		int32 InStep)
		: Meshes(InMeshes), Transforms(InTransforms),
		  OutPoints(InOutPoints), Step(InStep) {}

	void DoWork()
	{
		for (int32 m = 0; m < Meshes->Num(); ++m)
		{
			const FCachedMeshData& CM = (*Meshes)[m];
			if (!CM.MeshComp || CM.LocalVertices.Num() == 0)
				continue;

			const FTransform& X = (*Transforms)[m];
			TArray<FVector>& Dest = (*OutPoints)[m];
			Dest.Reserve(CM.LocalVertices.Num() / Step + 1);

			for (int32 i = 0; i < CM.LocalVertices.Num(); i += Step)
			{
				Dest.Add(X.TransformPosition(CM.LocalVertices[i]));
			}
		}
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FWorldPointsTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
