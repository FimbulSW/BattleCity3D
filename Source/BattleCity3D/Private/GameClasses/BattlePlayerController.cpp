#include "GameClasses/BattlePlayerController.h"
#include "EnhancedInputSubsystems.h"

void ABattlePlayerController::BeginPlay()
{
    Super::BeginPlay();
    bAutoManageActiveCameraTarget = false;

    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (IMC_Tank)
            {
                Subsys->AddMappingContext(IMC_Tank, /*Priority*/0);
                
            }
        }
    }
    SetShowMouseCursor(false);
    FInputModeGameOnly Mode; SetInputMode(Mode);

}