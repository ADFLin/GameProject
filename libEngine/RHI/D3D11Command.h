#pragma once
#ifndef D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B
#define D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B

#include "RHICommand.h"
#include "RHICommandListImpl.h"
#include "ShaderCore.h"

#include "D3D11Common.h"
#include "FixString.h"
#include "Core/ScopeExit.h"
#include "Core/TypeHash.h"


#include <D3D11Shader.h>


#include <D3Dcompiler.h>
#include <D3DX11async.h>

#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "D3DX11.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

namespace Render
{

	template<class T>
	struct ToShaderEnum {};
	template<> struct ToShaderEnum< ID3D11VertexShader > { static Shader::Type constexpr Result = Shader::eVertex; };
	template<> struct ToShaderEnum< ID3D11PixelShader > { static Shader::Type constexpr Result = Shader::ePixel; };
	template<> struct ToShaderEnum< ID3D11GeometryShader > { static Shader::Type constexpr Result = Shader::eGeometry; };
	template<> struct ToShaderEnum< ID3D11ComputeShader > { static Shader::Type constexpr Result = Shader::eCompute; };
	template<> struct ToShaderEnum< ID3D11HullShader > { static Shader::Type constexpr Result = Shader::eHull; };
	template<> struct ToShaderEnum< ID3D11DomainShader > { static Shader::Type constexpr Result = Shader::eDomain; };

	struct FrameSwapChain
	{

		TComPtr<IDXGISwapChain> ptr;
	};
	struct ShaderConstDataBuffer
	{
		TComPtr< ID3D11Buffer > resource;
		std::vector< uint8 >    updateData;
		uint32 updateDataSize = 0;

		bool initializeResource(ID3D11Device* device)
		{
			D3D11_BUFFER_DESC bufferDesc = { 0 };
			ZeroMemory(&bufferDesc, sizeof(bufferDesc));
			bufferDesc.ByteWidth = 512;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			VERIFY_D3D11RESULT_RETURN_FALSE(device->CreateBuffer(&bufferDesc, nullptr, &resource));
			return true;
		}

		void setUpdateValue(ShaderParameter const parameter, void const* value, int valueSize)
		{
			int idxDataEnd = parameter.offset + parameter.size;
			if( updateData.size() <= idxDataEnd )
			{
				updateData.resize(idxDataEnd);
			}

			::memcpy(&updateData[parameter.offset], value, parameter.size);
			if( idxDataEnd > updateDataSize )
			{
				updateDataSize = idxDataEnd;
			}
		}

		void updateResource(ID3D11DeviceContext* context)
		{
			if( updateDataSize )
			{
				context->UpdateSubresource(resource, 0, nullptr, &updateData[0], updateDataSize, updateDataSize);
				updateDataSize = 0;
			}
		}
	};


	struct D3D11ShaderBoundState
	{

		D3D11ShaderBoundState()
		{
			
		}

		bool initialize(TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext);


		void clear()
		{
			mConstBufferValueDirtyMask = 0;
			mConstBufferDirtyMask = 0;
			mSRVDirtyMask = 0;
			mUAVDirtyMask = 0;
			mSamplerDirtyMask = 0;
		}

		void setTexture(ShaderParameter const& parameter, RHITextureBase& texture);
		void setSampler(ShaderParameter const& parameter, RHISamplerState& sampler);
		void setUniformBuffer(ShaderParameter const& parameter, RHIVertexBuffer& buffer);

		void setShaderValue(ShaderParameter const parameter, void const* value, int valueSize);


		template< Shader::Type TypeValue >
		void commitState( ID3D11DeviceContext* context);
		template< Shader::Type TypeValue >
		void clearState(ID3D11DeviceContext* context);

		static int constexpr MaxConstBufferNum = 1;
		uint32 mConstBufferDirtyMask;
		uint32 mConstBufferValueDirtyMask;

		ShaderConstDataBuffer mConstBuffers[MaxConstBufferNum];

		static int constexpr MaxSimulatedBoundedBufferNum = 16;
		ID3D11Buffer* mBoundedConstBuffers[MaxSimulatedBoundedBufferNum];

		static int constexpr MaxSimulatedBoundedSRVNum = 16;
		ID3D11ShaderResourceView* mBoundedSRVs[MaxSimulatedBoundedSRVNum];
		uint32 mSRVDirtyMask;

