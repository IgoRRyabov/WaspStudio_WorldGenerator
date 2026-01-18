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
	
	float OrbitHeight = FMath::FRandRange(OrbitHeightMin, OrbitHeightMax);
	float OrbitRadius = FMath::FRandRange(OrbitRadiusMin, OrbitRadiusMax);
	
	FVector Pos;
	Pos.X = Center.X + FMath::Cos(Rad) * OrbitRadius;
	Pos.Y = Center.Y + FMath::Sin(Rad) * OrbitRadius;
	Pos.Z = Center.Z + OrbitHeight;
	
	Pos += FVector{
	FMath::FRandRange(-JitterLocation, JitterLocation),
	FMath::FRandRange(-JitterLocation, JitterLocation),
	FMath::FRandRange(-JitterLocation * 0.5f, JitterLocation) * 0.5f};
	
	FRotator LookAtRotator = UKismetMathLibrary::FindLookAtRotation(Pos, Center);
	LookAtRotator.Yaw += FMath::FRandRange(-JitterDegrees, JitterDegrees);
	LookAtRotator.Pitch += FMath::FRandRange(-JitterDegrees, JitterDegrees);
	
	BaseCamera->SetActorLocation(Pos);
	BaseCamera->SetActorRotation(LookAtRotator);
	
	CurrentShot++;
}



void UCameraControllerComponent::BeginPlay()
{
	Super::BeginPlay();
}
