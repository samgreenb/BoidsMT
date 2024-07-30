// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoidGpu.generated.h"

UCLASS()
class BOIDSMT_API ABoidGpu : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABoidGpu();

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

	int32 id;

	FVector position;
	FVector velocity;
	FVector acceleration;

	ABoidGpu* other;
	AActor* target;
	TSet<ABoidGpu*> AllBoids;


	FVector Steer(FVector v);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetAcceleration(FVector a);

	void UpdateBoid(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void SetId(int32 id);

	UFUNCTION(BlueprintCallable)
	int32 GetId();

	UFUNCTION(BlueprintCallable)
	FTransform GetBoidTransform();

	UFUNCTION(BlueprintCallable)
	void SetOther(ABoidGpu* o);

	UFUNCTION(BlueprintCallable)
	void SetTarget(AActor* o);

	UFUNCTION(BlueprintCallable)
	AActor* GetTarget();

	UFUNCTION(BlueprintCallable)
	FVector GetBoidVelocity();

	UFUNCTION(BlueprintCallable)
	FVector GetBoidPosition();

	UFUNCTION(BlueprintCallable)
	void SetAll(TSet<ABoidGpu*> aB);
};
