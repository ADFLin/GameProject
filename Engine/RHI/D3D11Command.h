#pragma once
#ifndef D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B
#define D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B

#include "RHICommand.h"
#include "RHICommandListImpl.h"
#include "RHIGlobalResource.h"
#include "ShaderCore.h"

#include "D3D11Common.h"
#include "InlineString.h"
#include "Core/ScopeGuard.h"
#include "Core/TypeHash.h"
#include "DataStructure/Array.h"

#include <D3D11Shader.h>
#include <D3Dcompiler.h>

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
#include "PlatformThread.h"



namespace Render
{
	template<class T>
	struct ToShaderEnum {};
	template<> struct ToShaderEnum< ID3D11VertexShader > { static EShader::Type constexpr Result = EShader::Vertex; };
	template<> struct ToShaderEnum< ID3D11PixelShader > { static EShader::Type constexpr Result = EShader::Pixel; };
	template<> struct ToShaderEnum< ID3D11GeometryShader > { static EShader::Type constexpr Result = EShader::Geometry; };
	template<> struct ToShaderEnum< ID3D11ComputeShader > { static EShader::Type constexpr Result = EShader::Compute; };
	template<> struct ToShaderEnum< ID3D11HullShader > { static EShader::Type constexpr Result = EShader::Hull; };
	template<> struct ToShaderEnum< ID3D11DomainShader > { static EShader::Type constexpr Result = EShader::Domain; };

	struct ShaderConstDataBuffer
	{
		TComPtr< ID3D11Buffer > resource;
		TArray< uint8 >    mDataBuffer;
		uint32 mUpdateDataSize = 0;

		bool initializeResource(ID3D11Device* device);
		void releaseResource();
		void updateBufferSize(int newSize);
		void updateValue(ShaderParameter const parameter, void const* value, int valueSize);
		void commit(ID3D11DeviceContext* context);
	};

	struct D3D11ResourceBoundState
	{

		D3D11ResourceBoundState()
		{
			
		}

		bool initialize(TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext);

		void releaseResource()
		{
			for (auto& buffer : mConstBuffers)
			{
				buffer.releaseResource();
			}
		}

		void clear()
		{
			mConstBufferValueDirtyMask = 0;
			mConstBufferDirtyMask = 0;
			mSRVDirtyMask = 0;
			mMaxSRVBoundIndex = INDEX_NONE;
			mSamplerDirtyMask = 0;

			std::fill_n(mBoundedConstBuffers, ARRAY_SIZE(mBoundedConstBuffers), nullptr);
			std::fill_n(mBoundedSRVs, ARRAY_SIZE(mBoundedSRVs), nullptr);
			std::fill_n(mBoundedConstBuffers, ARRAY_SIZE(mBoundedConstBuffers), nullptr);
			std::fill_n(mBoundedSamplers, ARRAY_SIZE(mBoundedSamplers), nullptr);
			mUAVUsageCount = 0;
			mUAVDirtyMask = 0;
		}

		void bindShader(class D3D11ShaderData& shader);
		void setTexture(ShaderParameter const& parameter, RHITextureBase& texture);
		bool clearTexture(ShaderParameter const& parameter);
		bool clearUAV(ShaderParameter const& parameter);
		void setRWTexture(ShaderParameter const& parameter, RHITextureBase* texture);
		void setSampler(ShaderParameter const& parameter, RHISamplerState& sampler);
		void setUniformBuffer(ShaderParameter const& parameter, RHIBuffer& buffer);
		void setStructuredBuffer(ShaderParameter const& parameter, RHIBuffer& buffer, EAccessOperator op);
		void setShaderValue(ShaderParameter const& parameter, void const* value, int valueSize);

		template< EShader::Type TypeValue >
		void commitState( ID3D11DeviceContext* context);
		template< EShader::Type TypeValue >
		void clearConstantBuffers(ID3D11DeviceContext* context);
		template< EShader::Type TypeValue >
		void postDrawPrimitive(ID3D11DeviceContext* context);

		template< EShader::Type TypeValue >
		void clearSRVResource(ID3D11DeviceContext* context,RHIResource& resource);
		template< EShader::Type TypeValue >
		void clearSRVResource(ID3D11DeviceContext* context);

