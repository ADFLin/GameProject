#pragma once
#ifndef D3D12Common_H_50BC1EFB_7DF9_4106_940C_B7F26AC43893
#define D3D12Common_H_50BC1EFB_7DF9_4106_940C_B7F26AC43893

#include "RHICommon.h"
#include "D3DSharedCommon.h"
#include "D3D12Buffer.h"
#include "D3D12Utility.h"

#include "LogSystem.h"
#include "Platform/Windows/ComUtility.h"

#include "InlineString.h"

#include <unordered_map>
#include "TypeMemoryOp.h"



namespace Render
{
	class D3D12Resource : public RHIResource
	{

	};

	class D3D12Texture1D;
	class D3D12Texture2D;
	class D3D12Texture3D;
	class D3D12TextureCube;
	class D3D12Buffer;
	class D3D12RasterizerState;
	class D3D12BlendState;
	class D3D12InputLayout;
	class D3D12ShaderResourceView;
	class D3D12SamplerState;
	class D3D12DepthStencilState;
	class D3D12SwapChain;

	template< class RHITextureType >
	struct TD3D12TypeTraits { typedef void ImplType; };

#define D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_RHI D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1
	using D3D12_DEPTH_STENCIL_DESC_RHI = D3D12_DEPTH_STENCIL_DESC1;

	template<>
	struct TD3D12TypeTraits< RHISwapChain >
	{
		typedef IDXGISwapChainRHI ResourceType;
		typedef D3D12SwapChain ImplType;
	};

	template<>
	struct TD3D12TypeTraits< RHIBuffer >
	{
		typedef ID3D12Resource ResourceType;
		typedef D3D12Buffer ImplType;
	};

	template<>
	struct TD3D12TypeTraits< RHITexture1D >
	{
		typedef ID3D12Resource ResourceType;
		typedef D3D12Texture1D ImplType;
	};

	template<>
	struct TD3D12TypeTraits< RHITexture2D >
	{
		typedef ID3D12Resource ResourceType;
		typedef D3D12Texture2D ImplType;
	};

	template<>
	struct TD3D12TypeTraits< RHITexture3D >
	{
		typedef ID3D12Resource ResourceType;
		typedef D3D12Texture3D ImplType;
	};

	template<>
	struct TD3D12TypeTraits< RHITextureCube >
	{
		typedef ID3D12Resource ResourceType;
		typedef D3D12TextureCube ImplType;
	};

	template<>
	struct TD3D12TypeTraits< RHIPipelineState >
	{
		typedef ID3D12PipelineState ResourceType;
		typedef RHIPipelineState ImplType;
	};

	struct D3D12Translate : D3DTranslate
	{
		using D3DTranslate::To;

		static D3D12_BLEND To(EBlend::Factor factor);
		static D3D12_BLEND_OP To(EBlend::Operation op);
		static D3D12_CULL_MODE To(ECullMode mode);
		static D3D12_FILL_MODE To(EFillMode mode);
		static D3D12_FILTER To(ESampler::Filter filter);
		static D3D12_TEXTURE_ADDRESS_MODE To(ESampler::AddressMode mode);
		static D3D12_COMPARISON_FUNC To(ECompareFunc func);
		static D3D12_STENCIL_OP To(EStencil op);

		static D3D12_SHADER_VISIBILITY ToVisibiltiy(EShader::Type type);


		static D3D12_PRIMITIVE_TOPOLOGY_TYPE ToTopologyType(EPrimitive type);
	};


	template< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubobjectID >
	struct TPSSubobjectStreamTraits {};
#define DEFINE_PS_SUBOBJECT_STREAM_DATA( ID , DATA )\
	template<>\
	struct TPSSubobjectStreamTraits<ID>\
	{\
		using DataType = DATA;\
	};

	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS, D3D12_SHADER_BYTECODE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS, D3D12_SHADER_BYTECODE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS, D3D12_SHADER_BYTECODE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_RHI, D3D12_DEPTH_STENCIL_DESC_RHI);
	DEFINE_PS_SUBOBJECT_STREAM_DATA(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC);

