#include "RHICommon.h"

#include "LogSystem.h"
#include "Platform/Windows/ComUtility.h"

#include "FixString.h"

#include <D3D11.h>
#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

#include <unordered_map>

#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D11RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hr = CODE; if( hr != S_OK ){ ERROR_MSG_GENERATE( hr , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D11RESULT( CODE , ERRORCODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D11RESULT_RETURN_FALSE( CODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )


#define SAFE_RELEASE( PTR ) if ( PTR ){ PTR->Release(); PTR = nullptr; }

namespace Render
{
	class D3D11Resource : public RHIResource
	{

	};

	template< class RHITextureType >
	struct TD3D11TypeTraits { typedef void ImplType; };

	class D3D11Texture1D;
	class D3D11Texture2D;
	class D3D11Texture3D;
	class D3D11TextureDepth;
	class D3D11VertexBuffer;
	class D3D11IndexBuffer;
	class D3D11UniformBuffer;
	class D3D11RasterizerState;
	class D3D11BlendState;
	class D3D11InputLayout;
	class D3D11ShaderResourceView;
	class D3D11SamplerState;
	class D3D11DepthStencilState;
	class D3D11SwapChain;

	template<>
	struct TD3D11TypeTraits< RHITexture1D > 
	{ 
		typedef ID3D11Texture1D ResourceType; 
		typedef D3D11Texture1D ImplType;
	};
	template<>
	struct TD3D11TypeTraits< RHITexture2D >
	{  
		typedef ID3D11Texture2D ResourceType;  
		typedef D3D11Texture2D ImplType;  
	};
	template<>
	struct TD3D11TypeTraits< RHITexture3D > 
	{ 
		typedef ID3D11Texture3D ResourceType; 
		typedef D3D11Texture3D ImplType;
	};
	//template<>
	//struct TD3D11TypeTraits< RHITextureCube > { typedef ID3D11TextureArray ResourceType; };
	template<>
	struct TD3D11TypeTraits< RHITextureDepth > 
	{ 
		typedef ID3D11Texture2D ResourceType; 
		typedef D3D11TextureDepth ImplType;
	};
	template<>
	struct TD3D11TypeTraits< RHIVertexBuffer > 
	{ 
		typedef ID3D11Buffer ResourceType;
		typedef D3D11VertexBuffer ImplType; 
	};
	template<>
	struct TD3D11TypeTraits< RHIIndexBuffer >
	{
		typedef ID3D11Buffer ResourceType; 
		typedef D3D11IndexBuffer ImplType;
	};
	template<>
	struct TD3D11TypeTraits< RHIRasterizerState > 
	{ 
		typedef ID3D11RasterizerState ResourceType;
		typedef D3D11RasterizerState ImplType;
	};
	template<>
	struct TD3D11TypeTraits< RHIBlendState > 
	{ 
		typedef ID3D11BlendState ResourceType;
		typedef D3D11BlendState ImplType;
	};
	template<>
	struct TD3D11TypeTraits< RHIInputLayout > 
	{ 
		typedef ID3D11InputLayout ResourceType;
		typedef D3D11InputLayout ImplType;
	};
	template<>
	struct TD3D11TypeTraits< RHIShaderResourceView >
	{
		typedef ID3D11ShaderResourceView ResourceType;
		typedef D3D11ShaderResourceView ImplType;
	};
	template<>
	struct TD3D11TypeTraits< RHISamplerState >
	{
		typedef ID3D11SamplerState ResourceType;
		typedef D3D11SamplerState ImplType;
	};
	template<>
	struct TD3D11TypeTraits< RHIDepthStencilState >
	{
		typedef ID3D11DepthStencilState ResourceType;
		typedef D3D11DepthStencilState ImplType;
	};

	template<>
	struct TD3D11TypeTraits< RHISwapChain >
	{
		typedef IDXGISwapChain ResourceType;
		typedef D3D11SwapChain ImplType;
	};

