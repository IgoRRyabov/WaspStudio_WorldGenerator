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
	{
		Min = FVector2D(FLT_MAX, FLT_MAX);
		Max = FVector2D(-FLT_MAX, -FLT_MAX);
	}

	bool IsValid() const
	{
		return Min.X <= Max.X && Min.Y <= Max.Y;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEST_TERRAIN_API UScreenBoundsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UScreenBoundsComponent();

	UFUNCTION(BlueprintCallable)
	FScreenBox ComputeScreenBounds();

	UFUNCTION(BlueprintCallable)
	bool IsVisible(FScreenBox& OutBounds);
protected:
	virtual void BeginPlay() override;

private:
	TArray<UStaticMeshComponent*> Meshes;
};
