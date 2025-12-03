#include "ScreenBoundsComponent.h"
#include <cfloat>
#include "Kismet/GameplayStatics.h"


UScreenBoundsComponent::UScreenBoundsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FScreenBox UScreenBoundsComponent::ComputeScreenBounds()
{
	FScreenBox Out;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || Meshes.Num() == 0)
		return Out;

	// Проходим по всем StaticMeshComponent’ам, включая башню и ствол
	for (UStaticMeshComponent* MeshComp : Meshes)
	{
		if (!MeshComp || !MeshComp->GetStaticMesh())
			continue;

		UStaticMesh* Mesh = MeshComp->GetStaticMesh();
		if (!Mesh->GetRenderData())
			continue;

		// Только LOD0 — он самый точный
		const FStaticMeshLODResources& LOD = Mesh->GetRenderData()->LODResources[0];
		const FPositionVertexBuffer& VB = LOD.VertexBuffers.PositionVertexBuffer;

		FVector2D Screen;
		FVector World;

		for (uint32 i = 0; i < VB.GetNumVertices(); i++)
		{
			const FVector LocalPos = FVector(VB.VertexPosition(i));
			World = MeshComp->GetComponentTransform().TransformPosition(LocalPos);

			// Проецируем в экранные координаты
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

bool UScreenBoundsComponent::IsVisible(FScreenBox& OutBounds)
{
	OutBounds = FScreenBox();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return false;

	bool bHasVisibleVertex = false;

	for (UStaticMeshComponent* MeshComp : Meshes)
	{
		if (!MeshComp || !MeshComp->GetStaticMesh())
			continue;

		const FStaticMeshLODResources& LOD =
			MeshComp->GetStaticMesh()->GetRenderData()->LODResources[0];

		const FPositionVertexBuffer& VB = LOD.VertexBuffers.PositionVertexBuffer;
		const FTransform& X = MeshComp->GetComponentTransform();

		for (uint32 i = 0; i < VB.GetNumVertices(); i++)
		{
			FVector World = X.TransformPosition(FVector(VB.VertexPosition(i)));

			FVector2D Screen;
			if (PC->ProjectWorldLocationToScreen(World, Screen))
			{
				OutBounds.Min.X = FMath::Min(OutBounds.Min.X, Screen.X);
				OutBounds.Min.Y = FMath::Min(OutBounds.Min.Y, Screen.Y);
				OutBounds.Max.X = FMath::Max(OutBounds.Max.X, Screen.X);
				OutBounds.Max.Y = FMath::Max(OutBounds.Max.Y, Screen.Y);

				bHasVisibleVertex = true;
			}
		}
	}

	return bHasVisibleVertex && OutBounds.IsValid();
}

void UScreenBoundsComponent::BeginPlay()
{
	Super::BeginPlay();

	TArray<UActorComponent*> All;
	GetOwner()->GetComponents(All);

	for (UActorComponent* C : All)
	{
		if (UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(C))
		{
			if (SM->GetStaticMesh())
			{
				Meshes.Add(SM);
				UE_LOG(LogTemp, Warning, TEXT("ScreenBounds: Added Mesh %s"), *SM->GetName());
			}
		}
	}
}

