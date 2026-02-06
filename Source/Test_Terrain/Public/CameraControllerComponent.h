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
	void UpdateCameraTransform(int32 ShotIndex);
	
	void Init(ACameraActor* Cam, AActor* TargetActor)
	{
		BaseCamera = Cam;
		RenderCamera = Cam->GetCameraComponent();
		SelectedActor = TargetActor;
	};
	
	UFUNCTION()
	void SetActor(AActor* TargetActor) {SelectedActor = TargetActor;};
	
	UFUNCTION()
	int GetMaxShot() const {return MaxShot;};

	UFUNCTION()
	int GetMaxShotOneObjects() const {return MaxShotOneObjects;};
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	int CurrentShot = 0;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	int MaxShot = 10;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	int MaxShotOneObjects = 2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitRadiusMin = 1300.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitRadiusMax = 1800.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitHeightMin = 7500.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitHeightMax = 9200.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera",meta = (ClampMin="0.0", ClampMax="7.5"))
	float JitterDegrees = 5.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera",meta = (ClampMin="0.0", ClampMax="500"))
	float JitterLocation = 150.f;
	
private:
	UPROPERTY()
	UCameraComponent* RenderCamera = nullptr;
	
	UPROPERTY()
	AActor* SelectedActor = nullptr;
	
	UPROPERTY()
	ACameraActor* BaseCamera = nullptr;
};
