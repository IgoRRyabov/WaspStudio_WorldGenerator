#include "CameraControllerComponent.h"

#include "Kismet/KismetMathLibrary.h"

UCameraControllerComponent::UCameraControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCameraControllerComponent::UpdateCameraTransform()
{
	if (!SelectedActor || !RenderCamera)return;
	
	if (CurrentShot >= MaxShot) CurrentShot = 0;
	
	const float Alpha = (float)(CurrentShot % MaxShot) / (float)MaxShot;
	const float AngleDeg = Alpha * 360.f;
	
	const FVector Center = SelectedActor->GetActorLocation();
	const float Rad = FMath::DegreesToRadians(AngleDeg);
	
	FVector Pos;
	Pos.X = Center.X + FMath::Cos(Rad) * OrbitRadius;
	Pos.Y = Center.Y + FMath::Sin(Rad) * OrbitRadius;
	Pos.Z = Center.Z + OrbitHeight;
	
	Pos += FVector{
	FMath::RandRange(-JitterLocation, JitterLocation),
	FMath::RandRange(-JitterLocation, JitterLocation),
	FMath::RandRange(-JitterLocation * 0.5f, JitterLocation) * 0.5f};
	
	FRotator LookAtRotator = UKismetMathLibrary::FindLookAtRotation(Pos, Center);
	LookAtRotator.Yaw += FMath::RandRange(-JitterDegrees, JitterDegrees);
	LookAtRotator.Pitch += FMath::RandRange(-JitterDegrees, JitterDegrees);
	
	BaseCamera->SetActorLocation(Pos);
	BaseCamera->SetActorRotation(LookAtRotator);
	
	CurrentShot++;
}



void UCameraControllerComponent::BeginPlay()
{
	Super::BeginPlay();
}
