#pragma once

#include "CoreMinimal.h"
#include "ScreenBoundsComponent.h"
#include "GameFramework/Actor.h"
#include "TargetActor.generated.h"

UCLASS()
class TEST_TERRAIN_API ATargetActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATargetActor();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	int ObjectID = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	int StateObject = 0;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere)
	UScreenBoundsComponent* ScreenBounds;
};
