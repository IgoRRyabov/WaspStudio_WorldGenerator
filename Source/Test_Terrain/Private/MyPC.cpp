#include "MyPC.h"

#include "ObjectBoundsHUD.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "DataCaptureController.h"

AMyPC::AMyPC()
{
	BaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = BaseCamera;
}

void AMyPC::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<AActor*> Found;
	TArray<AActor*> cam;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("BoundsTarget"), Found);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), cam);

	//if (cam[0])
		//SetViewTargetWithBlend(cam[0]);
	//Camera = Cast<UCameraComponent>(cam[0]->GetComponentByClass(UCameraComponent::StaticClass()));
	
	
	UE_LOG(LogTemp, Warning, TEXT("Found %d bounds actors"), Found.Num());

	Init();
	
	if (Found.Num() == 0)
		return;

	AObjectBoundsHUD* H = Cast<AObjectBoundsHUD>(GetHUD());
	if (!H)
		return;

	// перебираем найденные акторы
	for (AActor* Target : Found)
	{
		if (!Target) continue;

		UScreenBoundsComponent* Comp =
			Target->FindComponentByClass<UScreenBoundsComponent>();

		if (Comp)
		{
			H->BoundComponents.Add(Comp);
			UE_LOG(LogTemp, Warning, TEXT("Added %s to HUD bounds list"),
				*Target->GetName());
		}
	}
}

void AMyPC::Init()
{
	// DataCapture = GetWorld()->SpawnActor<ADataCaptureController>(DataCaptureController);
	// DataCapture->Init(BaseCamera);
	// UE_LOG(LogTemp, Warning, TEXT("Spawn %s"), *DataCapture->GetName());
}
