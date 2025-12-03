#pragma once

#include "CoreMinimal.h"
#include "ScreenBoundsComponent.h"
#include "GameFramework/HUD.h"
#include "ObjectBoundsHUD.generated.h"

UCLASS()
class TEST_TERRAIN_API AObjectBoundsHUD : public AHUD
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<UScreenBoundsComponent*> BoundComponents;

protected:
	virtual void DrawHUD() override;

	virtual void BeginPlay() override;
public:
	UPROPERTY()
	TArray<AActor*> CachedActors;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DetectionTag = "Detectable";
};
