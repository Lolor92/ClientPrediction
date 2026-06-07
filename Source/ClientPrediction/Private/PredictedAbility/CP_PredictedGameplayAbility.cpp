#include "PredictedAbility/CP_PredictedGameplayAbility.h"
#include "PredictedAbility/CP_PredictedAbilityComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

void UCP_PredictedGameplayAbility::InitializeAbility(UCP_PredictedAbilityComponent* InOwningComponent)
{
	OwningComponent = InOwningComponent;

	BP_OnAbilityGranted();
}

float UCP_PredictedGameplayAbility::PlayAvatarMontage(UAnimMontage* Montage, float PlayRate, FName StartSection) const
{
	return PlayMontageOnActor(GetAvatarActor(), Montage, PlayRate, StartSection);
}

AActor* UCP_PredictedGameplayAbility::GetAvatarActor() const
{
	return OwningComponent ? OwningComponent->GetOwner() : nullptr;
}

float UCP_PredictedGameplayAbility::PlayMontageOnActor(AActor* TargetActor, UAnimMontage* Montage, float PlayRate,
	FName StartSection) const
{
	if (!TargetActor || !Montage)
	{
		return 0.f;
	}

	USkeletalMeshComponent* MeshComponent = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!MeshComponent)
	{
		return 0.f;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return 0.f;
	}

	const float Duration = AnimInstance->Montage_Play(Montage, PlayRate);
	if (Duration > 0.f && StartSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(StartSection, Montage);
	}

	return Duration;
}

void UCP_PredictedGameplayAbility::HandleAbilityEvent_Implementation(FName EventName,
                                                                     const FCP_PredictedAbilityActivationInfo& ActivationInfo)
{
}

bool UCP_PredictedGameplayAbility::CanActivateAbility_Implementation() const
{
	return true;
}

void UCP_PredictedGameplayAbility::ActivateAbility_Implementation(const FCP_PredictedAbilityActivationInfo& ActivationInfo)
{
	CurrentActivationInfo = ActivationInfo;
}

bool UCP_PredictedGameplayAbility::TraceAvatarForward(
	float Distance,
	float Radius,
	TEnumAsByte<ECollisionChannel> TraceChannel,
	FHitResult& OutHit,
	bool bDrawDebug) const
{
	AActor* AvatarActor = GetAvatarActor();
	if (!AvatarActor)
	{
		return false;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Forward = AvatarActor->GetActorForwardVector();
	const FVector Start = AvatarActor->GetActorLocation() + Forward * 60.f + FVector::UpVector * 50.f;
	const FVector End = Start + Forward * Distance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CP_PredictedAbilityTrace), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);

	const bool bHit = World->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	if (bDrawDebug)
	{
		const FColor Color = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(World, Start, End, Color, false, 1.f, 0, 2.f);
		DrawDebugSphere(World, Start, Radius, 16, Color, false, 1.f);
		DrawDebugSphere(World, End, Radius, 16, Color, false, 1.f);
		if (bHit)
		{
			DrawDebugSphere(World, OutHit.ImpactPoint, Radius * 0.5f, 16, FColor::Yellow, false, 1.f);
		}
	}

	return bHit;
}

bool UCP_PredictedGameplayAbility::ProcessPredictedMeleeHit(
	const FCP_PredictedAbilityActivationInfo& ActivationInfo,
	UAnimMontage* HitReactMontage,
	float TraceDistance,
	float TraceRadius,
	TEnumAsByte<ECollisionChannel> TraceChannel,
	FHitResult& OutHit,
	bool bDrawDebug) const
{
	if (!ActivationInfo.bIsLocallyPredicted && !ActivationInfo.bIsAuthority)
	{
		return false;
	}

	if (!TraceAvatarForward(TraceDistance, TraceRadius, TraceChannel, OutHit, bDrawDebug))
	{
		return false;
	}

	AActor* HitActor = OutHit.GetActor();
	if (!HitActor || !HitReactMontage)
	{
		return false;
	}

	if (ActivationInfo.bIsAuthority)
	{
		if (OwningComponent)
		{
			OwningComponent->ConfirmHitReaction(HitActor, HitReactMontage, ActivationInfo.PredictionKey);
		}
		return true;
	}

	if (ActivationInfo.bIsLocallyPredicted)
	{
		if (OwningComponent)
		{
			OwningComponent->AddPredictedHitReaction(HitActor, ActivationInfo.PredictionKey);
		}

		if (UCP_PredictedAbilityComponent* HitAbilityComponent = HitActor->FindComponentByClass<UCP_PredictedAbilityComponent>())
		{
			const APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
			const APlayerState* PlayerState = AvatarPawn ? AvatarPawn->GetPlayerState() : nullptr;
			const float AttackerPingSeconds = PlayerState ? PlayerState->GetPingInMilliseconds() * 0.001f : 0.f;
			const float PredictionBuffer = FMath::Clamp(AttackerPingSeconds + 0.1f, 0.2f, 0.75f);
			const float BlendOutSettleTime = HitReactMontage->GetDefaultBlendOutTime() + 0.15f;
			HitAbilityComponent->BeginLocalPredictedTargetReaction(
				HitReactMontage->GetPlayLength() + PredictionBuffer + BlendOutSettleTime);
		}

		PlayMontageOnActor(HitActor, HitReactMontage);
		return true;
	}

	return false;
}
