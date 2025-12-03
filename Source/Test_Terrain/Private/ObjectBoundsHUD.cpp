#include "ObjectBoundsHUD.h"

#include "Engine/Canvas.h"

void AObjectBoundsHUD::DrawHUD()
{
	Super::DrawHUD();

	UE_LOG(LogTemp, Warning, TEXT("HUD TICK"));

	if (!Canvas) return;

	for (UScreenBoundsComponent* BoundsComponent : BoundComponents)
	{
		if (!BoundsComponent)
			continue;

		FScreenBounds2D B = BoundsComponent->ComputeScreenBounds();
		if (!B.IsValid())
			continue;

		const float X1 = (float)B.MinX;
		const float Y1 = (float)B.MinY;
		const float X2 = (float)B.MaxX;
		const float Y2 = (float)B.MaxY;

		const float Thickness = 2.f;
		const FLinearColor Color = FLinearColor::Green;

		DrawLine(X1, Y1, X2, Y1, Color, Thickness); // top
		DrawLine(X1, Y2, X2, Y2, Color, Thickness); // bottom
		DrawLine(X1, Y1, X1, Y2, Color, Thickness); // left
		DrawLine(X2, Y1, X2, Y2, Color, Thickness); // right
	}
}
