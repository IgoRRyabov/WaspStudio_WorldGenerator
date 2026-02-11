#include "DataCaptureController.h"

#include "MyGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "ScreenBoundsComponent.h"
#include "TargetActor.h"
#include "VectorUtil.h"
#include "Camera/CameraActor.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
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
		//NextActor();
		//CameraControllerComponent->UpdateCameraTransform();
	}
	OutputDirBaseName = GI->VehicleSpawnSettings.OutputDirBaseName;
	
	FString path = GI->GameRootDir;
	OutputDirBaseName = FPaths::Combine(path, *OutputDirBaseName);
	OutputDir = FPaths::Combine(OutputDirBaseName, "DataTXT");
		OutputDirBaseName = FPaths::Combine(OutputDirBaseName, "Image");

	SetParams();
	ApplyTimeOfDay();
	ApplyFogLevel();
}

void ADataCaptureController::SetParams()
{
	CameraControllerComponent->MaxShot = GI->VehicleSpawnSettings.MaxShot;
	CameraControllerComponent->OrbitHeightMin = GI->VehicleSpawnSettings.OrbitHeightMin;
	CameraControllerComponent->OrbitHeightMax = GI->VehicleSpawnSettings.OrbitHeightMax;
	CameraControllerComponent->OrbitRadiusMin = GI->VehicleSpawnSettings.OrbitRadiusMin;
	CameraControllerComponent->OrbitRadiusMax = GI->VehicleSpawnSettings.OrbitRadiusMax;
	CameraControllerComponent->JitterDegrees = GI->VehicleSpawnSettings.JitterDegrees;
	CameraControllerComponent->JitterLocation = GI->VehicleSpawnSettings.JitterLocation;
	CameraControllerComponent->MaxShotOneObjects = GI->VehicleSpawnSettings.MaxShotOneObjects;
	
	RenderH = GI->VehicleSpawnSettings.RenderH;
	RenderW = GI->VehicleSpawnSettings.RenderW;
	
	SpatialSampleCount = GI->VehicleSpawnSettings.SpatialSampleCount;
	TemporalSampleCount = GI->VehicleSpawnSettings.TemporalSampleCount;
}


