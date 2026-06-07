#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

USTRUCT()
struct FCP_PendingHitReactionPrediction
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY()
	int32 PredictionKey = 0;
};

USTRUCT()
struct FCP_ServerConfirmedHitReaction
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY()
	TObjectPtr<UAnimMontage> HitMontage = nullptr;

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
	void AddPredictedHitReaction(AActor* TargetActor, int32 PredictionKey);
	
private:
	void GrantDefaultAbilities();
	bool PlayHitReactionOnActor(AActor* TargetActor, UAnimMontage* HitMontage, float StartPosition) const;
	void BeginHitReactionMovementTolerance(float Duration);
	void EndHitReactionMovementTolerance();
	void EndLocalPredictedTargetReaction();
	void PlayConfirmedHitReaction(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey);
	void ScheduleTargetNetUpdate(AActor* TargetActor, float Delay) const;
	bool HasServerConfirmedHitReaction(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey) const;
	void MarkServerConfirmedHitReaction(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey);
	
	UFUNCTION(Server, Reliable)
	void ServerTryActivateAbilityByTag(FGameplayTag AbilityTag, int32 PredictionKey);

	UFUNCTION(Client, Reliable)
	void ClientPlayOwnedHitReaction(UAnimMontage* HitMontage, int32 PredictionKey);

	UPROPERTY(EditDefaultsOnly, Category="Predicted Ability")
	TArray<TSubclassOf<UCP_PredictedGameplayAbility>> DefaultAbilityClasses;

	UPROPERTY(EditDefaultsOnly, Category="Predicted Ability|Networking")
	bool bForceNetUpdateOnConfirmedHit = true;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCP_PredictedGameplayAbility>> GrantedAbilities;
	
	UPROPERTY(Transient)
	TObjectPtr<UCP_PredictedGameplayAbility> CurrentActiveAbility = nullptr;

	bool bSavedIgnoreClientMovementErrorChecksAndCorrection = false;
	bool bSavedServerAcceptClientAuthoritativePosition = false;
	int32 HitReactionMovementToleranceCount = 0;

	int32 LocalPredictedTargetReactionCount = 0;
	
	int32 NextPredictionKey = 1;
	int32 MakePredictionKey();
	
	UPROPERTY(Transient)
	TArray<FCP_PendingAbilityPrediction> PendingPredictions;

	UPROPERTY(Transient)
	TArray<FCP_PendingHitReactionPrediction> PendingHitReactionPredictions;

	UPROPERTY(Transient)
	TArray<FCP_ServerConfirmedHitReaction> ServerConfirmedHitReactions;
	
	void AddPendingPrediction(FGameplayTag AbilityTag, int32 PredictionKey);
	bool ConsumePendingPrediction(FGameplayTag AbilityTag, int32 PredictionKey);
	bool ConsumePredictedHitReaction(AActor* TargetActor, int32 PredictionKey);
};
