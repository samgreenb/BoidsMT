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
	targetWeight = 0.1;

	radius = 100.0f;
	avoidRadius = 500.0f;
	lerpFactor = 0.15;
	initialSpeed = 60.0f;
	minSpeed = 30.0f;
	maxSpeed = 90.0f;
	maxSteer = 1.0f;
	randomInitialSpeed = false;
	useTarget = false;
	debug = false;
}

// Called when the game starts or when spawned
void ABoid2::BeginPlay()
{
	Super::BeginPlay();
	SetActorRotation(FMath::VRand().Rotation());

	if (randomInitialSpeed) {
		velocity = GetActorForwardVector() * FMath::Lerp(minSpeed,maxSpeed,FMath::FRand());
	} 
	else
	{
		velocity = GetActorForwardVector() * initialSpeed;
	}
	
}

// Called every frame
void ABoid2::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (debug && GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Velocity : %f, %f, %f"), velocity.X, velocity.Y, velocity.Z));
	}

	FVector CA(0,0,0);
	int BCount = 0;

	FVector VM(0, 0, 0);

	FVector FC(0, 0, 0);

	FVector acceleration(0, 0, 0);
	if (useTarget && target != NULL) {
		FVector offsetToTarget = target->GetActorLocation() - GetActorLocation();
		acceleration = Steer(offsetToTarget) * targetWeight;
		//if (debug) {
		//	if (GEngine){
		//		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Actor Location : %f, %f, %f"), GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z));
		//		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Target location: %f, %f, %f"), target->GetActorLocation().X, target->GetActorLocation().Y, target->GetActorLocation().Z));
		//		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Offset         : %f, %f, %f"), offsetToTarget.X, offsetToTarget.Y, offsetToTarget.Z));
		//		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Acceleration   : %f, %f, %f"), acceleration.X, acceleration.Y, acceleration.Z));
		//	}
		//}
	}

	if (AllBoids.Num() > 1) {

		for (auto& Elem : AllBoids)
		{
			if (Elem == this) continue;

			//// distance check
			//FVector direction = GetActorLocation() - Elem->GetActorLocation();
			//float distance = direction.Length();
			//if (distance > radius) {
			//	continue;
			//}

			//// Collision avoidance
			//CA += direction.Normalize() * FVector((radius - distance));
			//BCount++;

			//// Velocity Matching (heading matching for now)
			//VM += Elem->GetActorForwardVector();

			//// Flock centering
			//FC += Elem->GetActorLocation();

			// distance check
			FVector distanceVector = Elem->GetActorLocation() - GetActorLocation();
			float distanceSqr = distanceVector.X * distanceVector.X + distanceVector.Y * distanceVector.Y + distanceVector.Z * distanceVector.Z;
			float distance = distanceVector.Length();

			if (distanceSqr > radius * radius) {
				continue;
			}

			BCount++;

			// Collision avoidance
			if (distanceSqr < avoidRadius * avoidRadius) {
				CA -= distanceVector / distanceSqr;
			}
			
			// Velocity Matching (heading matching for now)
			VM += Elem->velocity;

			// Flock centering
			FC += Elem->GetActorLocation();
		}

	}

	//FVector result = CA * collisionAvoidanceWeight + VM * velocityMatchingWeight + FC * flockCenteringWeight;
	//speed = FMath::Lerp(speed, result.Length(), lerpFactor);
	//if (speed < 3.0F) speed = 3.0F;
	//if (speed > 9.0F) speed = 9.0F;
	//result.Normalize();

	//result = FMath::Lerp(GetActorForwardVector(), result, lerpFactor);
	//if(result.Normalize()) SetActorRotation(result.Rotation());
	////result.Normalize();
	//
	////SetActorRotation(result.Rotation());
	//SetActorLocation(GetActorLocation() + result * speed);

	if (BCount > 0) {

		CA = Steer(CA) * collisionAvoidanceWeight;
		
		VM = Steer(VM) * velocityMatchingWeight;

		FC = FC / BCount;
		FC = FC - GetActorLocation();
		FC = Steer(FC) * flockCenteringWeight;
		
		if (CA.Length() > 0.0f) {
			acceleration += CA;
		}
		else {
			acceleration += VM;
			acceleration += FC;
		}
		
	}

	//if (debug && GEngine) {
	//	GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Acceleration : %f, %f, %f"), acceleration.X, acceleration.Y, acceleration.Z));
	//}

	velocity += acceleration * DeltaTime;
	float targetSpeed = velocity.Length();
	FVector dir = velocity / targetSpeed;
	targetSpeed = FMath::Clamp(targetSpeed, minSpeed, maxSpeed);
	velocity = dir * targetSpeed;

	SetActorRotation(dir.Rotation());
	SetActorLocation(GetActorLocation() + velocity * DeltaTime);

	//if (debug) {
	//	if (GEngine)
	//		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("NBoids in range: %i"), BCount));
	//}
}

void ABoid2::SetOther(ABoid2* o) {
	other = o;
}

void ABoid2::SetTarget(AActor* t) {
	target = t;
}

AActor* ABoid2::GetTarget() {
	return target;
}

void ABoid2::SetAll(TSet<ABoid2*> aB) {
	AllBoids = aB;
	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Boids received"));*/
}

FVector ABoid2::Steer(FVector v) {
	//if (GEngine) {
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Before : %f, %f, %f"), v.X, v.Y, v.Z));
	//}
	//v.Normalize();
	//if (GEngine) {
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("After : %f, %f, %f"), v.X, v.Y, v.Z));
	//}
	if(!v.Normalize()){
		return FVector(0, 0, 0);
	}
	FVector result = v * maxSpeed - velocity;
	return result.GetClampedToSize(0.0f, maxSteer);
	//return result;
	
}