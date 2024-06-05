// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Boid2.generated.h"

UCLASS()
class BOIDSMT_API ABoid2 : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABoid2();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float collisionAvoidanceWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float velocityMatchingWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float flockCenteringWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float lerpFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float speed;;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	ABoid2* other;
	TSet<ABoid2*> AllBoids;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void SetOther(ABoid2* o);

	UFUNCTION(BlueprintCallable)
	void SetAll(TSet<ABoid2*> aB);
};
