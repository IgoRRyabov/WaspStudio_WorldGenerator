#include "MyGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

void UMyGameInstance::Init()
{
	Super::Init();
	
	LoadVehicleSettingsFromJson();
}

bool UMyGameInstance::SaveVehicleSettingsToJson()
{
	FString OutJson;

	// true = pretty print
	if (!FJsonObjectConverter::UStructToJsonObjectString(
		FVehicleSpawnSettings::StaticStruct(),   // что сериализуем
		&VehicleSpawnSettings,                   // данные
		OutJson,
		0,                                       // CheckFlags
		0,                                       // SkipFlags
		0,                                       // Indent
		nullptr,                                 // ExportCallback
		true                                     // bPrettyPrint
	))
	{
		UE_LOG(LogTemp, Error, TEXT("UStructToJsonObjectString failed"));
		return false;
	}

	const FString Path = GetSettingsFilePath();
	if (!FFileHelper::SaveStringToFile(OutJson, *Path))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveVehicleSettingsToJson: SaveStringToFile failed: %s"), *Path);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Settings saved: %s"), *Path);
	return true;
}

bool UMyGameInstance::LoadVehicleSettingsFromJson()
{
	const FString Path = GetSettingsFilePath();

	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		// Файла нет — это нормально (останутся дефолты из структуры)
		UE_LOG(LogTemp, Warning, TEXT("LoadVehicleSettingsFromJson: file not found, using defaults: %s"), *Path);
		return false;
	}

	FVehicleSpawnSettings Loaded;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Loaded, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("LoadVehicleSettingsFromJson: JsonObjectStringToUStruct failed"));
		return false;
	}

	VehicleSpawnSettings = Loaded;
	UE_LOG(LogTemp, Log, TEXT("Settings loaded: %s"), *Path);
	return true;
}

FString UMyGameInstance::GetSettingsFilePath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("VehicleSpawnSettings.json"));
}
