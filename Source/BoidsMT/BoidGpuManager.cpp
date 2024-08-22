// Fill out your copyright notice in the Description page of Project Settings.


#include "BoidGpuManager.h"
#include "Kismet/KismetMathLibrary.h"

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
	executeInGPU = false;
	useObstacleAvoidance = false;
	numThinkGroups = 1;
	thinkGroupCounter = 0;

	debugCounter = 0;
	threadsShader = FVector(1);

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

	thinkGroupCounter++;
	thinkGroupCounter = thinkGroupCounter % numThinkGroups;

	if (executeInGPU) {
		ProcessGPU();
	}
	else if(useSpacePartitioning){
		ProcessCPUSpacePartitioning();
	}
	else {
		ProcessCPU();
	}

	TArray<FTransform> transforms;
	for (auto& Elem : AllBoids)
	{
		Elem->UpdateBoid(DeltaTime);
		//MeshInstances->UpdateInstanceTransform(Elem->GetId(), Elem->GetBoidTransform(), true, false, true);
		transforms.Add(Elem->GetBoidTransform());
	}
	MeshInstances->BatchUpdateInstancesTransforms(0, transforms, true, false, true);
	//MeshInstances->MarkRenderStateDirty();
}

void ABoidGpuManager::ProcessCPUSpacePartitioning()
{
	//double start = FPlatformTime::Seconds();
	FVector min = FVector(0);
	FVector max = FVector(0);

	for (auto& Elem : AllBoids)
	{
		min = Elem->GetBoidPosition();
		max = Elem->GetBoidPosition();
		break;
	}

	TArray<FTreeBoid> shaderBoids;
	for (auto& Elem : AllBoids)
	{
		FTreeBoid b;
		FVector p = Elem->GetBoidPosition();
		b.id = Elem->GetUniqueID();
		b.location = Elem->GetBoidPosition();
		b.forward = Elem->GetBoidVelocity();
		shaderBoids.Add(b);

		if (p.X < min.X) min.X = p.X;
		if (p.Y < min.Y) min.Y = p.Y;
		if (p.Z < min.Z) min.Z = p.Z;

		if (p.X > max.X) max.X = p.X;
		if (p.Y > max.Y) max.Y = p.Y;
		if (p.Z > max.Z) max.Z = p.Z;
	}

	min -= FVector(2);
	max += FVector(2);

	FBox NewBounds = FBox(min, max);
	FBoidOctree* BoidTree;	
	BoidTree = new FBoidOctree(NewBounds.GetCenter(), NewBounds.GetExtent().GetMax()); // const FVector & InOrigin, float InExtent

	for (auto& Elem : shaderBoids)
	{
		BoidTree->AddElement(&Elem);
	}
	//double end = FPlatformTime::Seconds();
	//UE_LOG(LogTemp, Warning, TEXT("tree built in %f mseconds."), (end - start)*1000);


	//double start2 = FPlatformTime::Seconds();
	for (auto& Elem : thinkGroups[thinkGroupCounter])
	{
		ProcessBoidSpacePartitioning(Elem, BoidTree);
	}
	//double end2 = FPlatformTime::Seconds();
	//UE_LOG(LogTemp, Warning, TEXT("boids processed in %f mseconds."), (end2 - start2)*1000);
}

void ABoidGpuManager::ProcessBoidSpacePartitioning(ABoidGpu* b, FBoidOctree* tree)
{
	FBoxCenterAndExtent bounds = FBoxCenterAndExtent(b->GetBoidPosition(), FVector(radius));

	FVector CA(0, 0, 0);
	int BCount = 0;

	FVector VM(0, 0, 0);

	FVector FC(0, 0, 0);

	FVector acceleration(0, 0, 0);

	if (useTarget && target != NULL) {
		FVector offsetToTarget = target->GetActorLocation() - b->GetBoidPosition();
		acceleration = Steer(offsetToTarget, b->GetBoidVelocity()) * targetWeight;
	}

	tree->FindElementsWithBoundsTest(bounds, [&](const FTreeBoid* foundBoid){
		
		if (b->GetUniqueID() == foundBoid->id) return;

		// Collision avoidance
		FVector distanceVector =  foundBoid->location - b->GetBoidPosition();
		float distance = distanceVector.Length();

		if (distance > radius) return;

		if (distance < avoidRadius) {
			float distanceSqr = distanceVector.X * distanceVector.X + distanceVector.Y * distanceVector.Y + distanceVector.Z * distanceVector.Z;
			CA -= distanceVector / distanceSqr;
		}

		// Velocity Matching (heading matching for now)
		VM += foundBoid->forward;

		// Flock centering
		FC += foundBoid->location;

		BCount++;

	});

	//UE_LOG(LogTemp, Warning, TEXT("bcount %i"), BCount);
	//FString log1 = CA.ToString();
	//FString log2 = FC.ToString();
	//FString log3 = VM.ToString();
	//UE_LOG(LogTemp, Warning, TEXT("CA %s"), *log1);
	//UE_LOG(LogTemp, Warning, TEXT("FC %s"), *log2);
	//UE_LOG(LogTemp, Warning, TEXT("VM %s"), *log3);

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

	//FString log4 = acceleration.ToString();
	//UE_LOG(LogTemp, Warning, TEXT("acc %s"), *log4);

	b->SetAcceleration(acceleration);
}

