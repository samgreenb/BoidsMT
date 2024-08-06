// Fill out your copyright notice in the Description page of Project Settings.


#include "BoidGpu.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ABoidGpu::ABoidGpu()
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
	acceleration = FVector(0, 0, 0);

	obstacleAvoidance = false;
	
}

// Called when the game starts or when spawned
void ABoidGpu::BeginPlay()
{
	Super::BeginPlay();
	SetActorRotation(FMath::VRand().Rotation());
	position = GetActorLocation();

	if (randomInitialSpeed) {
		velocity = GetActorForwardVector() * FMath::Lerp(minSpeed, maxSpeed, FMath::FRand());
	}
	else
	{
		velocity = GetActorForwardVector() * initialSpeed;
	}

}

void ABoidGpu::UpdateBoid(float DeltaTime)
{
	if (obstacleAvoidance) {
		FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
		RV_TraceParams.bTraceComplex = true;
		//RV_TraceParams.bTraceAsyncScene = true;
		RV_TraceParams.bReturnPhysicalMaterial = false;

		//Re-initialize hit info
		FHitResult RV_Hit(ForceInit);
		FVector f = GetBoidVelocity();
		f.Normalize();
		//call GetWorld() from within an actor extending class

		GetWorld()->LineTraceSingleByChannel(
			RV_Hit,		//result
			GetBoidPosition(),	//start
			GetBoidPosition() + (f * FVector(1000.0f)), //end
			ECC_WorldStatic, //collision channel
			RV_TraceParams
		);

		if (RV_Hit.bBlockingHit) {
			//FString log = RV_Hit.GetActor()->GetActorNameOrLabel();
			//UE_LOG(LogTemp, Warning, TEXT("got a hit with %s"), *log);

			FVector OAAcceleration;
			OAAcceleration = FVector::VectorPlaneProject(RV_Hit.ImpactNormal, f);
			OAAcceleration.Normalize();
			OAAcceleration *= 1000;
			OAAcceleration = Steer(OAAcceleration);

			velocity += OAAcceleration * DeltaTime;
			velocity = velocity.GetClampedToSize(minSpeed, maxSpeed);
			position = position + velocity * DeltaTime;
			return;
		}
	}


	velocity += acceleration * DeltaTime;
	velocity = velocity.GetClampedToSize(minSpeed, maxSpeed);
	position = position + velocity * DeltaTime;
}

// Called every frame
void ABoidGpu::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//velocity += acceleration * DeltaTime;
	//velocity = velocity.GetClampedToSize(minSpeed, maxSpeed);
	//position = position + velocity * DeltaTime;

	//UKismetSystemLibrary::LineTraceSingle(nullptr, GetActorLocation(), GetActorLocation() + GetActorForwardVector() * 10, ETraceTypeQuery::TraceTypeQuery1, false);

	// raycasting is here

	//FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	//RV_TraceParams.bTraceComplex = true;
	////RV_TraceParams.bTraceAsyncScene = true;
	//RV_TraceParams.bReturnPhysicalMaterial = false;

	////Re-initialize hit info
	//FHitResult RV_Hit(ForceInit);

	////call GetWorld() from within an actor extending class
	//GetWorld()->LineTraceSingleByChannel(
	//	RV_Hit,		//result
	//	GetActorLocation(),	//start
	//	GetActorLocation() + (GetActorForwardVector() * FVector(10.0f)), //end
	//	ECC_Pawn, //collision channel
	//	RV_TraceParams
	//);


	//float targetSpeed = velocity.Length();
	//FVector dir = velocity / targetSpeed;
	//targetSpeed = FMath::Clamp(targetSpeed, minSpeed, maxSpeed);
	//velocity = dir * targetSpeed;

	//SetActorRotation(dir.Rotation());
	//SetActorLocation(GetActorLocation() + velocity * DeltaTime);



	//SetActorLocationAndRotation(GetActorLocation() + velocity * DeltaTime, velocity.Rotation(), false, nullptr ,ETeleportType::TeleportPhysics);
	//TeleportTo(GetActorLocation() + velocity * DeltaTime, velocity.Rotation(), true, true);
	//SetWorldLocationAndRotation(GetActorLocation() + velocity * DeltaTime, velocity.Rotation(), true, true);
	//GetRootComponent()->SetWorldLocationAndRotation(GetActorLocation() + velocity * DeltaTime, velocity.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);

	//GetRootComponent()->MoveComponent(velocity * DeltaTime, velocity.Rotation().Quaternion(), false, nullptr, MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);

	//if (debug) {
	//	if (GEngine)
	//		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("NBoids in range: %i"), BCount));
	//}
}

void ABoidGpu::SetOther(ABoidGpu* o) {
	other = o;
}

void ABoidGpu::SetTarget(AActor * t) {
	target = t;
}

AActor* ABoidGpu::GetTarget() {
	return target;
}

void ABoidGpu::SetAll(TSet<ABoidGpu*> aB) {
	AllBoids = aB;
	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Boids received"));*/
}

FVector ABoidGpu::Steer(FVector v) {
	//if (GEngine) {
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Before : %f, %f, %f"), v.X, v.Y, v.Z));
	//}
	//v.Normalize();
	//if (GEngine) {
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("After : %f, %f, %f"), v.X, v.Y, v.Z));
	//}
	if (!v.Normalize()) {
		return FVector(0, 0, 0);
	}
	FVector result = v * maxSpeed - velocity;
	return result.GetClampedToSize(0.0f, maxSteer);
	//return result;

}

void ABoidGpu::SetAcceleration(FVector a) {
	acceleration = a;
}

void ABoidGpu::SetId(int32 i) {
	id = i;
}

int32 ABoidGpu::GetId() {
	return id;
}

FTransform ABoidGpu::GetBoidTransform() {
	FTransform t;
	t.SetLocation(position);
	t.SetRotation(velocity.Rotation().Quaternion());
	t.SetScale3D(FVector(1, 1, 1));
	return t;
}

FVector ABoidGpu::GetBoidVelocity() {
	return velocity;
}

FVector ABoidGpu::GetBoidPosition() {
	return position;
}

