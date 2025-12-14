#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "ScreenBoundsComponent.generated.h"

USTRUCT()
struct FCachedMeshData
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY()
	TArray<FVector> LocalVertices;
};


USTRUCT(BlueprintType)
struct FScreenBox
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FVector2D Min = FVector2D(FLT_MAX, FLT_MAX);
	UPROPERTY(BlueprintReadOnly) FVector2D Max = FVector2D(-FLT_MAX, -FLT_MAX);

	FORCEINLINE void Reset()
	{
		Min = FVector2D(FLT_MAX, FLT_MAX);
		Max = FVector2D(-FLT_MAX, -FLT_MAX);
	}

	FORCEINLINE void IncludePoint(const FVector2D& P)
	{
		Min.X = FMath::Min(Min.X, P.X);
		Min.Y = FMath::Min(Min.Y, P.Y);
		Max.X = FMath::Max(Max.X, P.X);
		Max.Y = FMath::Max(Max.Y, P.Y);
	}

	FORCEINLINE bool IsValid() const
	{
		return Min.X <= Max.X && Min.Y <= Max.Y
			&& FMath::IsFinite(Min.X) && FMath::IsFinite(Min.Y)
			&& FMath::IsFinite(Max.X) && FMath::IsFinite(Max.Y)
			&& Min.X != FLT_MAX && Min.Y != FLT_MAX
			&& Max.X != -FLT_MAX && Max.Y != -FLT_MAX;
	}
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEST_TERRAIN_API UScreenBoundsComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UScreenBoundsComponent();

	// Сэмплинг: 1 = все вершины, 2 = каждая 2-я, 4 = каждая 4-я и т.д.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds")
	int32 VertexSampleStep = 4;

	/** БЫСТРЫЙ тест: объект не позади камеры. */
	UFUNCTION(BlueprintCallable, Category="Bounds")
	bool FastAABBTest_FromCamera(UCameraComponent* Camera) const;

	/** Старый вариант: bbox относительно viewport (как у тебя в HUD). */
	UFUNCTION(BlueprintCallable, Category="Bounds")
	bool ComputeViewportBounds(FScreenBox& OutBounds) const;

	/**
	 * ВАЖНОЕ: bbox относительно РЕНДЕРА RenderW×RenderH, от конкретной камеры рендера.
	 * Начало координат = верхний левый угол.
	 */
	UFUNCTION(BlueprintCallable, Category="Bounds")
	bool ComputeRenderBoundsFromCamera(UCameraComponent* RenderCamera, int32 RenderW, int32 RenderH, FScreenBox& OutBounds) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FCachedMeshData> CachedMeshes;

private:
	static bool ProjectWorldToRenderPx(
		const FVector& World,
		const FMatrix& ViewProj,
		int32 RenderW,
		int32 RenderH,
		FVector2D& OutPx);

	static FMatrix BuildViewProjectionMatrixFromCamera(UCameraComponent* Cam, int32 RenderW, int32 RenderH);
};
