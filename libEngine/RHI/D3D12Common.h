#pragma once
#ifndef D3D12Common_H_50BC1EFB_7DF9_4106_940C_B7F26AC43893
#define D3D12Common_H_50BC1EFB_7DF9_4106_940C_B7F26AC43893

#include "RHICommon.h"

#include "LogSystem.h"
#include "Platform/Windows/ComUtility.h"

#include "FixString.h"

#include "D3D12.h"

#include <unordered_map>

#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D12RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hr = CODE; if( hr != S_OK ){ ERROR_MSG_GENERATE( hr , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D12RESULT( CODE , ERRORCODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D12RESULT_RETURN_FALSE( CODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )


#define SAFE_RELEASE( PTR ) if ( PTR ){ PTR->Release(); PTR = nullptr; }

namespace Render
{
	class D3D12Resource : public RHIResource
	{

	};

	template< class RHITextureType >
	struct TD3D12TypeTraits { typedef void ImplType; };

	class D3D12Texture2D;
	class D3D12TextureDepth;
	class D3D12VertexBuffer;
	class D3D12IndexBuffer;
	class D3D12UniformBuffer;
	class D3D12RasterizerState;
	class D3D12BlendState;
	class D3D12InputLayout;
	class D3D12ShaderResourceView;
	class D3D12SamplerState;
	class D3D12DepthStencilState;

	struct D3D12Translate
	{
		static D3D12_PRIMITIVE_TOPOLOGY To(EPrimitive type);
		static DXGI_FORMAT To(Vertex::Format format, bool bNormalized);
		static DXGI_FORMAT To(Texture::Format format);
		static DXGI_FORMAT To(Texture::DepthFormat format);
		static D3D12_BLEND To(Blend::Factor factor);
		static D3D12_BLEND_OP To(Blend::Operation op);
		static D3D12_CULL_MODE To(ECullMode mode);
		static D3D12_FILL_MODE To(EFillMode mode);
		static D3D12_FILTER To(Sampler::Filter filter);
		static D3D12_TEXTURE_ADDRESS_MODE To(Sampler::AddressMode mode);
		static D3D12_COMPARISON_FUNC To(ECompareFunc func);
		static D3D12_STENCIL_OP To(Stencil::Operation op);
	};

	template< class RHIResoureType >
	class TD3D12Resource : public RHIResoureType
	{
	public:

		~TD3D12Resource()
		{
			if (mResource != nullptr)
			{
				LogWarning(0, "D3D11Resource no release");
			}
		}
		virtual void incRef() 
		{ 
#if _DEBUG
			++mCount;
#endif
			mResource->AddRef(); 
		}
		virtual bool decRef() 
		{ 
#if _DEBUG
			--mCount;
			int count = mResource->Release();
			assert(mCount + 1 == count);
			return count == 1;
#else
			return mResource->Release() == 1; 
#endif
		}
		virtual void releaseResource() 
		{
#if _DEBUG
			if (mCount != 0)
			{
				LogWarning(0, "D3D11Resource refcount error");
			}
#endif
			mResource->Release();
			mResource = nullptr; 
		}

		typedef typename TD3D12TypeTraits< RHIResoureType >::ResourceType ResourceType;

		ResourceType* getResource() { return mResource; }

#if _DEBUG
		int mCount = 0;
#endif
		ResourceType* mResource;
	};

#if 0

	class D3D12ShaderResourceView : public TD3D12Resource< RHIShaderResourceView >
	{
	public:
		D3D12ShaderResourceView(ID3D12ShaderResourceView* resource)
		{
			mResource = resource;
		}
	};

	template< class RHITextureType >
	class TD3D12Texture : public TD3D12Resource< RHITextureType >
	{
	protected:
		TD3D11Texture(ID3D11RenderTargetView* RTV, ID3D11ShaderResourceView* SRV, ID3D11UnorderedAccessView* UAV)
			:mSRV(SRV), mRTV(RTV), mUAV(UAV)
		{
			if ( SRV )
				SRV->AddRef();
		}

		virtual void releaseResource()
		{
			SAFE_RELEASE(mSRV.mResource);
			SAFE_RELEASE(mRTV);
			
			SAFE_RELEASE(mUAV);
			TD3D11Resource< RHITextureType >::releaseResource();
		}
		virtual RHIShaderResourceView* getBaseResourceView() { return &mSRV; }
	public:
		D3D12ShaderResourceView  mSRV;
		ID3D12RenderTargetView*  mRTV;
		ID3D12UnorderedAccessView* mUAV;
	};