	struct D3D11Translate
	{
		static D3D_PRIMITIVE_TOPOLOGY To(EPrimitive type);
		static DXGI_FORMAT To(Vertex::Format format, bool bNormalized);
		static DXGI_FORMAT To(Texture::Format format);
		static DXGI_FORMAT To(Texture::DepthFormat format);
		static D3D11_BLEND To(Blend::Factor factor);
		static D3D11_BLEND_OP To(Blend::Operation op);
		static D3D11_CULL_MODE To(ECullMode mode);
		static D3D11_FILL_MODE To(EFillMode mode);
		static D3D11_MAP To(ELockAccess access);
		static D3D11_FILTER To(Sampler::Filter filter);
		static D3D11_TEXTURE_ADDRESS_MODE To(Sampler::AddressMode mode);
		static D3D11_COMPARISON_FUNC To(ECompareFunc func);
		static D3D11_STENCIL_OP To(Stencil::Operation op);
	};

#define USE_D3D11_RESOURCE_CUSTOM_COUNT 1

	template< class RHIResoureType >
	class TD3D11Resource : public RHIResoureType
	{
	public:

		~TD3D11Resource()
		{
			if (mResource != nullptr)
			{
				LogWarning(0, "D3D11Resource no release");
			}
		}

		int getRefCount() const
		{
			if (mResource)
			{
				mResource->AddRef();
				return mResource->Release();
			}
			return 0;
		}

		virtual void incRef() 
		{ 
#if USE_D3D11_RESOURCE_CUSTOM_COUNT
			++mCount;
#else
#if _DEBUG
			++mCount;
#endif
			int count = mResource->AddRef();
			assert(mCount + 1 <= count);
#endif
		}
		virtual bool decRef() 
		{ 
#if USE_D3D11_RESOURCE_CUSTOM_COUNT
			--mCount;
			return mCount == 0;
#else
#if _DEBUG
			--mCount;
			int count = mResource->Release();
			assert(mCount + 1 <= count);
			return count == 1;
#else
			return mResource->Release() == 1; 
#endif
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
			
#if USE_D3D11_RESOURCE_CUSTOM_COUNT
			int count = mResource->Release();
			if (count > 0)
			{
#if USE_RHI_RESOURCE_TRACE
				if (mTag)
				{
					LogDevMsg(0, "D3D11Resource: %p %p %d %s : %s , %s", this , mResource , count, mTypeName.c_str(), mTag, mTrace.toString().c_str());
				}
				else
				{
					LogDevMsg(0, "D3D11Resource: %p %p %d %s : %s", this, mResource, count, mTypeName.c_str(), mTrace.toString().c_str());
				}
#endif
			}
#else
			mResource->Release();
#endif
			mResource = nullptr; 
		}

		typedef typename TD3D11TypeTraits< RHIResoureType >::ResourceType ResourceType;

		ResourceType* getResource() { return mResource; }


		void release()
		{
			if (mResource)
			{
				releaseResource();
			}
		}

#if _DEBUG || USE_D3D11_RESOURCE_CUSTOM_COUNT
		int mCount = 0;
#endif
		ResourceType* mResource;
	};


	class D3D11ShaderResourceView : public TD3D11Resource< RHIShaderResourceView >
	{
	public:
		D3D11ShaderResourceView(ID3D11ShaderResourceView* resource)
		{
			mResource = resource;
		}
	};

	template< class RHITextureType >
	class TD3D11Texture : public TD3D11Resource< RHITextureType >
	{
	protected:
		TD3D11Texture(ID3D11RenderTargetView* RTV, ID3D11ShaderResourceView* SRV, ID3D11UnorderedAccessView* UAV)
			:mSRV(SRV), mRTV(RTV), mUAV(UAV)
		{

		}

		virtual void releaseResource()
		{
			mSRV.release();
			SAFE_RELEASE(mRTV);
			SAFE_RELEASE(mUAV);
			TD3D11Resource< RHITextureType >::releaseResource();
		}
		virtual RHIShaderResourceView* getBaseResourceView() { return &mSRV; }
	public:
		D3D11ShaderResourceView  mSRV;
		ID3D11RenderTargetView*  mRTV;
		ID3D11UnorderedAccessView* mUAV;
	};



	struct Texture1DCreationResult
	{
		TComPtr< ID3D11Texture1D > resource;
		TComPtr< ID3D11RenderTargetView >    RTV;
		TComPtr< ID3D11ShaderResourceView >  SRV;
		TComPtr< ID3D11UnorderedAccessView > UAV;
	};

