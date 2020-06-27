#pragma once
#ifndef RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
#define RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D

#include "RHICommon.h"
#include "RHIGlobalResource.h"

#include "CoreShare.h"

#include "SystemPlatform.h"


#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif

class DataCacheInterface;
struct ImageData;


enum class RHISytemName
{
	D3D11,
	D3D12,
	Opengl,
	Vulkan,
};

#if USE_RHI_RESOURCE_TRACE
#define RHI_TRACE_FUNC_NAME(NAME) NAME##Trace
#define RHI_TRACE_FUNC( NAME , ... ) RHI_TRACE_FUNC_NAME(NAME)(ResTraceInfo const& traceInfo, ##__VA_ARGS__)
#define RHI_TRACE_CODE( CODE )\
	auto* result = CODE;\
	if (result)\
	{\
		result->mTrace = traceInfo;\
	}\
	return result;
#else
#define RHI_TRACE_FUNC_NAME(NAME) NAME
#define RHI_TRACE_FUNC( NAME , ... ) NAME(__VA_ARGS__)
#define RHI_TRACE_CODE( CODE ) return CODE
#endif


namespace Render
{
	extern CORE_API class RHISystem* gRHISystem;

	FORCEINLINE bool RHIIsInitialized()
	{
		return gRHISystem != nullptr;
	}

	struct RHISystemInitParams
	{
		int  numSamples;
#if SYS_PLATFORM_WIN
		HWND hWnd;
		HDC  hDC;

		RHISystemInitParams()
		{
			numSamples = 1;
		}
#endif
	};



	class RHIRenderWindow
	{
	public:
		virtual RHITexture2D* getBackBufferTexture() = 0;
		virtual void Present() = 0;
	};

	RHI_API bool RHISystemInitialize(RHISytemName name , RHISystemInitParams const& initParam );
	RHI_API void RHISystemShutdown();
	RHI_API bool RHIBeginRender();
	RHI_API void RHIEndRender(bool bPresent);

	struct PlatformWindowInfo
	{
		HWND hWnd;
		HDC  hDC;
		int  Width;
		int  Height;
		bool bFullScreen;
		int  numSamples;

		PlatformWindowInfo()
		{
			numSamples = 1;
			bFullScreen = false;
		}

	};
	RHI_API RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info);

	RHI_API RHITexture1D*    RHICreateTexture1D(Texture::Format format, int length ,
										int numMipLevel = 0, uint32 creationFlags = TCF_DefalutValue,void* data = nullptr);

	RHI_API RHITexture2D*   RHI_TRACE_FUNC(RHICreateTexture2D,
		Texture::Format format, int w, int h,
		int numMipLevel = 0, int numSamples = 1, uint32 creationFlags = TCF_DefalutValue,
		void* data = nullptr, int dataAlign = 0);

	RHI_API RHITexture3D*    RHI_TRACE_FUNC(RHICreateTexture3D,
		Texture::Format format, int sizeX, int sizeY, int sizeZ , 
		int numMipLevel = 0, int numSamples = 1, uint32 creationFlags = TCF_DefalutValue , 
		void* data = nullptr);

	RHI_API RHITextureCube*  RHI_TRACE_FUNC(RHICreateTextureCube,
		Texture::Format format, int size, int numMipLevel = 0, uint32 creationFlags = TCF_DefalutValue, 
		void* data[] = nullptr);

	RHI_API RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray,
		Texture::Format format, int w, int h, int layerSize,
		int numMipLevel = 0, int numSamples = 1, uint32 creationFlags = TCF_DefalutValue, 
		void* data = nullptr);

	RHI_API RHITextureDepth* RHI_TRACE_FUNC(RHICreateTextureDepth,
		Texture::DepthFormat format, int w, int h , int numMipLevel = 1 , int numSamples = 1);

	RHI_API RHIVertexBuffer* RHI_TRACE_FUNC(RHICreateVertexBuffer,
		uint32 vertexSize, uint32 numVertices, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);

	RHI_API RHIIndexBuffer*  RHI_TRACE_FUNC(RHICreateIndexBuffer,
		uint32 nIndices, bool bIntIndex, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);

	RHI_API void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset = 0, uint32 size = 0 );
	RHI_API void  RHIUnlockBuffer(RHIVertexBuffer* buffer);
	RHI_API void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset = 0, uint32 size = 0);
	RHI_API void  RHIUnlockBuffer(RHIIndexBuffer* buffer);

	RHI_API RHIFrameBuffer*  RHICreateFrameBuffer();

