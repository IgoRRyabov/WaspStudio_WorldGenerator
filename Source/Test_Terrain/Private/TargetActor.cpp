#include "TargetActor.h"

ATargetActor::ATargetActor()
{
	PrimaryActorTick.bCanEverTick = false;

	//MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	//RootComponent = MeshComp;
	//SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
	
	ScreenBounds = CreateDefaultSubobject<UScreenBoundsComponent>(TEXT("ScreenBounds"));
	Tags.Add("Detectable");
}

void ATargetActor::BeginPlay()
{
	Super::BeginPlay();

}