	struct Texture2DCreationResult
	{
		TComPtr< ID3D11Texture2D > resource;
		TComPtr< ID3D11RenderTargetView >    RTV;
		TComPtr< ID3D11ShaderResourceView >  SRV;
		TComPtr< ID3D11UnorderedAccessView > UAV;
	};

	class D3D11Texture2D : public TD3D12Texture< RHITexture2D >
	{
	public:
		D3D11Texture2D(Texture::Format format, Texture2DCreationResult& creationResult)
			:TD3D11Texture< RHITexture2D >(creationResult.RTV.release(), creationResult.SRV.release(), creationResult.UAV.release())
		{
			mFormat = format;
			mResource = creationResult.resource.release();
			D3D11_TEXTURE2D_DESC desc;
			mResource->GetDesc(&desc);
			mSizeX = desc.Width;
			mSizeY = desc.Height;
			mNumSamples = desc.SampleDesc.Count;
			mNumMipLevel = desc.MipLevels;
		}


		bool update(int ox, int oy, int w, int h, Texture::Format format, void* data, int level)
		{
			TComPtr<ID3D11Device> device;
			mResource->GetDevice(&device);
			TComPtr<ID3D11DeviceContext> deviceContext;
			device->GetImmediateContext(&deviceContext);
			D3D11_BOX box;
			box.back = 0;
			box.front = 1;
			box.left = ox;
			box.right = ox + w;
			box.top = oy;
			box.bottom = oy + h;
			deviceContext->UpdateSubresource(mResource, level, &box, data, w * Texture::GetFormatSize(format), w * h * Texture::GetFormatSize(format));
			return true;
		}

		bool update(int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level)
		{
			TComPtr<ID3D11Device> device;
			mResource->GetDevice(&device);
			TComPtr<ID3D11DeviceContext> deviceContext;
			device->GetImmediateContext(&deviceContext);
			D3D11_BOX box;
			box.back = 0;
			box.front = 1;
			box.left = ox;
			box.right = ox + w;
			box.top = oy;
			box.bottom = oy + h;
			//@FIXME : error
			deviceContext->UpdateSubresource(mResource, level, &box, data, dataImageWidth * Texture::GetFormatSize(format), h * dataImageWidth * Texture::GetFormatSize(format));
			return true;
		}


	};


	class D3D11TextureDepth : public TD3D11Resource< RHITextureDepth >
	{
	public:
		D3D11TextureDepth(Texture::DepthFormat format, Texture2DCreationResult& creationResult)
			:mSRV( creationResult.SRV.release())
		{
			mFormat = format;
			mResource = creationResult.resource.release();
			D3D11_TEXTURE2D_DESC desc;
			mResource->GetDesc(&desc);
			mSizeX = desc.Width;
			mSizeY = desc.Height;
			mNumSamples = desc.SampleDesc.Count;
			mNumMipLevel = desc.MipLevels;

			if( mSRV.getResource() )
				mSRV.getResource()->AddRef();

			TComPtr<ID3D11Device> device;
			mResource->GetDevice(&device);
			HRESULT hr = device->CreateDepthStencilView(mResource, nullptr, &mDSV);
			if( hr != S_OK )
			{
				int i = 1;
			}
		}

		virtual void releaseResource()
		{
			SAFE_RELEASE(mSRV.mResource);
			SAFE_RELEASE(mDSV);
			TD3D11Resource< RHITextureDepth >::releaseResource();
		}

		D3D11ShaderResourceView  mSRV;
		ID3D11DepthStencilView*  mDSV;
	};

	template< class RHIBufferType >
	class TD3D11Buffer : public TD3D11Resource< RHIBufferType >
	{
	public:
		TD3D11Buffer(ID3D11Device* device, ID3D11Buffer* resource, uint32 creationFlags)
		{
			mResource = resource;
			mCreationFlags = creationFlags;
			if( creationFlags & BCF_CreateSRV )
			{
				HRESULT hr = device->CreateShaderResourceView(resource, NULL, &mSRVResource);
				if( hr != S_OK )
				{
					LogWarning(0, "Can't Create buffer's SRV ! error code : %d", hr);
				}
			}
			if( creationFlags & BCF_CreateUAV )
			{
				device->CreateUnorderedAccessView(resource, NULL, &mUAVResource);
			}
		}
		TComPtr< ID3D11ShaderResourceView > mSRVResource;
		TComPtr< ID3D11UnorderedAccessView > mUAVResource;
	};


