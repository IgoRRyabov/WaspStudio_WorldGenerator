#include "DataCaptureController.h"

#include "MyGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "ScreenBoundsComponent.h"
#include "TargetActor.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

ADataCaptureController::ADataCaptureController()
{
	PrimaryActorTick.bCanEverTick = false;

	CameraControllerComponent = CreateDefaultSubobject<UCameraControllerComponent>(TEXT("CameraControllerComponent"));
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
	
	GI = GetGameInstance<UMyGameInstance>();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleSpawnManager: GameInstance cast failed."));
		return;
	}
	
	if (!MyPC || !RenderCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("PC or RenderCamera not set"));
	}
	
	if (!CameraActor)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderCameraActor is NOT assigned"));
		return;
	}
	
	if (CameraControllerComponent)
	{
		CameraControllerComponent->Init(CameraActor, TargetSceneActor);
		NextActor();
		//CameraControllerComponent->UpdateCameraTransform();
	}
	
	SetParams();
}

void ADataCaptureController::SetParams() const
{
	CameraControllerComponent->MaxShot = GI->VehicleSpawnSettings.MaxShot;
	CameraControllerComponent->OrbitHeightMin = GI->VehicleSpawnSettings.OrbitHeightMin;
	CameraControllerComponent->OrbitHeightMax = GI->VehicleSpawnSettings.OrbitHeightMax;
	CameraControllerComponent->OrbitRadiusMin = GI->VehicleSpawnSettings.OrbitRadiusMin;
	CameraControllerComponent->OrbitRadiusMax = GI->VehicleSpawnSettings.OrbitRadiusMax;
	CameraControllerComponent->JitterDegrees = GI->VehicleSpawnSettings.JitterDegrees;
	CameraControllerComponent->JitterLocation = GI->VehicleSpawnSettings.JitterLocation;
	CameraControllerComponent->MaxShotOneObjects = GI->VehicleSpawnSettings.MaxShotOneObjects;
}

void ADataCaptureController::UpdateCameraTransform()
{
	if (CameraControllerComponent)
		CameraControllerComponent->UpdateCameraTransform();
}


void ADataCaptureController::NotifyRenderFinished()
{
	CurrentShot++;
	LocalCurrentShot++;
	UpdateCameraTransform();
	CaptureAndRenderOneFrame();
}

void ADataCaptureController::Start()
{
	CurrentShot = 0;
	LocalCurrentShot = 0;
	NextActor();
	CaptureAndRenderOneFrame();
}

void ADataCaptureController::CaptureAndRenderOneFrame()
{
	if (CurrentShot >= CameraControllerComponent->GetMaxShot())
	{
		UE_LOG(LogTemp, Warning, TEXT("Capture finished: %d shots"), CameraControllerComponent->GetMaxShot());
		return;
	}
	
	if (LocalCurrentShot >= CameraControllerComponent->GetMaxShotOneObjects())
	{
		NextActor();
		LocalCurrentShot = 0;
	}
	
	RenderCamera = CameraActor->GetCameraComponent();

	const FString FrameBaseName = FString::Printf(TEXT("frame_%05d"), CurrentShot);

	// 1) Сохраняем аннотацию именно под RenderW/RenderH
	SaveTxtForCurrentFrame(FrameBaseName);

	// 2) Запускаем Blueprint рендер
	StartRenderImage(MyPC, RenderW, RenderH, FrameBaseName);
}

void ADataCaptureController::SaveTxtForCurrentFrame(const FString& FrameBaseName)
{
	FString Txt;
	Txt.Reserve(Actors.Num() * 64);
	
	int32 Saved = 0;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), DetectionTag, Actors);
	
	for (AActor* A : Actors)
	{
		if (!IsValid(A)) continue;

		UScreenBoundsComponent* Bounds = A->FindComponentByClass<UScreenBoundsComponent>();
		if (!Bounds) continue;
		
		FScreenBox Box;
		if (!Bounds->ComputeBoundsFromCamera(RenderCamera, RenderW, RenderH, Box))
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

		ATargetActor* TA = Cast<ATargetActor>(A);
		
		Txt += FString::Printf(TEXT("class1_%d, class2_%d, %d, %d, %d, %d\n"), TA->ObjectID, TA->StateObject, MinX, MinY, MaxX, MaxY);
		++Saved;
	}

	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*OutputDir);

	const FString FullPath = FPaths::Combine(OutputDir, FrameBaseName + TEXT(".txt"));
	FFileHelper::SaveStringToFile(Txt, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8);

	UE_LOG(LogTemp, Warning, TEXT("Camera Render Pos = %f, %f, %f"), RenderCamera->GetComponentLocation().X, RenderCamera->GetComponentLocation().Y, RenderCamera->GetComponentLocation().Z);
	UE_LOG(LogTemp, Warning, TEXT("[Dataset] %s: saved %d boxes"), *FrameBaseName, Saved);
}

void ADataCaptureController::NextActor()
{
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), DetectionTag, AllActors);
	
	if (AllActors.Num() == 0) return;
	if (!AllActors[NumActors]) NumActors = 0;
	
	if (CameraControllerComponent)
		CameraControllerComponent->SetActor(AllActors[NumActors]);
	UpdateCameraTransform();
	
	NumActors++;
}
