#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerController.h"
#include "MyPC.generated.h"


class UCameraComponent;
class ADataCaptureController;

UCLASS()
class TEST_TERRAIN_API AMyPC : public APlayerController
{
	GENERATED_BODY()

public:

	AMyPC();
	
	virtual void BeginPlay() override;
	
	UCameraComponent* GetCameraComponent() const { return BaseCamera; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	UCameraComponent* BaseCamera;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dataset")
	TSubclassOf<ADataCaptureController> DataCaptureController;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dataset")
	ADataCaptureController* DataCapture = nullptr;
	
private:
	
	void Init();
	UCameraComponent* Camera;
};
