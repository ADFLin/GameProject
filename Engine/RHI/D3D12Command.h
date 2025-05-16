#pragma once
#ifndef D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C
#define D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C

#include "D3D12ShaderCommon.h"
#include "D3D12Buffer.h"

#include "RHICommand.h"
#include "RHICommandListImpl.h"
#include "RHIGlobalResource.h"

#include "ShaderCore.h"

#include "D3D12Common.h"
#include "InlineString.h"
#include "Core/ScopeGuard.h"
#include "Core/TypeHash.h"

#if RHI_USE_RESOURCE_TRACE
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
		TArray< ShaderInfo >    mShaders;
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

	class D3D12DynamicBuffer
	{
	public:
		bool initialize();

		void* lock(uint32 size);
		void  unlock(D3D12_VERTEX_BUFFER_VIEW& bufferView);
		void  unlock(D3D12_INDEX_BUFFER_VIEW& bufferView);

		D3D12BufferAllocation mLockedResource;	
	};


	struct D3D12ResourceBoundState
	{
	public:
		bool initialize( ID3D12DeviceRHI* device );
		void restState();
		void posetDrawCall()
		{
			postDrawOrDispatchCall();
		}
		void posetDispatchCall()
		{
			postDrawOrDispatchCall();
		}

		void postDrawOrDispatchCall();

		D3D12_GPU_VIRTUAL_ADDRESS getConstantGPUAddress()
		{
			return mGlobalConstAllocation.gpuAddress;
		}

		struct UAVBoundState
		{
			ID3D12Resource* resource = nullptr;
		};
		UAVBoundState mUAVStates[8];

		bool updateConstBuffer(D3D12ShaderData& shaderData);
		void updateConstantData(void const* pData, uint32 offset, uint32 size);

		D3D12ShaderData* mShaderData = nullptr;
		D3D12BufferAllocation   mGlobalConstAllocation;
		bool   mbGlobalConstCommited = false;

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
		void RHISetBlendState(RHIBlendState& blendState);
		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef);
		void RHISetViewport(ViewportInfo const& viewport);
		void RHISetViewports(ViewportInfo const viewports[], int numViewports);
		void RHISetScissorRect(int x, int y, int w, int h);

		bool determitPrimitiveTopologyUP(EPrimitive primitive, int num, uint32 const* pIndices, EPrimitive& outPrimitiveDetermited, D3D12_INDEX_BUFFER_VIEW& outIndexBufferView, int& outIndexNum);
		
		void postDrawPrimitive() 
		{
			for (auto & shader : mGfxBoundState->mShaders)
			{
				mResourceBoundStates[shader.type].posetDrawCall();
			}
		}
		void postDispatchCompute()
		{
			mResourceBoundStates[EShader::Compute].posetDispatchCall();
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

		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride);

		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance);

		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance);

		void RHIDrawMeshTasks(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);

		void RHIDrawMeshTasksIndirect(RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride);

	
		void RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 numInstance);
		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex, uint32 numInstance);


		void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler) 
		{
			mFixedShaderParams.transform = transform;
			mFixedShaderParams.color = color;
			mFixedShaderParams.texture = texture;
			mFixedShaderParams.sampler = sampler;
			mGfxBoundState = nullptr;
		}

		SimplePipelineParamValues mFixedShaderParams;

		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer);



		void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil);


		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);

		void RHISetIndexBuffer(RHIBuffer* indexBuffer);

		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);

		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);

		void RHIClearSRVResource(RHIResource* resource)
		{
			if (resource)
			{
				clearSRVResource(*resource);
			}
		}
		void RHIResourceTransition(TArrayView<RHIResource*> resources, EResourceTransition transition);

		void RHIResolveTexture(RHITextureBase& destTexture, int destSubIndex, RHITextureBase& srcTexture, int srcSubIndex);
		void RHIFlushCommand();



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



		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op);
		void setShaderRWSubTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
		{

		}
		void clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param) {}


		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer);

		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op);
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer){}

		void clearShaderRWTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param);

		void setShaderTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderSampler(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderUniformBuffer(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHIBuffer& buffer);
		void setShaderStorageBuffer(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op);

		void RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc);
		void RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc);
		void RHISetComputeShader(RHIShader* shader);


		void setGfxShaderBoundState(D3D12ShaderBoundState* newState);
		void setComputeShaderBoundState(D3D12ShaderBoundState* newState);

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
		void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op);
		void setShaderRWSubTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
		{

		}
		void setShaderRWTexture(EShader::Type shaderType, D3D12ShaderData& shaderData, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op);
		void clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param);
		void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer);

		void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op);
		void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
		{

		}

		void clearSRVResource(RHIResource& resource)
		{

		}

		void setShaderSamplerInternal(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler)
		{

		}

		void commitRenderTargetState();
		void commitGraphicsPipelineState(EPrimitive type);
		void commitMeshPipelineState();
		void commitComputePipelineState();

		void flushCommand()
		{
			mGraphicsCmdList->Close();
			ID3D12CommandList* ppCommandLists[] = { mGraphicsCmdList };
			mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		}

		void resetCommandList()
		{
			mFrameDataList[mFrameIndex].resetCommandList(mPiplineStateCommitted);
			commitRenderTargetState();
		}

		ID3D12PipelineState*     mPiplineStateCommitted = nullptr;

		D3D12RenderTargetsState* mRenderTargetsState;
		D3D12ShaderBoundState*  mGfxBoundState = nullptr;
		D3D12ShaderBoundState*  mComputeBoundState = nullptr;
		RHIInputLayoutRef       mInputLayoutPending;
		RHIRasterizerStateRef   mRasterizerStatePending;
		RHIDepthStencilStateRef mDepthStencilStatePending;
		RHIBlendStateRef        mBlendStateStatePending;

		TComPtr< ID3D12CommandSignature > mDrawCmdSignature;
		TComPtr< ID3D12CommandSignature > mDrawIndexedCmdSignature;
		TComPtr< ID3D12CommandSignature > mDispatchMeshCmdSignature;


		static constexpr int MaxViewportCount = 8;
		D3D12_RECT mScissorRects[MaxViewportCount];
		D3D12_RECT mViewportRects[MaxViewportCount];
		uint32     mNumViewports = 1;
		static constexpr int MaxInputStream = 16;
		InputStreamInfo mInputStreams[16];
		int  mNumInputStream = 0;
		bool mbInpuStreamDirty = false;

		RHIBufferRef mIndexBufferState;
		bool bIndexBufferStateDirty = false;


		void commitInputStreams(bool bForce);
		void commitInputBuffer(bool bForce);

		void updateCSUHeapUsage(D3D12PooledHeapHandle const& handle)
		{
			if (handle.chunk == nullptr)
			{
				return;
			}
			//mGraphicsCmdList->SetDescriptorHeaps(1, &handle.chunk->resource);
			//return;
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
			//mGraphicsCmdList->SetDescriptorHeaps(1, &handle.chunk->resource);
			//return;
			if (mUsedDescHeaps[1] != handle.chunk->resource)
			{
				if (mUsedDescHeaps[1] == nullptr)
					++mNumUsedHeaps;

				mUsedDescHeaps[1] = handle.chunk->resource;
				if (mNumUsedHeaps == 1)
					mGraphicsCmdList->SetDescriptorHeaps(mNumUsedHeaps, mUsedDescHeaps + 1);
				else
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


		TComPtr< ID3D12DeviceRHI >         mDevice;
		TComPtr< ID3D12CommandQueue >      mCommandQueue;
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

			bool beginFrame()
			{
				VERIFY_D3D_RESULT_RETURN_FALSE(cmdAllocator->Reset());
				VERIFY_D3D_RESULT_RETURN_FALSE(graphicsCmdList->Reset(cmdAllocator, nullptr));

				return true;
			}

			bool resetCommandList(ID3D12PipelineState* pInitialState)
			{
				VERIFY_D3D_RESULT_RETURN_FALSE(graphicsCmdList->Reset(cmdAllocator, pInitialState));
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

		TArray< FrameData > mFrameDataList;

	};



	class D3D12System : public RHISystem
		              , public ID3DErrorHandler
	{
	public:
		~D3D12System();
		RHISystemName getName() const { return RHISystemName::D3D12; }
		bool initialize(RHISystemInitParams const& initParam);
		void shutdown();
		void clearResourceReference();

		virtual ShaderFormat* createShaderFormat();

		bool RHIBeginRender();

		void RHIEndRender(bool bPresent);

		void RHIMarkRenderStateDirty()
		{
			//TODO:
		}

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}

		RHISwapChain*    RHICreateSwapChain(SwapChainCreationInfo const& info);

		TArray < TComPtr< ID3D12Resource > > mCopyResources;
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

		RHITexture1D*      RHICreateTexture1D(TextureDesc const& desc, void* data);
		RHITexture2D*      RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign);
		RHITexture3D*      RHICreateTexture3D(TextureDesc const& desc, void* data) { return nullptr; }
		RHITextureCube*    RHICreateTextureCube(TextureDesc const& desc, void* data[]) { return nullptr; }
		RHITexture2DArray* RHICreateTexture2DArray(TextureDesc const& desc, void* data) { return nullptr; }


		bool updateTexture2DSubresources(ID3D12Resource* textureResource, ETexture::Format format, void* data, uint32 ox, uint32 oy, uint32 width, uint32 height, uint32 rowPatch, uint32 level = 0);
		bool updateTexture2DSubresources(ID3D12Resource* textureResource, D3D12_RESOURCE_STATES states, ETexture::Format format, void* data, uint32 ox, uint32 oy, uint32 width, uint32 height, uint32 rowPatch, uint32 level = 0);
		bool updateTexture1DSubresources(ID3D12Resource* textureResource, ETexture::Format format, void* data, uint32 offset, uint32 length, uint32 level = 0);

		RHIBuffer*  RHICreateBuffer(BufferDesc const& desc, void* data);
		void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIBuffer* buffer);

		void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
		{

		}
		void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
		{

		}
		bool RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth);

		void RHIUpdateBuffer(RHIBuffer& buffer, int start, int numElements, void* data);

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

		ID3D12Resource* createDepthTexture2D(DXGI_FORMAT format, uint64 w , uint64 h, uint32 sampleCount, uint32 creationFlags)
		{
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = format;
			textureDesc.Width = w;
			textureDesc.Height = h;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			if (creationFlags & TCF_CreateUAV)
			{
				textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = sampleCount;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			D3D12_CLEAR_VALUE optimizedClearValue = {};
			optimizedClearValue.Format = format;
			optimizedClearValue.DepthStencil = { FRHIZBuffer::FarPlane , 0 };

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
			mShaderBoundStateMap.clear();
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


		bool mbInRendering;

		D3D12Context   mRenderContext;
		TComPtr<ID3D12DeviceRHI> mDevice;

		class D3D12ProfileCore* mProfileCore = nullptr;
		TRefCountPtr< D3D12SwapChain > mSwapChain;

		RHICommandListImpl* mImmediateCommandList = nullptr;

		void notifyFlushCommand();
		//ID3DErrorHandler
		void handleErrorResult(HRESULT errorResult) override;

	};

}//namespace Render


#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif



#endif // D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C
