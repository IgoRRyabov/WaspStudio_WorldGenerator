#include "ScreenBoundsComponent.h"

#include "MyGameInstance.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "SceneView.h"

UScreenBoundsComponent::UScreenBoundsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UScreenBoundsComponent::ComputeBoundsFromCamera(UCameraComponent* Camera, int32 RenderW, int32 RenderH,
	FScreenBox& OutBounds) const
{
	OutBounds.Reset();

	if (!Camera || RenderW <= 0 || RenderH <= 0)
		return false;

	// ДВА бокса:
	// 1) FullBox - проекция объекта в пикселях (может выходить за экран)
	// 2) InFrameBox - только то, что реально попало в кадр
	FScreenBox FullBox;
	FScreenBox InFrameBox;
	FullBox.Reset();
	InFrameBox.Reset();

	const FMatrix VP = BuildViewProjection(Camera, RenderW, RenderH);
	const int32 Step = FMath::Max(1, VertexSampleStep);

	bool bHasAnyProjected = false;
	bool bHasAnyInFrame = false;

	TArray<FVector> OcclusionWorldPoints;
	OcclusionWorldPoints.Reserve(FMath::Max(8, MaxOcclusionRays));
	
	int32 SeenInFramePoints = 0;
	
	auto ReservoirAdd = [&](const FVector& P)
	{
		SeenInFramePoints++;
		
		const int32 Cap = FMath::Max(8, MaxOcclusionRays);
		if (OcclusionWorldPoints.Num() < Cap)
		{
			OcclusionWorldPoints.Add(P);
		}
		else
		{
			const int32 j = FMath::RandRange(1, SeenInFramePoints);
			if (j <= Cap)
				OcclusionWorldPoints[j - 1] = P;
		}
	};
	
	for (const FCachedMeshData& CM : CachedMeshes)
	{
		const UStaticMeshComponent* SM = CM.MeshComp.Get();
		if (!SM) continue;

		const FTransform X = SM->GetComponentTransform();

		for (int32 i = 0; i < CM.LocalVertices.Num(); i += Step)
		{
			const FVector World = X.TransformPosition(CM.LocalVertices[i]);

			FVector2D Px;
			if (!ProjectWorldToPixel(World, VP, RenderW, RenderH, Px))
				continue;

			// Учитываем в FullBox всегда (даже если Px вне экрана)
			FullBox.Include(Px);
			bHasAnyProjected = true;

			// Учитываем в InFrameBox только если точка реально в кадре
			if (Px.X >= 0.f && Px.X < float(RenderW) &&
				Px.Y >= 0.f && Px.Y < float(RenderH))
			{
				InFrameBox.Include(Px);
				bHasAnyInFrame = true;
				
				if (bUseOcclusionFilter)
				{
					ReservoirAdd(World);
				}
			}
		}
	}

	// Если вообще ничего не спроецировалось или объект не дал валидный bbox
	if (!bHasAnyProjected || !FullBox.IsValid())
		return false;

	// Если в кадр не попала ни одна точка - объекта в кадре нет
	if (!bHasAnyInFrame || !InFrameBox.IsValid())
		return false;

	// ---- ФИЛЬТР "минимум 50% bbox в кадре" ----
	const float FullW = (FullBox.Max.X - FullBox.Min.X);
	const float FullH = (FullBox.Max.Y - FullBox.Min.Y);
	const float FullArea = FullW * FullH;

	const float VisW = (InFrameBox.Max.X - InFrameBox.Min.X);
	const float VisH = (InFrameBox.Max.Y - InFrameBox.Min.Y);
	const float VisArea = VisW * VisH;

	// защита от мусора
	if (FullArea <= KINDA_SMALL_NUMBER)
		return false;

	const float Ratio = VisArea / FullArea;

	// 1) минимум доля bbox в кадре (например 0.5)
	if (Ratio < MinInFrameBBoxRatio)
		return false;

	// 2) отсечь “еле видно”
	if (VisArea < MinVisibleAreaPx)
		return false;
	if (VisW < MinVisibleWidthPx)
		return false;
	if (VisH < MinVisibleHeightPx)
		return false;

	
	// Фильтр: окклюзия в процентах (по лучам)
	if (bUseOcclusionFilter)
	{
		if (OcclusionWorldPoints.Num() < FMath::Max(8, MinOcclusionRays / 2))
			return false;
		
		UWorld* World = GetWorld();
		if (!World) return false;
		
		AActor* Owner = GetOwner();
		if (!Owner) return false;
		
		const FVector CamLoc = Camera->GetComponentLocation();
		
		FCollisionQueryParams Params(SCENE_QUERY_STAT(BoundsOcclusion), bOcclusionTraceComplex);
		
		int32 Considered = 0;
		int32 Visible = 0;
		
		const int32 NeedVisibleMin = FMath::CeilToInt(float(MinOcclusionRays) * MinVisiblePointRatio);
		
		const int32 MaxRays = FMath::Max(MinOcclusionRays, MaxOcclusionRays);
		const int32 RaysToUse = FMath::Clamp(OcclusionWorldPoints.Num(),MinOcclusionRays, MaxRays);
		
		for (int32 idx = 0; idx < RaysToUse; idx++)
		{
			const FVector P = OcclusionWorldPoints[idx];
			
			FHitResult Hit;
			const bool bHit = World->LineTraceSingleByChannel(Hit, CamLoc, P, OcclusionTraceChannel, Params);
			
			bool bPointVisible = false;
			
			if (!bHit)
			{
				bPointVisible = true;
			}
			else
			{
				bPointVisible = Hit.GetActor() == Owner;
			}
			
			Considered++;
			if (bPointVisible) Visible++;
			
			const int32 Remaining = RaysToUse - Considered;
			if (Visible + Remaining < FMath::CeilToInt(float(RaysToUse) * MinVisiblePointRatio))
				return false;
			
			if (Considered >= MinOcclusionRays && Visible >= FMath::CeilToInt(float(Considered) * MinVisiblePointRatio))
			{
				//break;
			}
		}
		
		if (Considered < MinOcclusionRays)
			return false;
		
		const float VisibleRatio = float(Visible) / float(Considered);
		if (VisibleRatio < MinVisiblePointRatio)
			return false;
	}
	
	
	// OutBounds отдаём уже “видимый” bbox (в координатах кадра, 0,0 = top-left)
	OutBounds = InFrameBox;

	// на всякий — clamp
	OutBounds.Min.X = FMath::Clamp(OutBounds.Min.X, 0.f, float(RenderW - 1));
	OutBounds.Min.Y = FMath::Clamp(OutBounds.Min.Y, 0.f, float(RenderH - 1));
	OutBounds.Max.X = FMath::Clamp(OutBounds.Max.X, 0.f, float(RenderW - 1));
	OutBounds.Max.Y = FMath::Clamp(OutBounds.Max.Y, 0.f, float(RenderH - 1));

	return OutBounds.IsValid();
}

