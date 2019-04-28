#pragma once
#ifndef D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B
#define D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B

#include "RHICommand.h"
#include "RHICommandListImpl.h"
#include "ShaderCore.h"

#include "D3D11Common.h"
#include "FixString.h"
#include "Core/ScopeExit.h"


#include <D3D11Shader.h>


#include <D3Dcompiler.h>
#include <D3DX11async.h>

#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "D3DX11.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")


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
			VERIFY_D3D11RESULT_RETURN_FALSE(device->CreateBuffer(&bufferDesc, NULL, &resource));
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
				context->UpdateSubresource(resource, 0, NULL, &updateData[0], updateDataSize, updateDataSize);
				updateDataSize = 0;
			}
		}
	};


	struct D3D11ShaderBoundState
	{
		static int constexpr MaxConstBufferNum = 1;
		
		D3D11ShaderBoundState()
		{
			
		}

		bool initialize(TComPtr< ID3D11Device >& device, TComPtr<ID3D11DeviceContext >& deviceContext)
		{
			::memset(this, 0, sizeof(*this));
			for( int i = 0; i < MaxConstBufferNum; ++i )
			{
				if( !mConstBuffers[i].initializeResource(device) )
				{
					return false;
				}
			}

			return true;
		}
		uint32 mConstBufferDirtyMask;
		uint32 mConstBufferValueDirtyMask;

		ShaderConstDataBuffer mConstBuffers[MaxConstBufferNum];

		static int constexpr MaxSimulatedBoundedBufferNum = 16;
		ID3D11Buffer* mBoundedBuffers[MaxSimulatedBoundedBufferNum];

		static int constexpr MaxSimulatedBoundedSRVNum = 16;
		ID3D11ShaderResourceView* mBoundedSRVs[MaxSimulatedBoundedSRVNum];
		uint32 mSRVDirtyMask;

		static int constexpr MaxSimulatedBoundedUAVNum = 16;
		ID3D11ShaderResourceView* mBoundedUAVs[MaxSimulatedBoundedUAVNum];
		uint32 mUAVDirtyMask;

		static int constexpr MaxSimulatedBoundedSamplerNum = 16;
		ID3D11SamplerState* mBoundedSamplers[MaxSimulatedBoundedSamplerNum];
		uint32 mSamplerDirtyMask;

		void clear()
		{
			mConstBufferValueDirtyMask = 0;
			mConstBufferDirtyMask = 0;
			mSRVDirtyMask = 0;
			mUAVDirtyMask = 0;
			mSamplerDirtyMask = 0;
		}

		void setTexture(ShaderParameter const& parameter, RHITextureBase& texture)
		{
			auto resViewImpl = static_cast<D3D11ShaderResourceView*>( texture.getBaseResourceView() );
			if( resViewImpl )
			{
				if( mBoundedSRVs[parameter.mLoc] != resViewImpl->getResource() )
				{
					mBoundedSRVs[parameter.mLoc] = resViewImpl->getResource();
					mSRVDirtyMask |= BIT(parameter.mLoc);
				}
			}
		}

		void setUniformBuffer(ShaderParameter const& parameter, RHITextureBase& bufferResource)
		{

		}

		void setShaderValue(ShaderParameter const parameter, void const* value, int valueSize)
		{
			assert(parameter.bindIndex < MaxConstBufferNum);
			mConstBuffers[parameter.bindIndex].setUpdateValue(parameter, value, valueSize);
			mConstBufferValueDirtyMask |= BIT(parameter.bindIndex);
			if( mConstBuffers[parameter.bindIndex].resource.get() != mBoundedBuffers[parameter.bindIndex] )
			{
				mBoundedBuffers[parameter.bindIndex] = mConstBuffers[parameter.bindIndex].resource.get();
				mConstBufferDirtyMask |= BIT(parameter.bindIndex);
			}
		}


		template< Shader::Type TypeValue >
		void commitState( ID3D11DeviceContext* context);

	};


	class D3D11Context : public RHIContext
	{
	public:

		bool initialize(TComPtr< ID3D11Device >&  device , TComPtr<ID3D11DeviceContext >& deviceContext)
		{
			mDeviceContext = deviceContext;
			for( int i = 0; i < Shader::Count; ++i )
			{
				mBoundedShaders[i].resource = nullptr;
				mShaderBoundState[i].initialize(device, deviceContext);
			}
			return true;
		}
		void RHISetRasterizerState(RHIRasterizerState& rasterizerState)
		{
			mDeviceContext->RSSetState(D3D11Cast::GetResource(rasterizerState));
		}

		void RHISetBlendState(RHIBlendState& blendState)
		{
			mDeviceContext->OMSetBlendState(D3D11Cast::GetResource(blendState), Vector4(0, 0, 0, 0), 0xffffffff);
		}

		void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
		{

		}

		void RHISetViewport(int x, int y, int w, int h)
		{
			D3D11_VIEWPORT vp;
			vp.Width = w;
			vp.Height = h;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = x;
			vp.TopLeftY = y;
			mDeviceContext->RSSetViewports(1, &vp);
		}

		void RHISetScissorRect(int x, int y, int w, int h)
		{
			D3D11_RECT rect;
			rect.top = x;
			rect.left = y;
			rect.bottom = x + w;
			rect.right = y + h;
			mDeviceContext->RSSetScissorRects(1, &rect);
		}

		void RHIDrawPrimitive(PrimitiveType type, int start, int nv)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Conv::To(type));
			mDeviceContext->Draw(nv, start);
		}

		void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex, uint32 baseVertex)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Conv::To(type));
			mDeviceContext->DrawIndexed(nIndex , indexStart , baseVertex);
		}

		void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Conv::To(type));
		}
		void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, ECompValueType indexType, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
		{
			commitRenderShaderState();
			mDeviceContext->IASetPrimitiveTopology(D3D11Conv::To(type));
			if( numCommand )
			{
				mDeviceContext->DrawInstancedIndirect(D3D11Cast::GetResource(*commandBuffer), offset);
			}
			else
			{
				//
			}
		}
		void RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, int numInstance)
		{
			commitRenderShaderState();
			mDeviceContext->DrawInstanced(nv, numInstance, vStart, 0);
		}

		void RHIDrawPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride)
		{
			commitRenderShaderState();


		}

		void RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride, int* pIndices, int numIndex)
		{
			commitRenderShaderState();
		}


		void RHISetupFixedPipelineState(Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture, RHITexture2D const** textures)
		{

		}

		void RHISetFrameBuffer(RHIFrameBuffer& frameBuffer, RHITextureDepth* overrideDepthTexture /*= nullptr*/)
		{

		}

		void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer)
		{
			mDeviceContext->IASetIndexBuffer(D3D11Cast::To(indexBuffer)->getResource(), indexBuffer->getSize() == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);
		}

		void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
		{
			commitComputeState();
			mDeviceContext->Dispatch(numGroupX, numGroupY, numGroupZ);
		}

		void RHISetShaderProgram(RHIShaderProgram* shaderProgram);


		void commitRenderShaderState();

		void commitComputeState();


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
		void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler) {}
		void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState const& sampler) {}
		void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op) {}

		void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer) {}
		void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer) {}
		void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer) {}

		uint32                mBoundedShaderMask = 0;
		uint32                mBoundedShaderDirtyMask = 0;
		D3D11ShaderVariant    mBoundedShaders[Shader::Count];
		D3D11ShaderBoundState mShaderBoundState[Shader::Count];
		TComPtr< ID3D11DeviceContext >  mDeviceContext;
	};


	class D3D11System : public RHISystem
	{
	public:
		RHISytemName getName() const { return RHISytemName::D3D11; }
		bool initialize(RHISystemInitParam const& initParam);
		void shutdown(){}
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

		RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h, int numMipLevel, int numSamples)
		{
			return nullptr;
		}

		RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlag, void* data);

		RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlag, void* data)
		{

			return nullptr;
		}


		void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIVertexBuffer* buffer);
		void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size);
		void  RHIUnlockBuffer(RHIIndexBuffer* buffer);

		RHIFrameBuffer*   RHICreateFrameBuffer()
		{
			return nullptr;
		}

		RHIInputLayout*   RHICreateInputLayout(InputLayoutDesc const& desc);

		RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer)
		{
			return nullptr;
		}

		RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
		RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);

		RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
		{
			return nullptr;
		}

		RHIShader* RHICreateShader(Shader::Type type);
		RHIShaderProgram* RHICreateShaderProgram();
		void* lockBufferInternal(ID3D11Resource* resource, ELockAccess access, uint32 offset, uint32 size)
		{
			D3D11_MAPPED_SUBRESOURCE mappedData;
			mDeviceContext->Map(resource, 0, D3D11Conv::To(access), 0, &mappedData);
			return (uint8*)mappedData.pData + offset;
		}

		bool createTexture2DInternal( DXGI_FORMAT format, int width, int height, int numMipLevel, int numSamples, uint32 creationFlag, void* data, uint32 pixelSize , Texture2DCreationResult& outResult );

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


		bool compileShader(Shader::Type type, char const* code, int codeLen, char const* entryName, ShaderParameterMap& parameterMap, D3D11ShaderVariant& shaderResult, TComPtr< ID3D10Blob >* pOutbyteCode = nullptr)
		{

			TComPtr< ID3D10Blob > errorCode;
			TComPtr< ID3D10Blob > byteCode;
			FixString<32> profileName = FD3D11Utility::GetShaderProfile( mDevice , type );

			uint32 compileFlag = 0 /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
			VERIFY_D3D11RESULT(
				D3DX11CompileFromMemory(code, codeLen, "ShaderCode", NULL, NULL, entryName, profileName, compileFlag, 0, NULL, &byteCode, &errorCode, NULL),
				{
					LogWarning(0, "Compile Error %s", errorCode->GetBufferPointer());
			return false;
				}
			);

			TComPtr< ID3D11ShaderReflection > reflection;
			VERIFY_D3D11RESULT_RETURN_FALSE(D3DReflect(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflection.mPtr));

			D3D11_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);
			for( int i = 0; i < shaderDesc.BoundResources; ++i )
			{
				D3D11_SHADER_INPUT_BIND_DESC bindDesc;
				reflection->GetResourceBindingDesc(i, &bindDesc);

				switch( bindDesc.Type )
				{
				case D3D11_CT_CBUFFER:
				case D3D11_CT_TBUFFER:
					{
						ID3D11ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByName(bindDesc.Name);
						assert(buffer);
						D3D11_SHADER_BUFFER_DESC bufferDesc;
						buffer->GetDesc(&bufferDesc);
						if( FCString::Compare(bufferDesc.Name, "$Globals") == 0 )
						{
							for( int idxVar = 0; idxVar < bufferDesc.Variables; ++idxVar )
							{
								ID3D11ShaderReflectionVariable* var = buffer->GetVariableByIndex(idxVar);
								if( var )
								{
									D3D11_SHADER_VARIABLE_DESC varDesc;
									var->GetDesc(&varDesc);
									parameterMap.addParameter(varDesc.Name, bindDesc.BindPoint, varDesc.StartOffset, varDesc.Size);
								}
							}
						}
						else
						{
							//CBuffer TBuffer
							parameterMap.addParameter(bufferDesc.Name, bindDesc.BindPoint, 0, 0);
						}
					}
					break;
				case D3D11_CT_RESOURCE_BIND_INFO:
					{
						parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
					}
					break;
				case D3D_SIT_TEXTURE:
					{
						parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
					}
					break;
				}
			}

			switch( type )
			{
#define CASE_SHADER(  TYPE , FUN ,VAR )\
			case TYPE:\
				VERIFY_D3D11RESULT_RETURN_FALSE( mDevice->FUN(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), NULL, &VAR) );\
				break;

				CASE_SHADER(Shader::eVertex, CreateVertexShader, shaderResult.vertex);
				CASE_SHADER(Shader::ePixel, CreatePixelShader, shaderResult.pixel);
				CASE_SHADER(Shader::eGeometry, CreateGeometryShader, shaderResult.geometry);
				CASE_SHADER(Shader::eCompute, CreateComputeShader, shaderResult.compute);
				CASE_SHADER(Shader::eHull, CreateHullShader, shaderResult.hull);
				CASE_SHADER(Shader::eDomain, CreateDomainShader, shaderResult.domain);

#undef CASE_SHADER
			default:
				assert(0);
			}

			if( pOutbyteCode )
				*pOutbyteCode = byteCode;

			return true;
		}


		D3D11Context   mRenderContext;
		FrameSwapChain mSwapChain;
		TComPtr< ID3D11Device > mDevice;
		TComPtr< ID3D11DeviceContext > mDeviceContext;
		RHICommandListImpl* mImmediateCommandList = nullptr;
	};



}//namespace Render

#endif // D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B