void ADataCaptureController::UpdateCameraTransform()
{
	if (CameraControllerComponent)
		CameraControllerComponent->UpdateCameraTransform(LocalCurrentShot);
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

bool ADataCaptureController::IsBoxAtLeastVisiblePercent(const FScreenBox& Box, int32 W, int32 H, float MinPercent)
{
	if (!Box.IsValid()) return false;
	
	const float FullArea = Box.Area();
	if (FullArea <= KINDA_SMALL_NUMBER) return false;
	
	const FScreenBox Clamped = Box.IntersectRender(W,H);
	if (!Clamped.IsValid()) return false;
	
	const float VisibleArea = Clamped.Area();
	const float Ratio = VisibleArea / FullArea;
	
	UE_LOG(LogTemp, Warning, TEXT("Ratio: %f"), Ratio);
	
	return Ratio >= MinPercent;
}

void ADataCaptureController::CaptureAndRenderOneFrame()
{
	if (CurrentShot >= CameraControllerComponent->GetMaxShot())
	{
		UE_LOG(LogTemp, Warning, TEXT("Capture finished: %d shots"), CameraControllerComponent->GetMaxShot());
		FinishRender();
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
	StartRenderImage(MyPC, RenderW, RenderH, FrameBaseName, OutputDirBaseName, SpatialSampleCount, TemporalSampleCount);
}

void ADataCaptureController::SaveTxtForCurrentFrame(const FString& FrameBaseName)
{
	FString Txt;
	Txt.Reserve(Actors.Num() * 64);
	
	int32 Saved = 0;

	Actors.Reset();
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
		
		FString DirTA = TA->FolderId;
		FString GameFolder = GI->GameDataUserDir;
		
		FString Path = FPaths::Combine(GameFolder, DirTA);
		FString FinalPath = FPaths::Combine(Path, TEXT("params.txt"));
		
		FString Text;
		if (FFileHelper::LoadFileToString(Text, *FinalPath))
		{
			Txt += FString::Printf(TEXT("%s, %d, %d, %d, %d\n"), *Text, MinX, MinY, MaxX, MaxY);
		}else
		{
			Txt += FString::Printf(TEXT("class1_%d, class2_%d, %d, %d, %d, %d\n"), TA->ObjectID, TA->StateObject,  MinX, MinY, MaxX, MaxY);
		}
		
		
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
	//UGameplayStatics::GetAllActorsWithTag(GetWorld(), DetectionTag, AllActors);
	if (AllActors.IsEmpty()) UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetActor::StaticClass(), AllActors);
	
	if (AllActors.Num() == 0) return;
	
	// Reset if we reached the end
	if (AllActors.IsValidIndex(NumActors) == false)
	{
		NumActors = 0;
	}
	
	// Check if specific actor is valid (destroyed?) - though rarely happens if we just warned
	if (!AllActors[NumActors]) 
	{
		NumActors = 0; // Fallback or increment? 
	}
	
	if (CameraControllerComponent && AllActors.IsValidIndex(NumActors))
		CameraControllerComponent->SetActor(AllActors[NumActors]);
		
	UpdateCameraTransform();
	
	NumActors++;
}

void ADataCaptureController::ApplyTimeOfDay()
{
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyTimeOfDay: GameInstance not found"));
		return;
	}
	
	// Find DirectionalLight (sun)
	TArray<AActor*> Lights;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADirectionalLight::StaticClass(), Lights);
	if (Lights.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyTimeOfDay: No DirectionalLight found"));
		return;
	}
	
	ADirectionalLight* Sun = Cast<ADirectionalLight>(Lights[0]);
	if (!Sun)
	{
		return;
	}
	
	const ETimeOfDay Time = GI->VehicleSpawnSettings.TimeOfDay;
	
	FRotator NewRotation;
	float Intensity = 10.0f;
	FLinearColor LightColor = FLinearColor::White;
	
	// Post-process settings
	float ExposureBias = 0.0f;
	FLinearColor ColorGrading = FLinearColor::White;
	float SkyLightIntensity = 1.0f;
	
	switch (Time)
	{
		case ETimeOfDay::Morning:
			// Golden hour - low sun from East, warm tones
			NewRotation = FRotator(-20.0f, 80.0f, 0.0f);
			Intensity = 6.0f;
			LightColor = FLinearColor(1.0f, 0.75f, 0.5f);  // Golden orange
			ExposureBias = 0.5f;  // Slightly brighter
			ColorGrading = FLinearColor(1.0f, 0.95f, 0.85f);  // Warm tint
			SkyLightIntensity = 0.8f;
			break;
			
		case ETimeOfDay::Day:
			// High noon - sun at zenith, neutral bright
			NewRotation = FRotator(-85.0f, 0.0f, 0.0f);
			Intensity = 10.0f;
			LightColor = FLinearColor(1.0f, 0.98f, 0.95f);
			ExposureBias = 0.0f;
			ColorGrading = FLinearColor::White;
			SkyLightIntensity = 1.0f;
			break;
			
		case ETimeOfDay::Evening:
			// Sunset - low sun from West, deep orange/red
			NewRotation = FRotator(-15.0f, 260.0f, 0.0f);
			Intensity = 4.0f;
			LightColor = FLinearColor(1.0f, 0.5f, 0.2f);  // Deep sunset orange
			ExposureBias = 0.8f;  // Compensate for low intensity
			ColorGrading = FLinearColor(1.0f, 0.85f, 0.7f);  // Warm tint
			SkyLightIntensity = 0.6f;
			break;
			
		case ETimeOfDay::Night:
			// Moonlit night - dark but objects visible
			NewRotation = FRotator(-45.0f, 180.0f, 0.0f);
			Intensity = 2.0f;  // Low moonlight
			LightColor = FLinearColor(0.3f, 0.4f, 0.7f);  // Deep blue
			ExposureBias = -3.0f;  // Very dark exposure
			ColorGrading = FLinearColor(0.5f, 0.6f, 0.9f);  // Strong blue tint
			SkyLightIntensity = 0.2f;
			break;
	}
	
	// Apply sun rotation and lighting
	Sun->SetActorRotation(NewRotation);
	if (ULightComponent* LightComp = Sun->GetLightComponent())
	{
		LightComp->SetIntensity(Intensity);
		LightComp->SetLightColor(LightColor);
	}
	
	// Apply SkyLight intensity and recapture
	TArray<AActor*> SkyLights;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASkyLight::StaticClass(), SkyLights);
	for (AActor* SL : SkyLights)
	{
		if (ASkyLight* Sky = Cast<ASkyLight>(SL))
		{
			if (USkyLightComponent* SkyComp = Sky->GetLightComponent())
			{
				SkyComp->SetIntensity(SkyLightIntensity);
				SkyComp->RecaptureSky();
			}
		}
	}
	
	// Apply camera post-process settings
	if (RenderCamera)
	{
		FPostProcessSettings& PP = RenderCamera->PostProcessSettings;
		
		// Enable and set exposure bias
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = ExposureBias;
		
		// Color grading - global color multiplier
		PP.bOverride_ColorGain = true;
		PP.ColorGain = FVector4(ColorGrading.R, ColorGrading.G, ColorGrading.B, 1.0f);
		
		// Bloom for atmospheric effect
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = (Time == ETimeOfDay::Morning || Time == ETimeOfDay::Evening) ? 0.8f : 0.5f;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ApplyTimeOfDay: Set to %d, Intensity=%.1f, Exposure=%.1f"), 
		static_cast<int32>(Time), Intensity, ExposureBias);
}

