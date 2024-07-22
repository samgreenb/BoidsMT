// Fill out your copyright notice in the Description page of Project Settings.


#include "BoidGpuManager.h"
#include "ComputeShader/Public/ExampleComputeShader/ExampleComputeShader.h"

// Sets default values
ABoidGpuManager::ABoidGpuManager()
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
	numThinkGroups = 1;
	thinkGroupCounter = 0;

	MeshInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MeshInstances"));
}

// Called when the game starts or when spawned
void ABoidGpuManager::BeginPlay()
{
	Super::BeginPlay();
	
	//for (int i = 0; i++; i < numThinkGroups) {
	//	TSet<ABoidGpu*> set;
	//	thinkGroups.Add(set);
	//}

	FExampleComputeShaderDispatchParams Params(1, 1, 1);

	Params.Input[0] = 2;
	Params.Input[1] = 5;

	FExampleComputeShaderInterface::Dispatch(Params, [](int OutputVal) {
		// OutputVal == 10
		// Called when the results are back from the GPU.
		if (GEngine) 
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::FromInt(OutputVal));
	});
	
}

void ABoidGpuManager::SetThinkGroups()
{
	thinkGroups.SetNum(numThinkGroups);
	int i = 0;
	for (auto& Elem : AllBoids)
	{
		thinkGroups[i].Add(Elem);
		i++;
		i = i % numThinkGroups;
	}

}

// Called every frame
void ABoidGpuManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (auto& Elem : thinkGroups[thinkGroupCounter])
	{
		ProcessBoid(Elem);
	}
	thinkGroupCounter++;
	thinkGroupCounter  = thinkGroupCounter % numThinkGroups;

	TArray<FTransform> transforms;
	for (auto& Elem : AllBoids)
	{
		//MeshInstances->UpdateInstanceTransform(Elem->GetId(), Elem->GetBoidTransform(), true, false, true);
		transforms.Add(Elem->GetBoidTransform());
	}
	MeshInstances->BatchUpdateInstancesTransforms(0, transforms, true, false, true);
	//MeshInstances->MarkRenderStateDirty();
}

void ABoidGpuManager::ProcessBoid(ABoidGpu* b)
{
	FVector CA(0, 0, 0);
	int BCount = 0;

	FVector VM(0, 0, 0);

	FVector FC(0, 0, 0);

	FVector acceleration(0, 0, 0);

	if (useTarget && target != NULL) {
		FVector offsetToTarget = target->GetActorLocation() - b->GetBoidPosition();
		acceleration = Steer(offsetToTarget, b->GetBoidVelocity()) * targetWeight;
		//if (debug) {
		//	if (GEngine){
		//		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Actor Location : %f, %f, %f"), GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z));
		//		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Target location: %f, %f, %f"), target->GetActorLocation().X, target->GetActorLocation().Y, target->GetActorLocation().Z));
		//		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Offset         : %f, %f, %f"), offsetToTarget.X, offsetToTarget.Y, offsetToTarget.Z));
		//		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Acceleration   : %f, %f, %f"), acceleration.X, acceleration.Y, acceleration.Z));
		//	}
		//}
	}

	for (auto& Elem : AllBoids)
	{
		if (Elem == b) continue;

		// distance check
		FVector distanceVector = Elem->GetBoidPosition() - b->GetBoidPosition();
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
		VM += Elem->GetBoidVelocity();

		// Flock centering
		FC += Elem->GetBoidPosition();
	}

	if (BCount > 0) {

		CA = Steer(CA, b->GetBoidVelocity()) * collisionAvoidanceWeight;

		VM = Steer(VM, b->GetBoidVelocity()) * velocityMatchingWeight;

		FC = FC / BCount;
		FC = FC - b->GetBoidPosition();
		FC = Steer(FC, b->GetBoidVelocity()) * flockCenteringWeight;

		if (CA.Length() > 0.0f) {
			acceleration += CA;
		}
		else {
			acceleration += VM;
			acceleration += FC;
		}

	}

	b->SetAcceleration(acceleration);
}

void ABoidGpuManager::SetTarget(AActor* t) {
	target = t;
}

void ABoidGpuManager::SetAll(TSet<ABoidGpu*> aB) {
	AllBoids = aB;
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Boids received"));
}

FVector ABoidGpuManager::Steer(FVector v, FVector velocity) {
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
