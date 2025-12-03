#include "ScreenBoundsComponent.h"

#include "Kismet/GameplayStatics.h"


UScreenBoundsComponent::UScreenBoundsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FScreenBounds2D UScreenBoundsComponent::ComputeScreenBounds() const
{
	FScreenBounds2D Out;

	AActor* Owner = GetOwner();
	if (!Owner)
		return Out;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
		return Out;

	// 1) Берём мировую AABB всех компонентов
	FBox Box = Owner->GetComponentsBoundingBox(true);
	if (!Box.IsValid)
		return Out;

	const FVector Min = Box.Min;
	const FVector Max = Box.Max;

	// 8 углов бокса
	TArray<FVector> Corners;
	Corners.SetNum(8);
	Corners[0] = FVector(Min.X, Min.Y, Min.Z);
	Corners[1] = FVector(Min.X, Min.Y, Max.Z);
	Corners[2] = FVector(Min.X, Max.Y, Min.Z);
	Corners[3] = FVector(Min.X, Max.Y, Max.Z);
	Corners[4] = FVector(Max.X, Min.Y, Min.Z);
	Corners[5] = FVector(Max.X, Min.Y, Max.Z);
	Corners[6] = FVector(Max.X, Max.Y, Min.Z);
	Corners[7] = FVector(Max.X, Max.Y, Max.Z);

	int32 MinX = TNumericLimits<int32>::Max();
	int32 MinY = TNumericLimits<int32>::Max();
	int32 MaxX = TNumericLimits<int32>::Lowest();
	int32 MaxY = TNumericLimits<int32>::Lowest();

	// 2) Проецируем все углы в экран
	for (const FVector& WorldPos : Corners)
	{
		FVector2D ScreenPos;
		const bool bProjected =
			UGameplayStatics::ProjectWorldToScreen(PC, WorldPos, ScreenPos, true);

		if (!bProjected)
			continue;

		MinX = FMath::Min(MinX, (int32)ScreenPos.X);
		MinY = FMath::Min(MinY, (int32)ScreenPos.Y);
		MaxX = FMath::Max(MaxX, (int32)ScreenPos.X);
		MaxY = FMath::Max(MaxY, (int32)ScreenPos.Y);
	}

	if (MinX == TNumericLimits<int32>::Max())
		return Out; // ничего не спроецировалось (объект вне камеры)

	Out.MinX = MinX;
	Out.MinY = MinY;
	Out.MaxX = MaxX;
	Out.MaxY = MaxY;

	return Out;
}


