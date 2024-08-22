// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoidGpu.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ComputeShader/Public/ExampleComputeShader/ExampleComputeShader.h"
#include "BoidGpuManager.generated.h"

struct FTreeBoid
{
	uint32 id;
	FVector location;
	FVector forward;
};

struct FBoidSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static FBoxCenterAndExtent GetBoundingBox(const FTreeBoid* Element)
	{
		FBoxCenterAndExtent bc(Element->location, FVector(1, 1, 1));
		return bc;
	}

	FORCEINLINE static bool AreElementsEqual(const FTreeBoid* A, const FTreeBoid* B)
	{
		return A->id == B->id;
	}

	FORCEINLINE static void SetElementId(const FTreeBoid* Element, FOctreeElementId2 Id)
	{
	}
};

typedef TOctree2<FTreeBoid*, FBoidSemantics> FBoidOctree;

UCLASS()
class BOIDSMT_API ABoidGpuManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABoidGpuManager();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	bool executeInGPU;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	bool useSpacePartitioning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	bool useObstacleAvoidance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	int numThinkGroups;

	int debugCounter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	AActor* target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UInstancedStaticMeshComponent* MeshInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	FVector threadsShader;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	TSet<ABoidGpu*> AllBoids;

	TArray< TSet<ABoidGpu*> > thinkGroups;

	void ProcessBoid(ABoidGpu* b);
	void ProcessBoidSpacePartitioning(ABoidGpu* b, FBoidOctree* tree);
	void ProcessBoidGPU(FShaderBoidResult result, ABoidGpu* b, int id);

	FVector Steer(FVector v, FVector velocity);

	UFUNCTION(BlueprintCallable)
	void SetThinkGroups();

	void ProcessGPU();

	void ProcessCPU();

	void ProcessCPUSpacePartitioning();

	int thinkGroupCounter;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void SetAll(TSet<ABoidGpu*> aB);

	UFUNCTION(BlueprintCallable)
	void SetTarget(AActor* o);

};
