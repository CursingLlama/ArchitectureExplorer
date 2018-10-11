// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandController.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AHandController : public AActor
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	AHandController();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetHand(FName Name);

	void Grip();
	void Release();
	void PairController(AHandController* Controller);

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	
	
	UPROPERTY(VisibleAnywhere) class UMotionControllerComponent* MotionController = nullptr;
	UPROPERTY(EditDefaultsOnly) class UHapticFeedbackEffect_Base* HapticEffect = nullptr;
	
	UFUNCTION() void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION() void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	bool CanClimb() const;
	UPROPERTY() bool bCanClimb = false;
	UPROPERTY() bool bIsClimbing = false;
	UPROPERTY() FVector ClimbingStartPosition;
	
	UPROPERTY() AHandController* OtherController = nullptr;
};