#if USE_RHI_RESOURCE_TRACE
	RHI_API RHIInputLayout*  RHICreateInputLayoutTrace(ResTraceInfo const& traceInfo, InputLayoutDesc const& desc);
#else
	RHI_API RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);
#endif
	RHI_API RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);

	RHI_API RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
	RHI_API RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
	RHI_API RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);


	RHI_API RHIShader*        RHICreateShader(EShader::Type type);


#if USE_RHI_RESOURCE_TRACE
	RHI_API RHIShaderProgram* RHICreateShaderProgramTrace(ResTraceInfo const& traceInfo);
#else
	RHI_API RHIShaderProgram* RHICreateShaderProgram();
#endif

	class RHICommandList
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
		int x;
		int y;
		int w;
		int h;
		float zNear;
		float zFar;
	};
	RHI_API void RHISetViewport(RHICommandList& commandList, int x, int y, int w, int h, float zNear = 0, float zFar = 1);
	RHI_API void RHISetScissorRect(RHICommandList& commandList , int x = 0, int y = 0, int w = 0, int h = 0);

	
	RHI_API void RHIDrawPrimitive(RHICommandList& commandList, EPrimitive type, int vStart, int nv);
	RHI_API void RHIDrawIndexedPrimitive(RHICommandList& commandList, EPrimitive type, int indexStart, int nIndex, uint32 baseVertex = 0);
	RHI_API void RHIDrawPrimitiveIndirect(RHICommandList& commandList, EPrimitive type, RHIVertexBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	RHI_API void RHIDrawIndexedPrimitiveIndirect(RHICommandList& commandList, EPrimitive type, RHIVertexBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	
	RHI_API void RHIDrawPrimitiveInstanced(RHICommandList& commandList, EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance = 0);
	RHI_API void RHIDrawIndexedPrimitiveInstanced(RHICommandList& commandList, EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex = 0, uint32 baseInstance = 0);

	RHI_API void RHIDrawPrimitiveUP(RHICommandList& commandList, EPrimitive type, void const* pVertices, int numVertex, int vetexStride);
	RHI_API void RHIDrawIndexedPrimitiveUP(RHICommandList& commandList, EPrimitive type, void const* pVertices, int numVertex, int vetexStride, int const* pIndices, int numIndex);
	struct VertexDataInfo
	{
		void const* ptr;
		int   size;
		int   stride;
	};
	RHI_API void RHIDrawPrimitiveUP(RHICommandList& commandList, EPrimitive type, int numVertex, VertexDataInfo dataInfos[] , int numData );
	RHI_API void RHIDrawIndexedPrimitiveUP(RHICommandList& commandList, EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numVertexData, int const* pIndices, int numIndex);
	RHI_API void RHISetupFixedPipelineState(RHICommandList& commandList, Matrix4 const& transform, LinearColor const& color = LinearColor(1,1,1,1), RHITexture2D* textures[] = nullptr, int numTexture = 0);
	RHI_API void RHISetFrameBuffer(RHICommandList& commandList, RHIFrameBuffer* frameBuffer);

	enum class EClearBits
	{
		Color = 0x1,
		Depth = 0x2,
		Stencil = 0x4,
	};

	FORCEINLINE EClearBits operator | (EClearBits a, EClearBits b) { return EClearBits(uint8(a) | uint8(b)); }
	FORCEINLINE bool HaveBits( EClearBits a , EClearBits b ) { return !!(uint8(a) & uint8(b)); }

	RHI_API void RHIClearRenderTargets(RHICommandList& commandList, EClearBits clearBits , LinearColor colors[] ,int numColor , float depth = 0 , uint8 stenceil = 0 );

	RHI_API void RHISetInputStream(RHICommandList& commandList, RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream);

	RHI_API void RHISetIndexBuffer(RHICommandList& commandList, RHIIndexBuffer* indexBuffer);

	RHI_API void RHIDispatchCompute(RHICommandList& commandList, uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ );
	RHI_API void RHISetShaderProgram(RHICommandList& commandList, RHIShaderProgram* shaderProgram);

	struct GraphicShaderBoundState
	{
		RHIShader* vertexShader;
		RHIShader* pixelShader;
		RHIShader* geometryShader;
		RHIShader* hullShader;
		RHIShader* domainShader;

		GraphicShaderBoundState()
		{
			::memset(this, 0, sizeof(*this));
		}
	};
	RHI_API void RHISetGraphicsShaderBoundState(RHICommandList& commandList, GraphicShaderBoundState const& state);

	RHI_API void RHIFlushCommand(RHICommandList& commandList);

#define RHI_FUNC( FUNC ) virtual FUNC = 0

	class RHISystem
	{
	public:
		virtual ~RHISystem(){}
		virtual RHISytemName getName() const = 0;
		virtual bool initialize(RHISystemInitParams const& initParam) { return true; }
		virtual void shutdown(){}
		virtual class ShaderFormat* createShaderFormat() { return nullptr; }
		
		RHI_FUNC(RHICommandList& getImmediateCommandList());
		RHI_FUNC(RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info));
		RHI_FUNC(bool RHIBeginRender());
		RHI_FUNC(void RHIEndRender(bool bPresent));

		RHI_FUNC(RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel, uint32 creationFlags, void* data));

		RHI_FUNC(RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 creationFlags,
			void* data, int dataAlign));

		RHI_FUNC(RHITexture3D*    RHICreateTexture3D(
			Texture::Format format, int sizeX, int sizeY, int sizeZ,
			int numMipLevel, int numSamples , uint32 creationFlags , 
			void* data));
		RHI_FUNC(RHITextureCube*  RHICreateTextureCube(
			Texture::Format format, int size, 
			int numMipLevel, uint32 creationFlags, 
			void* data[]));

		RHI_FUNC(RHITexture2DArray* RHICreateTexture2DArray(
			Texture::Format format, int w, int h, int layerSize,
			int numMipLevel, int numSamples, uint32 creationFlags,
			void* data));

		RHI_FUNC(RHITextureDepth* RHICreateTextureDepth(
			Texture::DepthFormat format, int w, int h , 
			int numMipLevel, int numSamples ) );
		
		RHI_FUNC(RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data));
		RHI_FUNC(RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data));

		RHI_FUNC(void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size));
		RHI_FUNC(void  RHIUnlockBuffer(RHIVertexBuffer* buffer));
		RHI_FUNC(void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size));
		RHI_FUNC(void  RHIUnlockBuffer(RHIIndexBuffer* buffer));

		RHI_FUNC(RHIFrameBuffer*  RHICreateFrameBuffer());

		RHI_FUNC(RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc));

		RHI_FUNC(RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer));

		RHI_FUNC(RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer));
		RHI_FUNC(RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer));
		RHI_FUNC(RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer));

		RHI_FUNC(RHIShader* RHICreateShader(EShader::Type type));
		RHI_FUNC(RHIShaderProgram* RHICreateShaderProgram());
	};


