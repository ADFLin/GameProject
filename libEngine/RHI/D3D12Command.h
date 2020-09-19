#pragma once
#ifndef D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C
#define D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C

#include "RHICommand.h"
#include "RHICommandListImpl.h"
#include "ShaderCore.h"

#include "D3D12Common.h"
#include "FixString.h"
#include "Core/ScopeExit.h"
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



	struct D3D12ShaderBoundStateKey
	{
		enum Type
		{
			eGraphiscsState,
			eMeshState,
			eCompute,
		};
		union
		{
			struct
			{
				uint64 boundType : 3;
				uint64 shaderA   : 21;
				uint64 shaderB   : 20;
				uint64 shaderC   : 20;
			};

			uint64 valueA;
		};


		union
		{
			struct
			{
				uint64 shaderD   : 16;
				uint64 shaderE   : 16;
			};

			uint32 valueB;
		};

		void initialize(GraphicsShaderStateDesc const& state)
		{
			boundType = eGraphiscsState;
			shaderA = state.pixel ? state.pixel->mGUID : 0;
			shaderB = state.vertex ? state.vertex->mGUID : 0;
			shaderC = state.geometry ? state.geometry->mGUID : 0;
			shaderD = state.hull ? state.hull->mGUID : 0;
			shaderE = state.domain ? state.domain->mGUID : 0;
		}

		bool operator == (D3D12ShaderBoundStateKey const& rhs) const
		{
			return valueA == rhs.valueA && valueB == rhs.valueB;
		}
		uint32 getTypeHash() const
		{
			uint32 result = std::hash_value(valueA);
			HashCombine(result, valueB);
			return result;
		}
	};


	class D3D12ShaderBoundState
	{
	public:
		struct ShaderInfo
		{
			RHIShaderRef resource;
			uint rootSlotStart;
		};
		D3D12ShaderBoundStateKey   cachedKey;
		std::vector< ShaderInfo >   mShaders;
		TComPtr<ID3D12RootSignature> mRootSignature;
	};


	struct D3D12PipelineStateKey
	{
		D3D12ShaderBoundStateKey boundStateKey;

		union
		{
			struct
			{			
				uint64 inputLayoutID   : 12;
				uint64 rasterizerID    : 12;
				uint64 blendID         : 12;
				uint64 depthStenceilID : 12;
				uint64 renderTargetFormatID : 12;
				uint64 topologyType : 4;
			};

			uint64 value;
		};

		void initialize(
			GraphicsPipelineStateDesc const& stateDesc,
			D3D12ShaderBoundState* boundState,
			D3D12RenderTargetsState* renderTargetsState,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE  topologyType);

		bool operator == (D3D12PipelineStateKey const& rhs) const
		{
			return value == rhs.value && boundStateKey == rhs.boundStateKey;
		}

		uint32 getTypeHash() const
		{
			uint32 result = boundStateKey.getTypeHash();
			HashCombine(result, value);
			return result;
		}
	};


	struct D3D12ResourceBoundState
	{
	public:
		bool initialize( ID3D12DeviceRHI* device );

		void UpdateConstantData( void const* pData , uint32 offset , uint32 size )
		{
			memcpy(mConstDataPtr + offset, pData, size);
		}


		uint8* mConstDataPtr;
		TComPtr< ID3D12Resource > mConstBuffer;
		D3D12PooledHeapHandle     mConstBufferHandle;
	};

	class D3D12Context : public RHIContext
	{
	public:

		bool initialize( D3D12System* system );

		void release();

		void RHISetRasterizerState(RHIRasterizerState& rasterizerState) 
		{
			if (mRasterizerStatePending != &rasterizerState)
			{
				mRasterizerStatePending = &rasterizerState;
			}
		}
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

		void PostDrawPrimitive() {}
		void RHIDrawPrimitive(EPrimitive type, int start, int nv)
		{
			commitGraphicsState(type);
			mGraphicsCmdList->DrawInstanced(nv, 1, start, 0);
		}

		void RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex)
		{
			commitGraphicsState(type);
			mGraphicsCmdList->DrawIndexedInstanced(nIndex, 1, indexStart, baseVertex, 0);
		}

		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{
			commitGraphicsState(type);
		}
		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{
			commitGraphicsState(type);
		}
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
		{
			commitGraphicsState(type);
			mGraphicsCmdList->DrawInstanced(nv, numInstance,vStart, baseInstance);
		}

		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance)
		{
			commitGraphicsState(type);
			mGraphicsCmdList->DrawIndexedInstanced(nIndex, numInstance, indexStart, baseVertex, baseInstance);
		}

		void RHIDrawPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numVertexData)
		{
			commitGraphicsState(type);
		}

		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numVertexData, int const* pIndices, int numIndex) 
		{
			commitGraphicsState(type);
		}


		void RHIDrawMeshTasks(int start, int count)
		{


		}
		void RHIDrawMeshTasksIndirect(RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{

		}

		void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler) {}


		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer);

		void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil);


		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);

		void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer) {}

		void RHIFlushCommand();


		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
		{

		}

		void RHISetShaderProgram(RHIShaderProgram* shaderProgram) {}

	

		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim) {}
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) {}
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim) {}
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim) {}
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector2 const val[], int dim) {}
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim) {}
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim) {}
		void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) {}
		void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) {}
		void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim) {}

		void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView) {}

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture) {}
		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler) {}
		void clearShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param) {}
		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler) {}
		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op) {}
		void clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param) {}

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer) {}
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op) {}
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
		{

		}

		void RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& state);
		void RHISetMeshShaderBoundState(MeshShaderStateDesc const& state)
		{

		}
		void RHISetComputeShader(RHIShader* shader) {}

		void setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim) {}
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim) {}
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim) {}
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector2 const val[], int dim) {}
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim) {}
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) {}
		void setShaderMatrix43(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) {}
		void setShaderMatrix34(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) {}

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
		void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op) {}
		void clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param){}
		void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer) {}

		void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op)
		{

		}
		void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer)
		{

		}

		void clearSRVResource(RHIResource& resource)
		{

		}

		void SetShaderSamplerInternal(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler)
		{

		}


		void commitGraphicsState(EPrimitive type);


		D3D12RenderTargetsState* mRenderTargetsState;
		D3D12ShaderBoundState*  mBoundState = nullptr;
		RHIInputLayoutRef       mInputLayoutPending;
		RHIRasterizerStateRef   mRasterizerStatePending;
		RHIDepthStencilStateRef mDepthStencilStatePending;
		RHIBlendStateRef        mBlendStateStatePending;

		std::vector< ID3D12DescriptorHeap* > mUsedHeaps;
		uint32 mCSUHeapUsageMask;
		uint32 mSamplerHeapUsageMAsk;
		uint32 mRootSlotStart[EShader::Count];
		D3D12ResourceBoundState mResourceBoundStates[EShader::Count];



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
		RHISytemName getName() const { return RHISytemName::D3D12; }
		bool initialize(RHISystemInitParams const& initParam);
		void preShutdown() { }
		void shutdown();
		virtual ShaderFormat* createShaderFormat();

		bool RHIBeginRender()
		{
			if (!mRenderContext.beginFrame())
			{
				return false;
			}
			return true;
		}

		void RHIEndRender(bool bPresent)
		{
			mRenderContext.endFrame();

			mRenderContext.RHIFlushCommand();
			if (bPresent)
			{
				mSwapChain->Present(bPresent);
			}
			mRenderContext.moveToNextFrame(mSwapChain->mResource);
		}

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}

		RHISwapChain*    RHICreateSwapChain(SwapChainCreationInfo const& info);


		RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel, uint32 createFlags,
			void* data)
		{
			return nullptr;
		}


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

		RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 createFlags,
			void* data, int dataAlign);

		RHITexture3D*    RHICreateTexture3D(
			Texture::Format format, int sizeX, int sizeY, int sizeZ,
			int numMipLevel, int numSamples, uint32 createFlags,
			void* data)
		{
			return nullptr;
		}

		RHITextureCube*  RHICreateTextureCube(Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[]) { return nullptr; }

		RHITexture2DArray* RHICreateTexture2DArray(Texture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
		{
			return nullptr;
		}

		RHITexture2D*     RHICreateTextureDepth(Texture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags) { return nullptr; }
		RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data) { return nullptr; }


		void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIVertexBuffer* buffer);
		void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size) { return nullptr; }
		void  RHIUnlockBuffer(RHIIndexBuffer* buffer) { return; }

		RHIFrameBuffer*   RHICreateFrameBuffer() { return nullptr; }

		RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);

		RHISamplerState*      RHICreateSamplerState(SamplerStateInitializer const& initializer);
		RHIRasterizerState*   RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState*        RHICreateBlendState(BlendStateInitializer const& initializer);
		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

		RHIShader* RHICreateShader(EShader::Type type);
		RHIShaderProgram* RHICreateShaderProgram() { return nullptr; }

		
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

		std::unordered_map< D3D12ShaderBoundStateKey, D3D12ShaderBoundState*, MemberFuncHasher > mShaderBoundStateMap;
		void releaseShaderBoundState()
		{
			for (auto& pair : mShaderBoundStateMap)
			{
				delete pair.second;
			}
			mShaderBoundStateMap.empty();
		}
		D3D12ShaderBoundState* getShaderBoundState(GraphicsShaderStateDesc const& state);

		D3D12PipelineState* getPipelineState(
			GraphicsPipelineStateDesc const& stateDesc,
			D3D12ShaderBoundState* boundState, 
			D3D12RenderTargetsState* renderTargetsState);

		std::unordered_map< D3D12PipelineStateKey, TRefCountPtr< D3D12PipelineState >, MemberFuncHasher> mPipelineStateMap;


		D3D12Context   mRenderContext;
		TComPtr<ID3D12DeviceRHI> mDevice;

		TComPtr<ID3D12RootSignature> mRootSignature;
		TRefCountPtr< D3D12SwapChain > mSwapChain;

		RHICommandListImpl* mImmediateCommandList = nullptr;
	};

}//namespace Render


#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif



#endif // D3D12Command_H_AE2B294E_ACAB_461C_99EA_AB1F1C47045C
