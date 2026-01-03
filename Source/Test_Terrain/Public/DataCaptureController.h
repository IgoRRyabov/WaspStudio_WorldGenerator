#pragma once

#include "CoreMinimal.h"
#include "CameraControllerComponent.h"
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
	
	UFUNCTION(BlueprintCallable)
	void UpdateCameraTransform();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Render")
	ACameraActor* CameraActor = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Render")
	AActor* TargetSceneActor = nullptr;
	
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

	UFUNCTION(BlueprintCallable)
	void Start();
	
	void Init(UCameraComponent* CamComp) {RenderCamera = CamComp;};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dataset")
	UCameraComponent* RenderCamera = nullptr;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Render")
	UCameraControllerComponent* CameraControllerComponent = nullptr;
	
private:
	UPROPERTY()
	TArray<AActor*> Actors;
	
	UPROPERTY()
	TArray<AActor*> AllActors;
	
	UPROPERTY()
	AMyPC* MyPC = nullptr;
	
	int32 CurrentShot = 0;
	int32 LocalCurrentShot = 0;

	void CaptureAndRenderOneFrame();
	void SaveTxtForCurrentFrame(const FString& FrameBaseName);

	void NextActor();
	int32 NumActors = 0;
};
