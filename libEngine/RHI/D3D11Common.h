#include "RHICommon.h"

#include "LogSystem.h"
#include "Platform/Windows/ComUtility.h"

#include "D3D11.h"

#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D11RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hr = CODE; if( hr != S_OK ){ ERROR_MSG_GENERATE( hr , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D11RESULT( CODE , ERRORCODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D11RESULT_RETURN_FALSE( CODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )


#define SAFE_RELEASE( PTR ) if ( PTR ){ PTR->Release(); PTR = nullptr; }

namespace RenderGL
{
	class D3D11Resource : public RHIResource
	{

	};

	template< class RHITextureType >
	struct TD3D11TypeTraits { typedef void ImplType; };

	template<>
	struct TD3D11TypeTraits< RHITexture1D > { typedef ID3D11Texture1D ResourceType; };
	template<>
	struct TD3D11TypeTraits< RHITexture2D > { typedef ID3D11Texture2D ResourceType; };
	template<>
	struct TD3D11TypeTraits< RHITexture3D > { typedef ID3D11Texture3D ResourceType; };
	//template<>
	//struct TD3D11TextureTraits< RHITextureCube > { typedef ID3D11TextureArray ResourceType; };
	//template<>
	//struct TD3D11TextureTraits< RHITextureDepth > { typedef ID3D11Texture2D ResourceType; };
	template<>
	struct TD3D11TypeTraits< RHIVertexBuffer > { typedef ID3D11Buffer ResourceType; };
	template<>
	struct TD3D11TypeTraits< RHIIndexBuffer > { typedef ID3D11Buffer ResourceType; };
	template<>
	struct TD3D11TypeTraits< RHIUniformBuffer > { typedef ID3D11Buffer ResourceType; };
	template<>
	struct TD3D11TypeTraits< RHIBlendState > { typedef ID3D11BlendState ResourceType; };

	struct D3D11Conv
	{
		static D3D_PRIMITIVE_TOPOLOGY To(PrimitiveType type);
		static DXGI_FORMAT To(Texture::Format format);
		static D3D11_BLEND To(Blend::Factor factor);
		static D3D11_BLEND_OP To(Blend::Operation op);
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

	template< class RHITextureType >
	class TD3D11Texture : public TD3D11Resource< RHITextureType >
	{
	protected:
		TD3D11Texture(ID3D11RenderTargetView* RTV, ID3D11ShaderResourceView* SRV, ID3D11UnorderedAccessView* UAV)
			:mRTV(RTV), mSRV(SRV), mUAV(UAV)
		{

		}

		virtual void releaseResource()
		{
			SAFE_RELEASE(mRTV);
			SAFE_RELEASE(mSRV);
			SAFE_RELEASE(mUAV);
			TD3D11Resource< RHITextureType >::releaseResource();
		}
	public:
		ID3D11RenderTargetView* mRTV;
		ID3D11ShaderResourceView* mSRV;
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

	template< class RHITextureType >
	class TD3D11Buffer : public TD3D11Resource< RHITextureType >
	{



	};


	class D3D11VertexBuffer : public TD3D11Buffer< RHIVertexBuffer >
	{



	};

	class D3D11BlendState : public TD3D11Resource< RHIBlendState >
	{
	public:
		D3D11BlendState(ID3D11BlendState* blendStateResource)
		{
			mResource = blendStateResource;
		}
	};

	struct D3D11Cast
	{
		static D3D11Texture2D* To(RHITexture2D* tex) { return static_cast<D3D11Texture2D*>(tex); }
		static D3D11BlendState* To(RHIBlendState* state) { return static_cast<D3D11BlendState*>(state); }

		template < class T >
		static auto To(TRefCountPtr<T>& ptr) { return D3D11Cast::To(ptr.get()); }

		template< class T >
		static auto GetResource(T& RHIObject) { return D3D11Cast::To(&RHIObject)->getResource(); }

		template< class T >
		static auto GetResource(TRefCountPtr<T>& refPtr) { return D3D11Cast::To(refPtr)->getResource(); }
	};

}