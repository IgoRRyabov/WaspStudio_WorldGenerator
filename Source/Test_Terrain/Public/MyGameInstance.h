#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FVehicleSpawnSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 TotalVehicles = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 ClusterSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	float ClusterRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	float TraceUp = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	float TraceDown = 20000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	float GroundOffsetZ = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	float MinDistanceBetweenCenters = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 MaxPointTries = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	int MaxShot = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	int MaxShotOneObjects = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitRadiusMin = 1300.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitRadiusMax = 1800.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitHeightMin = 7000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float OrbitHeightMax = 9000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float JitterDegrees = 5.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float JitterLocation = 150.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 Seed = 9992;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 VertexSampleStep = 4;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Path")
	FString OutputDirBaseName = TEXT("BaseData");
	
	// Проверка на размер объекта, и на попадание в кадр
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Filter")
	float MinInFrameBBoxRatio = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Filter")
	float MinVisibleAreaPx = 900.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Filter")
	float MinVisibleWidthPx = 20.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Filter")
	float MinVisibleHeightPx = 20.f;
	
	// Включить проверку окклюзии
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Occlusion")
	bool bUseOcclusionFilter = true;

	// Минимальная доля "видимых" точек (0..1). Например 0.5 = хотя бы 50% точек без перекрытий
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Occlusion", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MinVisiblePointRatio = 95.f;

	// Максимум трасс на объект (чтобы не убить производительность)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Occlusion", meta=(ClampMin="8", ClampMax="4096"))
	int32 MaxOcclusionRays = 15;

	// Минимум трасс, чтобы решение было "стабильным"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Occlusion", meta=(ClampMin="8", ClampMax="512"))
	int32 MinOcclusionRays = 32;

	// Канал трассировки: обычно Visibility
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Occlusion")
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

	// Трассить Complex (по треугольникам) — точнее, но дороже
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bounds|Occlusion")
	bool bOcclusionTraceComplex = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics")
	int SpatialSampleCount = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics")
	int TemporalSampleCount = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics")
	int32 RenderW = 1920;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics")
	int32 RenderH = 1080;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics")
	int32 InderRenderWAndH = 0;
	
	int32 VersionSave = 3;
};


UCLASS()
class TEST_TERRAIN_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	FVehicleSpawnSettings VehicleSpawnSettings;
	
	UFUNCTION(BlueprintCallable, Category="Settings")
	bool SaveVehicleSettingsToJson();

	UFUNCTION(BlueprintCallable, Category="Settings")
	bool LoadVehicleSettingsFromJson();

	UFUNCTION()
	FString GetGameDataUserDir();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FString GameDataUserDir;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FString GameRootDir;
	
private:
	static FString GetSettingsFilePath();
	static FString GetGameFilePath();
};
