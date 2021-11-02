#include "RHICommon.h"

#include "D3DSharedCommon.h"

#include "LogSystem.h"
#include "Platform/Windows/ComUtility.h"

#include "InlineString.h"

#include <D3D11.h>

#include <unordered_map>



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
	class D3D11TextureCube;
	class D3D11VertexBuffer;
	class D3D11IndexBuffer;
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
	template<>
	struct TD3D11TypeTraits< RHITextureCube >
	{ 
		typedef ID3D11Texture2D ResourceType;
		typedef D3D11TextureCube ImplType;
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

	struct D3D11Translate : D3DTranslate
	{
		using D3DTranslate::To;

		static D3D11_BLEND To(EBlend::Factor factor);
		static D3D11_BLEND_OP To(EBlend::Operation op);
		static D3D11_CULL_MODE To(ECullMode mode);
		static D3D11_FILL_MODE To(EFillMode mode);
		static D3D11_MAP To(ELockAccess access);
		static D3D11_FILTER To(ESampler::Filter filter);
		static D3D11_TEXTURE_ADDRESS_MODE To(ESampler::AddressMode mode);
		static D3D11_COMPARISON_FUNC To(ECompareFunc func);
		static D3D11_STENCIL_OP To(EStencil op);
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
		TD3D11Texture(ID3D11ShaderResourceView* SRV, ID3D11UnorderedAccessView* UAV)
			:mSRV(SRV),mUAV(UAV)
		{

		}

		virtual void releaseResource()
		{
			mSRV.release();
			SAFE_RELEASE(mUAV);
			TD3D11Resource< RHITextureType >::releaseResource();
		}
		virtual RHIShaderResourceView* getBaseResourceView() { return &mSRV; }

	public:
		D3D11ShaderResourceView    mSRV;
		ID3D11UnorderedAccessView* mUAV;
	};



	struct Texture1DCreationResult
	{
		TComPtr< ID3D11Texture1D > resource;
		TComPtr< ID3D11ShaderResourceView >  SRV;
		TComPtr< ID3D11UnorderedAccessView > UAV;
	};

	class D3D11Texture1D : public TD3D11Texture< RHITexture1D >
	{
	public:
		D3D11Texture1D(ETexture::Format format, Texture1DCreationResult& creationResult)
			:TD3D11Texture< RHITexture1D >(creationResult.SRV.detach(), creationResult.UAV.detach())
		{
			mFormat = format;
			mResource = creationResult.resource.detach();
			D3D11_TEXTURE1D_DESC desc;
			mResource->GetDesc(&desc);
			mSize = desc.Width;
			mNumSamples = 1;
			mNumMipLevel = desc.MipLevels;
		}

		virtual bool update(int offset, int length, ETexture::Format format, void* data, int level = 0)
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
			deviceContext->UpdateSubresource(mResource, level, &box, data, length * ETexture::GetFormatSize(format), length * ETexture::GetFormatSize(format));
			return true;

		}
	};

	struct Texture2DCreationResult
	{
		TComPtr< ID3D11Texture2D > resource;
		TComPtr< ID3D11ShaderResourceView >  SRV;
		TComPtr< ID3D11UnorderedAccessView > UAV;
	};

	struct TextureCubeCreationResult
	{
		TComPtr< ID3D11Texture2D > resource;
		TComPtr< ID3D11ShaderResourceView >  SRV;
		TComPtr< ID3D11UnorderedAccessView > UAV;
	};

	struct D3D11ResourceViewStorage
	{
		void releaseResource()
		{
			for (auto& pair : mRTViewMap)
			{
				pair.second->Release();
			}
			mRTViewMap.clear();
		}

		struct RenderTargetKey
		{
			int level;
			int arrayIndex;

			uint32 getTypeHash() const
			{
				uint32 result = HashValue(level);
				HashCombine(result, arrayIndex);
				return result;
			}

			bool operator == (RenderTargetKey const& rhs) const
			{
				return level == rhs.level && arrayIndex == rhs.arrayIndex;
			}
		};


		ID3D11RenderTargetView* getRendnerTarget_Texture2D(ID3D11Resource* texture, ETexture::Format format, int level, int numSamples)
		{
			RenderTargetKey key;
			key.level = level;
			key.arrayIndex = 0;
			return getRednerTargetInternal(key, texture, [=](D3D11_RENDER_TARGET_VIEW_DESC& desc)
			{
				desc.Format = D3D11Translate::To(format);
				if (numSamples > 1)
				{
					desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
					desc.Texture2D.MipSlice = level;
				}
			});
		}

		ID3D11RenderTargetView* getRenderTarget_TextureCube(ID3D11Resource* texture, ETexture::Format format, ETexture::Face face, int level)
		{
			RenderTargetKey key;
			key.level = level;
			key.arrayIndex = face;
			return getRednerTargetInternal(key, texture, [=](D3D11_RENDER_TARGET_VIEW_DESC& desc)
			{
				desc.Format = D3D11Translate::To(format);
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.ArraySize = 1;
				desc.Texture2DArray.FirstArraySlice = (int)face;
				desc.Texture2DArray.MipSlice = level;
			});
		}


		template< class TFunc >
		ID3D11RenderTargetView* getRednerTargetInternal(RenderTargetKey const& key ,ID3D11Resource* texture, TFunc&& SetupDescFunc)
		{
			auto iter = mRTViewMap.find(key);
			if (iter != mRTViewMap.end())
			{
				return iter->second;
			}

			TComPtr<ID3D11Device> device;
			texture->GetDevice(&device);

			TComPtr< ID3D11RenderTargetView > RTView;
			D3D11_RENDER_TARGET_VIEW_DESC desc;
			SetupDescFunc(desc);
			VERIFY_D3D_RESULT(device->CreateRenderTargetView(texture, &desc, &RTView), return nullptr;);

			ID3D11RenderTargetView* result = RTView.detach();
			mRTViewMap.emplace(key, result);
			return result;
		}


#if 0

		ID3D11ShaderResourceView* getShaderResource_TextureCube(ID3D11Resource* texture, Texture::Format format, Texture::Face face, int level)
		{
			RenderTargetKey key;
			key.level = level;
			key.arrayIndex = face;
			return getShaderResourceInternal(key, texture, [=](D3D11_SHADER_RESOURCE_VIEW_DESC& desc)
			{
				desc.Format = D3D11Translate::To(format);
				desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				desc.Texture2DArray.ArraySize = 1;
				desc.Texture2DArray.FirstArraySlice = (int)face;
				desc.Texture2DArray.MipSlice = level;
			});
		}

		template< class TFunc >
		ID3D11ShaderResourceView* getShaderResourceInternal(RenderTargetKey const& key, ID3D11Resource* texture, TFunc&& SetupDescFunc)
		{
			auto iter = mRTViewMap.find(key);
			if (iter != mRTViewMap.end())
			{
				return iter->second;
			}

			TComPtr<ID3D11Device> device;
			texture->GetDevice(&device);

			TComPtr< ID3D11ShaderResourceView > SRView;
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			SetupDescFunc(desc);
			VERIFY_D3D11RESULT(device->CreateShaderResourceView(texture, &desc, &SRView), return nullptr;);

			ID3D11ShaderResourceView* result = SRView.release();
			mRTViewMap.emplace(key, result);
			return result;
		}
		std::unordered_map< RenderTargetKey, ID3D11ShaderResourceView*, MemberFuncHasher > mSRViewMap;
#endif
		std::unordered_map< RenderTargetKey, ID3D11RenderTargetView*, MemberFuncHasher > mRTViewMap;
	};

	class D3D11Texture2D : public TD3D11Texture< RHITexture2D >
	{
		using BaseClass = TD3D11Texture< RHITexture2D >;
	public:
		enum EDepthFormat
		{
			DepthFormat ,
		};
		D3D11Texture2D(ETexture::Format format, Texture2DCreationResult& creationResult)
			:TD3D11Texture< RHITexture2D >(creationResult.SRV.detach(), creationResult.UAV.detach())
		{
			mFormat = format;
			mResource = creationResult.resource.detach();
			D3D11_TEXTURE2D_DESC desc;
			mResource->GetDesc(&desc);
			mSizeX = desc.Width;
			mSizeY = desc.Height;
			mNumSamples = desc.SampleDesc.Count;
			mNumMipLevel = desc.MipLevels;
		}

		D3D11Texture2D(ETexture::Format format, Texture2DCreationResult& creationResult , EDepthFormat )
			:TD3D11Texture< RHITexture2D >(creationResult.SRV.detach(), creationResult.UAV.detach())
		{
			mFormat = format;
			mResource = creationResult.resource.detach();
			D3D11_TEXTURE2D_DESC desc;
			mResource->GetDesc(&desc);
			mSizeX = desc.Width;
			mSizeY = desc.Height;
			mNumSamples = desc.SampleDesc.Count;
			mNumMipLevel = desc.MipLevels;

			TComPtr<ID3D11Device> device;
			mResource->GetDevice(&device);
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
			depthStencilDesc.Format = D3D11Translate::To(format);
			switch (depthStencilDesc.Format)
			{
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			}

			if (mNumSamples > 1)
			{
				depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				depthStencilDesc.Texture2D.MipSlice = 0;
			}

			VERIFY_D3D_RESULT(device->CreateDepthStencilView(mResource, &depthStencilDesc, &mDSV), );

		}

		bool update(int ox, int oy, int w, int h, ETexture::Format format, void* data, int level)
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
			deviceContext->UpdateSubresource(mResource, level, &box, data, w * ETexture::GetFormatSize(format), w * h * ETexture::GetFormatSize(format));
			return true;
		}

		bool update(int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level)
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
			deviceContext->UpdateSubresource(mResource, level, &box, data, dataImageWidth * ETexture::GetFormatSize(format), h * dataImageWidth * ETexture::GetFormatSize(format));
			return true;
		}


		void releaseResource()
		{
			mViewStorage.releaseResource();
			SAFE_RELEASE(mDSV);
			BaseClass::releaseResource();
		}

		ID3D11RenderTargetView* getRenderTargetView(int level)
		{
			return mViewStorage.getRendnerTarget_Texture2D(mResource, mFormat, level, mNumSamples);
		}

		D3D11ResourceViewStorage mViewStorage;
		ID3D11DepthStencilView*  mDSV = nullptr;
	};


	struct Texture3DCreationResult
	{
		TComPtr< ID3D11Texture3D > resource;
		TComPtr< ID3D11ShaderResourceView >  SRV;
		TComPtr< ID3D11UnorderedAccessView > UAV;
	};


	class D3D11Texture3D : public TD3D11Texture< RHITexture3D >
	{
	public:
		D3D11Texture3D(ETexture::Format format, Texture3DCreationResult& creationResult)
			:TD3D11Texture< RHITexture3D >(creationResult.SRV.detach(), creationResult.UAV.detach())
		{
			mFormat = format;
			mResource = creationResult.resource.detach();
			D3D11_TEXTURE3D_DESC desc;
			mResource->GetDesc(&desc);
			mSizeX = desc.Width;
			mSizeY = desc.Height;
			mSizeZ = desc.Depth;
			mNumSamples = 1;
			mNumMipLevel = desc.MipLevels;
		}
	};

	class D3D11TextureCube : public TD3D11Texture< RHITextureCube >
	{
		using BaseClass = TD3D11Texture< RHITextureCube >;
	public:
		D3D11TextureCube(ETexture::Format format, TextureCubeCreationResult& creationResult)
			:TD3D11Texture< RHITextureCube >(creationResult.SRV.detach(), creationResult.UAV.detach())
		{
			mFormat = format;
			mResource = creationResult.resource.detach();
			D3D11_TEXTURE2D_DESC desc;
			mResource->GetDesc(&desc);
			mSize = desc.Width;
			mNumSamples = desc.SampleDesc.Count;
			mNumMipLevel = desc.MipLevels;
		}

		virtual bool update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, void* data, int level )
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
			box.front = (int)face;
			box.back = (int)face + 1;
			box.left = ox;
			box.right = ox + w;
			box.top = oy;
			box.bottom = oy + h;
			deviceContext->UpdateSubresource(mResource, level, &box, data, w * ETexture::GetFormatSize(format), w * h * ETexture::GetFormatSize(format));
			return true;
		}
		virtual bool update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level)
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
			box.front = (int)face;
			box.back = (int)face + 1;
			box.left = ox;
			box.right = ox + w;
			box.top = oy;
			box.bottom = oy + h;
			//@FIXME : error
			deviceContext->UpdateSubresource(mResource, level, &box, data, dataImageWidth * ETexture::GetFormatSize(format), h * dataImageWidth * ETexture::GetFormatSize(format));
			return true;
		}

		void releaseResource()
		{
			mViewStorage.releaseResource();
			BaseClass::releaseResource();
		}


		ID3D11RenderTargetView* getRenderTargetView(ETexture::Face face, int level)
		{
			return mViewStorage.getRenderTarget_TextureCube(mResource, mFormat, face, level);
		}
		D3D11ResourceViewStorage mViewStorage;
	};

	struct D3D11BufferCreationResult
	{
		TComPtr<ID3D11Buffer> resource;
		TComPtr<ID3D11ShaderResourceView> SRV;
		TComPtr<ID3D11UnorderedAccessView> UAV;
		uint32 flags;
	};

	template< class RHIBufferType >
	class TD3D11Buffer : public TD3D11Resource< RHIBufferType >
	{
	public:
		TD3D11Buffer(D3D11BufferCreationResult& creationResult)
		{
			mResource = creationResult.resource.detach();
			mSRV = creationResult.SRV.detach();
			mUAV = creationResult.UAV.detach();
			mCreationFlags = creationResult.flags;
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
			SAFE_RELEASE(mSRV);
			SAFE_RELEASE(mUAV);
			TD3D11Resource< RHIBufferType >::releaseResource();
		}

		ID3D11ShaderResourceView* mSRV;
		ID3D11UnorderedAccessView* mUAV;
	};


	class D3D11VertexBuffer : public TD3D11Buffer< RHIVertexBuffer >
	{
	public:
		using TD3D11Buffer< RHIVertexBuffer >::TD3D11Buffer;
		D3D11VertexBuffer(int numVertices , int vertexSize , D3D11BufferCreationResult& creationResult)
			:TD3D11Buffer< RHIVertexBuffer >(creationResult)
		{
			mNumElements = numVertices;
			mElementSize = vertexSize;
		}

		void  updateData(uint32 vStart, uint32 numVertices, void* data)
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
		D3D11IndexBuffer( int numIndices, bool bIntType ,D3D11BufferCreationResult& creationResult)
			:TD3D11Buffer< RHIIndexBuffer >(creationResult)
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

	class D3D11InputLayout : public TRefcountResource< RHIInputLayout >
	{
	public:
		D3D11InputLayout()
		{

		}

		void initialize(InputLayoutDesc const& desc);

		void releaseResource() override;

		bool haveAttribute(uint8 attribute) const
		{
			return !!(mAttriableMask & BIT(attribute));
		}

		ID3D11InputLayout* GetShaderLayout( ID3D11Device* device , RHIShader* shader);

		std::vector< D3D11_INPUT_ELEMENT_DESC > mDescList;
		uint32 mAttriableMask;
		ID3D11InputLayout* mUniversalResource;
		std::unordered_map< RHIShader*, ID3D11InputLayout* > mResourceMap;
	};


	struct D3D11RenderTargetsState
	{
		static constexpr int MaxSimulationBufferCount = 8;
		
		ID3D11RenderTargetView* colorBuffers[MaxSimulationBufferCount];
		int numColorBuffers;
		ID3D11DepthStencilView* depthBuffer;

		RHITextureRef   colorResources[MaxSimulationBufferCount];
		RHITexture2DRef depthResource;
		D3D11RenderTargetsState()
		{
			std::fill_n(colorBuffers, MaxSimulationBufferCount, nullptr);
			depthBuffer = nullptr;
			numColorBuffers = 0;
		}
	};
	class D3D11FrameBuffer : public TRefcountResource< RHIFrameBuffer >
	{
	public:
		virtual void setupTextureLayer(RHITextureCube& target, int level)
		{

		}
		virtual int  addTexture(RHITextureCube& target, ETexture::Face face, int level);
		virtual int  addTexture(RHITexture2D& target, int level);
		virtual int  addTexture(RHITexture2DArray& target, int indexLayer, int level)
		{
			return INDEX_NONE;

		}
		virtual void setTexture(int idx, RHITexture2D& target, int level);
		virtual void setTexture(int idx, RHITextureCube& target, ETexture::Face face, int level);
		virtual void setTexture(int idx, RHITexture2DArray& target, int indexLayer, int level)
		{


		}
		virtual void setDepth(RHITexture2D& target);
		virtual void removeDepth();
	
		bool bStateDirty = false;
		D3D11RenderTargetsState mRenderTargetsState;
	};

	class D3D11SwapChain : public TD3D11Resource< RHISwapChain >
	{
	public:

		D3D11SwapChain(TComPtr<IDXGISwapChain>& resource , D3D11Texture2D& colorTexture , D3D11Texture2D* depthTexture)
			:mColorTexture(&colorTexture)
			,mDepthTexture(depthTexture)
		{
			mResource = resource.detach();
			mRenderTargetsState.numColorBuffers = 1;
			mRenderTargetsState.colorBuffers[0] = mColorTexture->getRenderTargetView(0);
			if (mDepthTexture)
			{
				mRenderTargetsState.depthBuffer = mDepthTexture->mDSV;
			}

		}
		virtual RHITexture2D* getBackBufferTexture() override 
		{ 
			return mColorTexture; 
		}

		virtual void Present(bool bVSync) override
		{
			mResource->Present(bVSync ? 1 : 0, 0);
		}


		void bitbltToDevice(HDC hTargetDC)
		{
			TComPtr<IDXGISurface1> surface;
			VERIFY_D3D_RESULT(mResource->GetBuffer(0, IID_PPV_ARGS(&surface)), );
			HDC hDC;
			VERIFY_D3D_RESULT(surface->GetDC(FALSE, &hDC), );
			int w = mColorTexture->getSizeX();
			int h = mColorTexture->getSizeY();
			::BitBlt(hTargetDC, 0, 0, w, h, hDC, 0, 0, SRCCOPY);
			VERIFY_D3D_RESULT(surface->ReleaseDC(nullptr), );
		}

		virtual void releaseResource()
		{
			mColorTexture.release();
			mDepthTexture.release();

			TD3D11Resource< RHISwapChain >::releaseResource();
		}

		TRefCountPtr< D3D11Texture2D >  mColorTexture;
		TRefCountPtr< D3D11Texture2D >  mDepthTexture;
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
		static InlineString<32> GetShaderProfile( D3D_FEATURE_LEVEL featureLevel, EShader::Type type);
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