#undef DEFINE_PS_SUBOBJECT_STREAM_DATA

	struct PSSubobjectStreamDataBase
	{
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE ID;
	};


	template< class T >
	struct TValueWapper
	{
		T value;
		operator T() { return value; }
		TValueWapper& operator = (T p) { value = p; return *this; }
	};

	template< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubobjectID >
	struct TPSSubobjectStreamDataHelper
	{
		using BaseType = typename TPSSubobjectStreamTraits< SubobjectID >::DataType;
		using DataType = typename TSelect< std::is_class_v<BaseType>, BaseType, TValueWapper<BaseType> >::Type;
	};


	template< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubobjectID >
	struct alignas(void*) TPSSubobjectStreamData : PSSubobjectStreamDataBase , TPSSubobjectStreamDataHelper< SubobjectID >::DataType
	{
		TPSSubobjectStreamData() : TPSSubobjectStreamDataHelper< SubobjectID >::DataType() { ID = SubobjectID; }
		template<typename T>
		explicit TPSSubobjectStreamData(T&& value) : TPSSubobjectStreamDataHelper< SubobjectID >::DataType(std::forward<T>(value)) { ID = SubobjectID; }

		using TPSSubobjectStreamDataHelper< SubobjectID >::DataType::operator =;
	};

	class D3D12PipelineStateStream
	{
	public:
		D3D12_PIPELINE_STATE_STREAM_DESC getDesc() const
		{
			D3D12_PIPELINE_STATE_STREAM_DESC result;
			result.pPipelineStateSubobjectStream = (void*)mBuffer.data();
			result.SizeInBytes = mBuffer.size();
			return result;
		}

		template< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubobjectID >
		TPSSubobjectStreamData< SubobjectID >& addDataT()
		{
			using StreamDataType = TPSSubobjectStreamData< SubobjectID >;
			static_assert(std::is_trivially_destructible_v<StreamDataType>);
			size_t offset = mBuffer.size();
			mBuffer.insert(mBuffer.end(), sizeof(StreamDataType), 0);

			void* ptr = mBuffer.data() + offset;
			FTypeMemoryOp::Construct<StreamDataType>(ptr);
			return *reinterpret_cast<StreamDataType*>(ptr);
		}

		template< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubobjectID, typename T >
		TPSSubobjectStreamData< SubobjectID >& addDataT(T&& value)
		{
			using StreamDataType = TPSSubobjectStreamData< SubobjectID >;
			static_assert(std::is_trivially_destructible_v<StreamDataType>);
			size_t offset = mBuffer.size();
			mBuffer.insert(mBuffer.end(), sizeof(StreamDataType), 0);

			void* ptr = mBuffer.data() + offset;
			FTypeMemoryOp::Construct<StreamDataType>(ptr, std::forward<T>(value));
			return *reinterpret_cast<StreamDataType*>(ptr);
		}

		template< class T >
		T& addDataT()
		{
			static_assert(std::is_trivially_destructible_v<T>);
			size_t offset = mBuffer.size();
			mBuffer.insert(mBuffer.end(), sizeof(T), 0);
			void* ptr = mBuffer.data() + offset;
			FTypeMemoryOp::Construct<T>(ptr);
			return *reinterpret_cast<T*>(ptr);
		}

		TArray< uint8 > mBuffer;
	};

	template< D3D12_STATE_SUBOBJECT_TYPE SubobjectID >
	struct TSOSubobjectStreamTraits {};
#define DEFINE_SO_SUBOBJECT_STREAM_DATA( ID , DATA )\
	template<>\
	struct TSOSubobjectStreamTraits<ID>\
	{\
		using DataType = DATA;\
	};

	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG, D3D12_STATE_OBJECT_CONFIG);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, D3D12_GLOBAL_ROOT_SIGNATURE);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, D3D12_LOCAL_ROOT_SIGNATURE);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK, D3D12_NODE_MASK);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, D3D12_DXIL_LIBRARY_DESC);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION, D3D12_EXISTING_COLLECTION_DESC);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, D3D12_RAYTRACING_SHADER_CONFIG);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, D3D12_RAYTRACING_PIPELINE_CONFIG);
	DEFINE_SO_SUBOBJECT_STREAM_DATA(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, D3D12_HIT_GROUP_DESC);