FMatrix UScreenBoundsComponent::BuildViewProjection(UCameraComponent* Camera, int32 W, int32 H)
{
	FMinimalViewInfo VI;
	Camera->GetCameraView(0.f, VI);

	VI.AspectRatio = float(W) / float(H);
	VI.bConstrainAspectRatio = true;

	const FMatrix Proj = VI.CalculateProjectionMatrix();

	const FMatrix ViewRotation =
		FInverseRotationMatrix(VI.Rotation) *
		FMatrix(
			FPlane(0,0,1,0),
			FPlane(1,0,0,0),
			FPlane(0,1,0,0),
			FPlane(0,0,0,1)
		);

	const FMatrix View = FTranslationMatrix(-VI.Location) * ViewRotation;

	return View * Proj;
}

bool UScreenBoundsComponent::ProjectWorldToPixel(const FVector& World, const FMatrix& ViewProj, int32 W, int32 H,
	FVector2D& OutPx)
{
	const FVector4 Clip = ViewProj.TransformFVector4(FVector4(World, 1.f));

	if (Clip.W <= 0.f || !FMath::IsFinite(Clip.W))
		return false;

	const float InvW = 1.f / Clip.W;
	const float NX = Clip.X * InvW;
	const float NY = Clip.Y * InvW;

	OutPx.X = (NX * 0.5f + 0.5f) * W;
	OutPx.Y = (1.f - (NY * 0.5f + 0.5f)) * H;

	return FMath::IsFinite(OutPx.X) && FMath::IsFinite(OutPx.Y);
}

void UScreenBoundsComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedMeshes.Reset();
	
	const auto GI = GetWorld()->GetGameInstance<UMyGameInstance>();
	
	VertexSampleStep = GI->VehicleSpawnSettings.VertexSampleStep;

	TArray<UActorComponent*> Comps;
	GetOwner()->GetComponents(Comps);

	for (UActorComponent* C : Comps)
	{
		UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(C);
		if (!SM || !SM->GetStaticMesh()) continue;

		const FStaticMeshRenderData* RD = SM->GetStaticMesh()->GetRenderData();
		if (!RD || RD->LODResources.Num() == 0) continue;

		const FPositionVertexBuffer& VB =
			RD->LODResources[0].VertexBuffers.PositionVertexBuffer;

		FCachedMeshData Cache;
		Cache.MeshComp = SM;

		const uint32 Num = VB.GetNumVertices();
		Cache.LocalVertices.Reserve(Num);

		for (uint32 i = 0; i < Num; ++i)
		{
			Cache.LocalVertices.Add(FVector(VB.VertexPosition(i)));
		}

		CachedMeshes.Add(MoveTemp(Cache));
	}
}
