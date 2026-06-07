#include "PredictedAbility/CP_PredictedAbilityComponent.h"
#include "PredictedAbility/CP_PredictedGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

UCP_PredictedAbilityComponent::UCP_PredictedAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCP_PredictedAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	GrantDefaultAbilities();
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
		return false;
	}

	if (!Ability->CanActivateAbility())
	{
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

	CurrentActiveAbility = Ability;
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
		return;
	}

	UCP_PredictedGameplayAbility* Ability = FindAbilityByTag(AbilityTag);
	if (!Ability)
	{
		return;
	}

	FCP_PredictedAbilityActivationInfo ActivationInfo;
	ActivationInfo.PredictionKey = PredictionKey;
	ActivationInfo.AbilityTag = AbilityTag;
	ActivationInfo.bIsLocallyPredicted = false;
	ActivationInfo.bIsAuthority = GetOwnerRole() == ROLE_Authority;

	CurrentActiveAbility = Ability;
	Ability->ActivateAbility(ActivationInfo);
}

bool UCP_PredictedAbilityComponent::SendAbilityEvent(FGameplayTag AbilityTag, FName EventName)
{
	UCP_PredictedGameplayAbility* Ability = FindAbilityByTag(AbilityTag);
	if (!Ability)
	{
		return false;
	}

	Ability->HandleAbilityEvent(EventName, Ability->GetCurrentActivationInfo());
	return true;
}

bool UCP_PredictedAbilityComponent::SendEventToCurrentAbility(FName EventName)
{
	if (!CurrentActiveAbility)
	{
		return false;
	}

	CurrentActiveAbility->HandleAbilityEvent(EventName, CurrentActiveAbility->GetCurrentActivationInfo());
	return true;
}

void UCP_PredictedAbilityComponent::ConfirmHitReaction(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey)
{
	if (GetOwnerRole() != ROLE_Authority || !TargetActor || !HitMontage)
	{
		return;
	}

	if (UCP_PredictedAbilityComponent* TargetAbilityComponent = TargetActor->FindComponentByClass<UCP_PredictedAbilityComponent>())
	{
		TargetAbilityComponent->ClientPlayOwnedHitReaction(HitMontage, PredictionKey);

		// Keep the server fully authoritative while testing hit-reaction desync.
		// const float ToleranceDuration = HitMontage->GetPlayLength() + GetTargetOwnerOneWayLatency(TargetActor) + 0.1f;
		// TargetAbilityComponent->BeginHitReactionMovementTolerance(ToleranceDuration);
	}

	PlayConfirmedHitReaction(TargetActor, HitMontage, PredictionKey);
}

void UCP_PredictedAbilityComponent::MulticastPlayHitReaction_Implementation(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey, float ServerStartTime)
{
	if (!TargetActor || !HitMontage)
	{
		return;
	}

	if (GetOwnerRole() != ROLE_Authority && IsLocallyControlledAvatar())
	{
		if (ConsumePredictedHitReaction(TargetActor, PredictionKey))
		{
			return;
		}
	}

	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (TargetPawn && TargetPawn->IsLocallyControlled())
	{
		return;
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	const float CurrentServerTime = GameState ? GameState->GetServerWorldTimeSeconds() : ServerStartTime;
	const float StartPosition = FMath::Clamp(CurrentServerTime - ServerStartTime, 0.f, HitMontage->GetPlayLength());

	PlayHitReactionOnActor(TargetActor, HitMontage, StartPosition);
}

void UCP_PredictedAbilityComponent::ClientPlayOwnedHitReaction_Implementation(UAnimMontage* HitMontage, int32 PredictionKey)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		return;
	}

	PlayHitReactionOnActor(GetOwner(), HitMontage, 0.f);
}

bool UCP_PredictedAbilityComponent::PlayHitReactionOnActor(AActor* TargetActor, UAnimMontage* HitMontage, float StartPosition) const
{
	if (!TargetActor || !HitMontage)
	{
		return false;
	}

	USkeletalMeshComponent* MeshComponent = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!MeshComponent)
	{
		return false;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	if (AnimInstance->Montage_IsPlaying(HitMontage))
	{
		const float CurrentPos = AnimInstance->Montage_GetPosition(HitMontage);

		if (FMath::Abs(CurrentPos - StartPosition) < 0.20f)
		{
			return true;
		}

		AnimInstance->Montage_SetPosition(HitMontage, StartPosition);
		return true;
	}

	AnimInstance->Montage_Play(HitMontage, 1.f, EMontagePlayReturnType::MontageLength, StartPosition);
	return true;
}

float UCP_PredictedAbilityComponent::GetTargetOwnerOneWayLatency(AActor* TargetActor) const
{
	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	const APlayerState* PlayerState = TargetPawn ? TargetPawn->GetPlayerState() : nullptr;
	if (!PlayerState)
	{
		return 0.f;
	}

	constexpr float MaxDelaySeconds = 0.5f;
	return FMath::Clamp(PlayerState->GetPingInMilliseconds() * 0.001f * 0.5f, 0.f, MaxDelaySeconds);
}

void UCP_PredictedAbilityComponent::BeginHitReactionMovementTolerance(float Duration)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UCharacterMovementComponent>()
		: nullptr;
	if (!MovementComponent)
	{
		return;
	}

	if (HitReactionMovementToleranceCount == 0)
	{
		bSavedIgnoreClientMovementErrorChecksAndCorrection = MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection;
		bSavedServerAcceptClientAuthoritativePosition = MovementComponent->bServerAcceptClientAuthoritativePosition;
	}

	++HitReactionMovementToleranceCount;
	MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = true;
	MovementComponent->bServerAcceptClientAuthoritativePosition = true;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			EndHitReactionMovementTolerance();
		}),
		FMath::Max(Duration, 0.01f),
		false);
}