		template< EShader::Type TypeValue >
		void commitSAVState(ID3D11DeviceContext* context);

		void commitUAVState(ID3D11DeviceContext* context);


		static int constexpr MaxConstBufferNum = 1;
		uint32 mConstBufferDirtyMask;
		uint32 mConstBufferValueDirtyMask;

		ShaderConstDataBuffer mConstBuffers[MaxConstBufferNum];

		static int constexpr MaxSimulatedBoundedBufferNum = 16;
		ID3D11Buffer* mBoundedConstBuffers[MaxSimulatedBoundedBufferNum];

		static int constexpr MaxSimulatedBoundedSRVNum = 16;
		ID3D11ShaderResourceView* mBoundedSRVs[MaxSimulatedBoundedSRVNum];
		RHIResource*              mBoundedSRVResources[MaxSimulatedBoundedSRVNum];
		int32  mMaxSRVBoundIndex = INDEX_NONE;
		uint32 mSRVDirtyMask;

		static int constexpr MaxSimulatedBoundedUAVNum = 16;
		ID3D11UnorderedAccessView* mBoundedUAVs[MaxSimulatedBoundedUAVNum];
		uint32 mUAVDirtyMask;
		uint32 mUAVUsageCount = 0;

		static int constexpr MaxSimulatedBoundedSamplerNum = 16;
		ID3D11SamplerState* mBoundedSamplers[MaxSimulatedBoundedSamplerNum];
		uint32 mSamplerDirtyMask;

	};



	static constexpr uint32  D3D11BUFFER_ALIGN = 4;
	class D3D11DynamicBuffer
	{
	public:
		D3D11DynamicBuffer(){}

		~D3D11DynamicBuffer() { assert(mBuffers.empty()); }

		
		bool initialize(TComPtr< ID3D11Device >&  device, uint32 inBindFlags, uint32 initialBufferSizes[], int numInitialBuffer)
		{
			mBindFlags = inBindFlags;
			std::sort(initialBufferSizes, initialBufferSizes + numInitialBuffer, [](auto lhs, auto rhs) { return lhs < rhs; });
			for( int i = 0; i < numInitialBuffer; ++i )
			{
				uint32 bufferSize = (D3D11BUFFER_ALIGN * initialBufferSizes[i] + D3D11BUFFER_ALIGN - 1) / D3D11BUFFER_ALIGN;
				pushNewBuffer(device, bufferSize);
			}
			return true;
		}

		void release()
		{
			for( ID3D11Buffer* buffer : mBuffers )
			{
				buffer->Release();
			}
			mBuffers.clear();
			mBufferSizes.clear();
			mLockedIndex = -1;
		}

		bool pushNewBuffer(TComPtr< ID3D11Device >&  device, uint32 bufferSize)
		{
			D3D11_BUFFER_DESC bufferDesc = { 0 };
			bufferDesc.ByteWidth = bufferSize;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = mBindFlags;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;
			TComPtr<ID3D11Buffer> BufferResource;
			HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &BufferResource);
			if( hr != S_OK )
			{
				return false;
			}
			mBuffers.push_back(BufferResource.detach());
			mBufferSizes.push_back(bufferSize);
			return true;
		}

		void* lock( ID3D11DeviceContext* context , uint32 size )
		{
			size = (D3D11BUFFER_ALIGN * size + D3D11BUFFER_ALIGN - 1) / D3D11BUFFER_ALIGN;
			int index = 0;
			for( ; index < mBufferSizes.size(); ++index )
			{
				if ( mBufferSizes[index] >= size )
					break;
			}
			if( index == mBuffers.size() )
			{
				TComPtr< ID3D11Device > device;
				mBuffers[0]->GetDevice(&device);
				if( device == nullptr )
					return nullptr;
				if( !pushNewBuffer(device, size) )
					return nullptr;
			}

	
			D3D11_MAPPED_SUBRESOURCE mappedSubresource;
			HRESULT hr = context->Map(mBuffers[index], 0, D3D11_MAP_WRITE_DISCARD, 0 , &mappedSubresource );
			if( hr != S_OK )
			{
				return nullptr;
			}
			mLockedIndex = index;
			return mappedSubresource.pData;

		}
		ID3D11Buffer* unlock(ID3D11DeviceContext* context)
		{
			assert(mLockedIndex >= 0);
			ID3D11Buffer* buffer = mBuffers[mLockedIndex];
			context->Unmap( mBuffers[mLockedIndex] , 0);
			mLockedIndex = -1;
			return buffer;
		}