#undef DEFINE_SO_SUBOBJECT_STREAM_DATA


	class D3D12StateObjectDescBuilder
	{
	public:
		D3D12_STATE_OBJECT_DESC getDesc(D3D12_STATE_OBJECT_TYPE type)
		{
			finalize();
			D3D12_STATE_OBJECT_DESC result;
			result.Type = type;
			result.pSubobjects = mSubobjects.data();
			result.NumSubobjects = mSubobjects.size();
			return result;
		}

		template< D3D12_STATE_SUBOBJECT_TYPE SubobjectID >
		TSOSubobjectStreamTraits< SubobjectID >& addDataT()
		{
			static_assert(std::is_trivially_destructible_v<TSOSubobjectStreamTraits< SubobjectID >>);
			size_t offset = mBuffer.size();
			mBuffer.insert(mBuffer.end(), sizeof(TSOSubobjectStreamTraits< SubobjectID >), 0);

			void* ptr = mBuffer.data() + offset;
			FTypeMemoryOp::Construct<TSOSubobjectStreamTraits< SubobjectID >>(ptr);

			D3D12_STATE_SUBOBJECT subobject;
			subobject.Type = SubobjectID;
			subobject.pDesc = (void*)offset;
			mSubobjects.push_back(subobject);
			return *reinterpret_cast<TSOSubobjectStreamTraits< SubobjectID >*>(ptr);
		}

		void addString(WStringView const& str, LPCWSTR* pRef = nullptr)
		{
			size_t offset = mBuffer.size();
			CHECK(str.length() > 0);
			mBuffer.insert(mBuffer.end(), (uint8 const*)str.data() , (uint8 const*)(str.data()) + sizeof(*LPCWSTR(0)) * (str.length() + 1));
			LPWSTR strData = (LPWSTR)&mBuffer[offset];
			strData[str.length()] = 0;

			mStrings.push_back(LPCWSTR(offset));

			if (pRef)
			{
				*pRef = (LPCWSTR)(mStrings.size() - 1);
				mStringRef.push_back(pRef);
			}
		}

		void finalize()
		{
			for (auto& subobject : mSubobjects)
			{
				subobject.pDesc = mBuffer.data() + uint32(subobject.pDesc);
			}
			for (auto& ptr : mStrings)
			{
				ptr = LPCWSTR(mBuffer.data() + uint32(ptr));
			}

			for (auto pRef : mStringRef)
			{
				*pRef = *(mStrings.data() + uint32(*pRef));
			}
		}

		TArray< LPCWSTR* > mStringRef;
		TArray< LPCWSTR >  mStrings;
		TArray< D3D12_STATE_SUBOBJECT> mSubobjects;
		TArray< uint8 > mBuffer;
	};




