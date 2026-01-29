#include "VehicleSpawnArea.h"
#include "MyGameInstance.h"
#include "ScreenPass.h"
#include "TargetActor.h"
#include "VehicleSpawnBudgetSubsystem.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

AVehicleSpawnArea::AVehicleSpawnArea()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);

	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AVehicleSpawnArea::BeginPlay()
{
	Super::BeginPlay();
	
	if (VehicleClass.IsEmpty() || !GetWorld())
		return;
	
	GI = GetGameInstance<UMyGameInstance>();
	if (!GI) return;
	
	RandomRot = FRotator::MakeFromEuler(FVector(0.f, 0.f, FMath::FRandRange(0.f, 360.f)));
	
	CalculatePosition();
}

void AVehicleSpawnArea::CalculatePosition()
{
	if (!GI) return;

	for (int32 i = 0; i < GI->VehicleSpawnSettings.TotalVehicles; ++i)
	{
		TrySpawnOne();
	}
}

void AVehicleSpawnArea::TrySpawnOne()
{
	if (VehicleClass.IsEmpty() || !GetWorld() || !GI) return;

	const float MinDist = GI->VehicleSpawnSettings.MinDistanceBetweenCenters * 100;               // лучше брать из настроек
	const float MinDistSq = FMath::Square(MinDist);

	FVector Loc;
	bool bFound = false;

	for (int32 Attempt = 0; Attempt < 10; ++Attempt)
	{
		if (!CalcRandomPoint(Loc)) // <- сделаем bool
			continue;

		bool bTooClose = false;

		for (const FVector& P : SpawnedPointsWorld)
		{
			if (FVector::DistSquared2D(P, Loc) < MinDistSq)
			{
				bTooClose = true;
				break; // нашли конфликт — дальше проверять не надо
			}
		}

		if (!bTooClose)
		{
			bFound = true;
			break;
		}
	}

	if (!bFound) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	
	float RandOffset = UKismetMathLibrary::RandomFloatInRange(0, 30.f);
	RandomRot.Yaw += RandOffset;
	
	int count = UKismetMathLibrary::RandomIntegerInRange(0, VehicleClass.Num() - 1);
	
	ATargetActor* Vehicle = GetWorld()->SpawnActor<ATargetActor>(VehicleClass[count], Loc, RandomRot, Params);
	if (!Vehicle) return;

	SpawnedPointsWorld.Add(Loc);
}

bool AVehicleSpawnArea::CalcRandomPoint(FVector& OutPoint) const
{
	if (!GetWorld() || !Box) return false;

	const FVector Center = Box->GetComponentLocation();      // WORLD
	const FVector Extent = Box->GetScaledBoxExtent();        // WORLD size

	// Случайная точка по XY в пределах бокса (Z берем от Center)
	const float X = FMath::FRandRange(Center.X - Extent.X, Center.X + Extent.X);
	const float Y = FMath::FRandRange(Center.Y - Extent.Y, Center.Y + Extent.Y);

	const float TraceUp = 5000.f;
	const float TraceDown = 20000.f;

	const FVector Start(X, Y, Center.Z + TraceUp);
	const FVector End  (X, Y, Center.Z - TraceDown);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_GameTraceChannel1
	);

	if (!bHit) return false;

	OutPoint = Hit.ImpactPoint;
	return true;
}