void UCP_PredictedAbilityComponent::EndHitReactionMovementTolerance()
{
	if (HitReactionMovementToleranceCount <= 0)
	{
		return;
	}

	--HitReactionMovementToleranceCount;
	if (HitReactionMovementToleranceCount > 0)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UCharacterMovementComponent>()
		: nullptr;
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = bSavedIgnoreClientMovementErrorChecksAndCorrection;
	MovementComponent->bServerAcceptClientAuthoritativePosition = bSavedServerAcceptClientAuthoritativePosition;
}

void UCP_PredictedAbilityComponent::BeginLocalPredictedTargetReaction(float Duration)
{
	if (!GetOwner() || GetOwnerRole() != ROLE_SimulatedProxy)
	{
		return;
	}

	++LocalPredictedTargetReactionCount;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			EndLocalPredictedTargetReaction();
		}),
		FMath::Max(Duration, 0.01f),
		false);
}

void UCP_PredictedAbilityComponent::AddPredictedHitReaction(AActor* TargetActor, int32 PredictionKey)
{
	if (!TargetActor || PredictionKey <= 0)
	{
		return;
	}

	PendingHitReactionPredictions.RemoveAll([](const FCP_PendingHitReactionPrediction& Prediction)
	{
		return !Prediction.TargetActor;
	});

	for (const FCP_PendingHitReactionPrediction& Prediction : PendingHitReactionPredictions)
	{
		if (Prediction.TargetActor == TargetActor && Prediction.PredictionKey == PredictionKey)
		{
			return;
		}
	}

	FCP_PendingHitReactionPrediction PendingPrediction;
	PendingPrediction.TargetActor = TargetActor;
	PendingPrediction.PredictionKey = PredictionKey;
	PendingHitReactionPredictions.Add(PendingPrediction);

	constexpr int32 MaxPendingHitReactionPredictions = 16;
	if (PendingHitReactionPredictions.Num() > MaxPendingHitReactionPredictions)
	{
		PendingHitReactionPredictions.RemoveAt(0, PendingHitReactionPredictions.Num() - MaxPendingHitReactionPredictions);
	}
}

void UCP_PredictedAbilityComponent::EndLocalPredictedTargetReaction()
{
	if (LocalPredictedTargetReactionCount <= 0)
	{
		return;
	}

	--LocalPredictedTargetReactionCount;
	if (LocalPredictedTargetReactionCount > 0)
	{
		return;
	}
}

void UCP_PredictedAbilityComponent::PlayConfirmedHitReaction(AActor* TargetActor, UAnimMontage* HitMontage, int32 PredictionKey)
{
	if (GetOwnerRole() != ROLE_Authority || !TargetActor || !HitMontage)
	{
		return;
	}

	PlayHitReactionOnActor(TargetActor, HitMontage, 0.f);
	ForceTargetNetUpdate(TargetActor);

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	const float ServerStartTime = GameState ? GameState->GetServerWorldTimeSeconds() : 0.f;

	MulticastPlayHitReaction(TargetActor, HitMontage, PredictionKey, ServerStartTime);

	const float FinalUpdateDelay = HitMontage->GetPlayLength() + HitMontage->GetDefaultBlendOutTime() + 0.05f;
	ScheduleTargetNetUpdate(TargetActor, FinalUpdateDelay);
}

void UCP_PredictedAbilityComponent::ForceTargetNetUpdate(AActor* TargetActor) const
{
	if (!bForceNetUpdateOnConfirmedHit || GetOwnerRole() != ROLE_Authority || !TargetActor)
	{
		return;
	}

	TargetActor->ForceNetUpdate();
}

void UCP_PredictedAbilityComponent::ScheduleTargetNetUpdate(AActor* TargetActor, float Delay) const
{
	if (!bForceNetUpdateOnConfirmedHit || GetOwnerRole() != ROLE_Authority || !TargetActor)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (Delay <= 0.f)
	{
		ForceTargetNetUpdate(TargetActor);
		return;
	}

	TWeakObjectPtr<AActor> WeakTargetActor = TargetActor;
	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakTargetActor]()
		{
			if (AActor* StrongTargetActor = WeakTargetActor.Get())
			{
				ForceTargetNetUpdate(StrongTargetActor);
			}
		}),
		Delay,
		false);
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
		return;
	}

	if (!Ability->CanActivateAbility())
	{
		return;
	}
	
	FCP_PredictedAbilityActivationInfo ActivationInfo;
	ActivationInfo.PredictionKey = PredictionKey;
	ActivationInfo.AbilityTag = AbilityTag;
	ActivationInfo.bIsLocallyPredicted = false;
	ActivationInfo.bIsAuthority = true;

	CurrentActiveAbility = Ability;
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

bool UCP_PredictedAbilityComponent::ConsumePredictedHitReaction(AActor* TargetActor, int32 PredictionKey)
{
	if (!TargetActor || PredictionKey <= 0)
	{
		return false;
	}

	for (int32 Index = 0; Index < PendingHitReactionPredictions.Num(); ++Index)
	{
		const FCP_PendingHitReactionPrediction& PendingPrediction = PendingHitReactionPredictions[Index];

		if (PendingPrediction.TargetActor == TargetActor && PendingPrediction.PredictionKey == PredictionKey)
		{
			PendingHitReactionPredictions.RemoveAtSwap(Index);
			return true;
		}
	}

	return false;
}
