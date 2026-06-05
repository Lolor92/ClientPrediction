#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CP_BaseCharacter.generated.h"

class UCP_PredictedAbilityComponent;

UCLASS()
class CLIENTPREDICTION_API ACP_BaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACP_BaseCharacter();
	
	FORCEINLINE UCP_PredictedAbilityComponent* GetPredictedAbilityComponent() const { return PredictedAbilityComponent; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Predicted Ability", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCP_PredictedAbilityComponent> PredictedAbilityComponent = nullptr;
};