	class D3D11Texture1D : public TD3D11Texture< RHITexture1D >
	{
	public:
		D3D11Texture1D(Texture::Format format, Texture1DCreationResult& creationResult)
			:TD3D11Texture< RHITexture1D >(creationResult.RTV.release(), creationResult.SRV.release(), creationResult.UAV.release())
		{
			mFormat = format;
			mResource = creationResult.resource.release();
			D3D11_TEXTURE1D_DESC desc;
			mResource->GetDesc(&desc);
			mSize = desc.Width;
			mNumSamples = 1;
			mNumMipLevel = desc.MipLevels;
		}

		virtual bool update(int offset, int length, Texture::Format format, void* data, int level = 0)
		{

			TComPtr<ID3D11Device> device;
			mResource->GetDevice(&device);
			TComPtr<ID3D11DeviceContext> deviceContext;
			device->GetImmediateContext(&deviceContext);
			D3D11_BOX box;
			box.front = 0;
			box.back = 1;
			box.left = offset;
			box.right = offset + length;
			box.top = 0;
			box.bottom = 1;
			deviceContext->UpdateSubresource(mResource, level, &box, data, length * Texture::GetFormatSize(format), length * Texture::GetFormatSize(format));
			return true;

		}
	};

	struct Texture2DCreationResult
	{
		TComPtr< ID3D11Texture2D > resource;
		TComPtr< ID3D11RenderTargetView >    RTV;
		TComPtr< ID3D11ShaderResourceView >  SRV;
		TComPtr< ID3D11UnorderedAccessView > UAV;
	};

