#pragma once
#ifndef RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
#define RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D

#include "RHICommon.h"
#include "CoreShare.h"
#include "SystemPlatform.h"
#include "Module/ModuleInterface.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif
#include "TemplateMisc.h"


class DataCacheInterface;
struct ImageData;


enum class RHISystemName
{
	D3D11,
	D3D12,
	OpenGL,
	Vulkan,


	Count,
};

namespace Literal
{
	FORCEINLINE char const* ToString(RHISystemName name)
	{
		switch (name)
		{
		case RHISystemName::OpenGL: return "OpenGL";
		case RHISystemName::D3D11:  return "D3D11";
		case RHISystemName::D3D12: return "D3D12";
		case RHISystemName::Vulkan: return "Vulkan";
		}
		return "Unknown";
	};
}

#if RHI_USE_RESOURCE_TRACE
#define RHI_TRACE_FUNC_NAME(NAME) NAME##Trace
#define RHI_TRACE_FUNC( NAME , ... ) RHI_TRACE_FUNC_NAME(NAME)(RHI_TRACE_PARAM_DESC, ##__VA_ARGS__)

#define RHI_TRACE_CODE( CODE )\
	auto* result = CODE;\
	if (result)\
	{\
		result->setTraceData( traceInfo );\
	}\
	return result;
#define RHI_MAKE_TRACE_PARAM Render::ResTraceInfo(__FILE__, __FUNCTION__, __LINE__)
#define RHI_TRACE_PARAM_DESC ResTraceInfo const& traceInfo
#define RHI_TRACE_PARAM traceInfo
#define RHI_CALL_TRACE_FRUNC( NAME , ... ) RHI_TRACE_FUNC_NAME(NAME)(RHI_TRACE_PARAM, ##__VA_ARGS__)

#else
#define RHI_TRACE_FUNC_NAME(NAME) NAME
#define RHI_TRACE_FUNC( NAME , ... ) NAME(__VA_ARGS__)
#define RHI_TRACE_CODE( CODE ) return CODE
#define RHI_MAKE_TRACE_PARAM
#define RHI_TRACE_PARAM_DESC 
#define RHI_TRACE_PARAM
#define RHI_CALL_TRACE_FRUNC( NAME , ... ) NAME(__VA_ARGS__)
#endif


namespace Render
{
	extern CORE_API class RHISystem* GRHISystem;

	FORCEINLINE bool RHIIsInitialized()
	{
		return GRHISystem != nullptr;
	}

	struct RHISystemInitParams
	{
		int  numSamples;
		bool bVSyncEnable;
		bool bDebugMode;
		bool bMultithreadingSupported;

#if SYS_PLATFORM_WIN
		HWND hWnd;
		HDC  hDC;
#endif
		RHISystemInitParams()
		{
			numSamples = 1;
			bVSyncEnable = true;
			bDebugMode = false;
			bMultithreadingSupported = false;
		}

	};


	RHI_API bool RHISystemInitialize(RHISystemName name , RHISystemInitParams const& initParam );
	RHI_API void RHISystemShutdown();
	RHI_API bool RHISystemIsSupported(RHISystemName name);
	RHI_API bool RHIBeginRender();
	RHI_API void RHIEndRender(bool bPresent);
	RHI_API void RHIClearResourceReference();

	struct SwapChainCreationInfo
	{
#if SYS_PLATFORM_WIN
		HWND  windowHandle;
#endif
		bool  bWindowed;
		IntVector2 extent;
		ETexture::Format colorForamt;
		bool bCreateDepth;
		ETexture::Format depthFormat;

		uint32 textureCreationFlags = 0;
		int numSamples;
		int bufferCount;

		SwapChainCreationInfo()
		{
			windowHandle = NULL;
			extent = IntVector2(1, 1);
			bWindowed = false;
			bCreateDepth = false;
			colorForamt = ETexture::BGRA8;
			depthFormat = ETexture::D24S8;
			numSamples = 1;
			bufferCount = 2;
			textureCreationFlags = 0;
		}
	};

	RHI_API RHISwapChain* RHICreateSwapChain(SwapChainCreationInfo const& info);

	RHI_API RHITexture1D* RHI_TRACE_FUNC(RHICreateTexture1D,
		ETexture::Format format, int length ,
		int numMipLevel = 0, uint32 creationFlags = TCF_DefalutValue,
		void* data = nullptr);

