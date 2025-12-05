#pragma once

#include "CoreMinimal.h"
#include "MyPC.h"
#include "GameFramework/Actor.h"
#include "DataCaptureController.generated.h"


class ACameraActor;
class UMoviePipelineQueue;
class UScreenBoundsComponent;

UCLASS()
class TEST_TERRAIN_API ADataCaptureController : public AActor
{
	GENERATED_BODY()
	
public:
	ADataCaptureController();
	
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
	void StartRenderImage(AMyPC* MyPlayerController);
	
	UFUNCTION(BlueprintCallable)
	void FinishRender();
private:
	AMyPC* MyPC = nullptr;
};