	class D3D11Texture2D : public TD3D11Texture< RHITexture2D >
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
			if (format != mFormat)
			{
				int i = 1;
			}
			TComPtr<ID3D11Device> device;
			mResource->GetDevice(&device);
			TComPtr<ID3D11DeviceContext> deviceContext;
			device->GetImmediateContext(&deviceContext);
			D3D11_BOX box;
			box.front = 0;
			box.back = 1;
			box.left = ox;
			box.right = ox + w;
			box.top = oy;
			box.bottom = oy + h;
			deviceContext->UpdateSubresource(mResource, level, &box, data, w * Texture::GetFormatSize(format), w * h * Texture::GetFormatSize(format));
			return true;
		}

		bool update(int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level)
		{
			if (format != mFormat)
			{
				int i = 1;
			}
			TComPtr<ID3D11Device> device;
			mResource->GetDevice(&device);
			TComPtr<ID3D11DeviceContext> deviceContext;
			device->GetImmediateContext(&deviceContext);
			D3D11_BOX box;
			box.front = 0;
			box.back = 1;
			box.left = ox;
			box.right = ox + w;
			box.top = oy;
			box.bottom = oy + h;
			//@FIXME : error
			deviceContext->UpdateSubresource(mResource, level, &box, data, dataImageWidth * Texture::GetFormatSize(format), h * dataImageWidth * Texture::GetFormatSize(format));
			return true;
		}
	};
	struct Texture3DCreationResult
	{
		TComPtr< ID3D11Texture3D > resource;
		TComPtr< ID3D11RenderTargetView >    RTV;
		TComPtr< ID3D11ShaderResourceView >  SRV;
		TComPtr< ID3D11UnorderedAccessView > UAV;
	};
	class D3D11Texture3D : public TD3D11Texture< RHITexture3D >
	{
	public:
		D3D11Texture3D(Texture::Format format, Texture3DCreationResult& creationResult)
			:TD3D11Texture< RHITexture3D >(creationResult.RTV.release(), creationResult.SRV.release(), creationResult.UAV.release())
		{
			mFormat = format;
			mResource = creationResult.resource.release();
			D3D11_TEXTURE3D_DESC desc;
			mResource->GetDesc(&desc);
			mSizeX = desc.Width;
			mSizeY = desc.Height;
			mSizeZ = desc.Depth;
			mNumSamples = 1;
			mNumMipLevel = desc.MipLevels;
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
			mSRV.release();
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

#if USE_RHI_RESOURCE_TRACE
		virtual void setTraceData(ResTraceInfo const& trace)
		{
			mTrace = trace;
			if (mTag)
			{
				mResource->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(mTag) , mTag );
			}
		}
#endif
		void releaseResource()
		{
			mSRVResource.reset();
			mUAVResource.reset();
			TD3D11Resource< RHIBufferType >::releaseResource();
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
			mUniversalResource->Release();
			mUniversalResource = nullptr;
		}

		bool haveAttribute(uint8 attribute) const
		{
			for (auto const& desc : mDescList)
			{
				if (desc.SemanticIndex == attribute)
					return true;
			}
			return false;
		}
		ID3D11InputLayout* GetShaderLayout( ID3D11Device* device , RHIShader* shader);

		int mRefcount;
		std::vector< D3D11_INPUT_ELEMENT_DESC > mDescList;
		ID3D11InputLayout* mUniversalResource;
		std::unordered_map< RHIShader*, ID3D11InputLayout* > mResourceMap;
	};


	struct D3D11RenderTargetsState
	{
		static constexpr int MaxSimulationBufferCount = 8;
		ID3D11RenderTargetView* colorBuffers[MaxSimulationBufferCount];
		int numColorBuffers;
		ID3D11DepthStencilView* depthBuffer;

		D3D11RenderTargetsState()
		{
			memset(this, 0, sizeof(*this));
		}
	};

	class D3D11SwapChain : public TD3D11Resource< RHISwapChain >
	{
	public:

		D3D11SwapChain(TComPtr<IDXGISwapChain>& resource , D3D11Texture2D& colorTexture , D3D11TextureDepth* depthTexture)
			:mColorTexture(&colorTexture)
			,mDepthTexture(depthTexture)
		{
			mResource = resource.release();
			mRenderTargetsState.numColorBuffers = 1;
			mRenderTargetsState.colorBuffers[0] = mColorTexture->mRTV;
			if (mDepthTexture)
			{
				mRenderTargetsState.depthBuffer = mDepthTexture->mDSV;
			}

		}
		virtual RHITexture2D* getBackBufferTexture() override 
		{ 
			return mColorTexture; 
		}
		virtual void Present() override
		{
			mResource->Present(1, 0);
		}


		void BitbltToDevice(HDC hTargetDC)
		{
			TComPtr<IDXGISurface1> surface;
			VERIFY_D3D11RESULT(mResource->GetBuffer(0, IID_PPV_ARGS(&surface)), );
			HDC hDC;
			VERIFY_D3D11RESULT(surface->GetDC(FALSE, &hDC), );
			int w = mColorTexture->getSizeX();
			int h = mColorTexture->getSizeY();
			::BitBlt(hTargetDC, 0, 0, w, h, hDC, 0, 0, SRCCOPY);
			VERIFY_D3D11RESULT(surface->ReleaseDC(nullptr), );
		}

		virtual void releaseResource()
		{
			mColorTexture.release();
			mDepthTexture.release();

			TD3D11Resource< RHISwapChain >::releaseResource();
		}

		TRefCountPtr< D3D11Texture2D >    mColorTexture;
		TRefCountPtr< D3D11TextureDepth > mDepthTexture;
		D3D11RenderTargetsState mRenderTargetsState;

	};

	struct D3D11Cast
	{
		template< class TRHIResource >
		static auto* To(TRHIResource* resource) { return static_cast< typename TD3D11TypeTraits< TRHIResource >::ImplType*>(resource); }
		
		template< class TRHIResource >
		static auto& To(TRHIResource& resource) { return static_cast<typename TD3D11TypeTraits< TRHIResource >::ImplType& >(resource); }

		template < class T >
		static auto* To(TRefCountPtr<T>& ptr) { return D3D11Cast::To(ptr.get()); }

		template< class T >
		static auto GetResource(T& RHIObject) { return D3D11Cast::To(&RHIObject)->getResource(); }

		template< class T >
		static auto GetResource(T* RHIObject) { return RHIObject ? D3D11Cast::To(RHIObject)->getResource() : nullptr; }

		template< class T >
		static auto GetResource(TRefCountPtr<T>& refPtr) { return D3D11Cast::To(refPtr)->getResource(); }
	};


	class FD3D11Utility
	{
	public:
		static FixString<32> GetShaderProfile( ID3D11Device* device , EShader::Type type);
	};



	union D3D11ShaderVariant
	{
		ID3D11DeviceChild*    resource;

		ID3D11VertexShader*   vertex;
		ID3D11PixelShader*    pixel;
		ID3D11GeometryShader* geometry;
		ID3D11ComputeShader*  compute;
		ID3D11HullShader*     hull;
		ID3D11DomainShader*   domain;
	};


}