void ABoidGpuManager::ProcessCPU()
{

	for (auto& Elem : thinkGroups[thinkGroupCounter])
	{
		ProcessBoid(Elem);
	}
}

void ABoidGpuManager::ProcessGPU()
{
	FExampleComputeShaderDispatchParams Params(UKismetMathLibrary::FCeil(AllBoids.Num() / 256), 1, 1);

	Params.Input[0] = 2;
	Params.Input[1] = 6;

	TArray<FShaderBoid> shaderBoids;
	for (auto& Elem : AllBoids)
	{
		FShaderBoid b;
		b.location = FVector3f(Elem->GetBoidPosition());
		b.forward = FVector3f(Elem->GetBoidVelocity());
		shaderBoids.Add(b);
	}

	FExampleComputeShaderInterface::Dispatch(Params, shaderBoids, [&](TArray<FShaderBoidResult> shaderBoidsResult) {
		// OutputVal == 10
		// Called when the results are back from the GPU.

		//FString log1 = shaderBoidsResult[256].CA.ToString();
		//FString log2 = shaderBoidsResult[256].FC.ToString();
		//FString log3 = shaderBoidsResult[256].VM.ToString();
		//UE_LOG(LogTemp, Warning, TEXT("CA %s"), *log1);
		//UE_LOG(LogTemp, Warning, TEXT("FC %s"), *log2);
		//UE_LOG(LogTemp, Warning, TEXT("VM %s"), *log3);
		//UE_LOG(LogTemp, Warning, TEXT("Neigh %i"), shaderBoidsResult[256].neighbours);
		//UE_LOG(LogTemp, Warning, TEXT("Exit size %i"), shaderBoidsResult.Num());

		//UE_LOG(LogTemp, Warning, TEXT("HERE"));
		//UE_LOG(LogTemp, Warning, TEXT("%i"), OutputVal);
		//UE_LOG(LogTemp, Warning, TEXT("%i"), debugCounter);
		//debugCounter++;
		int counter = 0;
		for (auto& Elem : AllBoids) {
			ProcessBoidGPU(shaderBoidsResult[counter], Elem, counter);
			counter++;
		}

	});
}

void ABoidGpuManager::ProcessBoidGPU(FShaderBoidResult result, ABoidGpu* b, int id)
{
	FVector CA(result.CA);

	FVector VM(result.VM);

	FVector FC(result.FC);

	FVector acceleration(0, 0, 0);

	if (useTarget && target != NULL) {
		FVector offsetToTarget = target->GetActorLocation() - b->GetBoidPosition();
		acceleration = Steer(offsetToTarget, b->GetBoidVelocity()) * targetWeight;
	}

	if (result.neighbours > 0) {

		CA = Steer(CA, b->GetBoidVelocity()) * collisionAvoidanceWeight;

		VM = Steer(VM, b->GetBoidVelocity()) * velocityMatchingWeight;

		FC = FC / result.neighbours;
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
		float distance = distanceVector.Length();

		if (distance > radius) {
			continue;
		}

		BCount++;

		// Collision avoidance
		if (distance < avoidRadius) {
			float distanceSqr = distanceVector.X * distanceVector.X + distanceVector.Y * distanceVector.Y + distanceVector.Z * distanceVector.Z;
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
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Boids received"));
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
