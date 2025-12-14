#include "ScreenBoundsComponent.h"
#include <cfloat>
#include "Kismet/GameplayStatics.h"
#include "SceneView.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

UScreenBoundsComponent::UScreenBoundsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
		if (!SM) continue;

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
		Cache.LocalVertices.Reserve((int32)NumVerts);

		for (uint32 i = 0; i < NumVerts; ++i)
		{
			const FVector3f PosF = VB.VertexPosition(i);
			Cache.LocalVertices.Add(FVector(PosF)); // float->double
		}

		CachedMeshes.Add(MoveTemp(Cache));
	}
}

bool UScreenBoundsComponent::FastAABBTest_FromCamera(UCameraComponent* Camera) const
{
	if (!Camera) return false;
	const AActor* Owner = GetOwner();
	if (!Owner) return false;

	FVector Origin, Extent;
	Owner->GetActorBounds(false, Origin, Extent);

	const FVector CamLoc = Camera->GetComponentLocation();
	const FVector CamDir = Camera->GetForwardVector();

	const FVector ToObj = (Origin - CamLoc).GetSafeNormal();
	return FVector::DotProduct(CamDir, ToObj) > 0.f;
}

bool UScreenBoundsComponent::ComputeViewportBounds(FScreenBox& OutBounds) const
{
	OutBounds.Reset();

	UWorld* W = GetWorld();
	if (!W || CachedMeshes.Num() == 0) return false;

	APlayerController* PC = W->GetFirstPlayerController();
	if (!PC) return false;

	const int32 Step = FMath::Max(1, VertexSampleStep);
	bool bHasPoint = false;

	for (const FCachedMeshData& CM : CachedMeshes)
	{
		UStaticMeshComponent* MeshComp = CM.MeshComp.Get();
		if (!MeshComp || CM.LocalVertices.Num() == 0) continue;

		const FTransform X = MeshComp->GetComponentTransform();

		for (int32 i = 0; i < CM.LocalVertices.Num(); i += Step)
		{
			const FVector World = X.TransformPosition(CM.LocalVertices[i]);

			FVector2D S;
			if (PC->ProjectWorldLocationToScreen(World, S, false))
			{
				OutBounds.IncludePoint(S);
				bHasPoint = true;
			}
		}
	}

	return bHasPoint && OutBounds.IsValid();
}

/**
 * Строим ViewProjection матрицу СТРОГО по CameraComponent и RenderW/RenderH.
 * НЕ используем viewport/DPI вообще.
 */
FMatrix UScreenBoundsComponent::BuildViewProjectionMatrixFromCamera(UCameraComponent* Cam, int32 RenderW, int32 RenderH)
{
	// Берём параметры камеры (FOV, location, rotation)
	FMinimalViewInfo VI;
	Cam->GetCameraView(0.f, VI);

	VI.AspectRatio = (RenderH > 0) ? (float(RenderW) / float(RenderH)) : 1.777777f;
	VI.bConstrainAspectRatio = true;

	// Projection (учтёт FOV/Aspect)
	const FMatrix Proj = VI.CalculateProjectionMatrix();

	// ViewRotationMatrix как в UE (перестановка осей)
	const FMatrix ViewRotationMatrix =
		FInverseRotationMatrix(VI.Rotation) *
		FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1)
		);

	// View = Translate(-Loc) * ViewRotation
	const FMatrix View = FTranslationMatrix(-VI.Location) * ViewRotationMatrix;

	// Итог: ViewProj
	return View * Proj;
}

/**
 * Проецирование world->пиксели рендера (0,0 = левый верхний)
 * Возвращает false если точка позади камеры или вне отсечения (по W).
 */
bool UScreenBoundsComponent::ProjectWorldToRenderPx(
	const FVector& World,
	const FMatrix& ViewProj,
	int32 RenderW,
	int32 RenderH,
	FVector2D& OutPx)
{
	const FVector4 Clip = ViewProj.TransformFVector4(FVector4(World, 1.f));

	// позади камеры / некорректно
	if (Clip.W <= 0.f || !FMath::IsFinite(Clip.W))
		return false;

	// NDC
	const float InvW = 1.f / Clip.W;
	const float NdcX = Clip.X * InvW;
	const float NdcY = Clip.Y * InvW;
	const float NdcZ = Clip.Z * InvW;

	// можно оставить без отсечения по X/Y если хочешь bbox и для частично вне кадра,
	// но обычно для разметки лучше отсечь совсем “мимо”
	if (!FMath::IsFinite(NdcX) || !FMath::IsFinite(NdcY) || !FMath::IsFinite(NdcZ))
		return false;

	// Перевод в пиксели. X: [-1..1] => [0..W]
	OutPx.X = (NdcX * 0.5f + 0.5f) * float(RenderW);

	// Y в UE экранной системе идёт вниз, а NDC Y вверх -> инверсия
	OutPx.Y = (1.f - (NdcY * 0.5f + 0.5f)) * float(RenderH);

	return FMath::IsFinite(OutPx.X) && FMath::IsFinite(OutPx.Y);
}

bool UScreenBoundsComponent::ComputeRenderBoundsFromCamera(
	UCameraComponent* RenderCamera,
	int32 RenderW,
	int32 RenderH,
	FScreenBox& OutBounds) const
{
	OutBounds.Reset();

	if (!RenderCamera || RenderW <= 0 || RenderH <= 0 || CachedMeshes.Num() == 0)
		return false;

	// Быстрый тест “не позади камеры”
	if (!FastAABBTest_FromCamera(RenderCamera))
		return false;

	const FMatrix ViewProj = BuildViewProjectionMatrixFromCamera(RenderCamera, RenderW, RenderH);

	const int32 Step = FMath::Max(1, VertexSampleStep);
	bool bHasPoint = false;

	for (const FCachedMeshData& CM : CachedMeshes)
	{
		UStaticMeshComponent* MeshComp = CM.MeshComp.Get();
		if (!MeshComp || CM.LocalVertices.Num() == 0) continue;

		const FTransform X = MeshComp->GetComponentTransform();

		for (int32 i = 0; i < CM.LocalVertices.Num(); i += Step)
		{
			const FVector World = X.TransformPosition(CM.LocalVertices[i]);

			FVector2D Px;
			if (ProjectWorldToRenderPx(World, ViewProj, RenderW, RenderH, Px))
			{
				OutBounds.IncludePoint(Px);
				bHasPoint = true;
			}
		}
	}

	if (!bHasPoint || !OutBounds.IsValid())
		return false;

	// Clamp в границы рендера (важно для частично вылезших объектов)
	OutBounds.Min.X = FMath::Clamp(OutBounds.Min.X, 0.f, float(RenderW - 1));
	OutBounds.Min.Y = FMath::Clamp(OutBounds.Min.Y, 0.f, float(RenderH - 1));
	OutBounds.Max.X = FMath::Clamp(OutBounds.Max.X, 0.f, float(RenderW - 1));
	OutBounds.Max.Y = FMath::Clamp(OutBounds.Max.Y, 0.f, float(RenderH - 1));

	return OutBounds.IsValid();
}