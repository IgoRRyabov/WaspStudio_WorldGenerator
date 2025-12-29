#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
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

	FVector2D Min = FVector2D(FLT_MAX, FLT_MAX);
	FVector2D Max = FVector2D(-FLT_MAX, -FLT_MAX);

	void Reset()
	{
		Min = FVector2D(FLT_MAX, FLT_MAX);
		Max = FVector2D(-FLT_MAX, -FLT_MAX);
	}

	void Include(const FVector2D& P)
	{
		Min.X = FMath::Min(Min.X, P.X);
		Min.Y = FMath::Min(Min.Y, P.Y);
		Max.X = FMath::Max(Max.X, P.X);
		Max.Y = FMath::Max(Max.Y, P.Y);
	}

	bool IsValid() const
	{
		return Max.X > Min.X && Max.Y > Min.Y;
	}
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEST_TERRAIN_API UScreenBoundsComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UScreenBoundsComponent();

	UPROPERTY(EditAnywhere, Category="Bounds")
	int32 VertexSampleStep = 4;

	/**
	 * ГЛАВНАЯ ФУНКЦИЯ
	 * @return true — объект реально попал в кадр
	 */
	bool ComputeBoundsFromCamera(
		UCameraComponent* Camera,
		int32 RenderW,
		int32 RenderH,
		FScreenBox& OutBounds
	) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FCachedMeshData> CachedMeshes;

private:
	static FMatrix BuildViewProjection(UCameraComponent* Camera, int32 W, int32 H);
	static bool ProjectWorldToPixel(
		const FVector& World,
		const FMatrix& ViewProj,
		int32 W,
		int32 H,
		FVector2D& OutPx
	);
};
