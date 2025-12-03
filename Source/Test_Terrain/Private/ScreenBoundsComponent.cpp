#include "ScreenBoundsComponent.h"
#include <cfloat>
#include "Kismet/GameplayStatics.h"


UScreenBoundsComponent::UScreenBoundsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FScreenBox UScreenBoundsComponent::ComputeScreenBounds()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	return ComputeScreenBounds_Internal(PC);
}

bool UScreenBoundsComponent::IsVisible(FScreenBox& OutBounds)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// 1) быстрый frustum/AABB тест
	if (!FastAABBVisibilityTest(PC))
		return false;

	// 2) точный pixel-perfect тест
	OutBounds = ComputeScreenBounds_Internal(PC);

	return OutBounds.IsValid();
}

void UScreenBoundsComponent::BeginPlay()
{
	Super::BeginPlay();

	TArray<UActorComponent*> All;
	GetOwner()->GetComponents(All);

	for (UActorComponent* C : All)
	{
		if (auto* SM = Cast<UStaticMeshComponent>(C))
		{
			if (!SM->GetStaticMesh()) continue;

			const FStaticMeshRenderData* RD = SM->GetStaticMesh()->GetRenderData();
			if (!RD || RD->LODResources.Num() == 0) continue;

			const FStaticMeshLODResources& LOD = RD->LODResources[0];
			const FPositionVertexBuffer& VB = LOD.VertexBuffers.PositionVertexBuffer;

			if (VB.GetNumVertices() == 0) continue;

			FCachedMeshData Cache;
			Cache.MeshComp = SM;

			Cache.LocalVertices.Reserve(VB.GetNumVertices());
			FVector3f V3f;

			FBox Box(ForceInit);

			for (uint32 i = 0; i < VB.GetNumVertices(); i++)
			{
				V3f = VB.VertexPosition(i);
				FVector V(V3f);
				Cache.LocalVertices.Add(V);
				Box += V;
			}

			Cache.LocalBounds = Box;

			CachedMeshes.Add(Cache);
		}
	}
}

bool UScreenBoundsComponent::FastAABBVisibilityTest(APlayerController* PC) const
{
	if (!PC) return false;
	const AActor* Owner = GetOwner();
	if (!Owner) return false;

	// Получаем AABB актора
	FVector Origin;
	FVector Extent;
	Owner->GetActorBounds(false, Origin, Extent);

	const float Radius = Extent.GetMax();

	// Камера
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector Forward = CamRot.Vector();
	const FVector ToObj = (Origin - CamLoc).GetSafeNormal();

	// 1. Если объект позади камеры — пропускаем
	if (FVector::DotProduct(Forward, ToObj) < 0.f)
		return false;

	// 2. Быстрый проект в экран
	FVector2D Screen;
	if (!PC->ProjectWorldLocationToScreen(Origin, Screen))
		return false;

	return true;
}

FScreenBox UScreenBoundsComponent::ComputeScreenBounds_Internal(APlayerController* PC) const
{
	FScreenBox Out;
	if (!PC || CachedMeshes.Num() == 0)
		return Out;

	int32 Step = FMath::Max(1, VertexSampleStep);

	for (const FCachedMeshData& M : CachedMeshes)
	{
		if (!M.MeshComp || M.LocalVertices.Num() == 0)
			continue;

		const FTransform& X = M.MeshComp->GetComponentTransform();

		for (int32 i = 0; i < M.LocalVertices.Num(); i += Step)
		{
			const FVector World = X.TransformPosition(M.LocalVertices[i]);
			FVector2D Screen;

			if (PC->ProjectWorldLocationToScreen(World, Screen))
			{
				Out.Min.X = FMath::Min(Out.Min.X, Screen.X);
				Out.Min.Y = FMath::Min(Out.Min.Y, Screen.Y);
				Out.Max.X = FMath::Max(Out.Max.X, Screen.X);
				Out.Max.Y = FMath::Max(Out.Max.Y, Screen.Y);
			}
		}
	}

	return Out;
}