void ADataCaptureController::ApplyFogLevel()
{
	if (!GI) return;

	// Find Fog Actor
	TArray<AActor*> Fogs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AExponentialHeightFog::StaticClass(), Fogs);
	
	AExponentialHeightFog* FogActor = nullptr;
	if (Fogs.Num() > 0)
	{
		FogActor = Cast<AExponentialHeightFog>(Fogs[0]);
	}
	
	if (!FogActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyFogLevel: No ExponentialHeightFog found in scene"));
		return;
	}

	UExponentialHeightFogComponent* FogComp = FogActor->GetComponent();
	if (!FogComp) return;

	const EFogLevel Level = GI->VehicleSpawnSettings.FogLevel;
	
	float Density = 0.0f;
	float Falloff = 0.2f;
	float StartDist = 0.0f;
	bool bVolumetric = false;
	
	switch (Level)
	{
		case EFogLevel::None:
			Density = 0.000001;
			break;
			
		case EFogLevel::Light:
			Density = 0.004f;
			Falloff = 0.1f;
			StartDist = 500.0f;
			bVolumetric = true;
			break;
			
		case EFogLevel::Medium:
			Density = 0.004f;
			Falloff = 0.13f;
			StartDist = 400.0f;
			bVolumetric = true;
			break;
			
		case EFogLevel::Heavy:
			Density = 0.01f;
			Falloff = 0.07f;
			StartDist = 250.0f;
			bVolumetric = true;
			break;
	}
	
	FogComp->SetFogDensity(Density);
	FogComp->SetFogHeightFalloff(Falloff);
	FogComp->SetStartDistance(StartDist);
	FogComp->SetVolumetricFog(bVolumetric);
	
	// If heavy fog, maybe adjust max opacity?
	if (Level == EFogLevel::Heavy)
	{
		FogComp->SetFogMaxOpacity(1.0f);
	}
	else
	{
		FogComp->SetFogMaxOpacity(1.0f); // Default
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ApplyFogLevel: Set to %d, Density=%.3f"), 
		static_cast<int32>(Level), Density);
}
