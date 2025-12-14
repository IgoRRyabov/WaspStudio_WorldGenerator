#include "DataCaptureController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "ScreenBoundsComponent.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

ADataCaptureController::ADataCaptureController()
{
	PrimaryActorTick.bCanEverTick = false;

}

void ADataCaptureController::BeginPlay()
{
	Super::BeginPlay();

	MyPC = Cast<AMyPC>(GetWorld()->GetFirstPlayerController());
	if (!MyPC)
	{
		UE_LOG(LogTemp, Error, TEXT("DataCaptureController: MyPC not found"));
		return;
	}

	if (RenderCameraActor)
	{
		RenderCamera = RenderCameraActor->FindComponentByClass<UCameraComponent>();
		//MyPC->SetViewTargetWithBlend(RenderCameraActor);
	}

	if (!MyPC || !RenderCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("PC or RenderCamera not set"));
	}
}



void ADataCaptureController::NotifyRenderFinished()
{
	CurrentShot++;
	CaptureAndRenderOneFrame();
}

void ADataCaptureController::Start()
{
	CurrentShot = 0;
	CaptureAndRenderOneFrame();
}

void ADataCaptureController::CaptureAndRenderOneFrame()
{
	if (CurrentShot >= NumShots)
	{
		UE_LOG(LogTemp, Warning, TEXT("Capture finished: %d shots"), NumShots);
		return;
	}

	const FString FrameBaseName = FString::Printf(TEXT("frame_%05d"), CurrentShot);

	// 1) Сохраняем аннотацию именно под RenderW/RenderH
	SaveTxtForCurrentFrame(FrameBaseName);

	// 2) Запускаем Blueprint рендер
	StartRenderImage(MyPC, RenderW, RenderH, FrameBaseName);
}

void ADataCaptureController::SaveTxtForCurrentFrame(const FString& FrameBaseName)
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), DetectionTag, Actors);

	FString Txt;
	Txt.Reserve(Actors.Num() * 64);

	int32 Saved = 0;

	for (AActor* A : Actors)
	{
		if (!IsValid(A)) continue;

		UScreenBoundsComponent* Bounds = A->FindComponentByClass<UScreenBoundsComponent>();
		if (!Bounds) continue;

		FScreenBox Box;
		if (!Bounds->ComputeRenderBounds(RenderCamera, RenderW, RenderH, Box))
			continue;

		// clamp в пределах рендера
		Box.Min.X = FMath::Clamp(Box.Min.X, 0.f, float(RenderW - 1));
		Box.Min.Y = FMath::Clamp(Box.Min.Y, 0.f, float(RenderH - 1));
		Box.Max.X = FMath::Clamp(Box.Max.X, 0.f, float(RenderW - 1));
		Box.Max.Y = FMath::Clamp(Box.Max.Y, 0.f, float(RenderH - 1));

		if (!Box.IsValid())
			continue;

		const int32 MinX = FMath::RoundToInt(Box.Min.X);
		const int32 MinY = FMath::RoundToInt(Box.Min.Y);
		const int32 MaxX = FMath::RoundToInt(Box.Max.X);
		const int32 MaxY = FMath::RoundToInt(Box.Max.Y);

		Txt += FString::Printf(TEXT("%s %d %d %d %d\n"), *A->GetName(), MinX, MinY, MaxX, MaxY);
		++Saved;
	}

	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*OutputDir);

	const FString FullPath = FPaths::Combine(OutputDir, FrameBaseName + TEXT(".txt"));
	FFileHelper::SaveStringToFile(Txt, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8);

	UE_LOG(LogTemp, Warning, TEXT("[Dataset] %s: saved %d boxes"), *FrameBaseName, Saved);
}