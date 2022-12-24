#pragma once
#ifndef D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C
#define D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C

#include "D3D12ShaderCommon.h"

#include "RHICommand.h"
#include "RHICommandListImpl.h"
#include "RHIGlobalResource.h"

#include "ShaderCore.h"

#include "D3D12Common.h"
#include "InlineString.h"
#include "Core/ScopeGuard.h"
#include "Core/TypeHash.h"

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif




class IDXGIAdapter1;
class IDXGISwapChain3;
class IDXGIFactory1;

namespace Render
{
	class D3D12System;

	constexpr int ConstBufferMultipleSize = 256;

	constexpr uint32 D3D12_BUFFER_SIZE_ALIGN = 4;

	class D3D12ShaderBoundState
	{
	public:
		struct ShaderInfo
		{
			EShader::Type type;
			D3D12ShaderData* data;
			uint rootSlotStart;
		};
		ShaderBoundStateKey     cachedKey;
		std::vector< ShaderInfo >    mShaders;
		TComPtr<ID3D12RootSignature> mRootSignature;
	};


	struct D3D12PipelineStateKey
	{
		ShaderBoundStateKey boundStateKey;

		union
		{
			struct
			{			
				uint64 inputLayoutID   : 12;
				uint64 rasterizerID    : 12;
				uint64 blendID         : 12;
				uint64 depthStenceilID : 12;
				uint64 renderTargetFormatID : 12;
				uint64 primitiveType : 4;
			};

			uint64 value;
		};

		void initialize(
			GraphicsRenderStateDesc const& renderState,
			D3D12ShaderBoundState* boundState,
			D3D12RenderTargetsState* renderTargetsState);

		void initialize(
			MeshRenderStateDesc const& renderState,
			D3D12ShaderBoundState* boundState,
			D3D12RenderTargetsState* renderTargetsState);

		void initialize(D3D12ShaderBoundState* boundState);

		bool operator == (D3D12PipelineStateKey const& rhs) const
		{
			return value == rhs.value && boundStateKey == rhs.boundStateKey;
		}

		uint32 getTypeHash() const
		{
			uint32 result = boundStateKey.getTypeHash();
			result = HashCombine(result, value);
			return result;
		}
	};

	class D3D12DynamicBufferPool
	{

		struct PageInfo
		{
			uint32 size;
			uint32 usageSize;
			void*  cpuAddress;
			TComPtr< ID3D12Resource > resource;
		};

		void* alloc(uint32 size, D3D12_GPU_VIRTUAL_ADDRESS& outAddress )
		{
			int usageIndex = INDEX_NONE;
			for (uint32 index : mFreePageIndices)
			{
				auto& page = mAllocedPages[index];

				if (page.usageSize + size < size)
				{
					usageIndex = index;
					break;
				}
			}

			if (usageIndex == INDEX_NONE)
			{



			}

		}

		void freeAll()
		{

			for (auto& page : mAllocedPages)
			{
				page.usageSize = 0;
			}

		}

		bool allocNewBuffer(ID3D12DeviceRHI* device, uint32 size);

		ID3D12DeviceRHI* mDevice;
		std::vector< PageInfo > mAllocedPages;
		uint32 mPageSize;
		std::vector<uint32> mFreePageIndices;

	};

	class D3D12DynamicBuffer
	{
	public:
		bool initialize(ID3D12DeviceRHI* device, uint32 bufferSizes[] , int numBuffers );
		bool allocNewBuffer(uint32 size);

		void* lock(ID3D12GraphicsCommandListRHI* commandList, uint32 size);
		ID3D12Resource*  unlock(ID3D12GraphicsCommandListRHI* commandList, D3D12_VERTEX_BUFFER_VIEW& bufferView);
		ID3D12Resource*  unlock(ID3D12GraphicsCommandListRHI* commandList, D3D12_INDEX_BUFFER_VIEW& bufferView);

