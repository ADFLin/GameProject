#include "RHICommon.h"

#include "LogSystem.h"
#include "Platform/Windows/ComUtility.h"

#include "FixString.h"

#include "D3D11.h"

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

	class D3D11Texture2D;
	class D3D11VertexBuffer;
	class D3D11IndexBuffer;
	class D3D11UniformBuffer;
	class D3D11RasterizerState;
	class D3D11BlendState;
	class D3D11InputLayout;
	class D3D11ShaderResourceView;

	template<>
	struct TD3D11TypeTraits< RHITexture1D > 
	{ 
		typedef ID3D11Texture1D ResourceType; 
	};
	template<>
	struct TD3D11TypeTraits< RHITexture2D >
	{  
		typedef ID3D11Texture2D ResourceType;  
		typedef D3D11Texture2D ImplType;  
	};
	template<>
	struct TD3D11TypeTraits< RHITexture3D > { typedef ID3D11Texture3D ResourceType; };
	//template<>
	//struct TD3D11TextureTraits< RHITextureCube > { typedef ID3D11TextureArray ResourceType; };
	//template<>
	//struct TD3D11TextureTraits< RHITextureDepth > { typedef ID3D11Texture2D ResourceType; };
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

	struct D3D11Conv
	{
		static D3D_PRIMITIVE_TOPOLOGY To(PrimitiveType type);
		static DXGI_FORMAT To(Vertex::Format format, bool bNormalize);
		static DXGI_FORMAT To(Texture::Format format);
		static D3D11_BLEND To(Blend::Factor factor);
		static D3D11_BLEND_OP To(Blend::Operation op);
		static D3D11_CULL_MODE To(ECullMode mode);
		static D3D11_FILL_MODE To(EFillMode mode);
		static D3D11_MAP To(ELockAccess access);
	};

	template< class RHIResoureType >
	class TD3D11Resource : public RHIResoureType
	{
	public:
		virtual void incRef() { mResource->AddRef(); }
		virtual bool decRef() { return mResource->Release() == 1; }
		virtual void releaseResource() { mResource->Release(); mResource = nullptr; }

		typedef typename TD3D11TypeTraits< RHIResoureType >::ResourceType ResourceType;

		ResourceType* getResource() { return mResource; }
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
		D3D11ShaderResourceView  mSRV;
		ID3D11RenderTargetView*  mRTV;
		ID3D11UnorderedAccessView* mUAV;
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

		bool update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level)
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
			deviceContext->UpdateSubresource(mResource, level, &box, data, w * pixelStride, w * h * pixelStride);
			return true;
		}


	};

	template< class RHIBufferType >
	class TD3D11Buffer : public TD3D11Resource< RHIBufferType >
	{
	public:
		TD3D11Buffer(ID3D11Device* device, ID3D11Buffer* resource, uint32 creationFlag)
		{
			mResource = resource;
			if( creationFlag & BCF_CreateSRV )
			{
				device->CreateShaderResourceView(resource, NULL, &mSRVResource);
			}
			if( creationFlag & BCF_CreateUAV )
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


	};

	class D3D11RasterizerState : public TD3D11Resource< RHIRasterizerState >
	{
	public:
		D3D11RasterizerState(ID3D11RasterizerState* rasterizerResource)
		{
			mResource = rasterizerResource;
		}
	};

	class D3D11BlendState : public TD3D11Resource< RHIBlendState >
	{
	public:
		D3D11BlendState(ID3D11BlendState* blendStateResource)
		{
			mResource = blendStateResource;
		}
	};

	class D3D11InputLayout : public TD3D11Resource< RHIInputLayout >
	{
	public:
		D3D11InputLayout(ID3D11InputLayout* inputLayout)
		{
			mResource = inputLayout;
		}
	};

	struct D3D11Cast
	{
		template< class RHIResource >
		static auto To(RHIResource* resource) { return static_cast< typename TD3D11TypeTraits< RHIResource >::ImplType*>(resource); }

		template < class T >
		static auto To(TRefCountPtr<T>& ptr) { return D3D11Cast::To(ptr.get()); }

		template< class T >
		static auto GetResource(T& RHIObject) { return D3D11Cast::To(&RHIObject)->getResource(); }

		template< class T >
		static auto GetResource(TRefCountPtr<T>& refPtr) { return D3D11Cast::To(refPtr)->getResource(); }
	};


	class FD3D11Utility
	{
	public:
		static FixString<32> GetShaderProfile( ID3D11Device* device , Shader::Type type);
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