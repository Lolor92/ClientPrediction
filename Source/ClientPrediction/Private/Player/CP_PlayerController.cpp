#include "Player/CP_PlayerController.h"
#include "Character/CP_PlayerCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "PredictedAbility/CP_PredictedAbilityComponent.h"

ACP_PlayerController::ACP_PlayerController()
{
	bReplicates = true;
}

void ACP_PlayerController::SpawnCharacter()
{
	if (!HasAuthority() || !CharacterClass || !GetWorld()) return;

	APlayerStart* PlayerStart = FindPlayerStart();
	if (!PlayerStart) return;

	const FVector SpawnLoc = PlayerStart->GetActorLocation();
	const FRotator SpawnRot = PlayerStart->GetActorRotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ACP_PlayerCharacter* NewChar = GetWorld()->SpawnActor<ACP_PlayerCharacter>(CharacterClass, SpawnLoc, SpawnRot, Params);
	if (!NewChar) return;

	Possess(NewChar); // server possession replicates to client
}

APlayerStart* ACP_PlayerController::FindPlayerStart() const
{
	for (TActorIterator<APlayerStart> PlayerStartIterator(GetWorld()); PlayerStartIterator; ++PlayerStartIterator)
	{
		APlayerStart* CurrentPlayerStart = *PlayerStartIterator;

		// Return the first PlayerStart found
		if (CurrentPlayerStart) return CurrentPlayerStart;
	}
	return nullptr;
}

void ACP_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority()) SpawnCharacter();

	PlayerCharacter = Cast<ACP_PlayerCharacter>(GetCharacter());

	// Cast input component to enhanced input component
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACP_PlayerController::Move);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACP_PlayerController::Look);
		EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ACP_PlayerController::Zoom);
		BindTaggedInputActions(InputTagsMap, this, &ACP_PlayerController::AbilityInputTagTriggered);
	}
}

void ACP_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// Check and add input mapping context
    	if (InputContext)
    	{
    		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    		{
    			Subsystem->AddMappingContext(InputContext, 0);
    		}
    	}
}

void ACP_PlayerController::Move(const FInputActionValue& MoveActionValue)
{
// Retrieve 2D input vector (X: right/left, Y: forward/backward)
	const FVector2d InputVector = MoveActionValue.Get<FVector2D>();

	// Get Yaw rotation from controller from movement direction
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);

	// Calculate forward and right directions based on Yaw rotation
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// Add movement input to pawn if valid
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(Forward, InputVector.Y);
		ControlledPawn->AddMovementInput(Right, InputVector.X);
	}
}

void ACP_PlayerController::Look(const FInputActionValue& LookActionValue)
{
// Get look input as 2D vector
	const FVector2D LookVector = LookActionValue.Get<FVector2D>();

	// Apply yaw (horizontal) and pitch (vertical) rotation input
	AddYawInput(LookVector.X);
	AddPitchInput(LookVector.Y);
}

void ACP_PlayerController::Zoom(const FInputActionValue& InputActionValue)
{
	const float InputAxisValue = InputActionValue.Get<float>();
	
	if (InputAxisValue > 0.f && ZoomLevel > 0)
	{
		ZoomLevel--;
		SetZoomLevel();
	}
	else if (InputAxisValue < 0.f && ZoomLevel < 4)
	
	{
		ZoomLevel++;
		SetZoomLevel();
	}

	if (!GetWorldTimerManager().IsTimerActive(ZoomTimerHandle))
	{
		GetWorldTimerManager().SetTimer(ZoomTimerHandle, this, &ACP_PlayerController::SetZoomLevel, GetWorld()->DeltaTimeSeconds, true, 0.f);
	}
}

void ACP_PlayerController::AbilityInputTagTriggered(FGameplayTag InputTag, bool bPressed)
{
	if (!bPressed) return;

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UCP_PredictedAbilityComponent* PredictedAbilityComponent = ControlledPawn->FindComponentByClass<UCP_PredictedAbilityComponent>();
	if (!PredictedAbilityComponent)
	{
		return;
	}

	PredictedAbilityComponent->TryActivateAbilityByTag(InputTag);
}

void ACP_PlayerController::SetZoomLevel() const
{
	if (!PlayerCharacter) return;
	
	const float CameraDistance = ZoomLevel* 200.f;
	PlayerCharacter->GetSpringArm()->TargetArmLength = FMath::FInterpTo(PlayerCharacter->GetSpringArm()->TargetArmLength, CameraDistance, GetWorld()->GetDeltaSeconds(), 10.f);

	// Adjust Camera position when it gets very close
	if (ZoomLevel == 0)
	{
		PlayerCharacter->GetCamera()->SetRelativeLocation(FVector(50.f, 0.f, 70.f));
	}
	else
	{
		PlayerCharacter->GetCamera()->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	}
}