	RHI_API RHITexture2D*   RHI_TRACE_FUNC(RHICreateTexture2D,
		ETexture::Format format, int w, int h,
		int numMipLevel = 0, int numSamples = 1, uint32 creationFlags = TCF_DefalutValue,
		void* data = nullptr, int dataAlign = 0);

	RHI_API RHITexture3D*    RHI_TRACE_FUNC(RHICreateTexture3D,
		ETexture::Format format, int sizeX, int sizeY, int sizeZ , 
		int numMipLevel = 0, int numSamples = 1, uint32 creationFlags = TCF_DefalutValue , 
		void* data = nullptr);

	RHI_API RHITextureCube*  RHI_TRACE_FUNC(RHICreateTextureCube,
		ETexture::Format format, int size, int numMipLevel = 0, uint32 creationFlags = TCF_DefalutValue, 
		void* data[] = nullptr);

	RHI_API RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray,
		ETexture::Format format, int w, int h, int layerSize,
		int numMipLevel = 0, int numSamples = 1, uint32 creationFlags = TCF_DefalutValue, 
		void* data = nullptr);

	RHI_API RHITexture2D* RHI_TRACE_FUNC(RHICreateTextureDepth,
		ETexture::Format format, int w, int h , int numMipLevel = 1 , int numSamples = 1, uint32 creationFlags = 0);

	RHI_API RHITexture1D*      RHI_TRACE_FUNC(RHICreateTexture1D, TextureDesc const& desc, void* data = nullptr);
	RHI_API RHITexture2D*      RHI_TRACE_FUNC(RHICreateTexture2D, TextureDesc const& desc, void* data = nullptr, int dataAlign = 0);
	RHI_API RHITexture3D*      RHI_TRACE_FUNC(RHICreateTexture3D, TextureDesc const& desc, void* data = nullptr);
	RHI_API RHITextureCube*    RHI_TRACE_FUNC(RHICreateTextureCube,TextureDesc const& desc,void* data[] = nullptr);
	RHI_API RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray, TextureDesc const& desc, void* data = nullptr);
	RHI_API RHITexture2D*      RHI_TRACE_FUNC(RHICreateTextureDepth,TextureDesc const& desc);

	RHI_API RHIBuffer* RHI_TRACE_FUNC(RHICreateBuffer,
		uint32 elementSize, uint32 numElements, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);

	RHI_API RHIBuffer* RHI_TRACE_FUNC(RHICreateVertexBuffer,
		uint32 vertexSize, uint32 numVertices, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);

	RHI_API RHIBuffer* RHI_TRACE_FUNC(RHICreateIndexBuffer,
		uint32 nIndices, bool bIntIndex, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);

	RHI_API void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset = 0, uint32 size = 0 );
	RHI_API void  RHIUnlockBuffer(RHIBuffer* buffer);

	RHI_API void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData);
	RHI_API void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData);

	RHI_API bool RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level = 0, int dataWidth = 0);

	//RHI_API void* RHILockTexture(RHITextureBase* texture, ELockAccess access, uint32 offset = 0, uint32 size = 0);
	//RHI_API void  RHIUnlockTexture(RHITextureBase* texture);

	RHI_API RHIFrameBuffer*  RHICreateFrameBuffer();

	RHI_API RHIInputLayout*       RHI_TRACE_FUNC(RHICreateInputLayout, InputLayoutDesc const& desc);
	RHI_API RHISamplerState*      RHI_TRACE_FUNC(RHICreateSamplerState, SamplerStateInitializer const& initializer);
	RHI_API RHIRasterizerState*   RHI_TRACE_FUNC(RHICreateRasterizerState, RasterizerStateInitializer const& initializer);
	RHI_API RHIBlendState*        RHI_TRACE_FUNC(RHICreateBlendState, BlendStateInitializer const& initializer);
	RHI_API RHIDepthStencilState* RHI_TRACE_FUNC(RHICreateDepthStencilState, DepthStencilStateInitializer const& initializer);


	RHI_API RHIShader*        RHICreateShader(EShader::Type type);


	RHI_API RHIShaderProgram* RHI_TRACE_FUNC(RHICreateShaderProgram);


	class RHICommandList : public Noncopyable
	{
	public:
		virtual ~RHICommandList() {}
		RHI_API static RHICommandList& GetImmediateList();
	};


	//
	RHI_API void RHISetRasterizerState(RHICommandList& commandList , RHIRasterizerState& rasterizerState);
	RHI_API void RHISetBlendState(RHICommandList& commandList , RHIBlendState& blendState);
	RHI_API void RHISetDepthStencilState(RHICommandList& commandList, RHIDepthStencilState& depthStencilState, uint32 stencilRef = -1);

	struct ViewportInfo
	{
		float x;
		float y;
		float w;
		float h;
		float zNear;
		float zFar;

		ViewportInfo() = default;
		ViewportInfo(float inX, float inY, float inW, float inH, float inZNear = GRHIClipZMin, float inZFar = 1.0)
			:x(inX),y(inY),w(inW),h(inH),zNear(inZNear),zFar(inZFar)
		{
		}
	};
	RHI_API void RHISetViewport(RHICommandList& commandList, float x, float y, float w, float h, float zNear = GRHIClipZMin, float zFar = 1);
	RHI_API void RHISetViewports(RHICommandList& commandList, ViewportInfo const viewports[] , int numViewports );

	RHI_API void RHISetScissorRect(RHICommandList& commandList , int x = 0, int y = 0, int w = 0, int h = 0);

	
	RHI_API void RHIDrawPrimitive(RHICommandList& commandList, EPrimitive type, int vStart, int nv);
	RHI_API void RHIDrawIndexedPrimitive(RHICommandList& commandList, EPrimitive type, int indexStart, int nIndex, uint32 baseVertex = 0);
	RHI_API void RHIDrawPrimitiveIndirect(RHICommandList& commandList, EPrimitive type, RHIBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	RHI_API void RHIDrawIndexedPrimitiveIndirect(RHICommandList& commandList, EPrimitive type, RHIBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	
	RHI_API void RHIDrawPrimitiveInstanced(RHICommandList& commandList, EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance = 0);
	RHI_API void RHIDrawIndexedPrimitiveInstanced(RHICommandList& commandList, EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex = 0, uint32 baseInstance = 0);

	RHI_API void RHIDrawPrimitiveUP(RHICommandList& commandList, EPrimitive type, void const* pVertices, int numVertices, int vetexStride);
	RHI_API void RHIDrawIndexedPrimitiveUP(RHICommandList& commandList, EPrimitive type, void const* pVertices, int numVertices, int vetexStride, uint32 const* pIndices, int numIndex);
	struct VertexDataInfo
	{
		void const* ptr;
		int   size;
		int   stride;
	};
	RHI_API void RHIDrawPrimitiveUP(RHICommandList& commandList, EPrimitive type, int numVertices, VertexDataInfo dataInfos[] , int numData );
	RHI_API void RHIDrawIndexedPrimitiveUP(RHICommandList& commandList, EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex);
	
	RHI_API void RHIDrawMeshTasks(RHICommandList& commandList, uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ);
	RHI_API void RHIDrawMeshTasksIndirect(RHICommandList& commandList, RHIBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	
	
	RHI_API void RHISetFixedShaderPipelineState(RHICommandList& commandList, Matrix4 const& transform, LinearColor const& color = LinearColor(1,1,1,1), RHITexture2D* texture = nullptr, RHISamplerState* sampler = nullptr);
	RHI_API void RHISetFrameBuffer(RHICommandList& commandList, RHIFrameBuffer* frameBuffer);

	enum class EClearBits : uint8
	{
		Color   = 0x1,
		Depth   = 0x2,
		Stencil = 0x4,
		All     = 0xff,
	};

	FORCEINLINE EClearBits operator | (EClearBits a, EClearBits b) { return EClearBits(uint8(a) | uint8(b)); }
	FORCEINLINE bool HaveBits( EClearBits a , EClearBits b ) { return !!(uint8(a) & uint8(b)); }

	RHI_API void RHIClearRenderTargets(RHICommandList& commandList, EClearBits clearBits , LinearColor colors[] ,int numColor , float depth = (float)FRHIZBuffer::FarPlane , uint8 stenceil = 0 );

	RHI_API void RHISetInputStream(RHICommandList& commandList, RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);

	RHI_API void RHISetIndexBuffer(RHICommandList& commandList, RHIBuffer* indexBuffer);

	RHI_API void RHIDispatchCompute(RHICommandList& commandList, uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ );
	RHI_API void RHISetShaderProgram(RHICommandList& commandList, RHIShaderProgram* shaderProgram);

	RHI_API void RHISetGraphicsShaderBoundState(RHICommandList& commandList, GraphicsShaderStateDesc const& stateDesc);

	RHI_API void RHISetMeshShaderBoundState(RHICommandList& commandList, MeshShaderStateDesc const& stateDesc);


	struct GraphicsRenderStateDesc
	{
		RHIRasterizerState*   rasterizerState;
		RHIDepthStencilState* depthStencilState;
		RHIBlendState*        blendState;

		EPrimitive          primitiveType;
		RHIInputLayout*     inputLayout;

		GraphicsRenderStateDesc()
		{
			::memset(this, 0, sizeof(*this));
		}

		GraphicsRenderStateDesc(ENoInit) {}
	};

	struct GraphicsPipelineStateDesc : GraphicsRenderStateDesc
	{

		GraphicsShaderStateDesc shaderBoundState;
		RHIShaderProgram*   shaderProgram;


		GraphicsPipelineStateDesc()
			:GraphicsRenderStateDesc(NoInit)
		{
			::memset(this, 0, sizeof(*this));
		}
	};

	struct MeshRenderStateDesc
	{
		RHIRasterizerState*   rasterizerState;
		RHIDepthStencilState* depthStencilState;
		RHIBlendState*        blendState;

		MeshRenderStateDesc()
		{
			::memset(this, 0, sizeof(*this));
		}
		MeshRenderStateDesc(ENoInit) {}
	};

	struct MeshPipelineStateDesc : MeshRenderStateDesc
	{
		RHIRasterizerState*   rasterizerState;
		RHIDepthStencilState* depthStencilState;
		RHIBlendState*        blendState;

		MeshShaderStateDesc shaderBoundState;
		RHIShaderProgram*   shaderProgram;

		MeshPipelineStateDesc()
			:MeshRenderStateDesc(NoInit)
		{
			::memset(this, 0, sizeof(*this));
		}
	};


	RHI_API void RHISetComputeShader(RHICommandList& commandList, RHIShader* shader);

	RHI_API void RHIFlushCommand(RHICommandList& commandList);

#define RHI_FUNC( FUNC ) virtual FUNC = 0

	class RHISystem
	{
	public:
		virtual ~RHISystem(){}
		virtual RHISystemName getName() const = 0;
		virtual bool initialize(RHISystemInitParams const& initParam) { return true; }
		virtual void clearResourceReference(){}
		virtual void shutdown(){}
		virtual class ShaderFormat* createShaderFormat() { return nullptr; }
		
		RHI_FUNC(RHICommandList& getImmediateCommandList());
		RHI_FUNC(RHISwapChain* RHICreateSwapChain(SwapChainCreationInfo const& info));
		RHI_FUNC(bool RHIBeginRender());
		RHI_FUNC(void RHIEndRender(bool bPresent));

		RHI_FUNC(RHITexture1D*      RHICreateTexture1D(TextureDesc const& desc, void* data));
		RHI_FUNC(RHITexture2D*      RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign));
		RHI_FUNC(RHITexture3D*      RHICreateTexture3D(TextureDesc const& desc, void* data));
		RHI_FUNC(RHITextureCube*    RHICreateTextureCube(TextureDesc const& desc, void* data[]));
		RHI_FUNC(RHITexture2DArray* RHICreateTexture2DArray(TextureDesc const& desc, void* data));
		RHI_FUNC(RHITexture2D*      RHICreateTextureDepth(TextureDesc const& desc));
		
		RHI_FUNC(RHIBuffer*  RHICreateBuffer(uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data));

		RHI_FUNC(void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size));
		RHI_FUNC(void  RHIUnlockBuffer(RHIBuffer* buffer));

		RHI_FUNC(void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData));
		RHI_FUNC(void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData));

		RHI_FUNC(bool RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth));

		//RHI_FUNC(void* RHILockTexture(RHITextureBase* texture, ELockAccess access, uint32 offset, uint32 size));
		//RHI_FUNC(void  RHIUnlockTexture(RHITextureBase* texture));

		RHI_FUNC(RHIFrameBuffer*  RHICreateFrameBuffer());

		RHI_FUNC(RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc));

		RHI_FUNC(RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer));

		RHI_FUNC(RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer));
		RHI_FUNC(RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer));
		RHI_FUNC(RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer));

		RHI_FUNC(RHIShader* RHICreateShader(EShader::Type type));
		RHI_FUNC(RHIShaderProgram* RHICreateShaderProgram());
	};

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

	enum class EStructuredBufferType : uint8
	{
		Const ,
		Buffer ,
		RWBuffer ,
	};

	template< class T >
	class TStructuredBuffer
	{
	public:
		bool initializeResource(uint32 numElement, EStructuredBufferType type = EStructuredBufferType::Const, bool bCPUAccessRead = false )
		{
			uint32 creationFlags = 0;
			switch (type)
			{
			case EStructuredBufferType::Const:
				creationFlags = BCF_UsageConst | BCF_CpuAccessWrite;
				break;
			case EStructuredBufferType::Buffer:
				creationFlags = BCF_CpuAccessWrite | BCF_Structured | BCF_CreateSRV;
				break;
			case EStructuredBufferType::RWBuffer:
				creationFlags = BCF_Structured | BCF_CreateUAV | BCF_CreateSRV;
				break;
			} 
			if (bCPUAccessRead)
				creationFlags |= BCF_CPUAccessRead;
			mResource = RHICreateBuffer(sizeof(T), numElement, creationFlags);
			if( !mResource.isValid() )
				return false;
			return true;
		}

		void releaseResource()
		{
			mResource.release();
		}

		uint32 getElementNum() { return mResource->getSize() / sizeof(T); }

		RHIBuffer* getRHI() { return mResource; }

		bool updateBuffer(TArray<T> const& updatedData) { return updateBuffer(MakeConstView(updatedData)); }
		bool updateBuffer(TArrayView<const T> const& updatedData)
		{
			T* pData = lock();
			if (pData == nullptr)
				return false;
			std::copy(updatedData.data(), updatedData.data() + updatedData.size(), pData);
			unlock();
			return true;
		}

		T*   lock()
		{
			return (T*)RHILockBuffer(mResource, ELockAccess::WriteDiscard);
		}
		void unlock()
		{
			RHIUnlockBuffer(mResource);
		}

		RHIBufferRef mResource;
	};

	struct TextureLoadOption
	{
		bool   bHDR = false;
		bool   bSRGB = false;
		bool   bUseHalf = true;
		bool   bFlipV = false;
		bool   bAutoMipMap = false;
		int    numMipLevel = 0;
		uint32 creationFlags = TCF_DefalutValue;


		ETexture::Format getFormat( int numComponent ) const;

		bool isConvertFloatToHalf() const
		{
			return bHDR && (bUseHalf && isNeedConvertFloatToHalf());
		}
		bool isRGBTextureSupported() const;
		bool isNeedConvertFloatToHalf() const;

		TextureLoadOption& FlipV(bool value = true) { bFlipV = value; return *this; }
		TextureLoadOption& HDR(bool value = true) { bHDR = value; return *this; }
		TextureLoadOption& SRGB(bool value = true) { bSRGB = value; return *this; }
		TextureLoadOption& AutoMipMap(bool value = true){ bAutoMipMap = value;  return *this;  }
		TextureLoadOption& MipLevel(int value = 0){  numMipLevel = value;  return *this;  }
	};


	class RHIUtility
	{
	public:
		static RHITexture2D* LoadTexture2DFromFile(char const* path, TextureLoadOption const& option = TextureLoadOption() );
		static RHITexture2D* LoadTexture2DFromFile(DataCacheInterface& dataCache, char const* path, TextureLoadOption const& option);
		static RHITextureCube* LoadTextureCubeFromFile(char const* paths[], TextureLoadOption const& option = TextureLoadOption());
		static RHITexture2D* CreateTexture2D(ImageData const& imageData, TextureLoadOption const& option);
	};


	class RHISystemFactory
	{
	public:
		virtual ~RHISystemFactory() {}
		virtual RHISystem* createRHISystem(RHISystemName name) = 0;
	};

	CORE_API void RHIRegisterSystem(RHISystemName name, RHISystemFactory* factory);
	CORE_API void RHIUnregisterSystem(RHISystemName name);

	using RHISystemEvent = std::function<void()>;
	CORE_API void RHIRegisterSystemInitializeEvent(RHISystemEvent event);
	CORE_API void RHIRegisterSystemShutdownEvent(RHISystemEvent event);

	template<RHISystemName SystemName, typename TSystem >
	class TRHISystemModule : public IModuleInterface
		                   , public RHISystemFactory
	{
	public:
		void startupModule() override
		{
			RHIRegisterSystem(SystemName, this);
		}

		void shutdownModule() override
		{
			RHIUnregisterSystem(SystemName);
		}

		RHISystem* createRHISystem(RHISystemName name) override
		{
			return new TSystem();
		}
	};

#define EXPORT_RHI_SYSTEM_MODULE( SYSTEM_NAME , SYSTEM )\
	using SYSTEM##Module = TRHISystemModule<SYSTEM_NAME,SYSTEM >;\
	EXPORT_MODULE(SYSTEM##Module)

}//namespace Render




#endif // RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