		void beginFrame()
		{
			for (auto& info : mAllocedBuffers)
			{
				info.indexNext = 0;
			}
		}
		struct BufferInfo
		{
			uint32 size;
			std::vector< ID3D12Resource* > resources;
			int indexNext = 0;

			
			ID3D12Resource* fetchResource(ID3D12DeviceRHI* device)
			{
				if (indexNext == resources.size())
				{
					return allocResource(device);
				}
				return resources[indexNext];
			}
			ID3D12Resource* allocResource(ID3D12DeviceRHI* device);
		};

		ID3D12DeviceRHI* mDevice;
		std::vector< BufferInfo > mAllocedBuffers;
		D3D12_GPU_VIRTUAL_ADDRESS mLockedGPUAddress;
		void*  mLockPtr;
		uint32 mLockedSize;

		int32  mLockedIndex = INDEX_NONE;
		ID3D12Resource* mLockedResource = nullptr;
		
	};


	struct D3D12ResourceBoundState
	{
	public:
		bool initialize( ID3D12DeviceRHI* device );

		void updateConstantData( void const* pData , uint32 offset , uint32 size )
		{
			memcpy(mConstDataPtr + mConstDataOffset + offset, pData, size);
			mCurrentUpdatedSize = Math::Max(mCurrentUpdatedSize, offset + size);
		}

		void restFrame()
		{
			mConstDataOffset = 0;
			mCurrentUpdatedSize = 0;
		}

		void resetDrawCall()
		{
			if ( mCurrentUpdatedSize )
			{
				mConstDataOffset += ConstBufferMultipleSize * ((mCurrentUpdatedSize + ConstBufferMultipleSize - 1) / ConstBufferMultipleSize);
				mCurrentUpdatedSize = 0;
			}
		}

		D3D12_GPU_VIRTUAL_ADDRESS getConstantGPUAddress()
		{
			return mConstBuffer->GetGPUVirtualAddress() + mConstDataOffset;
		}

		struct UAVBoundState
		{
			ID3D12Resource* resource = nullptr;
		};
		UAVBoundState mUAVStates[8];
		

		uint32 mBufferSize;
		uint8* mConstDataPtr;
		uint32 mConstDataOffset = 0;
		uint32 mCurrentUpdatedSize = 0;
		TComPtr< ID3D12Resource > mConstBuffer;
	};

	class D3D12RenderStateCache
	{




	};

	class D3D12Device
	{




	};

	class D3D12Context : public RHIContext
	{
	public:

		bool initialize( D3D12System* system );

		void release();

		void RHISetRasterizerState(RHIRasterizerState& rasterizerState);
		void RHISetBlendState(RHIBlendState& blendState) 
		{
			if (mBlendStateStatePending != &blendState)
			{
				mBlendStateStatePending = &blendState;
			}
		}
		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef) 
		{
			if (mDepthStencilStatePending != &depthStencilState)
			{
				mDepthStencilStatePending = &depthStencilState;
			}
			mGraphicsCmdList->OMSetStencilRef(stencilRef);
		}
		void RHISetViewport(float x, float y, float w, float h, float zNear, float zFar);
		void RHISetViewports(ViewportInfo const viewports[], int numViewports);
		void RHISetScissorRect(int x, int y, int w, int h);

