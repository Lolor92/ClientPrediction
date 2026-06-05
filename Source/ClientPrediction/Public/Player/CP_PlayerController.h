#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "EnhancedInputComponent.h"
#include "CP_PlayerController.generated.h"

class ACP_PlayerCharacter;
class ACP_PlayerController;
struct FInputActionValue;
class UInputMappingContext;
class UInputAction;

UCLASS()
class CLIENTPREDICTION_API ACP_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ACP_PlayerController();
	
	void SpawnCharacter();
	APlayerStart* FindPlayerStart() const;
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY()
	ACP_PlayerCharacter* PlayerCharacter = nullptr;
	
private:
	void Move(const FInputActionValue& MoveActionValue);
	void Look(const FInputActionValue& LookActionValue);
	void Zoom(const FInputActionValue& InputActionValue);
	void AbilityInputTagTriggered(FGameplayTag InputTag, bool bPressed);
	void SetZoomLevel() const;
	
	template<class TargetClass, typename TargetFunction>
	void BindTaggedInputActions(TMap<TObjectPtr<UInputAction>, FGameplayTag>& InputTagsMap, TargetClass* TargetObject, TargetFunction Callback);
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> InputContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TMap<TObjectPtr<UInputAction>, FGameplayTag> InputTagsMap;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ACharacter> CharacterClass;

	// Camera Zoom Variables
	int32 ZoomLevel = 3;
	FTimerHandle ZoomTimerHandle;
};

template <class TargetClass, typename TargetFunction>
void ACP_PlayerController::BindTaggedInputActions(TMap<TObjectPtr<UInputAction>, FGameplayTag>& InputTagsMap,
	TargetClass* TargetObject, TargetFunction Callback)
{
	if (InputTagsMap.IsEmpty()) return;

	if (UEnhancedInputComponent* EnhanceInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		for (const TPair<TObjectPtr<UInputAction>, FGameplayTag>& Pair : InputTagsMap)
		{
			UInputAction* Action = Pair.Key;
			FGameplayTag Tag = Pair.Value;

			if (Action && Tag.IsValid())
			{
				// Pressed
				EnhanceInput->BindAction(Action, ETriggerEvent::Started, TargetObject, Callback, Tag, true);
				// Released (Completed)
				EnhanceInput->BindAction(Action, ETriggerEvent::Completed, TargetObject, Callback, Tag, false);
			}
		}
	}
}
