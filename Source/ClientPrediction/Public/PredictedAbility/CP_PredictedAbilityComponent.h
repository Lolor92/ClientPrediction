#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "CP_PredictedAbilityComponent.generated.h"

class UAnimMontage;
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
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	bool SendEventToCurrentAbility(FName EventName);

	UFUNCTION(BlueprintCallable, Category="Predicted Ability|Animation")
	void ConfirmHitReaction(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReaction(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey, float ServerStartTime);

	bool ShouldSuppressLocomotionAnimation() const { return LocalPredictedTargetReactionCount > 0; }
	void BeginLocalPredictedTargetReaction(float Duration);
	
private:
	void GrantDefaultAbilities();
	bool PlayHitReactionOnActor(AActor* TargetActor, UAnimMontage* HitMontage, float StartPosition) const;
	float GetTargetOwnerOneWayLatency(AActor* TargetActor) const;
	void BeginHitReactionMovementTolerance(float Duration);
	void EndHitReactionMovementTolerance();
	void EndLocalPredictedTargetReaction();
	void RestoreLocalPredictionNetworkSmoothing();
	void PlayConfirmedHitReaction(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey);
	
	UFUNCTION(Server, Reliable)
	void ServerTryActivateAbilityByTag(FGameplayTag AbilityTag, int32 PredictionKey);

	UFUNCTION(Client, Reliable)
	void ClientPlayOwnedHitReaction(UAnimMontage* HitMontage, int32 PredictionKey);

	UPROPERTY(EditDefaultsOnly, Category="Predicted Ability")
	TArray<TSubclassOf<UCP_PredictedGameplayAbility>> DefaultAbilityClasses;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCP_PredictedGameplayAbility>> GrantedAbilities;
	
	UPROPERTY(Transient)
	TObjectPtr<UCP_PredictedGameplayAbility> CurrentActiveAbility = nullptr;

	bool bSavedIgnoreClientMovementErrorChecksAndCorrection = false;
	bool bSavedServerAcceptClientAuthoritativePosition = false;
	int32 HitReactionMovementToleranceCount = 0;

	bool bSavedReplicateMovementForLocalPrediction = true;
	ENetworkSmoothingMode SavedNetworkSmoothingModeForLocalPrediction = ENetworkSmoothingMode::Exponential;
	int32 LocalPredictedTargetReactionCount = 0;
	
	int32 NextPredictionKey = 1;
	int32 MakePredictionKey();
	
	UPROPERTY(Transient)
	TArray<FCP_PendingAbilityPrediction> PendingPredictions;
	
	void AddPendingPrediction(FGameplayTag AbilityTag, int32 PredictionKey);
	bool ConsumePendingPrediction(FGameplayTag AbilityTag, int32 PredictionKey);
};