		bool determitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, EPrimitive& outPrimitiveDetermited, D3D12_INDEX_BUFFER_VIEW& outIndexBufferView, int& outIndexNum, ID3D12Resource*& outIndexResource);
		
		void postDrawPrimitive() 
		{
			for (auto & shader : mBoundState->mShaders)
			{
				mResourceBoundStates[shader.type].resetDrawCall();
			}
		}
		void RHIDrawPrimitive(EPrimitive type, int start, int nv)
		{
			commitGraphicsPipelineState(type);
			mGraphicsCmdList->DrawInstanced(nv, 1, start, 0);
			postDrawPrimitive();
		}

		void RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex)
		{
			commitGraphicsPipelineState(type);
			mGraphicsCmdList->DrawIndexedInstanced(nIndex, 1, indexStart, baseVertex, 0);
			postDrawPrimitive();
		}

		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{
			commitGraphicsPipelineState(type);
			postDrawPrimitive();
		}
		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{
			commitGraphicsPipelineState(type);
			postDrawPrimitive();
		}
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
		{
			commitGraphicsPipelineState(type);
			mGraphicsCmdList->DrawInstanced(nv, numInstance,vStart, baseInstance);
			postDrawPrimitive();
		}

		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance)
		{
			commitGraphicsPipelineState(type);
			mGraphicsCmdList->DrawIndexedInstanced(nIndex, numInstance, indexStart, baseVertex, baseInstance);
			postDrawPrimitive();
		}

	
		void RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData);

		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex);


		void RHIDrawMeshTasks(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
		{
			commitMeshPipelineState();
			mGraphicsCmdList->DispatchMesh(numGroupX, numGroupY, numGroupZ);
			postDrawPrimitive();
		}

		void RHIDrawMeshTasksIndirect(RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{

		}

		void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler) 
		{
			mFixedShaderParams.transform = transform;
			mFixedShaderParams.color = color;
			mFixedShaderParams.texture = texture;
			mFixedShaderParams.sampler = sampler;
			mBoundState = nullptr;
		}

		SimplePipelineParamValues mFixedShaderParams;

		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer);

		void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil);


		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);

		void RHISetIndexBuffer(RHIBuffer* indexBuffer);

		void RHIFlushCommand();


		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);

		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);

	

		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector2 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim) { setShaderValueT(shaderProgram, param, val, dim); }
		void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);

		template< class T >
		void setShaderValueT(RHIShaderProgram& shaderProgram, ShaderParameter const& param, T const val[], int dim)
		{
			auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
			shaderProgramImpl.setupShader(param, [this, &val, dim](EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& shaderParam)
			{
				setShaderValueT(shaderType, shaderData, shaderParam, val, dim);
			});
		}

		void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView) {}

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);
		void clearShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param) {}
		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler);



		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op) {}
		void clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param) {}

		void clearShaderRWTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param);
		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer);

		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOperator op) {}
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer){}

		void setShaderTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderSampler(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderUniformBuffer(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHIBuffer& buffer);


		void RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc);
		void RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc);
		void RHISetComputeShader(RHIShader* shader);


		void setShaderBoundState(D3D12ShaderBoundState* newState);

		void setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim) { setShaderValueT(shader, param, val, dim, 3 * sizeof(float)); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector2 const val[], int dim) { setShaderValueT(shader, param, val, dim, 2 * sizeof(float)); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim) { setShaderValueT(shader, param, val, dim, 3 * sizeof(float)); }
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderMatrix22(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim, 2 * sizeof(float)); }
		void setShaderMatrix43(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim, 3 * sizeof(float)); }
		void setShaderMatrix34(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim); }


		template< class T >
		void setShaderValueT(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, T const val[], int dim)
		{
			setShaderValueInternal(shaderType, shaderData, param, (uint8 const*)val, dim * sizeof(T));
		}

		template< class T >
		void setShaderValueT(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, T const val[], int dim, uint32 elementSize, uint32 stride = 4 * sizeof(float))
		{
			setShaderValueInternal(shaderType, shaderData, param, (uint8 const*)val, dim * sizeof(T), elementSize, stride);
		}

		template< class T >
		void setShaderValueT(RHIShader& shader, ShaderParameter const& param, T const val[], int dim) 
		{
			D3D12Shader& shaderImpl = static_cast<D3D12Shader&>(shader);
			setShaderValueInternal(shaderImpl.getType(), shaderImpl, param, (uint8 const*)val, dim * sizeof(T));
		}
		template< class T >
		void setShaderValueT(RHIShader& shader, ShaderParameter const& param, T const val[], int dim, uint32 elementSize, uint32 stride = 4 * sizeof(float))
		{
			D3D12Shader& shaderImpl = static_cast<D3D12Shader&>(shader);
			setShaderValueInternal(shaderImpl.getType(), shaderImpl, param, (uint8 const*)val, dim * sizeof(T), elementSize , stride);
		}

		void setShaderValueInternal(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, uint8 const* pData, uint32 size, uint32 elementSize, uint32 stride = 4 * sizeof(float));
		void setShaderValueInternal(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, uint8 const* pData, uint32 size);

		void setShaderResourceView(RHIShader& shader, ShaderParameter const& param, RHIShaderResourceView const& resourceView) {}

		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler) 
		{
			setShaderTexture(shader, param, texture);
			setShaderSampler(shader, paramSampler, sampler);
		}
		void clearShaderTexture(RHIShader& shader, ShaderParameter const& param)
		{

		}
		void setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);
		void setShaderRWTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);
		void clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param);
		void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer);

		void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOperator op)
		{

		}
		void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
		{

		}

		void clearSRVResource(RHIResource& resource)
		{

		}

		void setShaderSamplerInternal(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler)
		{

		}


		void commitGraphicsPipelineState(EPrimitive type);
		void commitMeshPipelineState();
		void commitComputePipelineState();


		D3D12RenderTargetsState* mRenderTargetsState;
		D3D12ShaderBoundState*  mBoundState = nullptr;
		D3D12ShaderBoundState*  mComputeBoundState = nullptr;
		RHIInputLayoutRef       mInputLayoutPending;
		RHIRasterizerStateRef   mRasterizerStatePending;
		RHIDepthStencilStateRef mDepthStencilStatePending;
		RHIBlendStateRef        mBlendStateStatePending;


		D3D12_RECT mScissorRects[8];
		D3D12_RECT mViewportRects[8];
		uint32 mNumViewports = 1;

		void updateCSUHeapUsage(D3D12PooledHeapHandle const& handle)
		{
			if (handle.chunk == nullptr)
			{
				return;
			}
			if (mUsedDescHeaps[0] != handle.chunk->resource)
			{
				if (mUsedDescHeaps[0] == nullptr)
					++mNumUsedHeaps;
				mUsedDescHeaps[0] = handle.chunk->resource;
				mGraphicsCmdList->SetDescriptorHeaps(mNumUsedHeaps, mUsedDescHeaps);
			}
		}

		void updateSamplerHeapUsage(D3D12PooledHeapHandle const& handle)
		{
			if (handle.chunk == nullptr)
			{
				return;
			}
			if (mUsedDescHeaps[1] != handle.chunk->resource)
			{
				if (mUsedDescHeaps[1] == nullptr)
					++mNumUsedHeaps;

				mUsedDescHeaps[1] = handle.chunk->resource;
				if (mNumUsedHeaps == 1)
					mUsedDescHeaps[0] = mUsedDescHeaps[1];

				mGraphicsCmdList->SetDescriptorHeaps(mNumUsedHeaps, mUsedDescHeaps);
			}
		}
		ID3D12DescriptorHeap* mUsedDescHeaps[2];
		int mNumUsedHeaps = 0;


		uint32 getRootSlot(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param)
		{
			auto const& slotInfo = shaderData.rootSignature.slots[param.bindIndex];
			return mRootSlotStart[shaderType] + slotInfo.slotOffset;
		}
		uint32 mRootSlotStart[EShader::Count];
		D3D12ResourceBoundState mResourceBoundStates[EShader::Count];

		D3D12DynamicBuffer mVertexBufferUP;
		D3D12DynamicBuffer mIndexBufferUP;


		TComPtr< ID3D12DeviceRHI > mDevice;
		TComPtr< ID3D12CommandQueue >  mCommandQueue;
		TComPtr< ID3D12CommandAllocator >  mCommandAllocator;
		
		ID3D12GraphicsCommandListRHI* mGraphicsCmdList;

		TComPtr< ID3D12Fence1 > mFence;
		HANDLE mFenceEvent;
		struct FrameData
		{
			TComPtr< ID3D12CommandAllocator >       cmdAllocator;
			TComPtr< ID3D12GraphicsCommandListRHI > graphicsCmdList;

			bool init(ID3D12DeviceRHI* device)
			{
				VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
				VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&graphicsCmdList)));
				fenceValue = 0;
				return true;
			}
			bool reset()
			{
				VERIFY_D3D_RESULT_RETURN_FALSE(cmdAllocator->Reset());
				VERIFY_D3D_RESULT_RETURN_FALSE(graphicsCmdList->Reset(cmdAllocator, nullptr));

				return true;
			}

			uint64 fenceValue;
		};

		int32 mFrameIndex;

		bool configFromSwapChain(D3D12SwapChain* swapChain);
		bool beginFrame();
		void endFrame();
		void waitForGpu();
		void waitForGpu(ID3D12CommandQueue* cmdQueue);
		void moveToNextFrame(IDXGISwapChainRHI* swapChain);

		std::vector< FrameData > mFrameDataList;

	};



	class D3D12System : public RHISystem
	{
	public:
		~D3D12System();
		RHISystemName getName() const { return RHISystemName::D3D12; }
		bool initialize(RHISystemInitParams const& initParam);
		void preShutdown();
		void shutdown();
		virtual ShaderFormat* createShaderFormat();

		bool RHIBeginRender();

		void RHIEndRender(bool bPresent);

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}

		RHISwapChain*    RHICreateSwapChain(SwapChainCreationInfo const& info);


		std::vector < TComPtr< ID3D12Resource > > mCopyResources;
		TComPtr< ID3D12GraphicsCommandListRHI >   mCopyCmdList;
		TComPtr< ID3D12CommandQueue > mCopyCmdQueue;
		TComPtr< ID3D12CommandAllocator > mCopyCmdAllocator;

		uint64 GetRequiredIntermediateSize(ID3D12Resource* pDestinationResource, uint32 firstSubresource, uint32 numSubresources)
		{
			auto Desc = pDestinationResource->GetDesc();
			uint64 RequiredSize = 0;
			mDevice->GetCopyableFootprints(&Desc, firstSubresource, numSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
			return RequiredSize;
		}


		RHITexture1D*    RHICreateTexture1D(
			ETexture::Format format, int length,
			int numMipLevel, uint32 createFlags,
			void* data);

		RHITexture2D*    RHICreateTexture2D(
			ETexture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 createFlags,
			void* data, int dataAlign);

		RHITexture3D*    RHICreateTexture3D(
			ETexture::Format format, int sizeX, int sizeY, int sizeZ,
			int numMipLevel, int numSamples, uint32 createFlags,
			void* data)
		{
			return nullptr;
		}

		RHITextureCube*  RHICreateTextureCube(ETexture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[]) { return nullptr; }

		RHITexture2DArray* RHICreateTexture2DArray(ETexture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
		{
			return nullptr;
		}

		RHITexture2D*     RHICreateTextureDepth(ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags) { return nullptr; }

		bool updateTexture2DSubresources(ID3D12Resource* textureResource, ETexture::Format format, void* data, uint32 ox, uint32 oy, uint32 width , uint32 height, uint32 rowPatch, uint32 level = 0);
		bool updateTexture1DSubresources(ID3D12Resource* textureResource, ETexture::Format format, void* data, uint32 offset, uint32 length, uint32 level = 0);

		RHIBuffer*  RHICreateBuffer(uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data);
		void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIBuffer* buffer);

		void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, std::vector< uint8 >& outData)
		{

		}
		void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, std::vector< uint8 >& outData)
		{

		}


		RHIFrameBuffer*   RHICreateFrameBuffer();

		RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);

		RHISamplerState*      RHICreateSamplerState(SamplerStateInitializer const& initializer);
		RHIRasterizerState*   RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState*        RHICreateBlendState(BlendStateInitializer const& initializer);
		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

		RHIShader* RHICreateShader(EShader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();

		
		bool getHardwareAdapter(
			IDXGIFactory1* pFactory,
			_In_ D3D_FEATURE_LEVEL featureLevel,
			_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false);

		static void MemcpySubresource(
			const D3D12_MEMCPY_DEST* pDest,
			const D3D12_SUBRESOURCE_DATA* pSrc,
			SIZE_T RowSizeInBytes,
			UINT NumRows,
			UINT NumSlices) noexcept
		{
			for (UINT z = 0; z < NumSlices; ++z)
			{
				auto pDestSlice = static_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
				auto pSrcSlice = static_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
				for (UINT y = 0; y < NumRows; ++y)
				{
					memcpy(pDestSlice + pDest->RowPitch * y,
						pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
						RowSizeInBytes);
				}
			}
		}

		ID3D12Resource* createDepthTexture2D(DXGI_FORMAT format, uint64 w , uint64 h, uint32 sampleCount)
		{
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = format;
			textureDesc.Width = w;
			textureDesc.Height = h;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = sampleCount;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			D3D12_CLEAR_VALUE optimizedClearValue = {};
			optimizedClearValue.Format = format;
			optimizedClearValue.DepthStencil = { 1.0f, 0 };

			ID3D12Resource* textureResource;
			VERIFY_D3D_RESULT(mDevice->CreateCommittedResource(
				&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&optimizedClearValue,
				IID_PPV_ARGS(&textureResource)), return nullptr;);

			return textureResource;
		}

		static void MemcpySubresource(
			const D3D12_MEMCPY_DEST* pDest,
			const D3D12_SUBRESOURCE_DATA* pSrc,
			SIZE_T RowSizeInBytes,
			UINT NumRows) noexcept
		{
			auto pDestSlice = static_cast<BYTE*>(pDest->pData);
			auto pSrcSlice = static_cast<const BYTE*>(pSrc->pData);
			for (UINT y = 0; y < NumRows; ++y)
			{
				memcpy(pDestSlice + pDest->RowPitch * y,
					pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
					RowSizeInBytes);
			}
		}

		UINT64 UpdateSubresources(
			ID3D12GraphicsCommandList* pCmdList,
			ID3D12Resource* pDestinationResource,
			ID3D12Resource* pIntermediate,
			UINT FirstSubresource,
			UINT NumSubresources,
			UINT64 RequiredSize,
			const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			const UINT* pNumRows,
			const UINT64* pRowSizesInBytes,
			const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept;

		UINT64 UpdateSubresources(
			ID3D12GraphicsCommandList* pCmdList,
			ID3D12Resource* pDestinationResource,
			ID3D12Resource* pIntermediate,
			UINT64 IntermediateOffset,
			UINT FirstSubresource,
			UINT NumSubresources,
			const D3D12_SUBRESOURCE_DATA* pSrcData);


		void waitCopyCommand()
		{
			if (!mCopyResources.empty())
			{
				mRenderContext.waitForGpu(mCopyCmdQueue);
				mCopyResources.clear();
			}
		}

		std::unordered_map< ShaderBoundStateKey, D3D12ShaderBoundState*, MemberFuncHasher > mShaderBoundStateMap;
		void cleanupShaderBoundState()
		{
			for (auto& pair : mShaderBoundStateMap)
			{
				delete pair.second;
			}
			mShaderBoundStateMap.empty();
		}

		
		D3D12ShaderBoundState* getShaderBoundState(GraphicsShaderStateDesc const& stateDesc);
		D3D12ShaderBoundState* getShaderBoundState(MeshShaderStateDesc const& stateDesc);
		D3D12ShaderBoundState* getShaderBoundState(RHIShader& computeShader);

		D3D12ShaderBoundState* getShaderBoundState(D3D12ShaderProgram& shaderProgram);
		D3D12PipelineState* getPipelineState(
			GraphicsRenderStateDesc const& renderState,
			D3D12ShaderBoundState* boundState, 
			D3D12RenderTargetsState* renderTargetsState);

		D3D12PipelineState* getPipelineState(
			MeshRenderStateDesc const& renderState,
			D3D12ShaderBoundState* boundState,
			D3D12RenderTargetsState* renderTargetsState);

		D3D12PipelineState* getPipelineState(D3D12ShaderBoundState* boundState);
		void cleanupPipelineState()
		{
			mPipelineStateMap.clear();
		}

		std::unordered_map< D3D12PipelineStateKey, TRefCountPtr< D3D12PipelineState >, MemberFuncHasher> mPipelineStateMap;


		D3D12Context   mRenderContext;
		TComPtr<ID3D12DeviceRHI> mDevice;

		class D3D12ProfileCore* mProfileCore = nullptr;
		TRefCountPtr< D3D12SwapChain > mSwapChain;

		RHICommandListImpl* mImmediateCommandList = nullptr;
	};

}//namespace Render


#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif



#endif // D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C
