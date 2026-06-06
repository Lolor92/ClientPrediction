#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CP_AnimNotifyState_AbilityEvent.generated.h"

UCLASS()
class CLIENTPREDICTION_API UCP_AnimNotifyState_AbilityEvent : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Predicted Ability")
	FName BeginEventName = TEXT("Hit");
};
