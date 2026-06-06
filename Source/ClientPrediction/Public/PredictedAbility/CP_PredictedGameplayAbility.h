#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "CP_PredictedGameplayAbility.generated.h"

class UCP_PredictedAbilityComponent;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FCP_PredictedAbilityActivationInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Predicted Ability")
	int32 PredictionKey = 0;

	UPROPERTY(BlueprintReadOnly, Category="Predicted Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly, Category="Predicted Ability")
	bool bIsLocallyPredicted = false;

	UPROPERTY(BlueprintReadOnly, Category="Predicted Ability")
	bool bIsAuthority = false;
};

UCLASS(Abstract, Blueprintable)
class CLIENTPREDICTION_API UCP_PredictedGameplayAbility : public UObject
{
	GENERATED_BODY()
	
public:
	virtual void InitializeAbility(UCP_PredictedAbilityComponent* InOwningComponent);
	
	UFUNCTION(BlueprintNativeEvent, Category="Predicted Ability")
	bool CanActivateAbility() const;

	UFUNCTION(BlueprintNativeEvent, Category="Predicted Ability")
	void ActivateAbility(const FCP_PredictedAbilityActivationInfo& ActivationInfo);

	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	FGameplayTag GetAbilityTag() const { return AbilityTag; }
	
	UFUNCTION(BlueprintImplementableEvent, Category="Predicted Ability")
	void BP_OnAbilityGranted();
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability|Animation")
	float PlayAvatarMontage(UAnimMontage* Montage, float PlayRate = 1.f, FName StartSection = NAME_None) const;
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	UCP_PredictedAbilityComponent* GetOwningPredictedAbilityComponent() const { return OwningComponent; }

	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	AActor* GetAvatarActor() const;
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability")
	FCP_PredictedAbilityActivationInfo GetCurrentActivationInfo() const { return CurrentActivationInfo; }
	
	UFUNCTION(BlueprintNativeEvent, Category="Predicted Ability")
	void HandleAbilityEvent(FName EventName, const FCP_PredictedAbilityActivationInfo& ActivationInfo);
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability|Animation")
	float PlayMontageOnActor(AActor* TargetActor, UAnimMontage* Montage, float PlayRate = 1.f, FName StartSection = NAME_None) const;
	
	UFUNCTION(BlueprintCallable, Category="Predicted Ability|Targeting")
	bool TraceAvatarForward(
		float Distance,
		float Radius,
		TEnumAsByte<ECollisionChannel> TraceChannel,
		FHitResult& OutHit,
		bool bDrawDebug = false
	) const;

	UFUNCTION(BlueprintCallable, Category="Predicted Ability|Melee")
	bool ProcessPredictedMeleeHit(
		const FCP_PredictedAbilityActivationInfo& ActivationInfo,
		UAnimMontage* HitReactMontage,
		float TraceDistance,
		float TraceRadius,
		TEnumAsByte<ECollisionChannel> TraceChannel,
		FHitResult& OutHit,
		bool bDrawDebug = false
	) const;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Predicted Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(Transient)
	TObjectPtr<UCP_PredictedAbilityComponent> OwningComponent = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category="Predicted Ability")
	FCP_PredictedAbilityActivationInfo CurrentActivationInfo;
	
};
