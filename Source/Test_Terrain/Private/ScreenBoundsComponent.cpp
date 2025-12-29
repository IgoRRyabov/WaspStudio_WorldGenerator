#include "ScreenBoundsComponent.h"
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

	const FMatrix VP = BuildViewProjection(Camera, RenderW, RenderH);
	const int32 Step = FMath::Max(1, VertexSampleStep);

	bool bHasVisiblePoint = false;

	for (const FCachedMeshData& CM : CachedMeshes)
	{
		const UStaticMeshComponent* SM = CM.MeshComp.Get();
		if (!SM) continue;

		const FTransform X = SM->GetComponentTransform();

		for (int32 i = 0; i < CM.LocalVertices.Num(); i += Step)
		{
			const FVector World = X.TransformPosition(CM.LocalVertices[i]);

			FVector2D Px;
			if (ProjectWorldToPixel(World, VP, RenderW, RenderH, Px))
			{
				if (Px.X >= 0 && Px.X < RenderW &&
					Px.Y >= 0 && Px.Y < RenderH)
				{
					OutBounds.Include(Px);
					bHasVisiblePoint = true;
				}
			}
		}
	}

	if (!bHasVisiblePoint || !OutBounds.IsValid())
		return false;

	return true;
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
