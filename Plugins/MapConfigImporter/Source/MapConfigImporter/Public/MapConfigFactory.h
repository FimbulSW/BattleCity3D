// MapConfigFactory.h
#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MapConfigFactory.generated.h"
/**
 * 
 */
UCLASS()
class MAPCONFIGIMPORTER_API UMapConfigFactory : public UFactory
{
	GENERATED_BODY()
public:
    UMapConfigFactory();

	virtual bool FactoryCanImport(const FString& Filename) override;

    virtual UObject* FactoryCreateFile(
        UClass* InClass,
        UObject* InParent,
        FName InName,
        EObjectFlags Flags,
        const FString& Filename,
        const TCHAR* Parms,
        FFeedbackContext* Warn,
        bool& bOutOperationCanceled
    ) override;

	~UMapConfigFactory();
};
