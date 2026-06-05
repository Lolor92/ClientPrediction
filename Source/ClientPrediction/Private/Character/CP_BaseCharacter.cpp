#include "Character/CP_BaseCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PredictedAbility/CP_PredictedAbilityComponent.h"

ACP_BaseCharacter::ACP_BaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	// Collision defaults.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	// Base movement tuning.
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;
	GetCharacterMovement()->MaxCustomMovementSpeed = 400.0f;

	GetCharacterMovement()->MaxJumpApexAttemptsPerSimulation = 1;
	GetCharacterMovement()->JumpZVelocity = 850.f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->GravityScale = 2.5f;
	
	PredictedAbilityComponent = CreateDefaultSubobject<UCP_PredictedAbilityComponent>("Predicted Ability Component");
}
