#include "D3D12RayTracing.h"

#include "D3D12Common.h"
#include "D3D12Buffer.h"
#include "D3D12ShaderCommon.h"

#include <vector>
#include <memory>


namespace Render
{
	void D3D12BottomLevelAccelerationStructure::releaseResource()
	{
		if (mResultAllocation.ptr)
		{
			mResultAllocation.ptr->Release();
			mResultAllocation.ptr = nullptr;
		}
		if (mScartchAllocation.ptr)
		{
			mScartchAllocation.ptr->Release();
			mScartchAllocation.ptr = nullptr;
		}
	}

	bool D3D12BottomLevelAccelerationStructure::create(RayTracingGeometryDesc const* geometries, int numGeometries, EAccelerationStructureBuildFlags flags)
	{
		mGeometryDescs.resize(numGeometries);
		for (int i = 0; i < numGeometries; ++i)
		{
			auto& dst = mGeometryDescs[i];
			auto const& src = geometries[i];

			if (src.type == ERayTracingGeometryType::Triangles)
			{
				dst.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				dst.Flags = src.bOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

				dst.Triangles.VertexBuffer.StartAddress = D3D12Cast::To(src.vertexBuffer)->getResource()->GetGPUVirtualAddress() + src.vertexOffset;
				dst.Triangles.VertexBuffer.StrideInBytes = src.vertexStride;
				dst.Triangles.VertexCount = src.vertexCount;
				dst.Triangles.VertexFormat = D3D12Translate::To(src.vertexFormat, false);
				dst.Triangles.IndexBuffer = src.indexBuffer ? D3D12Cast::To(src.indexBuffer)->getResource()->GetGPUVirtualAddress() + src.indexOffset : 0;
				dst.Triangles.IndexFormat = src.indexBuffer ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN; //TODO: 16bit support
				dst.Triangles.IndexCount = src.indexCount;
				dst.Triangles.Transform3x4 = 0;
			}
			else
			{
				dst.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
				dst.Flags = src.bOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
				
				dst.AABBs.AABBs.StartAddress = D3D12Cast::To(src.vertexBuffer)->getResource()->GetGPUVirtualAddress() + src.vertexOffset;
				dst.AABBs.AABBs.StrideInBytes = src.vertexStride;
				dst.AABBs.AABBCount = src.vertexCount;
			}
		}

		mInputDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		mInputDesc.Flags = D3D12Translate::To(flags);
		mInputDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		mInputDesc.NumDescs = numGeometries;
		mInputDesc.pGeometryDescs = mGeometryDescs.data();

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		mSystem->mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&mInputDesc, &info);

