#include "Character/CP_PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ACP_PlayerCharacter::ACP_PlayerCharacter()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("Spring Arm");
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->TargetArmLength = 750.f;
	SpringArm->SetRelativeLocation(FVector(0.f, 25.f, 50.f));
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;
}
