#pragma once

#include "CoreMinimal.h"
#include "CP_BaseCharacter.h"
#include "CP_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class CLIENTPREDICTION_API ACP_PlayerCharacter : public ACP_BaseCharacter
{
	GENERATED_BODY()

public:
	ACP_PlayerCharacter();
	
	FORCEINLINE USpringArmComponent* GetSpringArm() const { return SpringArm.Get(); }
	FORCEINLINE UCameraComponent* GetCamera() const { return Camera.Get(); }
	
private:
	// Camera components.
	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<USpringArmComponent> SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<UCameraComponent> Camera = nullptr;
};