		{
			D3D12_RESOURCE_DESC bufferDesc = FD3D12Init::BufferDesc(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			D3D12_HEAP_PROPERTIES heapProps = FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT);
			if (FAILED(mSystem->mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&mResultAllocation.ptr))))
				return false;
			mResultAllocation.gpuAddress = mResultAllocation.ptr->GetGPUVirtualAddress();
			mResultAllocation.size = (uint32)info.ResultDataMaxSizeInBytes;
		}

		{
			D3D12_RESOURCE_DESC bufferDesc = FD3D12Init::BufferDesc(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			D3D12_HEAP_PROPERTIES heapProps = FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT);
			if (FAILED(mSystem->mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&mScartchAllocation.ptr))))
				return false;
			mScartchAllocation.gpuAddress = mScartchAllocation.ptr->GetGPUVirtualAddress();
			mScartchAllocation.size = (uint32)info.ScratchDataSizeInBytes;
		}

		return true;
	}


	void D3D12TopLevelAccelerationStructure::releaseResource()
	{
		if (mResultAllocation.ptr)
		{
			mResultAllocation.ptr->Release();
			mResultAllocation.ptr = nullptr;
		}
		if (mScartchAllocation.ptr)
		{
			mScartchAllocation.ptr->Release();
			mScartchAllocation.ptr = nullptr;
		}
		if (mInstanceAllocaton.ptr)
		{
			mInstanceAllocaton.ptr->Release();
			mInstanceAllocaton.ptr = nullptr;
		}
		D3D12DescriptorHeapPool::FreeHandle(mSRV);
	}

	bool D3D12TopLevelAccelerationStructure::create(uint32 numInstances, EAccelerationStructureBuildFlags flags)
	{
		mInputDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		mInputDesc.Flags = D3D12Translate::To(flags);
		mInputDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		mInputDesc.NumDescs = numInstances;
		mInputDesc.pGeometryDescs = nullptr;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		mSystem->mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&mInputDesc, &info);

		{
			D3D12_RESOURCE_DESC bufferDesc = FD3D12Init::BufferDesc(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			D3D12_HEAP_PROPERTIES heapProps = FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT);
			if (FAILED(mSystem->mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&mResultAllocation.ptr))))
				return false;
			mResultAllocation.gpuAddress = mResultAllocation.ptr->GetGPUVirtualAddress();
			mResultAllocation.size = (uint32)info.ResultDataMaxSizeInBytes;
		}

		{
			D3D12_RESOURCE_DESC bufferDesc = FD3D12Init::BufferDesc(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			D3D12_HEAP_PROPERTIES heapProps = FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT);
			if (FAILED(mSystem->mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&mScartchAllocation.ptr))))
				return false;
			mScartchAllocation.gpuAddress = mScartchAllocation.ptr->GetGPUVirtualAddress();
			mScartchAllocation.size = (uint32)info.ScratchDataSizeInBytes;
		}

		{
			D3D12_RESOURCE_DESC bufferDesc = FD3D12Init::BufferDesc(numInstances * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_RESOURCE_FLAG_NONE);
			D3D12_HEAP_PROPERTIES heapProps = FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD);
			if (FAILED(mSystem->mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mInstanceAllocaton.ptr))))
				return false;
			mInstanceAllocaton.gpuAddress = mInstanceAllocaton.ptr->GetGPUVirtualAddress();
			mInstanceAllocaton.size = numInstances * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
		}

		mInputDesc.InstanceDescs = mInstanceAllocaton.gpuAddress;
		
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.RaytracingAccelerationStructure.Location = mResultAllocation.gpuAddress;
		
		mSRV = D3D12DescriptorHeapPool::Alloc(nullptr, &srvDesc);
		
		return true;
	}

	bool D3D12RayTracingPipelineState::initialize(D3D12System* system, RayTracingPipelineStateInitializer const& initializer)
	{
		mSystem = system;
		D3D12StateObjectDescBuilder builder;
		builder.reserve(128, 64 * 1024);

		auto AddLibrary = [&](RHIShader* shader, wchar_t const* exportName)
		{
			if (!shader) return;
			D3D12Shader* d3d12Shader = static_cast<D3D12Shader*>(shader);
			
			auto& libDesc = builder.addDataT<D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY>();
			libDesc.DXILLibrary = d3d12Shader->getByteCode();
			libDesc.NumExports = 1;

			size_t exportOffset = builder.allocRaw(sizeof(D3D12_EXPORT_DESC));
			D3D12_EXPORT_DESC* pExport = builder.getPtr<D3D12_EXPORT_DESC>(exportOffset);
			libDesc.pExports = pExport;

			std::wstring entryName = FCString::CharToWChar(d3d12Shader->entryPoint.c_str());
			builder.addString(entryName.c_str(), &pExport->ExportToRename);
			builder.addString(exportName, &pExport->Name);
			
			// We can still use pExport safely here as long as we don't call anything 
			// that reallocates the buffer. The smart addString handles the safety.
			pExport->Flags = D3D12_EXPORT_FLAG_NONE;
		};

		// 1. Ray Generation
		AddLibrary(initializer.rayGenShader, L"RayGen");

		// 2. Miss Shaders
		if (initializer.missShader) 
		{
			AddLibrary(initializer.missShader, L"RayMiss");
		}
		for (int i = 0; i < initializer.missShaders.size(); ++i)
		{
			std::wstring name = L"Miss_" + std::to_wstring(i);
			AddLibrary(initializer.missShaders[i].generalShader, name.c_str());
		}

		// 3. Hit Groups
		if (initializer.hitGroupShader)
		{
			AddLibrary(initializer.hitGroupShader, L"CH_Default");
			auto& hgDesc = builder.addDataT<D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP>();
			builder.addString(L"HitGroup", &hgDesc.HitGroupExport);
			builder.addString(L"CH_Default", &hgDesc.ClosestHitShaderImport);
			hgDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		}

		for (int i = 0; i < initializer.hitGroups.size(); ++i)
		{
			auto const& group = initializer.hitGroups[i];
			std::wstring hgName = L"HitGroup_" + std::to_wstring(i);
			std::wstring chName = L"CH_" + std::to_wstring(i);

			AddLibrary(group.closestHitShader, chName.c_str());

			auto& hgDesc = builder.addDataT<D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP>();
			builder.addString(hgName.c_str(), &hgDesc.HitGroupExport);
			builder.addString(chName.c_str(), &hgDesc.ClosestHitShaderImport);
			
			if (group.intersectionShader)
			{
				std::wstring anyName = L"ANY_" + std::to_wstring(i);
				AddLibrary(group.intersectionShader, anyName.c_str());
				builder.addString(anyName.c_str(), &hgDesc.IntersectionShaderImport);
				hgDesc.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
			}
			else
			{
				hgDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
			}

			auto shaderImpl = static_cast<D3D12Shader*>(group.closestHitShader);
			if (!shaderImpl->rootSignature.localParameters.empty())
			{
				D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
				desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
				desc.Desc_1_1.NumParameters = (UINT)shaderImpl->rootSignature.localParameters.size();
				desc.Desc_1_1.pParameters = shaderImpl->rootSignature.localParameters.data();
				desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

				TComPtr<ID3DBlob> signature;
				TComPtr<ID3DBlob> error;
				if (SUCCEEDED(D3D12SerializeVersionedRootSignature(&desc, &signature, &error)))
				{
					ID3D12RootSignature* pLocalRootSig = nullptr;
					system->mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pLocalRootSig));
					if (pLocalRootSig)
					{
						mLocalRootSignatures.push_back(TComPtr<ID3D12RootSignature>(pLocalRootSig));
						auto& localRootSubobject = builder.addDataT<D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE>();
						localRootSubobject.pLocalRootSignature = pLocalRootSig;

						auto& association = builder.addDataT<D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
						association.pSubobjectToAssociate = &builder.mSubobjects[builder.mSubobjects.size() - 2];
						association.NumExports = 1;

						size_t exportOffset = builder.allocRaw(sizeof(LPCWSTR));
						association.pExports = builder.getPtr<LPCWSTR>(exportOffset);
						builder.addString(hgName.c_str(), &association.pExports[0]);
					}
				}
			}
		}

		// 4. Configs
		{
			auto& shaderConfig = builder.addDataT<D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG>();
			shaderConfig.MaxPayloadSizeInBytes = initializer.maxPayloadSize;
			shaderConfig.MaxAttributeSizeInBytes = initializer.maxAttributeSize;
		}
		{
			auto& pipelineConfig = builder.addDataT<D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG>();
			pipelineConfig.MaxTraceRecursionDepth = initializer.maxRecursionDepth;
		}

		// 5. Global Root Signature
		 mBoundState = system->getRayTracingBoundState(initializer);

		if (mBoundState && mBoundState->mRootSignature)
		{
			auto& globalRootSig = builder.addDataT<D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE>();
			globalRootSig.pGlobalRootSignature = mBoundState->mRootSignature;
		}

		// Calculate Global Parameter Count for SBT Offset
		mGlobalParameterCount = 0;
		if (mBoundState)
		{
			for (auto const& shaderInfo : mBoundState->mShaders)
			{
				if (shaderInfo.data)
				{
					uint32 numParams = (uint32)shaderInfo.data->rootSignature.parameters.size();
					mGlobalParameterCount = std::max(mGlobalParameterCount, shaderInfo.rootSlotStart + numParams);
				}
			}
		}

		D3D12_STATE_OBJECT_DESC stateObjectDesc = builder.getDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
		HRESULT hr = system->mDevice->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&mStateObject));
		if (FAILED(hr)) return false;

		// 6. Query Shader Identifiers
		TComPtr<ID3D12StateObjectProperties> stateObjectProps;
		mStateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProps));

		if (initializer.rayGenShader)
		{
			mRayGen.shader = initializer.rayGenShader;
			mRayGen.shaderData = static_cast<D3D12Shader*>(initializer.rayGenShader);
			mRayGen.localDataSize = mRayGen.shaderData->rootSignature.localDataSize;
			void* id = stateObjectProps->GetShaderIdentifier(L"RayGen");
			if (id) FMemory::Copy(mRayGen.identifier, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			else FMemory::Zero(mRayGen.identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		}

		if (initializer.missShader) 
		{
			ShaderGroup group;
			group.shader = initializer.missShader;
			group.shaderData = static_cast<D3D12Shader*>(initializer.missShader);
			group.localDataSize = group.shaderData->rootSignature.localDataSize;
			void* id = stateObjectProps->GetShaderIdentifier(L"RayMiss");
			if (id) FMemory::Copy(group.identifier, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			else FMemory::Zero(group.identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			mMissGroups.push_back(group);
		}
		for (int i = 0; i < initializer.missShaders.size(); ++i)
		{
			ShaderGroup group;
			group.shader = initializer.missShaders[i].generalShader;
			group.shaderData = static_cast<D3D12Shader*>(group.shader);
			group.localDataSize = group.shaderData ? group.shaderData->rootSignature.localDataSize : 0;
			std::wstring name = L"Miss_" + std::to_wstring(i);
			void* id = stateObjectProps->GetShaderIdentifier(name.c_str());
			if (id) FMemory::Copy(group.identifier, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			else FMemory::Zero(group.identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			mMissGroups.push_back(group);
		}

		if (initializer.hitGroupShader && initializer.hitGroups.empty())
		{
			ShaderGroup group;
			group.shader = initializer.hitGroupShader;
			group.shaderData = static_cast<D3D12Shader*>(initializer.hitGroupShader);
			group.localDataSize = group.shaderData->rootSignature.localDataSize;
			void* id = stateObjectProps->GetShaderIdentifier(L"HitGroup");
			if (id) FMemory::Copy(group.identifier, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			else FMemory::Zero(group.identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			mHitGroups.push_back(group);
		}

		for (int i = 0; i < (int)initializer.hitGroups.size(); ++i)
		{
			ShaderGroup group;
			group.shader = initializer.hitGroups[i].closestHitShader;
			group.shaderData = static_cast<D3D12Shader*>(group.shader);
			group.localDataSize = initializer.hitGroups[i].localDataSize;
			if (group.localDataSize == 0 && group.shaderData)
				group.localDataSize = group.shaderData->rootSignature.localDataSize;

			std::wstring name = L"HitGroup_" + std::to_wstring(i);
			void* id = stateObjectProps->GetShaderIdentifier(name.c_str());
			if (id) FMemory::Copy(group.identifier, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			else FMemory::Zero(group.identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			mHitGroups.push_back(group);
		}

		return true;
	}
	RHIRayTracingPipelineState* D3D12System::RHICreateRayTracingPipelineState(RayTracingPipelineStateInitializer const& initializer)
	{
		auto* state = new D3D12RayTracingPipelineState();
		if (!state->initialize(this, initializer))
		{
			delete state;
			return nullptr;
		}
		return state;
	}

	RHIBottomLevelAccelerationStructure* D3D12System::RHICreateBottomLevelAccelerationStructure(RayTracingGeometryDesc const* geometries, int numGeometries, EAccelerationStructureBuildFlags flags)
	{
		auto* blas = new D3D12BottomLevelAccelerationStructure(this);
		if (!blas->create(geometries, numGeometries, flags))
		{
			delete blas;
			return nullptr;
		}
		return blas;
	}

	RHITopLevelAccelerationStructure* D3D12System::RHICreateTopLevelAccelerationStructure(uint32 numInstances, EAccelerationStructureBuildFlags flags)
	{
		auto* tlas = new D3D12TopLevelAccelerationStructure(this);
		if (!tlas->create(numInstances, flags))
		{
			delete tlas;
			return nullptr;
		}
		return tlas;
	}

	RHIRayTracingShaderTable* D3D12System::RHICreateRayTracingShaderTable(RHIRayTracingPipelineState* pipelineState)
	{
		return new D3D12RayTracingShaderTable(static_cast<D3D12RayTracingPipelineState*>(pipelineState));
	}


	D3D12RayTracingShaderTable::D3D12RayTracingShaderTable(D3D12RayTracingPipelineState* pso)
		:mPSO(pso)
		,mSystem(pso->mSystem)
	{
		mResourceType = EResourceType::RayTracingShaderTable;

		// Initialize RayGen
		mRayGenTable.stride = Math::AlignUp<uint32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + pso->mRayGen.localDataSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		mRayGenTable.resize(mSystem, 1, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		mRayGenTable.numRecords = 1;
		FMemory::Copy(mRayGenTable.pMapped, pso->mRayGen.identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		mRayGenRecordShaders.push_back(pso->mRayGen.shader);

		initMissTable();
		initHitTable();
	}

	D3D12RayTracingShaderTable::~D3D12RayTracingShaderTable()
	{
		mRayGenTable.release();
		mMissTable.release();
		mHitTable.release();
	}

	void D3D12RayTracingShaderTable::initMissTable()
	{
		uint32 maxLocalSize = 0;
		for (auto const& group : mPSO->mMissGroups) maxLocalSize = std::max(maxLocalSize, group.localDataSize);
		// Align stride to 64 to ensure total size (Stride * Num) is always a multiple of 64
		mMissTable.stride = Math::AlignUp<uint32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + maxLocalSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		mMissTable.resize(mSystem, (uint32)mPSO->mMissGroups.size(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		mMissTable.numRecords = (uint32)mPSO->mMissGroups.size();
		
		mMissRecordShaders.resize(mMissTable.numRecords);
		mMissRecordIndices.resize(mMissTable.numRecords);

		for (int i = 0; i < (int)mPSO->mMissGroups.size(); ++i)
		{
			FMemory::Copy((uint8*)mMissTable.pMapped + i * mMissTable.stride, mPSO->mMissGroups[i].identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			mMissRecordShaders[i] = mPSO->mMissGroups[i].shader;
			mMissRecordIndices[i] = i;
		}
	}

	void D3D12RayTracingShaderTable::initHitTable()
	{
		uint32 maxLocalSize = 0;
		for (auto const& group : mPSO->mHitGroups) maxLocalSize = std::max(maxLocalSize, group.localDataSize);
		// Align stride to 64 to ensure total size (Stride * Num) is always a multiple of 64
		mHitTable.stride = Math::AlignUp<uint32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + maxLocalSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		mHitTable.resize(mSystem, (uint32)mPSO->mHitGroups.size(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		mHitTable.numRecords = (uint32)mPSO->mHitGroups.size();
		
		mHitRecordShaders.resize(mHitTable.numRecords);
		mHitRecordIndices.resize(mHitTable.numRecords);

		for (int i = 0; i < (int)mPSO->mHitGroups.size(); ++i)
		{
			FMemory::Copy((uint8*)mHitTable.pMapped + i * mHitTable.stride, mPSO->mHitGroups[i].identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			mHitRecordShaders[i] = mPSO->mHitGroups[i].shader;
			mHitRecordIndices[i] = i;
		}
	}



	void D3D12RayTracingShaderTable::setMissRecord(uint32 recordIndex, void* data, uint32 size, uint32 offset, int shaderIndex)
	{
		if (recordIndex >= mMissTable.numRecords)
		{
			if (recordIndex >= mMissTable.capacity)
			{
				mMissTable.resize(mSystem, recordIndex + 1, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
			}
			mMissTable.numRecords = recordIndex + 1;
		}

		uint32 psoHgCount = (uint32)mPSO->mMissGroups.size();
		int groupIndex = (shaderIndex == -1) ? (recordIndex % psoHgCount) : (shaderIndex % psoHgCount);

		if (mMissRecordIndices.size() <= recordIndex) mMissRecordIndices.resize(std::max((uint32)mMissRecordIndices.size() * 2, recordIndex + 1), -1);
		mMissRecordIndices[recordIndex] = groupIndex;

		if (mMissRecordShaders.size() <= recordIndex) mMissRecordShaders.resize(std::max((uint32)mMissRecordShaders.size() * 2, recordIndex + 1), nullptr);
		mMissRecordShaders[recordIndex] = mPSO->mMissGroups[groupIndex].shader;

		FMemory::Copy((uint8*)mMissTable.pMapped + recordIndex * mMissTable.stride, mPSO->mMissGroups[groupIndex].identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		if (data && size > 0)
		{
			updateRecordData(mMissTable, recordIndex, data, size, offset);
		}
	}

	void D3D12RayTracingShaderTable::setHitRecord(uint32 recordIndex, void* data, uint32 size, uint32 offset, int shaderIndex)
	{
		if (recordIndex >= mHitTable.numRecords)
		{
			if (recordIndex >= mHitTable.capacity)
			{
				mHitTable.resize(mSystem, recordIndex + 1, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
			}

			// Fill identifiers for all new records up to 'recordIndex'
			uint32 psoHgCount = (uint32)mPSO->mHitGroups.size();

			if (mHitRecordIndices.size() <= recordIndex) mHitRecordIndices.resize(std::max((uint32)mHitRecordIndices.size() * 2, recordIndex + 1), -1);
			if (mHitRecordShaders.size() <= recordIndex) mHitRecordShaders.resize(std::max((uint32)mHitRecordShaders.size() * 2, recordIndex + 1), nullptr);

			for (uint32 i = mHitTable.numRecords; i <= recordIndex; ++i)
			{
				int groupIndex = (shaderIndex == -1) ? (i % psoHgCount) : (shaderIndex % psoHgCount);
				mHitRecordIndices[i] = groupIndex;
				mHitRecordShaders[i] = mPSO->mHitGroups[groupIndex].shader;
				FMemory::Copy((uint8*)mHitTable.pMapped + i * mHitTable.stride, mPSO->mHitGroups[groupIndex].identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			}
			mHitTable.numRecords = recordIndex + 1;
		}
		else
		{
			uint32 psoHgCount = (uint32)mPSO->mHitGroups.size();
			int groupIndex = (shaderIndex == -1) ? (recordIndex % psoHgCount) : (shaderIndex % psoHgCount);
			
			if (mHitRecordIndices.size() <= recordIndex) mHitRecordIndices.resize(std::max((uint32)mHitRecordIndices.size() * 2, recordIndex + 1), -1);
			mHitRecordIndices[recordIndex] = groupIndex;

			if (mHitRecordShaders.size() <= recordIndex) mHitRecordShaders.resize(std::max((uint32)mHitRecordShaders.size() * 2, recordIndex + 1), nullptr);
			mHitRecordShaders[recordIndex] = mPSO->mHitGroups[groupIndex].shader;

			FMemory::Copy((uint8*)mHitTable.pMapped + recordIndex * mHitTable.stride, mPSO->mHitGroups[groupIndex].identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		}

		if (data && size > 0)
		{
			updateRecordData(mHitTable, recordIndex, data, size, offset);
		}
	}



	void D3D12RayTracingShaderTable::updateRecordData(TableData& table, uint32 recordIndex, void* data, uint32 size, uint32 offset)
	{
		CHECK(recordIndex < table.numRecords);
		FMemory::Copy((uint8*)table.pMapped + recordIndex * table.stride + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + offset, data, size);
	}

	void D3D12RayTracingShaderTable::updateShaderRecord(EShader::Type shaderType, D3D12ShaderData& shaderData, uint32 recordIndex, void* data, uint32 size, uint32 offset)
	{
		switch (shaderType)
		{
		case EShader::RayGen:
			if (mPSO->mRayGen.shaderData == &shaderData)
			{
				updateRecordData(mRayGenTable, 0, data, size, offset);
			}
			break;
		case EShader::RayMiss:
			for (uint32 i = 0; i < mMissTable.numRecords; ++i)
			{
				if (mMissRecordShaders[i] && (D3D12ShaderData*)static_cast<D3D12Shader*>(mMissRecordShaders[i]) == &shaderData)
				{
					updateRecordData(mMissTable, i, data, size, offset);
				}
			}
			break;
		case EShader::RayHit:
		case EShader::RayClosestHit:
		case EShader::RayAnyHit:
		case EShader::RayIntersection:
			for (uint32 j = 0; j < mHitTable.numRecords; ++j)
			{
				if (mHitRecordShaders[j] && (D3D12ShaderData*)static_cast<D3D12Shader*>(mHitRecordShaders[j]) == &shaderData)
				{
					updateRecordData(mHitTable, j, data, size, offset);
				}
			}
			break;
		}
	}

	void D3D12RayTracingShaderTable::upload()
	{
		// No-op for persistent mapped buffers
	}

	void D3D12RayTracingShaderTable::setShader(uint32 recordIndex, RHIShader* shader, int optionalGroupIndex)
	{
		int groupIndex = -1;

		// 1. If explicit group index is provided, use it directly (Explicit Override)
		if (optionalGroupIndex != -1)
		{
			groupIndex = optionalGroupIndex;
		}
		else
		{
			// 2. Try to find group index by pointer matching
			for(int i = 0; i < (int)mPSO->mHitGroups.size(); ++i)
			{
				if(mPSO->mHitGroups[i].shaderData == (D3D12ShaderData*)static_cast<D3D12Shader*>(shader)) 
				{
					groupIndex = i;
					break;
				}
			}
		}

		if (groupIndex != -1)
		{
			if (recordIndex >= mHitTable.numRecords)
			{
				if (recordIndex >= mHitTable.capacity)
				{
					mHitTable.resize(mSystem, recordIndex + 1, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
				}

				uint32 psoHgCount = (uint32)mPSO->mHitGroups.size();
				if (mHitRecordIndices.size() <= recordIndex) mHitRecordIndices.resize(std::max((uint32)mHitRecordIndices.size() * 2, recordIndex + 1), -1);
				if (mHitRecordShaders.size() <= recordIndex) mHitRecordShaders.resize(std::max((uint32)mHitRecordShaders.size() * 2, recordIndex + 1), nullptr);

				for (uint32 i = mHitTable.numRecords; i <= recordIndex; ++i)
				{
					int idx = (i == recordIndex) ? groupIndex : (int)(i % psoHgCount);
					mHitRecordIndices[i] = idx;
					mHitRecordShaders[i] = (i == recordIndex) ? shader : mPSO->mHitGroups[idx].shader;
					FMemory::Copy((uint8*)mHitTable.pMapped + i * mHitTable.stride, mPSO->mHitGroups[idx].identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				}
				mHitTable.numRecords = recordIndex + 1;
			}
			else
			{
				mHitRecordIndices[recordIndex] = groupIndex;
				mHitRecordShaders[recordIndex] = shader;
				FMemory::Copy((uint8*)mHitTable.pMapped + recordIndex * mHitTable.stride, mPSO->mHitGroups[groupIndex].identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			}
		}
		else
		{
			LogWarning(0, "SBT setShader: Record %u FAILED to match shader %p", recordIndex, shader);
		}
	}

	void D3D12RayTracingShaderTable::setValue(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, void* data)
	{
		// 1. Resolve Table & Shader Context
		TableData* table = nullptr;
		TArray<RHIShader*>* shaderState = nullptr;

		switch (type)
		{
		case EShader::RayGen:
			table = &mRayGenTable;
			shaderState = &mRayGenRecordShaders;
			break;
		case EShader::RayMiss:
			table = &mMissTable;
			shaderState = &mMissRecordShaders;
			break;
		case EShader::RayHit:
			table = &mHitTable;
			shaderState = &mHitRecordShaders;
			break;
		default: return;
		}

		// 2. Resolve Offset using pre-calculated dataOffset from Reflection
		RHIShader* boundShader = (shaderState && recordIndex < shaderState->size()) ? (*shaderState)[recordIndex] : nullptr;

		uint32 sbtOffset = 0;
		uint32 dataSize = 0;

		if (boundShader)
		{
			D3D12Shader* d3dShader = static_cast<D3D12Shader*>(boundShader);
			bool bFound = false;

			if (parameter.bindIndex < d3dShader->rootSignature.slots.size())
			{
				auto const& slot = d3dShader->rootSignature.slots[parameter.bindIndex];
				if (slot.bLocal)
				{
					sbtOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + slot.dataOffset;
					dataSize = slot.dataSize;
					bFound = true;
				}
			}

			if (!bFound)
			{
				sbtOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + parameter.bindIndex * 8;
				dataSize = 8;
			}
		}
		else
		{
			sbtOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + parameter.bindIndex * 8;
			dataSize = 8;
		}

		if (recordIndex < table->numRecords && dataSize > 0)
		{
			uint32 localDataOffset = sbtOffset - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			updateRecordData(*table, recordIndex, data, dataSize, localDataOffset);
		}
	}

	void D3D12RayTracingShaderTable::setHandleValue(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, void* data, uint32 size)
	{
		TableData* table = nullptr;
		TArray<RHIShader*>* shaderState = nullptr;
		
		switch (type)
		{
		case EShader::RayGen: table = &mRayGenTable; shaderState = &mRayGenRecordShaders; break;
		case EShader::RayMiss: table = &mMissTable; shaderState = &mMissRecordShaders; break;
		case EShader::RayHit: table = &mHitTable; shaderState = &mHitRecordShaders; break;
		default: return;
		}

		RHIShader* boundShader = (shaderState && recordIndex < shaderState->size()) ? (*shaderState)[recordIndex] : nullptr;
		
		uint32 sbtOffset = 0;
		if (boundShader)
		{
			D3D12Shader* d3dShader = static_cast<D3D12Shader*>(boundShader);
			bool bFound = false;

			if (parameter.bindIndex < d3dShader->rootSignature.slots.size())
			{
				auto const& slot = d3dShader->rootSignature.slots[parameter.bindIndex];
				if (slot.bLocal)
				{
					sbtOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + slot.dataOffset;
					bFound = true;
				}
			}

			if (!bFound)
			{
				sbtOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + parameter.bindIndex * 8; 
			}
		}
		else
		{
			sbtOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + parameter.bindIndex * 8; 
		}
		
		if (recordIndex < table->numRecords)
		{
			uint32 localDataOffset = sbtOffset - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			updateRecordData(*table, recordIndex, data, size, localDataOffset);
		}
	}

	void D3D12RayTracingShaderTable::setTexture(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHITextureBase* texture)
	{
		if (!texture) return;
		auto* d3dTex = static_cast<D3D12Texture2D*>(texture);
		
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = d3dTex->mSRV.getGPUHandle();

		// Write the 64-bit handle to the SBT
		setHandleValue(recordIndex, type, parameter, &srvHandle, sizeof(srvHandle));
	}

	void D3D12RayTracingShaderTable::setRWTexture(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHITextureBase* texture, EAccessOp accessOp)
	{
		if (!texture) return;
		auto* d3dTex = static_cast<D3D12Texture2D*>(texture);

		D3D12_GPU_DESCRIPTOR_HANDLE uavHandle = d3dTex->mUAV.getGPUHandle();

		// Write the 64-bit handle to the SBT
		setHandleValue(recordIndex, type, parameter, &uavHandle, sizeof(uavHandle));
	}

	void D3D12RayTracingShaderTable::setSampler(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHISamplerState* sampler)
	{
		if (!sampler) return;
		auto* d3dSampler = static_cast<D3D12SamplerState*>(sampler);

		D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = d3dSampler->mHandle.getGPUHandle();

		// Write the 64-bit handle to the SBT
		setHandleValue(recordIndex, type, parameter, &samplerHandle, sizeof(samplerHandle));
	}

	void D3D12RayTracingShaderTable::setStorageBuffer(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHIBuffer* buffer, EAccessOp accessOp)
	{
		if (!buffer) return;

		D3D12Buffer* d3dBuf = static_cast<D3D12Buffer*>(buffer);
		
		// For Storage Buffers (SRV/UAV), we prefer Root Descriptors (GPU Address) if possible, 
		// but should support Tables (Handles) if that's what the Root Signature says.
		// Current setHandleValue logic writes 8 bytes regardless.
		
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;

		auto checkAllocation = [](D3D12Buffer* buf) -> D3D12_GPU_VIRTUAL_ADDRESS
		{
			if (buf->mDynamicAllocation.ptr != nullptr) return buf->mDynamicAllocation.gpuAddress;
			if (buf->getResource() != nullptr) return buf->getResource()->GetGPUVirtualAddress();
			return 0;
		};

		gpuAddr = checkAllocation(d3dBuf);

		if (gpuAddr != 0)
		{
			// Prioritize Root Descriptor (GPU Virtual Address)
			setHandleValue(recordIndex, type, parameter, &gpuAddr, sizeof(gpuAddr));
		}
		else
		{
			// Fallback: Descriptor Table (GPU Descriptor Handle)
			D3D12_GPU_DESCRIPTOR_HANDLE handle = { 0 };
			bool bFoundHandle = false;

			// Assuming EAccessOp has ReadOnly, WriteOnly, ReadAndWrite based on typical naming/errors
			if (accessOp == EAccessOp::WriteOnly || accessOp == EAccessOp::ReadAndWrite)
			{
				if (d3dBuf->mUAV.isValid())
				{
					handle = d3dBuf->mUAV.getGPUHandle();
					bFoundHandle = true;
				}
			}
			else
			{
				if (d3dBuf->mSRV.isValid())
				{
					handle = d3dBuf->mSRV.getGPUHandle();
					bFoundHandle = true;
				}
			}

			if (bFoundHandle)
			{
				setHandleValue(recordIndex, type, parameter, &handle, sizeof(handle));
			}
			else
			{
				// Error: Buffer has no valid Address AND no valid Handle for the requested access
				LogWarning(0, "SBT setStorageBuffer: Buffer %p failed to bind. Address=0, ValidHandle=%d, Access=%d", 
					buffer, bFoundHandle, (int)accessOp);
			}
		}
	}

	void D3D12RayTracingShaderTable::setUniformBuffer(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHIBuffer* buffer)
	{
		// Similar to StorageBuffer, but for Constant Buffers (CBV)
		if (!buffer) return;

		D3D12Buffer* d3dBuf = static_cast<D3D12Buffer*>(buffer);
		
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;
		if (d3dBuf->mDynamicAllocation.ptr != nullptr) gpuAddr = d3dBuf->mDynamicAllocation.gpuAddress;
		else if (d3dBuf->getResource() != nullptr) gpuAddr = d3dBuf->getResource()->GetGPUVirtualAddress();

		if (gpuAddr != 0)
		{
			// For Root CBV
			setHandleValue(recordIndex, type, parameter, &gpuAddr, sizeof(gpuAddr));
		}
		else // Handle fallback?
		{
			// If buffer has a persistent CBV descriptor?
			if (d3dBuf->mCBV.isValid())
			{
				D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle = d3dBuf->mCBV.getGPUHandle();
				setHandleValue(recordIndex, type, parameter, &cbvHandle, sizeof(cbvHandle));
			}
		}
	}

	void D3D12RayTracingShaderTable::TableData::release()
	{
		if (pMapped)
		{
			allocation.ptr->Unmap(0, nullptr);
			pMapped = nullptr;
		}
		if (allocation.ptr)
		{
			allocation.ptr->Release();
			allocation.ptr = nullptr;
		}
		capacity = 0;
	}

	bool D3D12RayTracingShaderTable::TableData::resize(D3D12System* system, uint32 numRecords, uint32 alignment)
	{
		if (numRecords <= capacity && pMapped) return true;

		uint32 newCapacity = std::max(capacity * 2, numRecords);
		uint32 newSize = Math::AlignUp<uint32>(newCapacity * stride, alignment);

		D3D12BufferAllocation newAlloc;
		D3D12_RESOURCE_DESC desc = FD3D12Init::BufferDesc(newSize, D3D12_RESOURCE_FLAG_NONE);
		D3D12_HEAP_PROPERTIES props = FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD);
		if (FAILED(system->mDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&newAlloc.ptr))))
			return false;

		newAlloc.gpuAddress = newAlloc.ptr->GetGPUVirtualAddress();
		newAlloc.size = newSize;

		void* newMapped = nullptr;
		if (FAILED(newAlloc.ptr->Map(0, nullptr, &newMapped)))
		{
			newAlloc.ptr->Release();
			return false;
		}

		if (pMapped)
		{
			FMemory::Copy(newMapped, pMapped, capacity * stride);
			release();
		}
		else
		{
			FMemory::Zero(newMapped, newSize);
		}

		allocation = newAlloc;
		pMapped = newMapped;
		capacity = newCapacity;
		return true;
	}
}
