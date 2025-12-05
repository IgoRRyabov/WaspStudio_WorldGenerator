#include "DataCaptureController.h"

ADataCaptureController::ADataCaptureController()
{
	PrimaryActorTick.bCanEverTick = false;

}

void ADataCaptureController::BeginPlay()
{
	Super::BeginPlay();

	MyPC = Cast<AMyPC>(GetWorld()->GetFirstPlayerController());
	
	//StartRenderImage(MyPC);
}

void ADataCaptureController::FinishRender()
{
}