		static int constexpr MaxSimulatedBoundedUAVNum = 16;
		ID3D11ShaderResourceView* mBoundedUAVs[MaxSimulatedBoundedUAVNum];
		uint32 mUAVDirtyMask;

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
			mBuffers.push_back(BufferResource.release());
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
				if( pushNewBuffer(device, size) )
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
		std::vector< ID3D11Buffer* > mBuffers;
		std::vector< uint32 >        mBufferSizes;
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
		void RHISetViewport(int x, int y, int w, int h, float zNear, float zFar);
		void RHISetScissorRect(int x, int y, int w, int h);

		void PostDrawPrimitive();
		void RHIDrawPrimitive(EPrimitive type, int start, int nv)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
			mDeviceContext->Draw(nv, start);
			PostDrawPrimitive();
		}

		void RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
			mDeviceContext->DrawIndexed(nIndex , indexStart , baseVertex);
			PostDrawPrimitive();
		}

		void RHIDrawPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
			PostDrawPrimitive();
		}
		void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
			if( numCommand )
			{
				mDeviceContext->DrawInstancedIndirect(D3D11Cast::GetResource(*commandBuffer), offset);
			}
			else
			{
				//
			}
			PostDrawPrimitive();
		}
		void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
			mDeviceContext->DrawInstanced(nv, numInstance, vStart, baseInstance);
			PostDrawPrimitive();
		}

		void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Translate::To(type));
			mDeviceContext->DrawIndexedInstanced( nIndex , numInstance , indexStart , baseVertex , baseInstance);
			PostDrawPrimitive();
		}

		void RHIDrawPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numVertexData);

		void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numVertexData, int const* pIndices, int numIndex);


		void RHISetupFixedPipelineState(Matrix4 const& transform, LinearColor const& color , RHITexture2D* textures[], int numTexture)
		{

		}

		void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer, RHITextureDepth* overrideDepthTexture /*= nullptr*/)
		{

		}

	
		void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream)
		{
			int const MaxStreamNum = 16;
			assert(numInputStream <= MaxStreamNum);
			ID3D11Buffer* buffers[MaxStreamNum];
			UINT strides[MaxStreamNum];
			UINT offsets[MaxStreamNum];
			
			for( int i = 0; i < numInputStream; ++i )
			{
				if( inputStreams[i].buffer )
				{
					buffers[i] = D3D11Cast::GetResource(inputStreams[i].buffer);
					offsets[i] = inputStreams[i].offset;
					strides[i] = inputStreams[i].stride >= 0 ? inputStreams[i].stride : inputStreams[i].buffer->getElementSize();
				}

			}

			mInputLayout = inputLayout;
			if( numInputStream )
			{
				mDeviceContext->IASetVertexBuffers(0, numInputStream, buffers, strides, offsets);
			}		
		}
		void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer)
		{
			if( indexBuffer )
			{
				mDeviceContext->IASetIndexBuffer(D3D11Cast::To(indexBuffer)->getResource(), indexBuffer->isIntType() ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);
			}
			else
			{
				mDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
			}	
		}

		void RHIFlushCommand()
		{
			mDeviceContext->Flush();
		}


		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
		{
			commitComputeState();
			mDeviceContext->Dispatch(numGroupX, numGroupY, numGroupZ);
		}

		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);


		void commitRenderShaderState();

		void commitComputeState();
		bool determitPrimitiveTopologyUP(EPrimitive primitiveType, int num, int const* pIndices, D3D_PRIMITIVE_TOPOLOGY& outPrimitiveTopology, ID3D11Buffer** outIndexBuffer, int& outIndexNum);


		//
		template< Shader::Type TypeValue >
		void setShader(D3D11ShaderVariant const& shaderVariant);

		template < class ValueType >
		void setShaderValueT(RHIShaderProgram& shaderProgram, ShaderParameter const& param, ValueType const val[], int dim);

		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim);
		void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim);
		void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);
		void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim);

		void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView) {}

		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture);
		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler) 
		{
			setShaderTexture(shaderProgram, param, texture);
			setShaderSampler(shaderProgram, paramSampler, sampler);
		}
		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler);
		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op) {}

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer) {}
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer) {}


		uint32                mBoundedShaderMask = 0;
		uint32                mBoundedShaderDirtyMask = 0;
		D3D11ShaderVariant    mBoundedShaders[Shader::Count];
		D3D11ShaderBoundState mShaderBoundState[Shader::Count];

		D3D11DynamicBuffer    mDynamicVBuffer;
		D3D11DynamicBuffer    mDynamicIBuffer;

		RHIShader* mVertexShader = nullptr;
		TComPtr< ID3D11DeviceContext >  mDeviceContext;
		TComPtr< ID3D11Device > mDevice;
		RHIInputLayout* mInputLayout = nullptr;

	};




	class D3D11System : public RHISystem
	{
	public:
		RHISytemName getName() const { return RHISytemName::D3D11; }
		bool initialize(RHISystemInitParams const& initParam);
		void shutdown()
		{
			mRenderContext.release();
		}
		virtual ShaderFormat* createShaderFormat();



		bool RHIBeginRender()
		{
			return true;
		}

		void RHIEndRender(bool bPresent)
		{

		}

		RHICommandList&  getImmediateCommandList()
		{
			return *mImmediateCommandList;
		}
		RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info)
		{
			return nullptr;
		}
		RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel, uint32 createFlags ,
			void* data)
		{
			return nullptr;
		}

		RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 createFlags,
			void* data, int dataAlign);


		RHITexture3D*    RHICreateTexture3D(
			Texture::Format format, int sizeX, int sizeY, int sizeZ, 
			int numMipLevel, int numSamples , uint32 createFlags, 
			void* data)
		{
			return nullptr;
		}

		RHITextureCube*  RHICreateTextureCube(Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
		{

			return nullptr;
		}

		RHITexture2DArray* RHICreateTexture2DArray(Texture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
		{
			return nullptr;
		}

		RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h, int numMipLevel, int numSamples);

		RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlag, void* data);

		RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlag, void* data);


		void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIVertexBuffer* buffer);
		void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIIndexBuffer* buffer);

		RHIFrameBuffer*   RHICreateFrameBuffer()
		{
			return nullptr;
		}

		RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);
		RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);

		RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

		RHIShader* RHICreateShader(Shader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();

		bool createTexture2DInternal(DXGI_FORMAT format, int width, int height, int numMipLevel, int numSamples, uint32 creationFlag, void* data, uint32 pixelSize, bool bDepth, Texture2DCreationResult& outResult);
		void* lockBufferInternal(ID3D11Resource* resource, ELockAccess access, uint32 offset, uint32 size);


		bool createFrameSwapChain(HWND hWnd, int w, int h, bool bWindowed, FrameSwapChain& outResult)
		{
			HRESULT hr;
			TComPtr<IDXGIFactory> factory;
			hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

			DXGI_SWAP_CHAIN_DESC swapChainDesc; ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
			swapChainDesc.OutputWindow = hWnd;
			swapChainDesc.Windowed = bWindowed;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.BufferDesc.Width = w;
			swapChainDesc.BufferDesc.Height = h;
			swapChainDesc.BufferCount = 1;
			swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
			//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
			swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

			VERIFY_D3D11RESULT_RETURN_FALSE(factory->CreateSwapChain(mDevice, &swapChainDesc, &outResult.ptr));


			return true;
		}

		struct InputLayoutKey
		{
			InputLayoutKey(InputLayoutDesc const& desc)
				:elements(desc.mElements)
			{
				hash = 0x1a21df14;
				for( auto const& e : elements )
				{
					if (e.attribute == Vertex::ATTRIBUTE_UNUSED)
						continue;

					HashCombine(hash, HashValue(&e , sizeof(e)) );
				}
			}
			std::vector< InputElementDesc > elements;
			uint32 hash;

			uint32 getHash() const { return hash; }

			bool operator == (InputLayoutKey const& rhs) const
			{
				return hash == rhs.hash && elements == rhs.elements;
			}
		};

		std::unordered_map< InputLayoutKey, RHIInputLayoutRef , MemberFuncHasher > mInputLayoutMap;
		D3D11Context   mRenderContext;
		FrameSwapChain mSwapChain;
		TComPtr< ID3D11Device > mDevice;
		TComPtr< ID3D11DeviceContext > mDeviceContext;
		RHICommandListImpl* mImmediateCommandList = nullptr;
	};



}//namespace Render


#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

#endif // D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B
