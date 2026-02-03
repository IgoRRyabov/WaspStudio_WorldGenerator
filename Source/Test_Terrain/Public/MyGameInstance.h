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
	float MinDistanceBetweenCenters = 100.f;

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
	
	int32 VersionSave = 1;
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
	FString GetGameRootDir();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FString GameRootDir;
	
private:
	static FString GetSettingsFilePath();
};
