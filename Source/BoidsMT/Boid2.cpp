// Fill out your copyright notice in the Description page of Project Settings.


#include "Boid2.h"

// Sets default values
ABoid2::ABoid2()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	collisionAvoidanceWeight = 0.6f;
	velocityMatchingWeight = 0.3;
	flockCenteringWeight = 0.1;

	radius = 100.0f;
	lerpFactor = 0.15;
	speed = 3.0f;
}

// Called when the game starts or when spawned
void ABoid2::BeginPlay()
{
	Super::BeginPlay();
	SetActorRotation(FMath::VRand().Rotation());
}

// Called every frame
void ABoid2::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CA(0,0,0);
	int BCount = 0;

	FVector VM(0, 0, 0);

	FVector FC(0, 0, 0);

	int i = 0;
	if (AllBoids.Num() > 0) {
		i++;

		for (auto& Elem : AllBoids)
		{
			// distance check
			FVector direction = GetActorLocation() - Elem->GetActorLocation();
			float distance = direction.Length();
			if (distance > radius) {
				continue;
			}

			// Collision avoidance
			CA += direction.Normalize() * FVector((radius - distance));
			BCount++;

			// Velocity Matching (heading matching for now)
			VM += Elem->GetActorForwardVector();

			// Flock centering
			FC += Elem->GetActorLocation();
		}

		CA = CA / BCount;

		VM = VM / BCount;

		FC = FC / BCount;

		FC = FC - GetActorLocation();
	}

	FVector result = CA * collisionAvoidanceWeight + VM * velocityMatchingWeight + FC * flockCenteringWeight;
	speed = FMath::Lerp(speed, result.Length(), lerpFactor);
	if (speed < 3.0F) speed = 3.0F;
	if (speed > 9.0F) speed = 9.0F;
	result.Normalize();

	result = FMath::Lerp(GetActorForwardVector(), result, lerpFactor);
	if(result.Normalize()) SetActorRotation(result.Rotation());
	//result.Normalize();
	
	//SetActorRotation(result.Rotation());
	SetActorLocation(GetActorLocation() + result * speed);
}

void ABoid2::SetOther(ABoid2* o) {
	other = o;
}

void ABoid2::SetAll(TSet<ABoid2*> aB) {
	AllBoids = aB;
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Boids received"));
}