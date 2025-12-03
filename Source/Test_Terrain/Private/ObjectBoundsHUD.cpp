#include "ObjectBoundsHUD.h"

#include "TargetActor.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void AObjectBoundsHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	// ищем акторы по тегу
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), DetectionTag, Actors);

	for (AActor* A : Actors)
	{
		if (!A) continue;

		UScreenBoundsComponent* B = A->FindComponentByClass<UScreenBoundsComponent>();
		if (!B) continue;

		FScreenBox Box;
		if (!B->IsVisible(Box))
			continue; // камера НЕ видит — не рисуем рамку

		float X1 = Box.Min.X;
		float Y1 = Box.Min.Y;
		float X2 = Box.Max.X;
		float Y2 = Box.Max.Y;

		// толщина рамки
		float Thickness = 3.f;
		FLinearColor Color = FLinearColor(0, 1, 0);

		DrawLine(X1, Y1, X2, Y1, Color, Thickness);
		DrawLine(X2, Y1, X2, Y2, Color, Thickness);
		DrawLine(X2, Y2, X1, Y2, Color, Thickness);
		DrawLine(X1, Y2, X1, Y1, Color, Thickness);
	}
}
