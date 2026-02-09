#pragma once
#ifndef D3D12RayTracing_H_9B5E3C1A_8D4F_4A1B_9E2D_5C3F6A7B8E9D
#define D3D12RayTracing_H_9B5E3C1A_8D4F_4A1B_9E2D_5C3F6A7B8E9D

#include "D3D12Command.h"
#include "RHIRayTracingTypes.h"

namespace Render
{
	class D3D12BottomLevelAccelerationStructure : public TRefcountResource< RHIBottomLevelAccelerationStructure >
	{
	public:
		D3D12BottomLevelAccelerationStructure(D3D12System* system)
			:mSystem(system)
		{

		}

		~D3D12BottomLevelAccelerationStructure()
		{
			releaseResource();
		}

		void releaseResource();
		bool create(RayTracingGeometryDesc const* geometries, int numGeometries, EAccelerationStructureBuildFlags flags);

		D3D12BufferAllocation mScartchAllocation;
		D3D12BufferAllocation mResultAllocation;

		TArray< D3D12_RAYTRACING_GEOMETRY_DESC > mGeometryDescs;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS mInputDesc = {};
		D3D12System* mSystem;
	};

	class D3D12TopLevelAccelerationStructure : public TRefcountResource< RHITopLevelAccelerationStructure >
	{
	public:
		D3D12TopLevelAccelerationStructure(D3D12System* system)
			:mSystem(system)
		{

		}

		~D3D12TopLevelAccelerationStructure()
		{
			releaseResource();
		}

		void releaseResource();

		bool create(uint32 numInstances, EAccelerationStructureBuildFlags flags);

		D3D12BufferAllocation mScartchAllocation;
		D3D12BufferAllocation mResultAllocation;
		D3D12BufferAllocation mInstanceAllocaton;
		D3D12PooledHeapHandle mSRV;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS mInputDesc = {};
		D3D12System* mSystem;
	};


	class D3D12RayTracingPipelineState : public TRefcountResource< RHIRayTracingPipelineState >
	{
	public:

		bool initialize(D3D12System* system, RayTracingPipelineStateInitializer const& initializer);


		struct ShaderGroup
		{
			uint8            identifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
			uint32           localDataSize = 0;
			RHIShader*       shader = nullptr;
			D3D12ShaderData* shaderData = nullptr;
		};
		//RayGen
		ShaderGroup mRayGen;
		//Miss
		TArray< ShaderGroup > mMissGroups;
		//Hit Groups
		TArray< ShaderGroup > mHitGroups;


		TComPtr< ID3D12StateObject > mStateObject;
		D3D12ShaderBoundState* mBoundState = nullptr;
		TArray< TComPtr<ID3D12RootSignature> > mLocalRootSignatures;
		D3D12System* mSystem = nullptr;
		uint32 mGlobalParameterCount = 0;
	};

	class D3D12RayTracingShaderTable : public TRefcountResource< RHIRayTracingShaderTable >
	{
	public:
		D3D12RayTracingShaderTable(class D3D12RayTracingPipelineState* pso);
		virtual ~D3D12RayTracingShaderTable();

		void setMissRecord(uint32 recordIndex, void* data, uint32 size, uint32 offset = 0, int shaderIndex = -1);
		void setHitRecord(uint32 recordIndex, void* data, uint32 size, uint32 offset = 0, int shaderIndex = -1);

		
		CORE_API void setShader(uint32 recordIndex, RHIShader* shader, int optionalGroupIndex = -1) override;
		CORE_API void setValue(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, void* data) override;
		CORE_API void setTexture(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHITextureBase* texture) override;
		CORE_API void setRWTexture(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHITextureBase* texture, EAccessOp accessOp) override;
		CORE_API void setStorageBuffer(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHIBuffer* buffer, EAccessOp accessOp) override;
		CORE_API void setUniformBuffer(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHIBuffer* buffer) override;
		CORE_API void setSampler(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHISamplerState* sampler) override;



		void setHandleValue(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, void* data, uint32 size);

		void upload();

		D3D12_GPU_VIRTUAL_ADDRESS getRayGenGPUAddr() const { return mRayGenTable.allocation.gpuAddress; }
		uint32 getRayGenSize() const { return mRayGenTable.getUsedSize(); }

		D3D12_GPU_VIRTUAL_ADDRESS getMissGPUAddr() const { return mMissTable.allocation.gpuAddress; }
		uint32 getMissSize() const { return mMissTable.getUsedSize(); }
		uint32 getMissStride() const { return mMissTable.stride; }

		D3D12_GPU_VIRTUAL_ADDRESS getHitGPUAddr() const { return mHitTable.allocation.gpuAddress; }
		uint32 getHitSize() const { return mHitTable.getUsedSize(); }
		uint32 getHitStride() const { return mHitTable.stride; }

	private:
		friend class D3D12Context;
		void updateShaderRecord(EShader::Type shaderType, D3D12ShaderData& shaderData, uint32 recordIndex, void* data, uint32 size, uint32 offset);

		struct TableData
		{
			uint32 stride = 0;
			uint32 capacity = 0;
			uint32 numRecords = 0;
			D3D12BufferAllocation allocation;
			void* pMapped = nullptr;

			void release();
			bool resize(D3D12System* system, uint32 numRecords, uint32 alignment);
			uint32 getUsedSize() const { return numRecords * stride; }
		};

		// Track bound shaders to resolve parameter layout in setValue
		TArray< RHIShader* > mRayGenRecordShaders;
		TArray< RHIShader* > mMissRecordShaders;
		TArray< int >        mMissRecordIndices; // Stores index into mPSO->mMissGroups
		TArray< RHIShader* > mHitRecordShaders;
		TArray< int >        mHitRecordIndices;  // Stores index into mPSO->mHitGroups

		TableData mRayGenTable;
		TableData mMissTable;
		TableData mHitTable;

		D3D12RayTracingPipelineState* mPSO;
		D3D12System* mSystem;

		void initMissTable();
		void initHitTable();

		void updateRecordData(TableData& table, uint32 index, void* data, uint32 size, uint32 offset);
	};

}

#endif // D3D12RayTracing_H_9B5E3C1A_8D4F_4A1B_9E2D_5C3F6A7B8E9D
