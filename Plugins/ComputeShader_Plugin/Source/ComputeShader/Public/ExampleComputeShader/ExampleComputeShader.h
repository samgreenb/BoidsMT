#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialRenderProxy.h"

#include "ExampleComputeShader.generated.h"

// Struct that contains the number of groups to dispatch
struct COMPUTESHADER_API FExampleComputeShaderDispatchParams
{
	int X;
	int Y;
	int Z;

	
	int Input[2];
	int Output;
	
	

	FExampleComputeShaderDispatchParams(int x, int y, int z)
		: X(x)
		, Y(y)
		, Z(z)
	{
	}
};


// Struct that defines the input buffer to the shader
struct COMPUTESHADER_API FShaderBoid
{
	FVector3f location;
	float padding;
	FVector3f forward;
	float padding2;
};

//Struct that defines the output / readback buffer for the shader
struct COMPUTESHADER_API FShaderBoidResult
{
	FVector3f CA;
	FVector3f VM;
	FVector3f FC;
	uint32 neighbours;
};

class COMPUTESHADER_API FExampleComputeShaderInterface {

public:

	static void DispatchRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FExampleComputeShaderDispatchParams Params, TArray<FShaderBoid> shaderBoids,
		TFunction<void(TArray<FShaderBoidResult> shaderBoidsResult)> AsyncCallback
	);


	static void DispatchGameThread(
		FExampleComputeShaderDispatchParams Params, TArray<FShaderBoid> shaderBoids,
		TFunction<void(TArray<FShaderBoidResult> shaderBoidsResult)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[Params, shaderBoids, AsyncCallback](FRHICommandListImmediate& RHICmdList)
		{
			DispatchRenderThread(RHICmdList, Params, shaderBoids, AsyncCallback);
		});
	}

	// This is the method called from the main program code to dispatch the shader
	static void Dispatch(
		FExampleComputeShaderDispatchParams Params, TArray<FShaderBoid> shaderBoids,
		TFunction<void(TArray<FShaderBoidResult> shaderBoidsResult)> AsyncCallback
	)
	{
		if (IsInRenderingThread()) {
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, shaderBoids, AsyncCallback);
		}else{
			DispatchGameThread(Params, shaderBoids, AsyncCallback);
		}
	}
};

UCLASS()
class COMPUTESHADER_API UExampleComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	
	virtual void Activate() override {

		FExampleComputeShaderDispatchParams Params(1, 1, 1);
		Params.Input[0] = Arg1;
		Params.Input[1] = Arg2;

	}
	
	
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UExampleComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, int Arg1, int Arg2) {
		UExampleComputeShaderLibrary_AsyncExecution* Action = NewObject<UExampleComputeShaderLibrary_AsyncExecution>();
		Action->Arg1 = Arg1;
		Action->Arg2 = Arg2;
		Action->RegisterWithGameInstance(WorldContextObject);

		return Action;
	}

	
	int Arg1;
	int Arg2;
	
};