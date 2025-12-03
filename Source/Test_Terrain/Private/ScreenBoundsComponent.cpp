#include "ScreenBoundsComponent.h"
#include <cfloat>
#include "Kismet/GameplayStatics.h"


UScreenBoundsComponent::UScreenBoundsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UScreenBoundsComponent::BeginPlay()
{
	Super::BeginPlay();

	TArray<UActorComponent*> Comps;
	GetOwner()->GetComponents(Comps);

	for (UActorComponent* C : Comps)
	{
		if (UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(C))
		{
			UStaticMesh* Mesh = SM->GetStaticMesh();
			if (!Mesh) continue;

			const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
			if (!RenderData || RenderData->LODResources.Num() == 0) continue;

			const FStaticMeshLODResources& LOD = RenderData->LODResources[0];
			const FPositionVertexBuffer& VB = LOD.VertexBuffers.PositionVertexBuffer;
			const uint32 NumVerts = VB.GetNumVertices();
			if (NumVerts == 0) continue;

			FCachedMeshData Cache;
			Cache.MeshComp = SM;
			Cache.LocalVertices.Reserve(NumVerts);

			for (uint32 i = 0; i < NumVerts; ++i)
			{
				const FVector3f PosF = VB.VertexPosition(i);
				Cache.LocalVertices.Add(FVector(PosF));
			}

			CachedMeshes.Add(Cache);
		}
	}
}

bool UScreenBoundsComponent::ComputeScreenBounds(FScreenBox& OutBounds)
{
	OutBounds = FScreenBox();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || CachedMeshes.Num() == 0)
		return false;

	if (bUseAsyncTask)
	{
		return ComputeScreenBounds_Async(OutBounds);
	}
	else
	{
		return ComputeScreenBounds_Sync(PC, OutBounds);
	}
}

//
// СИНХРОННАЯ версия (как раньше, но с кешем и sampling)
//
bool UScreenBoundsComponent::ComputeScreenBounds_Sync(
	APlayerController* PC,
	FScreenBox& OutBounds) const
{
	const int32 Step = FMath::Max(1, VertexSampleStep);
	bool bHasPoint = false;

	for (const FCachedMeshData& CM : CachedMeshes)
	{
		if (!CM.MeshComp || CM.LocalVertices.Num() == 0)
			continue;

		const FTransform& X = CM.MeshComp->GetComponentTransform();

		for (int32 i = 0; i < CM.LocalVertices.Num(); i += Step)
		{
			const FVector World = X.TransformPosition(CM.LocalVertices[i]);

			FVector2D S;
			if (PC->ProjectWorldLocationToScreen(World, S))
			{
				OutBounds.Min.X = FMath::Min(OutBounds.Min.X, S.X);
				OutBounds.Min.Y = FMath::Min(OutBounds.Min.Y, S.Y);
				OutBounds.Max.X = FMath::Max(OutBounds.Max.X, S.X);
				OutBounds.Max.Y = FMath::Max(OutBounds.Max.Y, S.Y);

				bHasPoint = true;
			}
		}
	}

	return bHasPoint && OutBounds.IsValid();
}

bool UScreenBoundsComponent::FastAABBTest(APlayerController* PC) const
{
	if (!PC) return false;
	const AActor* Owner = GetOwner();
	if (!Owner) return false;

	// AABB
	FVector Origin, Extent;
	Owner->GetActorBounds(false, Origin, Extent);

	// Камера
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	FVector Dir = (Origin - CamLoc).GetSafeNormal();

	// Позади камеры?
	if (FVector::DotProduct(CamRot.Vector(), Dir) < 0.f)
		return false;

	return true;
}

bool UScreenBoundsComponent::IsVisible(FScreenBox& OutBounds)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return false;

	// 1) Быстрый отсекающий тест
	if (!FastAABBTest(PC))
		return false;

	// 2) Точный pixel-perfect расчёт
	return ComputeScreenBounds_Async(OutBounds);
}

//
// АСИНХРОННАЯ версия: TaskGraph считает world-позиции вершин,
// GT только проецирует их в экран.
//
bool UScreenBoundsComponent::ComputeScreenBounds_Async(
	FScreenBox& OutBounds)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return false;

	// Собрали трансформы мешей
	TArray<FTransform> MeshTransforms;
	MeshTransforms.SetNum(CachedMeshes.Num());
	for (int32 i = 0; i < CachedMeshes.Num(); ++i)
		MeshTransforms[i] = CachedMeshes[i].MeshComp ?
			CachedMeshes[i].MeshComp->GetComponentTransform() :
			FTransform::Identity;

	// Буфер world-точек
	TArray<TArray<FVector>> WorldPoints;
	WorldPoints.SetNum(CachedMeshes.Num());

	// ---- ASYNC TASK ----
	FAsyncTask<FWorldPointsTask> Task(&CachedMeshes, &MeshTransforms, &WorldPoints, VertexSampleStep);
	Task.StartBackgroundTask();
	Task.EnsureCompletion();
	// ---------------------

	bool bHasPoint = false;

	// Проецируем world → экран
	for (const TArray<FVector>& MeshPts : WorldPoints)
	{
		for (const FVector& P : MeshPts)
		{
			FVector2D S;
			if (PC->ProjectWorldLocationToScreen(P, S))
			{
				OutBounds.IncludePoint(S);
				bHasPoint = true;
			}
		}
	}

	return bHasPoint && OutBounds.IsValid();
}
