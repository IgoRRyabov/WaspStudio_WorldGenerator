#include "CameraControllerComponent.h"

#include "Kismet/KismetMathLibrary.h"

UCameraControllerComponent::UCameraControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCameraControllerComponent::UpdateCameraTransform(int32 ShotIndex)
{
	if (!SelectedActor || !RenderCamera) return;
	
	// Ensure we handle division by zero
	const int32 TotalShots = (MaxShotOneObjects > 0) ? MaxShotOneObjects : 1;
	const int32 SafeIndex = ShotIndex % TotalShots;
	
	const float Alpha = (float)SafeIndex / (float)TotalShots;
	const float AngleDeg = Alpha * 360.f;
	
	const FVector Center = SelectedActor->GetActorLocation();
	const float Rad = FMath::DegreesToRadians(AngleDeg);
	
	float OrbitHeight = FMath::FRandRange(OrbitHeightMin * 100, OrbitHeightMax * 100);
	float OrbitRadius = FMath::FRandRange(OrbitRadiusMin * 100, OrbitRadiusMax * 100);
	
	FVector Pos;
	Pos.X = Center.X + FMath::Cos(Rad) * OrbitRadius;
	Pos.Y = Center.Y + FMath::Sin(Rad) * OrbitRadius;
	Pos.Z = Center.Z + OrbitHeight;
	
	Pos += FVector{
	FMath::FRandRange(-JitterLocation * 100, JitterLocation * 100),
	FMath::FRandRange(-JitterLocation * 100, JitterLocation * 100),
	FMath::FRandRange(-JitterLocation * 100 * 0.5f, JitterLocation * 100) * 0.5f};
	
	FRotator LookAtRotator = UKismetMathLibrary::FindLookAtRotation(Pos, Center);
	LookAtRotator.Yaw += FMath::FRandRange(-JitterDegrees, JitterDegrees);
	LookAtRotator.Pitch += FMath::FRandRange(-JitterDegrees, JitterDegrees);
	
	BaseCamera->SetActorLocation(Pos);
	BaseCamera->SetActorRotation(LookAtRotator);
	
	CurrentShot = ShotIndex; // Updating internal state just in case
}



void UCameraControllerComponent::BeginPlay()
{
	Super::BeginPlay();
}