#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

	template< class T >
	class TStructuredBuffer
	{
	public:
		bool initializeResource(uint32 numElement, uint32 creationFlags = BCF_DefalutValue| BCF_UsageConst | BCF_UsageDynamic )
		{
			mResource = RHICreateVertexBuffer(sizeof(T), numElement, creationFlags);
			if( !mResource.isValid() )
				return false;
			return true;
		}

		void releaseResources()
		{
			mResource->release();
		}

		uint32 getElementNum() { return mResource->getSize() / sizeof(T); }

		RHIVertexBuffer* getRHI() { return mResource; }

		T*   lock()
		{
			return (T*)RHILockBuffer(mResource, ELockAccess::WriteDiscard);
		}
		void unlock()
		{
			RHIUnlockBuffer(mResource);
		}

		TRefCountPtr<RHIVertexBuffer> mResource;
	};

	struct TextureLoadOption
	{
		bool   bHDR = false;
		bool   bSRGB = false;
		bool   bReverseH = false;
		int    numMipLevel = 0;
		uint32 creationFlags = TCF_DefalutValue;


		Texture::Format getFormat( int numComponent ) const;

		TextureLoadOption& ReverseH(bool value = true) { bReverseH = value; return *this; }
		TextureLoadOption& HDR(bool value = true) { bHDR = value; return *this; }
		TextureLoadOption& SRGB(bool value = true) { bSRGB = value; return *this; }
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

}//namespace Render




#endif // RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
