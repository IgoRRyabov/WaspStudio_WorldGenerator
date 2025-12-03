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
	int32 VertexSampleStep = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ScreenBounds")
	bool bUseAsyncTask = false; // пока не используем, оставим флаг для будущего

	UFUNCTION(BlueprintCallable)
	bool IsVisible(FScreenBox& OutBounds);

	UFUNCTION(BlueprintCallable)
	FScreenBox ComputeScreenBounds();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FCachedMeshData> CachedMeshes;

	// быстрый frustum test
	bool FastAABBVisibilityTest(APlayerController* PC) const;

	// точный проход по вершинам
	FScreenBox ComputeScreenBounds_Internal(APlayerController* PC) const;
};
