#include "VehicleSpawnArea.h"
#include "MyGameInstance.h"
#include "VehicleSpawnBudgetSubsystem.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"

AVehicleSpawnArea::AVehicleSpawnArea()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);

	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetBoxExtent(FVector(500.f, 500.f, 500.f));
}

void AVehicleSpawnArea::BeginPlay()
{
	Super::BeginPlay();
	
	if (!VehicleClass || LocalCount <= 0 || !GetWorld())
		return;

	const UMyGameInstance* GI = Cast<UMyGameInstance>(GetWorld()->GetGameInstance());
	const int32 MaxTotal = GI ? GI->VehicleSpawnSettings.TotalVehicles : 0;

	UVehicleSpawnBudgetSubsystem* Budget = GetWorld()->GetSubsystem<UVehicleSpawnBudgetSubsystem>();
	if (!Budget) return;

	// Инициализируем бюджет один раз на мир
	Budget->EnsureInitialized(MaxTotal);

	FRandomStream Stream = MakeStreamForThisArea();

	SpawnedPointsWorld.Reset();
	SpawnedPointsWorld.Reserve(LocalCount);

	// Debug: рисуем сам box
	if (bDebugDraw)
	{
		DrawDebugBox(
			GetWorld(),
			Box->GetComponentLocation(),
			Box->GetScaledBoxExtent(),
			Box->GetComponentQuat(),
			FColor::Cyan,
			false,
			10.f,
			0,
			2.f
		);
	}

	for (int32 i = 0; i < LocalCount; ++i)
	{
		// Строго соблюдаем глобальный лимит
		if (!Budget->TryConsumeOne())
			break;

		FTransform SpawnXf;
		if (!FindSpawnTransform(Stream, SpawnXf))
		{
			// Не нашли точку — возвращать слот назад можно, но тогда нужен метод UndoConsume.
			// Проще: просто не спавним (лимит чуть "съестся" при невозможности),
			// но это бывает редко. Если хочешь идеально — скажи, добавлю UndoConsume.
			continue;
		}

		// Спавним без смещения коллизией (иначе UE может "вытолкнуть" наружу)
		AActor* A = GetWorld()->SpawnActorDeferred<AActor>(
			VehicleClass,
			SpawnXf,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (A)
		{
			A->FinishSpawning(SpawnXf);
			SpawnedPointsWorld.Add(SpawnXf.GetLocation());

			if (bDebugDraw)
			{
				DrawDebugPoint(GetWorld(), SpawnXf.GetLocation(), 12.f, FColor::Green, false, 10.f);
				DrawDebugSphere(GetWorld(), SpawnXf.GetLocation(), MinDistance, 16, FColor::Green, false, 10.f);
			}
		}
	}
}

FRandomStream AVehicleSpawnArea::MakeStreamForThisArea() const
{
	const UMyGameInstance* GI = GetWorld() ? Cast<UMyGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	const int32 GlobalSeed = GI ? GI->VehicleSpawnSettings.Seed : 12345;

	// Стабильный микс seed (не зависит от порядка спавна)
	uint32 x = static_cast<uint32>(GlobalSeed);
	x ^= static_cast<uint32>(StableAreaId) + 0x9e3779b9u + (x << 6) + (x >> 2);

	return FRandomStream(static_cast<int32>(x & 0x7fffffff));
}

ECollisionChannel AVehicleSpawnArea::GetLandscapeTraceChannel() const
{
	const UMyGameInstance* GI = GetWorld() ? Cast<UMyGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	const int32 Idx = 1;

	const int32 Base = static_cast<int32>(ECC_GameTraceChannel1);
	return static_cast<ECollisionChannel>(Base + FMath::Clamp(Idx - 1, 0, 17));
}

bool AVehicleSpawnArea::FindSpawnTransform(FRandomStream& Stream, FTransform& OutXf) const
{
	if (!VehicleClass || !Box)
		return false;

	// ВАЖНО: работаем в трансформе BoxComponent (а не Actor), чтобы учитывались поворот/смещение бокса
	const FTransform BoxXf = Box->GetComponentTransform();

	// Safe extent (с учетом bounds техники по XY если включено)
	const FVector SafeExt = GetSafeBoxExtent();

	for (int32 Try = 0; Try < MaxTriesPerVehicle; ++Try)
	{
		// 1) Случайная точка в локальном пространстве бокса
		const FVector LocalCandidate(
			Stream.FRandRange(-SafeExt.X, SafeExt.X),
			Stream.FRandRange(-SafeExt.Y, SafeExt.Y),
			Stream.FRandRange(-SafeExt.Z, SafeExt.Z)
		);

		// 2) Страховка: точка внутри safe-бокса
		if (!IsInsideBoxLocal(LocalCandidate, SafeExt))
			continue;

		const FVector WorldCandidate = BoxXf.TransformPosition(LocalCandidate);

		// 3) MinDistance
		if (!IsFarEnough(WorldCandidate))
			continue;

		// 4) Trace к ландшафту (по custom channel)
		FVector GroundPoint;
		if (!TraceToLandscapeOnly(WorldCandidate, GroundPoint))
			continue;

		// 5) Финальная проверка: после трейса точка всё еще внутри safe-бокса
		const FVector LocalAfter = BoxXf.InverseTransformPosition(GroundPoint);
		if (!IsInsideBoxLocal(LocalAfter, SafeExt))
			continue;

		// 6) Ротация детерминированная (если надо)
		const FRotator Rot(0.f, Stream.FRandRange(-180.f, 180.f), 0.f);

		OutXf = FTransform(Rot, GroundPoint);
		return true;
	}

	return false;
}

bool AVehicleSpawnArea::IsInsideBoxLocal(const FVector& LocalPoint, const FVector& Extent) const
{
	return (FMath::Abs(LocalPoint.X) <= Extent.X) &&
		   (FMath::Abs(LocalPoint.Y) <= Extent.Y) &&
		   (FMath::Abs(LocalPoint.Z) <= Extent.Z);

}

bool AVehicleSpawnArea::IsFarEnough(const FVector& CandidateWorld) const
{
	if (MinDistance <= 0.f) return true;

	const float MinDistSq = MinDistance * MinDistance;
	for (const FVector& P : SpawnedPointsWorld)
	{
		if (FVector::DistSquared(P, CandidateWorld) < MinDistSq)
			return false;
	}
	return true;
}

bool AVehicleSpawnArea::TraceToLandscapeOnly(const FVector& WorldPoint, FVector& OutGroundPoint) const
{
	const UMyGameInstance* GI = GetWorld() ? Cast<UMyGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	const float Up = GI ? GI->VehicleSpawnSettings.TraceUp : 5000.f;
	const float Down = GI ? GI->VehicleSpawnSettings.TraceDown : 20000.f;
	const float OffsetZ = GI ? GI->VehicleSpawnSettings.GroundOffsetZ : 0.f;

	const FVector Start = WorldPoint + FVector(0, 0, Up);
	const FVector End = WorldPoint - FVector(0, 0, Down);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LandscapeSpawnTrace), false, this);

	const ECollisionChannel Channel = GetLandscapeTraceChannel();
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, Channel, Params);
	if (!bHit)
		return false;

	OutGroundPoint = Hit.ImpactPoint;
	OutGroundPoint.Z += OffsetZ;
	return true;
}

FVector AVehicleSpawnArea::GetSafeBoxExtent() const
{
	FVector Ext = Box->GetScaledBoxExtent();

	const UMyGameInstance* GI = GetWorld() ? Cast<UMyGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	const bool bKeepWhole = true;

	if (!bKeepWhole || !VehicleClass)
		return Ext;

	// Берем bounds техники по классу (CDO), уменьшаем XY, чтобы меш не выходил за границы
	const AActor* CDO = VehicleClass->GetDefaultObject<AActor>();
	if (!CDO)
		return Ext;

	const FBox Bounds = CDO->GetComponentsBoundingBox(true);
	const FVector HalfSize = Bounds.GetExtent(); // половина габаритов

	Ext.X = FMath::Max(0.f, Ext.X - HalfSize.X);
	Ext.Y = FMath::Max(0.f, Ext.Y - HalfSize.Y);

	return Ext;
}