#define USE_D3D12_RESOURCE_CUSTOM_COUNT 1

	struct CNameable
	{
		template< typename T >
		auto Requires(T& t, wchar_t const* str) -> decltype(
			t.SetName(str)
		);
	};

	template< class RHIResoureType >
	class TD3D12Resource : public RHIResoureType
	{
	public:
		~TD3D12Resource()
		{
			if (mResource != nullptr)
			{
				LogWarning(0, "D3D12Resource no release");
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
#if USE_D3D12_RESOURCE_CUSTOM_COUNT
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
#if USE_D3D12_RESOURCE_CUSTOM_COUNT
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
			if (mResource == nullptr)
				return;
#if _DEBUG
			if (mCount != 0)
			{
				LogWarning(0, "D3D12Resource refcount error");
			}
#endif

#if USE_D3D12_RESOURCE_CUSTOM_COUNT
			int count = mResource->Release();
			if (count > 0)
			{
#if RHI_USE_RESOURCE_TRACE
				if (mTag)
				{
					LogDevMsg(0, "D3D12Resource: %p %p %d %s : %s , %s", this, mResource, count, mTypeName.c_str(), mTag, mTrace.toString().c_str());
				}
				else
				{
					LogDevMsg(0, "D3D12Resource: %p %p %d %s : %s", this, mResource, count, mTypeName.c_str(), mTrace.toString().c_str());
				}
#endif
			}
#else
			mResource->Release();
#endif
			mResource = nullptr;
		}

		typedef typename TD3D12TypeTraits< RHIResoureType >::ResourceType ResourceType;

		ResourceType* getResource() { return mResource; }

		virtual void setDebugName(char const* name) override
		{ 
			if constexpr ( TCheckConcept< CNameable , ResourceType >::Value )
			{
				if (mResource)
				{
					mResource->SetName(FCString::CharToWChar(name).c_str());
				}
			}
		}


		void release()
		{
			if (mResource)
			{
				releaseResource();
			}
		}

#if _DEBUG || USE_D3D12_RESOURCE_CUSTOM_COUNT
		int mCount = 0;
#endif
		ResourceType* mResource = nullptr;
	};


	struct D3D12RenderTargetsState
	{
		static constexpr int MaxSimulationBufferCount = 8;
		struct BufferState
		{
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			TComPtr< ID3D12Resource > resource;
			D3D12PooledHeapHandle RTVHandle;
		};

		struct DepthBufferState
		{
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			TComPtr< ID3D12Resource > resource;
			D3D12PooledHeapHandle DSVHandle;
		};

		void updateState()
		{
			for (int i = 0; i < MaxSimulationBufferCount; ++i)
			{
				if (!colorBuffers[i].resource.isValid())
				{
					numColorBuffers = i;
					break;
				}
			}
			updateFormatGUID();
		}


		BufferState colorBuffers[MaxSimulationBufferCount];
		int numColorBuffers = 0;
		DepthBufferState depthBuffer;

		uint32 formatGUID = 0;

		struct FormatKey
		{
			DXGI_FORMAT colors[MaxSimulationBufferCount];
			int numColors = 0;
			DXGI_FORMAT depth;


			bool operator == (FormatKey const& rhs) const
			{
				if (numColors != rhs.numColors)
					return false;
				if (depth != rhs.depth)
					return false;
				for (int i = 0; i < numColors; ++i)
				{
					if (colors[i] != rhs.colors[i])
						return false;
				}

				return true;
			}

			uint32 getTypeHash() const
			{
				uint32 result = HashValue(numColors);
				for (int i = 0; i < numColors; ++i)
					result = HashCombine(result, colors[i]);

				result = HashCombine(result, depth);
				return result;
			}
		};

		void getFormatKey(FormatKey& outKey) const
		{
			outKey.numColors = numColorBuffers;
			for (int i = 0; i < numColorBuffers; ++i)
			{
				outKey.colors[i] = colorBuffers[i].format;
			}
			outKey.depth = depthBuffer.format;
		}


		void updateFormatGUID()
		{
			static uint32 NextGUID = 0;
			static std::unordered_map< FormatKey, uint32 , MemberFuncHasher > FormatIDMap;
			FormatKey key;
			getFormatKey(key);

			auto iter = FormatIDMap.find(key);
			if (iter != FormatIDMap.end())
			{
				formatGUID = iter->second;
				return;
			}

			formatGUID = NextGUID;
			++NextGUID;
			FormatIDMap[key] = formatGUID;
		}

		void releasePoolHandle()
		{
			for (int i = 0; i < MaxSimulationBufferCount; ++i)
			{
				D3D12DescriptorHeapPool::FreeHandle(colorBuffers[i].RTVHandle);
			}

			D3D12DescriptorHeapPool::FreeHandle(depthBuffer.DSVHandle);
		}
	};

	class D3D12FrameBuffer : public TRefcountResource< RHIFrameBuffer >
	{
	public:

		virtual int  addTexture(RHITextureCube& target, ETexture::Face face, int level = 0) 
		{ 
			return INDEX_NONE;
		}
		virtual int  addTexture(RHITexture2D& target, int level = 0) override;
		virtual int  addTexture(RHITexture2DArray& target, int indexLayer, int level = 0) { return INDEX_NONE; }
		virtual void setTexture(int idx, RHITexture2D& target, int level = 0) { setTextureInternal(idx, target, level); }
		virtual void setTexture(int idx, RHITextureCube& target, ETexture::Face face, int level = 0) {  }
		virtual void setTexture(int idx, RHITexture2DArray& target, int indexLayer, int level = 0) {  }

		virtual int  addTextureArray(RHITextureCube& target, int level)
		{
			return INDEX_NONE;
		}
		virtual void setTextureArray(int idx, RHITextureCube& target, int level)
		{

		}

		void setDepth(RHITexture2D& target) override;
		virtual void removeDepth();

		void setTextureInternal(int index, RHITexture2D& target, int level);
		int getFreeSlot();

		bool bStateDirty = false;
		D3D12RenderTargetsState mRenderTargetsState;
	};


	class D3D12SwapChain : public TD3D12Resource< RHISwapChain >
	{
	public:

		bool initialize(TComPtr<IDXGISwapChainRHI>& resource, TComPtr<ID3D12DeviceRHI>& device , int bufferCount);

		virtual void resizeBuffer(int w, int h) override
		{
			mResource->ResizeBuffers(mRenderTargetsStates.size(), w, h, DXGI_FORMAT_UNKNOWN, 0);
		}
		virtual RHITexture2D* getBackBufferTexture() override
		{
			return nullptr;
		}
		virtual RHITexture2D* getDepthTexture() override
		{
			return nullptr;
		}

		virtual void present(bool bVSync) override
		{
			mResource->Present(bVSync ? 1 : 0, 0);
		}

		virtual void releaseResource()
		{
			for (auto& state : mRenderTargetsStates)
			{
				state.releasePoolHandle();
			}
			mRenderTargetsStates.empty();
		}

		TArray< D3D12RenderTargetsState > mRenderTargetsStates;
	};

	class D3D12InputLayout : public TRefcountResource< RHIInputLayout >
	{
	public:
		D3D12InputLayout(InputLayoutDesc const& desc);

		D3D12_INPUT_LAYOUT_DESC getDesc() const { return { mDescList.data(), (uint32)mDescList.size() }; }

		TArray< D3D12_INPUT_ELEMENT_DESC > mDescList;
		uint32 mAttriableMask;
	};


	class D3D12PipelineState : public TD3D12Resource< RHIPipelineState >
	{



	};

	template< class TRHIResource >
	class TD3D12Texture : public TD3D12Resource< TRHIResource >
	{
	public:
		TD3D12Texture(TextureDesc const& desc)
		{
			mDesc = desc;
		}

		virtual void releaseResource()
		{
			TD3D12Resource< TRHIResource >::releaseResource();
			D3D12DescriptorHeapPool::FreeHandle(mSRV);
			D3D12DescriptorHeapPool::FreeHandle(mRTVorDSV);
			D3D12DescriptorHeapPool::FreeHandle(mUAV);
		}

		D3D12_RESOURCE_STATES mCurrentStates;
		D3D12PooledHeapHandle mSRV;
		D3D12PooledHeapHandle mRTVorDSV;
		D3D12PooledHeapHandle mUAV;
	};

	class D3D12Texture1D : public TD3D12Texture< RHITexture1D >
	{
	public:
		D3D12Texture1D(TextureDesc const& desc, TComPtr< ID3D12Resource >& resource);
		virtual bool update(int offset, int length, ETexture::Format format, void* data, int level = 0);
	};

	class D3D12Texture2D : public TD3D12Texture< RHITexture2D >
	{
	public:
		D3D12Texture2D(TextureDesc const& desc, TComPtr< ID3D12Resource >& resource);
	};

	class D3D12Texture3D : public TD3D12Texture< RHITexture3D >
	{
	public:
		D3D12Texture3D(TextureDesc const& desc, TComPtr< ID3D12Resource >& resource);
	};

	class D3D12TextureCube : public TD3D12Texture< RHITextureCube >
	{
	public:
		D3D12TextureCube(TextureDesc const& desc, TComPtr< ID3D12Resource >& resource);
	};

	class D3D12Buffer : public TD3D12Resource< RHIBuffer >
	{
	public:
		bool initialize(BufferDesc const& desc, TComPtr<ID3D12Resource>& resource)
		{
			mDesc = desc;
			mResource = resource.detach();
			return true;
		}
		bool initialize(BufferDesc const& desc)
		{
			mDesc = desc;
			return true;
		}
		virtual void releaseResource();

		bool isDyanmic() const
		{
			return !!(getDesc().creationFlags & BCF_CpuAccessWrite);
		}

		D3D12PooledHeapHandle mCBV;
		D3D12PooledHeapHandle mSRV;
		D3D12PooledHeapHandle mUAV;

		D3D12BufferAllocation mDynamicAllocation;
	};

	class D3D12RasterizerState : public TRefcountResource< RHIRasterizerState >
	{
	public:
		D3D12RasterizerState(RasterizerStateInitializer const& initializer);

		D3D12_RASTERIZER_DESC mDesc;
		bool bEnableScissor;
	};

	class D3D12BlendState : public TRefcountResource< RHIBlendState >
	{
	public:
		D3D12BlendState(BlendStateInitializer const& initializer);

		D3D12_BLEND_DESC mDesc;

	};

	class D3D12DepthStencilState : public TRefcountResource< RHIDepthStencilState >
	{
	public:
		D3D12DepthStencilState(DepthStencilStateInitializer const& initializer);

		D3D12_DEPTH_STENCIL_DESC_RHI mDesc;
	};

	class D3D12SamplerState : public TRefcountResource< RHISamplerState >
	{
	public:
		D3D12SamplerState(SamplerStateInitializer const& initializer);

		D3D12_SAMPLER_DESC    mDesc;
		D3D12PooledHeapHandle mHandle;
	};
}

#endif // D3D12Common_H_50BC1EFB_7DF9_4106_940C_B7F26AC43893