		ID3D11Buffer* getLockedBuffer() { return mBuffers[mLockedIndex]; }
		TArray< ID3D11Buffer* > mBuffers;
		TArray< uint32 >        mBufferSizes;
		uint32 mBindFlags;
		int    mLockedIndex = -1;
	};

	class D3D11Context : public RHIContext
	{
	public:

		bool initialize(TComPtr< ID3D11Device >&  device , TComPtr<ID3D11DeviceContext >& deviceContext);

		void release();



		void RHISetRasterizerState(RHIRasterizerState& rasterizerState);
		void RHISetBlendState(RHIBlendState& blendState);
		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef);
		void RHISetViewport(float x, float y, float w, float h, float zNear, float zFar);
		void RHISetViewports(ViewportInfo const viewports[], int numViewports);
		void RHISetScissorRect(int x, int y, int w, int h);

		void postDrawPrimitive();
		void RHIDrawPrimitive(EPrimitive type, int start, int nv);

		void RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex);

		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride);
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance);

		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance);

		void RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData);

		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex);


		void RHIDrawMeshTasks(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
		{


		}
		void RHIDrawMeshTasksIndirect(RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{

		}

		void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color , RHITexture2D* texture, RHISamplerState* sampler);



		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer);

		void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil);

	
		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);
		void RHISetIndexBuffer(RHIBuffer* indexBuffer)
		{
			if( indexBuffer )
			{
				mDeviceContext->IASetIndexBuffer(D3D11Cast::To(indexBuffer)->getResource(), D3D11Translate::IndexType(indexBuffer), 0);
			}
			else
			{
				mDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
			}	
		}

		void RHIFlushCommand()
		{
			//mDeviceContext->Flush();
			mRenderTargetsState = nullptr;
		}


		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
		{
			commitComputeState();
			mDeviceContext->Dispatch(numGroupX, numGroupY, numGroupZ);
		}

		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);




		void commitGraphicsShaderState();

		void commitComputeState();
		bool determitPrimitiveTopologyUP(EPrimitive primitiveType, int num, uint32 const* pIndices, D3D_PRIMITIVE_TOPOLOGY& outPrimitiveTopology, ID3D11Buffer** outIndexBuffer, int& outIndexNum);


		//
		template< EShader::Type TypeValue >
		void setShader(D3D11ShaderVariant const& shaderVariant);

		template < class ValueType >
		void setShaderValueT(RHIShaderProgram& shaderProgram, ShaderParameter const& param, ValueType const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector2 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);

		void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView) {}

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);
		void clearShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param);
		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);
		void clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param);

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer);
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOperator op);
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer);

		void RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc);
		void RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc)
		{

		}
		void RHISetComputeShader(RHIShader* shader);


		template < class ValueType >
		void setShaderValueT(RHIShader& shader, ShaderParameter const& param, ValueType const val[], int dim);


		void setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector2 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim);
		void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderMatrix43(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim); }
		void setShaderMatrix34(RHIShader& shader, ShaderParameter const& param, float const val[], int dim) { setShaderValueT(shader, param, val, dim); }

		void setShaderResourceView(RHIShader& shader, ShaderParameter const& param, RHIShaderResourceView const& resourceView) {}

		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler);
		void clearShaderTexture(RHIShader& shader, ShaderParameter const& param);
		void setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op);
		void clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param);
		void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer);
		void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOperator op);
		void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer);

		void clearSRVResource(RHIResource& resource);

		void setGfxShaderProgram(RHIShaderProgram& shaderProgram);

		void markRenderStateDirty();

		D3D11RenderTargetsState* mRenderTargetsState = nullptr;


		uint32                  mGfxBoundedShaderMask = 0;
		uint32                  mBoundedShaderDirtyMask = 0;
		D3D11ShaderVariant      mBoundedShaders[EShader::Count];
		D3D11ResourceBoundState mResourceBoundStates[EShader::Count];

		D3D11DynamicBuffer    mDynamicVBuffer;
		D3D11DynamicBuffer    mDynamicIBuffer;

		D3D11_VIEWPORT mViewportStates[8];

		SimplePipelineParamValues mFixedShaderParams;
		bool bUseFixedShaderPipeline = true;

		RHIResource* mVertexShader = nullptr;
		TArray< uint8 >* mVertexShaderByteCode;
		TComPtr< ID3D11DeviceContext >  mDeviceContext;
		TComPtr< ID3D11Device > mDevice;
		RHIInputLayout* mInputLayout = nullptr;

	};



	class D3D11System : public RHISystem
	{
	public:
		RHISystemName getName() const { return RHISystemName::D3D11; }
		bool initialize(RHISystemInitParams const& initParam);
		void clearResourceReference();
		void shutdown();
		virtual ShaderFormat* createShaderFormat();

		bool RHIBeginRender();

		Mutex mMutexContext;

		void RHIEndRender(bool bPresent);

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}

		RHISwapChain*      RHICreateSwapChain(SwapChainCreationInfo const& info);
		RHITexture1D*      RHICreateTexture1D(TextureDesc const& desc, void* data);
		RHITexture2D*      RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign);
		RHITexture3D*      RHICreateTexture3D(TextureDesc const& desc, void* data);
		RHITextureCube*    RHICreateTextureCube(TextureDesc const& desc, void* data[]);
		RHITexture2DArray* RHICreateTexture2DArray(TextureDesc const& desc, void* data);
		RHITexture2D*      RHICreateTextureDepth(TextureDesc const& desc);

		RHIBuffer*  RHICreateBuffer(uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data);

		void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIBuffer* buffer);

		void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData);
		void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData);

		bool RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth);
		RHIFrameBuffer*   RHICreateFrameBuffer();

		RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);
		RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);

		RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

		RHIShader* RHICreateShader(EShader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();

		bool createTexture1DInternal(TextureDesc const& desc, void* data, Texture1DCreationResult& outResult);
		bool createTexture2DInternal(TextureDesc const& desc, void* data, int dataAlign, bool bDepth, Texture2DCreationResult& outResult);
		bool createTexture3DInternal(TextureDesc const& desc, void* data, Texture3DCreationResult& outResult);
		bool createTextureCubeInternal(TextureDesc const& desc, void* data[], TextureCubeCreationResult& outResult);
		

		bool createStagingTexture(ID3D11Texture2D* texture, TComPtr< ID3D11Texture2D >& outTexture, int level = 0);


		void* lockBufferInternal(ID3D11Resource* resource, ELockAccess access, uint32 offset, uint32 size);

		struct InputLayoutKey
		{
			InputLayoutKey(InputLayoutDesc const& desc)
				:elements(desc.mElements)
			{
				hash = 0x1a21df14;
				for( auto const& e : elements )
				{
					if (e.attribute == EVertex::ATTRIBUTE_UNUSED)
						continue;

					hash = HashCombine(hash, HashValue(&e , sizeof(e)) );
				}
			}
			TArray< InputElementDesc > elements;
			uint32 hash;

			uint32 getTypeHash() const { return hash; }

			bool operator == (InputLayoutKey const& rhs) const
			{
				return hash == rhs.hash && elements == rhs.elements;
			}
		};

		TRefCountPtr< D3D11SwapChain > mSwapChain;
		bool mbVSyncEnable = false;

		std::unordered_map< InputLayoutKey, RHIInputLayoutRef , MemberFuncHasher > mInputLayoutMap;
		D3D11Context   mRenderContext;
		TComPtr< ID3D11Device > mDevice;
		TComPtr< ID3D11DeviceContext > mDeviceContext;
		TComPtr< ID3D11DeviceContext > mDeviceContextImmdiate;
		RHICommandListImpl* mImmediateCommandList = nullptr;
	};



}//namespace Render


#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

#endif // D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B
