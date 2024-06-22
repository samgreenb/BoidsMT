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
	float targetWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float avoidRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float lerpFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float initialSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float minSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float maxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float maxSteer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	bool useTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	bool randomInitialSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	bool debug;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FVector velocity;

	ABoid2* other;
	AActor* target;
	TSet<ABoid2*> AllBoids;

	FVector Steer(FVector v);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void SetOther(ABoid2* o);

	UFUNCTION(BlueprintCallable)
	void SetTarget(AActor* o);

	UFUNCTION(BlueprintCallable)
	AActor* GetTarget();

	UFUNCTION(BlueprintCallable)
	void SetAll(TSet<ABoid2*> aB);
};