	class D3D11VertexBuffer : public TD3D11Buffer< RHIVertexBuffer >
	{
	public:
		using TD3D11Buffer< RHIVertexBuffer >::TD3D11Buffer;
		D3D11VertexBuffer(ID3D11Device* device, ID3D11Buffer* resource, int numVertices , int vertexSize , uint32 creationFlags)
			:TD3D11Buffer< RHIVertexBuffer >(device, resource, creationFlags)
		{
			mNumElements = numVertices;
			mElementSize = vertexSize;
		}
		virtual void  resetData(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
		{


		}
		virtual void  updateData(uint32 vStart, uint32 numVertices, void* data)
		{
			TComPtr<ID3D11Device> device;
			mResource->GetDevice(&device);

			TComPtr<ID3D11DeviceContext> context;
			device->GetImmediateContext(&context);

			D3D11_BOX box = { 0 };
			box.left  = vStart * mElementSize;
			box.right = vStart + numVertices * mElementSize;
			context->UpdateSubresource(mResource, 0, &box, data, 0, 0);
		}
	};

	class D3D11IndexBuffer : public TD3D11Buffer< RHIIndexBuffer >
	{
	public:
		D3D11IndexBuffer(ID3D11Device* device, ID3D11Buffer* resource, int numIndices, bool bIntType , uint32 creationFlags)
			:TD3D11Buffer< RHIIndexBuffer >(device, resource, creationFlags)
		{
			mNumElements = numIndices;
			mElementSize = bIntType ? 4 : 2;
		}

	};

	class D3D11RasterizerState : public TD3D11Resource< RHIRasterizerState >
	{
	public:
		D3D11RasterizerState(ID3D11RasterizerState* inResource)
		{
			mResource = inResource;
		}
	};

	class D3D11BlendState : public TD3D11Resource< RHIBlendState >
	{
	public:
		D3D11BlendState(ID3D11BlendState* inResource)
		{
			mResource = inResource;
		}
	};


	class D3D11DepthStencilState : public TD3D11Resource< RHIDepthStencilState >
	{
	public:
		D3D11DepthStencilState(ID3D11DepthStencilState* inResource)
		{
			mResource = inResource;
		}
	};

	class D3D11SamplerState : public TD3D11Resource< RHISamplerState >
	{
	public:
		D3D11SamplerState(ID3D11SamplerState* inResource)
		{
			mResource = inResource;
		}

	};

	class D3D11InputLayout : public RHIInputLayout
	{
	public:
		D3D11InputLayout()
		{
			mRefcount = 0;
		}

		void incRef() override
		{
			++mRefcount;

		}
		bool decRef() override
		{
			--mRefcount;
			return mRefcount <= 0;
		}

		void releaseResource() override
		{
			for (auto& pair : mResourceMap)
			{
				pair.second->Release();
			}
			mResourceMap.clear();
			mResource->Release();
			mResource = nullptr;
		}

		ID3D12InputLayout* GetShaderLayout( ID3D11Device* device , RHIShader* shader);

		int mRefcount;
		std::vector< D3D11_INPUT_ELEMENT_DESC > elements;
		ID3D12InputLayout* mResource;
		std::unordered_map< RHIShader*, ID3D11InputLayout* > mResourceMap;
	};

	struct D3D12Cast
	{
		template< class TRHIResource >
		static auto* To(TRHIResource* resource) { return static_cast< typename TD3D12TypeTraits< TRHIResource >::ImplType*>(resource); }
		
		template< class TRHIResource >
		static auto& To(TRHIResource& resource) { return static_cast<typename TD3D12TypeTraits< TRHIResource >::ImplType& >(resource); }

		template < class T >
		static auto* To(TRefCountPtr<T>& ptr) { return D3D12Cast::To(ptr.get()); }

		template< class T >
		static auto GetResource(T& RHIObject) { return D3D12Cast::To(&RHIObject)->getResource(); }

		template< class T >
		static auto GetResource(T* RHIObject) { return RHIObject ? D3D12Cast::To(RHIObject)->getResource() : nullptr; }

		template< class T >
		static auto GetResource(TRefCountPtr<T>& refPtr) { return D3D12Cast::To(refPtr)->getResource(); }
	};


	class FD3D12Utility
	{
	public:
		static FixString<32> GetShaderProfile( ID3D12Device* device , Shader::Type type);
	};

	union D3D12ShaderVariant
	{
		ID3D12DeviceChild*    resource;

		ID3D12VertexShader*   vertex;
		ID3D12PixelShader*    pixel;
		ID3D12GeometryShader* geometry;
		ID3D12ComputeShader*  compute;
		ID3D12HullShader*     hull;
		ID3D12DomainShader*   domain;
	};
#endif
}

#endif // D3D12Common_H_50BC1EFB_7DF9_4106_940C_B7F26AC43893