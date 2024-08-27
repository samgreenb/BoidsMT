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

void ABoidGpuManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Distributes agents in equal sized groups for the staggered think rate implementation
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

void ABoidGpuManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	thinkGroupCounter++;
	thinkGroupCounter = thinkGroupCounter % numThinkGroups;

	// This section runs the selected version of the algorithm
	if (executeInGPU) {
		ProcessGPU();
	}
	else if(useSpacePartitioning){
		ProcessCPUSpacePartitioning();
	}
	else {
		ProcessCPU();
	}

	// This code runs every frame independently of the implementation used, and updates the Instanced Static Meshes object to update the visual
	// representation of the agents
	TArray<FTransform> transforms;
	for (auto& Elem : AllBoids)
	{
		Elem->UpdateBoid(DeltaTime);
		transforms.Add(Elem->GetBoidTransform());
	}
	MeshInstances->BatchUpdateInstancesTransforms(0, transforms, true, false, true);
}

// This code sets up the octree and calls the amin function for each boid
void ABoidGpuManager::ProcessCPUSpacePartitioning()
{
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

	for (auto& Elem : thinkGroups[thinkGroupCounter])
	{
		ProcessBoidSpacePartitioning(Elem, BoidTree);
	}
}

// This section queries the octree for each boid and calculates the acceleration vector
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

		FVector distanceVector =  foundBoid->location - b->GetBoidPosition();
		float distance = distanceVector.Length();

		if (distance > radius) return;

		if (distance < avoidRadius) {
			float distanceSqr = distanceVector.X * distanceVector.X + distanceVector.Y * distanceVector.Y + distanceVector.Z * distanceVector.Z;
			CA -= distanceVector / distanceSqr;
		}

		VM += foundBoid->forward;

		FC += foundBoid->location;

		BCount++;

	});

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

// Helper method to iterate over all boids in the initial implementation
void ABoidGpuManager::ProcessCPU()
{
	for (auto& Elem : thinkGroups[thinkGroupCounter])
	{
		ProcessBoid(Elem);
	}
}

// This code prepares the information of the agents for the buffer and initiates the dispatcch of the compute shader
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

		// This code runs when the GPU sends the data back and runs the main method for each boid
		int counter = 0;
		for (auto& Elem : AllBoids) {
			ProcessBoidGPU(shaderBoidsResult[counter], Elem, counter);
			counter++;
		}

	});
}

// This method uses the information received from the GPU to calculate the final acceleration vector for a given boid
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

// This code calculates the acceleration vector for a given boid in the initial implementation
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

		// Velocity Matching
		VM += Elem->GetBoidVelocity();

		// Flock centering
		FC += Elem->GetBoidPosition();
	}

	// If neighbours are found alla cceleration vectors are calculated
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
}

// Calculates the steering necessary to accelerate in a given direction
FVector ABoidGpuManager::Steer(FVector v, FVector velocity) {
	if (!v.Normalize()) {
		return FVector(0, 0, 0);
	}
	FVector result = v * maxSpeed - velocity;
	return result.GetClampedToSize(0.0f, maxSteer);
}
