#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CP_PredictedAbilityComponent.generated.h"

class UCP_PredictedGameplayAbility;

USTRUCT()
struct FCP_PendingAbilityPrediction
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag AbilityTag;

	UPROPERTY()
	int32 PredictionKey = 0;
};

UCLASS(ClassGroup=(ClientPrediction), meta=(BlueprintSpawnableComponent))
class CLIENTPREDICTION_API UCP_PredictedAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCP_PredictedAbilityComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	bool IsLocallyControlledAvatar() const;
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	UCP_PredictedGameplayAbility* FindAbilityByTag(FGameplayTag AbilityTag) const;
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	bool TryActivateAbilityByTag(FGameplayTag AbilityTag);
	
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastConfirmAbilityStarted(FGameplayTag AbilityTag, int32 PredictionKey);
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	bool SendAbilityEvent(FGameplayTag AbilityTag, FName EventName);
	
private:
	void GrantDefaultAbilities();
	
	UFUNCTION(Server, Reliable)
	void ServerTryActivateAbilityByTag(FGameplayTag AbilityTag, int32 PredictionKey);

	UPROPERTY(EditDefaultsOnly, Category="Predicted Ability")
	TArray<TSubclassOf<UCP_PredictedGameplayAbility>> DefaultAbilityClasses;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCP_PredictedGameplayAbility>> GrantedAbilities;
	
	int32 NextPredictionKey = 1;
	int32 MakePredictionKey();
	
	UPROPERTY(Transient)
	TArray<FCP_PendingAbilityPrediction> PendingPredictions;
	
	void AddPendingPrediction(FGameplayTag AbilityTag, int32 PredictionKey);
	bool ConsumePendingPrediction(FGameplayTag AbilityTag, int32 PredictionKey);
};
