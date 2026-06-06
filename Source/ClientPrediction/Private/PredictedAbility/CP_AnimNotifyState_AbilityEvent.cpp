#include "PredictedAbility/CP_AnimNotifyState_AbilityEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PredictedAbility/CP_PredictedAbilityComponent.h"

void UCP_AnimNotifyState_AbilityEvent::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	UCP_PredictedAbilityComponent* AbilityComponent = OwnerActor->FindComponentByClass<UCP_PredictedAbilityComponent>();
	if (!AbilityComponent) return;

	AbilityComponent->SendEventToCurrentAbility(BeginEventName);
}
