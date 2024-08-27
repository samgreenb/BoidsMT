#include "BoidGpu.h"
#include "Kismet/KismetSystemLibrary.h"

ABoidGpu::ABoidGpu()
{
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

// Called when the game starts or when spawned. Randomizes the agent forward vector and velocity
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

// Method called every frame by the manager to update the agent velo0city using the acceleration vector
void ABoidGpu::UpdateBoid(float DeltaTime)
{
	if (obstacleAvoidance) {
		FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
		RV_TraceParams.bTraceComplex = true;
		RV_TraceParams.bReturnPhysicalMaterial = false;

		FHitResult RV_Hit(ForceInit);
		FVector f = GetBoidVelocity();
		f.Normalize();

		GetWorld()->LineTraceSingleByChannel(
			RV_Hit,		//result
			GetBoidPosition(),	//start
			GetBoidPosition() + (f * FVector(1000.0f)), //end
			ECC_WorldStatic, //collision channel
			RV_TraceParams
		);

		// If a ray cast hits the usual acceleration calculation is s ignored and a obstacle avoidance acceleration is used
		if (RV_Hit.bBlockingHit) {

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

// Calculates the steering necessary to accelerate in a given direction
FVector ABoidGpu::Steer(FVector v) {

	if (!v.Normalize()) {
		return FVector(0, 0, 0);
	}
	FVector result = v * maxSpeed - velocity;
	return result.GetClampedToSize(0.0f, maxSteer);

}

void ABoidGpu::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Not used

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

