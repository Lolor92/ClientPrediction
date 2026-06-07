#include "AnimInstance/CP_AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "PredictedAbility/CP_PredictedAbilityComponent.h"

void UCP_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	APawn* PawnOwner = TryGetPawnOwner();
	if (!PawnOwner) return;

	Character = Cast<ACharacter>(PawnOwner);
	if (!Character) return;

	CharacterMovementComponent = Character->GetCharacterMovement();
	PredictedAbilityComponent = Character->FindComponentByClass<UCP_PredictedAbilityComponent>();
}

void UCP_AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!Character || !CharacterMovementComponent) return;

	if (PredictedAbilityComponent && PredictedAbilityComponent->ShouldSuppressLocomotionAnimation())
	{
		bIsAccelerating = false;
		GroundSpeed = 0.f;
		IsAirBorne = CharacterMovementComponent->IsFalling();

		AimRotation = Character->GetBaseAimRotation();
		MovementRotation = Character->GetActorRotation();
		MovementOffsetYaw = 0.f;

		return;
	}
	
	/* ---------- Movement ---------- */
	bIsAccelerating = CharacterMovementComponent->GetCurrentAcceleration().Size() > 0.f;
	GroundSpeed     = UKismetMathLibrary::VSizeXY(CharacterMovementComponent->Velocity);
	IsAirBorne      = CharacterMovementComponent->IsFalling();

	/* ---------- Rotation ---------- */
	AimRotation      = Character->GetBaseAimRotation();
	MovementRotation = UKismetMathLibrary::MakeRotFromX(Character->GetVelocity());
	MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

	// Orientation logic
	Character->bUseControllerRotationYaw = bIsAccelerating;
}
