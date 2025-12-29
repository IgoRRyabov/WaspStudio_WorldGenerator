#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "CameraControllerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEST_TERRAIN_API UCameraControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCameraControllerComponent();

	UFUNCTION(BlueprintCallable)
	void UpdateCameraTransform();
	
	void Init(ACameraActor* Cam, AActor* TargetActor)
	{
		BaseCamera = Cam;
		RenderCamera = Cam->GetCameraComponent();
		SelectedActor = TargetActor;
	};
	
	UFUNCTION()
	void SetActor(AActor* TargetActor) {SelectedActor = TargetActor;};
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	int CurrentShot = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	int MaxShot = 10;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitRadius = 1500.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitHeight = 800.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float JitterDegrees = 12.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float JitterLocation = 150.f;
	
private:
	UPROPERTY()
	UCameraComponent* RenderCamera = nullptr;
	
	UPROPERTY()
	AActor* SelectedActor = nullptr;
	
	UPROPERTY()
	ACameraActor* BaseCamera = nullptr;
};
