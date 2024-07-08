// Fill out your copyright notice in the Description page of Project Settings.


#include "BoidGpuManager.h"

// Sets default values
ABoidGpuManager::ABoidGpuManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABoidGpuManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABoidGpuManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
