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
	
	for (AActor* A : CachedActors)
	{
		if (!IsValid(A)) continue;

		UScreenBoundsComponent* Bounds = A->FindComponentByClass<UScreenBoundsComponent>();
		if (!Bounds) continue;

		//FScreenBox Box;
		//if (Bounds->IsVisible(Box))
		// {
		// 	float X1 = Box.Min.X;
		// 	float Y1 = Box.Min.Y;
		// 	float X2 = Box.Max.X;
		// 	float Y2 = Box.Max.Y;
		//
		// 	// толщина рамки
		// 	float Thickness = 3.f;
		// 	FLinearColor Color = FLinearColor(0, 1, 0);
		//
		// 	DrawLine(X1, Y1, X2, Y1, Color, Thickness);
		// 	DrawLine(X2, Y1, X2, Y2, Color, Thickness);
		// 	DrawLine(X2, Y2, X1, Y2, Color, Thickness);
		// 	DrawLine(X1, Y2, X1, Y1, Color, Thickness);
		// }
	}
}

void AObjectBoundsHUD::BeginPlay()
{
	Super::BeginPlay();

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), DetectionTag, CachedActors);

	UE_LOG(LogTemp, Warning, TEXT("Cached %d actors with tag %s"), 
		CachedActors.Num(), *DetectionTag.ToString());
}
