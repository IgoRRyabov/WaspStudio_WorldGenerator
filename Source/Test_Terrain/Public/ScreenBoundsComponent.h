#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScreenBoundsComponent.generated.h"

USTRUCT(BlueprintType)
struct FScreenBounds2D
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 MinX = 0;
	UPROPERTY(BlueprintReadOnly) int32 MinY = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxX = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxY = 0;

	bool IsValid() const { return MaxX > MinX && MaxY > MinY; }
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEST_TERRAIN_API UScreenBoundsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UScreenBoundsComponent();

	UFUNCTION(BlueprintCallable)
	FScreenBounds2D ComputeScreenBounds() const;
	
};
