#include "PredictedAbility/CP_PredictedAbilityComponent.h"
#include "PredictedAbility/CP_PredictedGameplayAbility.h"
#include "GameFramework/Pawn.h"

UCP_PredictedAbilityComponent::UCP_PredictedAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCP_PredictedAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	GrantDefaultAbilities();
	
	UE_LOG(LogTemp, Log, TEXT("PredictedAbilityComponent ready on %s. OwnerRole=%d"),
		*GetNameSafe(GetOwner()), static_cast<int32>(GetOwnerRole()));
}

bool UCP_PredictedAbilityComponent::IsLocallyControlledAvatar() const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());
	return PawnOwner && PawnOwner->IsLocallyControlled();
}

UCP_PredictedGameplayAbility* UCP_PredictedAbilityComponent::FindAbilityByTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid()) return nullptr;

	for (UCP_PredictedGameplayAbility* Ability : GrantedAbilities)
	{
		if (Ability && Ability->GetAbilityTag() == AbilityTag)
		{
			return Ability;
		}
	}

	return nullptr;
}

bool UCP_PredictedAbilityComponent::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
	UCP_PredictedGameplayAbility* Ability = FindAbilityByTag(AbilityTag);
	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("No predicted ability found for tag %s on %s"),
			*AbilityTag.ToString(),
			*GetNameSafe(GetOwner()));
		return false;
	}

	if (!Ability->CanActivateAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("Predicted ability blocked. Ability=%s Tag=%s"),
			*GetNameSafe(Ability),
			*AbilityTag.ToString());
		return false;
	}

	const int32 PredictionKey = MakePredictionKey();
	
	if (IsLocallyControlledAvatar() && GetOwnerRole() != ROLE_Authority)
	{
		AddPendingPrediction(AbilityTag, PredictionKey);
	}

	FCP_PredictedAbilityActivationInfo ActivationInfo;
	ActivationInfo.PredictionKey = PredictionKey;
	ActivationInfo.AbilityTag = AbilityTag;
	ActivationInfo.bIsLocallyPredicted = IsLocallyControlledAvatar();
	ActivationInfo.bIsAuthority = GetOwnerRole() == ROLE_Authority;

	Ability->ActivateAbility(ActivationInfo);
	
	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerTryActivateAbilityByTag(AbilityTag, PredictionKey);
	}
	
	return true;
}

void UCP_PredictedAbilityComponent::MulticastConfirmAbilityStarted_Implementation(FGameplayTag AbilityTag,
	int32 PredictionKey)
{
	if (GetOwnerRole() == ROLE_Authority) return;
	
	const bool bWasPredictedHere = ConsumePendingPrediction(AbilityTag, PredictionKey);
	if (bWasPredictedHere)
	{
		UE_LOG(LogTemp, Log, TEXT("Confirmed predicted ability without replay. Owner=%s Tag=%s PredictionKey=%d"),
			*GetNameSafe(GetOwner()),
			*AbilityTag.ToString(),
			PredictionKey);
		return;
	}

	UCP_PredictedGameplayAbility* Ability = FindAbilityByTag(AbilityTag);
	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Confirmed ability missing on client. Owner=%s Tag=%s PredictionKey=%d"),
			*GetNameSafe(GetOwner()),
			*AbilityTag.ToString(),
			PredictionKey);
		return;
	}

	FCP_PredictedAbilityActivationInfo ActivationInfo;
	ActivationInfo.PredictionKey = PredictionKey;
	ActivationInfo.AbilityTag = AbilityTag;
	ActivationInfo.bIsLocallyPredicted = false;
	ActivationInfo.bIsAuthority = GetOwnerRole() == ROLE_Authority;

	Ability->ActivateAbility(ActivationInfo);
}

bool UCP_PredictedAbilityComponent::SendAbilityEvent(FGameplayTag AbilityTag, FName EventName)
{
	UCP_PredictedGameplayAbility* Ability = FindAbilityByTag(AbilityTag);
	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability event %s failed. No ability for tag %s on %s"),
			*EventName.ToString(),
			*AbilityTag.ToString(),
			*GetNameSafe(GetOwner()));
		return false;
	}

	Ability->HandleAbilityEvent(EventName, Ability->GetCurrentActivationInfo());
	return true;
}

void UCP_PredictedAbilityComponent::GrantDefaultAbilities()
{
	GrantedAbilities.Reset();

	for (TSubclassOf<UCP_PredictedGameplayAbility> AbilityClass : DefaultAbilityClasses)
	{
		if (!AbilityClass) continue;

		UCP_PredictedGameplayAbility* Ability = NewObject<UCP_PredictedGameplayAbility>(this, AbilityClass);
		if (!Ability) continue;

		Ability->InitializeAbility(this);
		GrantedAbilities.Add(Ability);
	}
}

void UCP_PredictedAbilityComponent::ServerTryActivateAbilityByTag_Implementation(FGameplayTag AbilityTag, int32 PredictionKey)
{
	UCP_PredictedGameplayAbility* Ability = FindAbilityByTag(AbilityTag);
	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server could not find predicted ability. Owner=%s Tag=%s PredictionKey=%d"),
			*GetNameSafe(GetOwner()),
			*AbilityTag.ToString(),
			PredictionKey);
		return;
	}

	if (!Ability->CanActivateAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("Server blocked predicted ability. Owner=%s Ability=%s Tag=%s PredictionKey=%d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Ability),
			*AbilityTag.ToString(),
			PredictionKey);
		return;
	}
	
	FCP_PredictedAbilityActivationInfo ActivationInfo;
	ActivationInfo.PredictionKey = PredictionKey;
	ActivationInfo.AbilityTag = AbilityTag;
	ActivationInfo.bIsLocallyPredicted = false;
	ActivationInfo.bIsAuthority = true;

	Ability->ActivateAbility(ActivationInfo);
	
	MulticastConfirmAbilityStarted(AbilityTag, PredictionKey);
}

int32 UCP_PredictedAbilityComponent::MakePredictionKey()
{
	if (NextPredictionKey <= 0)
	{
		NextPredictionKey = 1;
	}

	return NextPredictionKey++;
}

void UCP_PredictedAbilityComponent::AddPendingPrediction(FGameplayTag AbilityTag, int32 PredictionKey)
{
	FCP_PendingAbilityPrediction PendingPrediction;
	PendingPrediction.AbilityTag = AbilityTag;
	PendingPrediction.PredictionKey = PredictionKey;

	PendingPredictions.Add(PendingPrediction);
}

bool UCP_PredictedAbilityComponent::ConsumePendingPrediction(FGameplayTag AbilityTag, int32 PredictionKey)
{
	for (int32 Index = 0; Index < PendingPredictions.Num(); ++Index)
	{
		const FCP_PendingAbilityPrediction& PendingPrediction = PendingPredictions[Index];

		if (PendingPrediction.AbilityTag == AbilityTag && PendingPrediction.PredictionKey == PredictionKey)
		{
			PendingPredictions.RemoveAtSwap(Index);
			return true;
		}
	}

	return false;
}
