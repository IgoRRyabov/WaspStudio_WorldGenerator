#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPC.generated.h"


UCLASS()
class TEST_TERRAIN_API AMyPC : public APlayerController
{
	GENERATED_BODY()

public:
	
	virtual void BeginPlay() override;

};
