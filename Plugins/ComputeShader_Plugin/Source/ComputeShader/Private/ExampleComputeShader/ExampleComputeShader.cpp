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

DECLARE_STATS_GROUP(TEXT("ExampleComputeShader"), STATGROUP_ExampleComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("ExampleComputeShader Execute"), STAT_ExampleComputeShader_Execute, STATGROUP_ExampleComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
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
		* Here's where you define one or more of the input parameters for your shader.
		* Some examples:
		*/
		// SHADER_PARAMETER(uint32, MyUint32) // On the shader side: uint32 MyUint32;
		// SHADER_PARAMETER(FVector3f, MyVector) // On the shader side: float3 MyVector;

		// SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture) // On the shader side: Texture2D<float4> MyTexture; (float4 should be whatever you expect each pixel in the texture to be, in this case float4(R,G,B,A) for 4 channels)
		// SHADER_PARAMETER_SAMPLER(SamplerState, MyTextureSampler) // On the shader side: SamplerState MySampler; // CPP side: TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();

		// SHADER_PARAMETER_ARRAY(float, MyFloatArray, [3]) // On the shader side: float MyFloatArray[3];

		// SHADER_PARAMETER_UAV(RWTexture2D<FVector4f>, MyTextureUAV) // On the shader side: RWTexture2D<float4> MyTextureUAV;
		// SHADER_PARAMETER_UAV(RWStructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWStructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_UAV(RWBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;

		// SHADER_PARAMETER_SRV(StructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: StructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Texture2D<FVector4f>, MyReadOnlyTexture) // On the shader side: Texture2D<float4> MyReadOnlyTexture;

		// SHADER_PARAMETER_STRUCT_REF(FMyCustomStruct, MyCustomStruct)

		//SHADER_PARAMETER_SRV(StructuredBuffer<FShaderBoid>, InputBoids) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
		//SHADER_PARAMETER_UAV(RWStructuredBuffer<FShaderBoidResult>, OutputBoids) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;
		
		SHADER_PARAMETER(int, NumberBoids) // On the shader side: uint32 MyUint32;
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FShaderBoid>, InputBoids)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FShaderBoid>, OutputBoids)

		//SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, Input)
		//SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int>, Output)
		

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

		/*
		* Here you define constants that can be used statically in the shader code.
		* Example:
		*/
		// OutEnvironment.SetDefine(TEXT("MY_CUSTOM_CONST"), TEXT("1"));

		/*
		* These defines are used in the thread count section of our shader
		*/
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_ExampleComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_ExampleComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_ExampleComputeShader_Z);

		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// FForwardLightingParameters::ModifyCompilationEnvironment(Parameters.Platform, OutEnvironment);
	}
private:
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FExampleComputeShader, "/ComputeShaderShaders/ExampleComputeShader/ExampleComputeShader.usf", "ExampleComputeShader", SF_Compute);

void FExampleComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FExampleComputeShaderDispatchParams Params, TArray<FShaderBoid> shaderBoids, TFunction<void(TArray<FShaderBoidResult> shaderBoidsResult)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);

	{
		SCOPE_CYCLE_COUNTER(STAT_ExampleComputeShader_Execute);
		DECLARE_GPU_STAT(ExampleComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "ExampleComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, ExampleComputeShader);
		
		typename FExampleComputeShader::FPermutationDomain PermutationVector;
		
		// Add any static permutation options here
		// PermutationVector.Set<FExampleComputeShader::FMyPermutationName>(12345);

		TShaderMapRef<FExampleComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		

		bool bIsShaderValid = ComputeShader.IsValid();
		int numBoids = shaderBoids.Num();
		if (bIsShaderValid) {
			FExampleComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FExampleComputeShader::FParameters>();
			
			//FString log = shaderBoids[0].location.ToString();
			//FString log2 = shaderBoids[0].forward.ToString();
			//UE_LOG(LogTemp, Warning, TEXT("%s"), *log);
			//UE_LOG(LogTemp, Warning, TEXT("%s"), *log2);


			//Input buffer of boids
			//TArray<FShaderBoid> boidst;
			//FShaderBoid a;
			//a.forward = FVector3f(7.0f);
			//a.location = FVector3f(8.0f);
			//boidst.Add(a);
			uint32 size1 = shaderBoids.Num();
			//UE_LOG(LogTemp, Warning, TEXT("Input Size %i"), size1);
			//UE_LOG(LogTemp, Warning, TEXT("Size of fv3f %i"), sizeof(FVector3f));
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
			//
			//output buffer of acceleration values
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
			//

			//TArray<FShaderBoid> boidst;
			//FShaderBoid test[10];
			//boidst.Add(test[0]);
			//const void* RawData = (void*)test;
			//int NumInputs = 10;
			//int InputSize = sizeof(FShaderBoid);
			//
			//FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), InputSize, NumInputs, RawData, InputSize * NumInputs);
			//FRDGBufferRef InputBuffer2 = CreateStructuredUploadBuffer(GraphBuilder, TEXT("InputBoidsBuffer"), boidst);

			//PassParameters->InputBoids = GraphBuilder.Create
			//PassParameters->InputBoids = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer2, PF_R32_SINT));
			//PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer2, PF_R32_SINT));
			//PassParameters->InputBoids = GraphBuilder.CreateSRV(FRHIShaderResourceView(InputBuffer2, PF_R32_SINT));

			//FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
			//	FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1),
			//	TEXT("OutputBuffer"));

			//PassParameters->OutputBoids = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));
			

			//auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
			auto GroupCount = FIntVector(Params.X, Params.Y, Params.Z);
			FString loggc = GroupCount.ToString();
			//UE_LOG(LogTemp, Warning, TEXT("group count %s"), *loggc);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteExampleComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
			});

			
			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteExampleComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, IOBuffer, 0u);

			auto RunnerFunc = [GPUBufferReadback, numBoids, AsyncCallback](auto&& RunnerFunc) -> void {
				if (GPUBufferReadback->IsReady()) {
					
					FShaderBoidResult* Buffer = (FShaderBoidResult*)GPUBufferReadback->Lock(1);
					TArray<FShaderBoidResult> readbackArray(Buffer, numBoids);

					int OutVal = 513;

					GPUBufferReadback->Unlock();

					//#if WITH_EDITOR
						//FShaderBoidResult buffer0 = Buffer[0];
						//FString log = buffer0.VM.ToString();
						//UE_LOG(LogTemp, Warning, TEXT("%s"), *log);
					//	//GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(buffer0.FC.ToString()));
					//#endif

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

			// We exit here as we don't want to crash the game if the shader is not found or has an error.
			
		}
	}

	GraphBuilder.Execute();
}