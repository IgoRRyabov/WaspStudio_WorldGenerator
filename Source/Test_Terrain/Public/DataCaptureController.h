#pragma once

#include "CoreMinimal.h"
#include "MyPC.h"
#include "Camera/CameraComponent.h"
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

	// BP реализует: запускает MoviePipeline рендер одного кадра
	UFUNCTION(BlueprintImplementableEvent, Category="Dataset")
	void StartRenderImage(AMyPC* PC, int32 InRenderW, int32 InRenderH, const FString& FrameBaseName);

	// BP должен вызвать это, когда кадр отрендерен
	UFUNCTION(BlueprintCallable, Category="Dataset")
	void NotifyRenderFinished();

	UPROPERTY(EditAnywhere, Category="Dataset")
	FName DetectionTag = "Detectable";

	UPROPERTY(EditAnywhere, Category="Dataset")
	FString OutputDir = TEXT("C:/Dataset");

	UPROPERTY(EditAnywhere, Category="Dataset")
	int32 RenderW = 1920;

	UPROPERTY(EditAnywhere, Category="Dataset")
	int32 RenderH = 1080;

	UPROPERTY(EditAnywhere, Category="Dataset")
	int32 NumShots = 1;

	UPROPERTY(EditAnywhere, Category="Dataset")
	ACameraActor* RenderCameraActor = nullptr;

	UFUNCTION(BlueprintCallable)
	void Start();
private:
	UPROPERTY()
	AMyPC* MyPC = nullptr;

	UPROPERTY()
	UCameraComponent* RenderCamera = nullptr;
	
	int32 CurrentShot = 0;

	void CaptureAndRenderOneFrame();
	void SaveTxtForCurrentFrame(const FString& FrameBaseName);
};
