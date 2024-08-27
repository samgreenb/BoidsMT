#include "ExampleComputeShader.h"
#include "ComputeShader/Public/ExampleComputeShader/ExampleComputeShader.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MeshDrawShaderBindings.h"
#include "RHIGPUReadback.h"
#include "MeshPassUtils.h"
#include "MaterialShader.h"


/*

The code for this basic empty compute shader was adapted from Shadeups Bare-bones Compute Shader in UE5

https://unreal.shadeup.dev/docs/compute/base

*/

DECLARE_STATS_GROUP(TEXT("ExampleComputeShader"), STATGROUP_ExampleComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("ExampleComputeShader Execute"), STAT_ExampleComputeShader_Execute, STATGROUP_ExampleComputeShader);

class COMPUTESHADER_API FExampleComputeShader: public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FExampleComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FExampleComputeShader, FGlobalShader);
	
	
	class FExampleComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<
		FExampleComputeShader_Perm_TEST
	>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		/*
		* This is the declaration of the input parameters of the compute shader using unreal macros
		*/
		
		SHADER_PARAMETER(int, NumberBoids)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FShaderBoid>, InputBoids)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FShaderBoid>, OutputBoids)
		

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_ExampleComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_ExampleComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_ExampleComputeShader_Z);
	}
private:
};

IMPLEMENT_GLOBAL_SHADER(FExampleComputeShader, "/ComputeShaderShaders/ExampleComputeShader/ExampleComputeShader.usf", "ExampleComputeShader", SF_Compute);

// This is the code that creeates the buffers, sets up the pass parameters, dispatches the compute shader and reads back from the GPU
void FExampleComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FExampleComputeShaderDispatchParams Params, TArray<FShaderBoid> shaderBoids, TFunction<void(TArray<FShaderBoidResult> shaderBoidsResult)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);

	{
		SCOPE_CYCLE_COUNTER(STAT_ExampleComputeShader_Execute);
		DECLARE_GPU_STAT(ExampleComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "ExampleComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, ExampleComputeShader);
		
		typename FExampleComputeShader::FPermutationDomain PermutationVector;

		TShaderMapRef<FExampleComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);


		bool bIsShaderValid = ComputeShader.IsValid();
		int numBoids = shaderBoids.Num();
		if (bIsShaderValid) {
			FExampleComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FExampleComputeShader::FParameters>();

			// This section creates the input buffer with the required size and populates it with the data created in the main program
			uint32 size1 = shaderBoids.Num();
			if (size1 > 0)
			{
				FRDGBufferRef InputBuffer4 = CreateStructuredBuffer(
					GraphBuilder,
					TEXT("InputBoidsBuffer"),
					sizeof(FShaderBoid),
					size1,
					shaderBoids.GetData(),
					sizeof(FShaderBoid) * size1,
					ERDGInitialDataFlags::None);
				PassParameters->InputBoids = GraphBuilder.CreateSRV(InputBuffer4, PF_R32_UINT);
			}
			
			// This section creates the output buffer with the required size to be able thold data for every boid
			TArray<FShaderBoidResult> boidsrt;
			FShaderBoidResult b;
			boidsrt.Add(b);
			uint32 size2 = boidsrt.Num();
			FRDGBufferRef IOBuffer = NULL;
			if (size1 > 0)
			{
				IOBuffer = CreateStructuredBuffer(
					GraphBuilder,
					TEXT("OutputBoidsBuffer"),
					sizeof(FShaderBoidResult),
					size1,
					boidsrt.GetData(),
					sizeof(FShaderBoidResult) * 1,
					ERDGInitialDataFlags::None);
				PassParameters->OutputBoids = GraphBuilder.CreateUAV(IOBuffer, PF_R32_UINT);
			}
			PassParameters->NumberBoids = numBoids;

			// Setting the group count calculated in the main program
			auto GroupCount = FIntVector(Params.X, Params.Y, Params.Z);
			FString loggc = GroupCount.ToString();

			// This sections dispatches the shader on the GPU
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteExampleComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
			});

			// This section marks the output buffer as the readback buffer
			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteExampleComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, IOBuffer, 0u);

			// THis code will be called when the GPU is done running the shader and will convert the received buffer to an array of boid data to be used
			// in the main program
			auto RunnerFunc = [GPUBufferReadback, numBoids, AsyncCallback](auto&& RunnerFunc) -> void {
				if (GPUBufferReadback->IsReady()) {
					
					FShaderBoidResult* Buffer = (FShaderBoidResult*)GPUBufferReadback->Lock(1);
					TArray<FShaderBoidResult> readbackArray(Buffer, numBoids);

					int OutVal = 513;

					GPUBufferReadback->Unlock();

					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, readbackArray]() {
						AsyncCallback(readbackArray);
					});

					delete GPUBufferReadback;
				} else {
					AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
						RunnerFunc(RunnerFunc);
					});
				}
			};

			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
				RunnerFunc(RunnerFunc);
			});
			
		} else {
			#if WITH_EDITOR
				GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The compute shader has a problem.")));
			#endif
			
		}
	}

	GraphBuilder.Execute();
}