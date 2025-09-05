#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BattlePlayerController.generated.h"

class UInputMappingContext;

UCLASS()
class BATTLECITY3D_API ABattlePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> IMC_Tank;

protected:
    virtual void BeginPlay() override;
};