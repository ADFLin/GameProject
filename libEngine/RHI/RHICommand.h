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

namespace Render
{
	extern CORE_API class RHISystem* gRHISystem;

	enum class RHISytemName
	{
		D3D11,
		Opengl,
	};

	struct RHISystemInitParam
	{
		int  numSamples;
#if SYS_PLATFORM_WIN
		HWND hWnd;
		HDC  hDC;

		RHISystemInitParam()
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

	bool RHISystemInitialize(RHISytemName name , RHISystemInitParam const& initParam );
	void RHISystemShutdown();
	bool RHIBeginRender();
	void RHIEndRender(bool bPresent);

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

	void             RHIFlushCommand();

	RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info);

	RHITexture1D*    RHICreateTexture1D(Texture::Format format, int length ,
										int numMipLevel = 0, uint32 creationFlags = TCF_DefalutValue,void* data = nullptr);
	RHITexture2D*    RHICreateTexture2D(Texture::Format format, int w, int h, 
										int numMipLevel = 0 ,int numSamples = 1, uint32 creationFlags = TCF_DefalutValue, void* data = nullptr, int dataAlign = 0);
	RHITexture3D*    RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ , 
										int numMipLevel = 0, int numSamples = 1, uint32 creationFlags = TCF_DefalutValue , void* data = nullptr);

	RHITextureCube*  RHICreateTextureCube(Texture::Format format, int size, int numMipLevel = 0, uint32 creationFlags = TCF_DefalutValue, void* data[] = nullptr);
	RHITexture2DArray* RHICreateTexture2DArray(Texture::Format format, int w, int h, int layerSize,
											   int numMipLevel = 0, int numSamples = 1, uint32 creationFlags = TCF_DefalutValue, void* data = nullptr);
	RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h , int numMipLevel = 1 , int numSamples = 1);


	RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);
	RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);

	void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset = 0, uint32 size = 0 );
	void  RHIUnlockBuffer(RHIVertexBuffer* buffer);
	void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset = 0, uint32 size = 0);
	void  RHIUnlockBuffer(RHIIndexBuffer* buffer);

	RHIFrameBuffer*  RHICreateFrameBuffer();

	RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);

	RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);

	RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
	RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
	RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

	RHIShader*        RHICreateShader(Shader::Type type);
	RHIShaderProgram* RHICreateShaderProgram();


	class RHICommandList
	{
	public:
		virtual ~RHICommandList() {}
		static RHICommandList& GetImmediateList();
	};


	//
	void RHISetRasterizerState(RHICommandList& commandList , RHIRasterizerState& rasterizerState);
	void RHISetBlendState(RHICommandList& commandList , RHIBlendState& blendState);
	void RHISetDepthStencilState(RHICommandList& commandList, RHIDepthStencilState& depthStencilState, uint32 stencilRef = -1);

	void RHISetViewport(RHICommandList& commandList, int x, int y, int w, int h);
	void RHISetScissorRect(RHICommandList& commandList , int x = 0, int y = 0, int w = 0, int h = 0);

	
	void RHIDrawPrimitive(RHICommandList& commandList, PrimitiveType type, int vStart, int nv);
	void RHIDrawIndexedPrimitive(RHICommandList& commandList, PrimitiveType type, int indexStart, int nIndex, uint32 baseVertex = 0);
	void RHIDrawPrimitiveIndirect(RHICommandList& commandList, PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	void RHIDrawIndexedPrimitiveIndirect(RHICommandList& commandList, PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	void RHIDrawPrimitiveInstanced(RHICommandList& commandList, PrimitiveType type, int vStart, int nv, int numInstance );

	void RHIDrawPrimitiveUP(RHICommandList& commandList, PrimitiveType type, void const* pVertices, int numVerex, int vetexStride);
	void RHIDrawIndexedPrimitiveUP(RHICommandList& commandList, PrimitiveType type, void const* pVertices, int numVerex, int vetexStride , int const* pIndices , int numIndex );

	void RHISetupFixedPipelineState(RHICommandList& commandList, Matrix4 const& transform, RHITexture2D* textures[] = nullptr, int numTexture = 0);
	void RHISetFrameBuffer(RHICommandList& commandList, RHIFrameBuffer* frameBuffer, RHITextureDepth* overrideDepthTexture = nullptr);

	void RHISetInputStream(RHICommandList& commandList, RHIInputLayout& inputLayout, InputStreamInfo inputStreams[], int numInputStream);

	void RHISetIndexBuffer(RHICommandList& commandList, RHIIndexBuffer* indexBuffer);

	void RHIDispatchCompute(RHICommandList& commandList, uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ );
	void RHISetShaderProgram(RHICommandList& commandList, RHIShaderProgram* shaderProgram);


#define RHI_FUNC( FUN ) virtual FUN = 0

	class RHISystem
	{
	public:
		virtual RHISytemName getName() const = 0;
		virtual bool initialize(RHISystemInitParam const& initParam) { return true; }
		virtual void shutdown(){}
		virtual class ShaderFormat* createShaderFormat() { return nullptr; }
		
		RHI_FUNC(RHICommandList& getImmediateCommandList());
		RHI_FUNC(RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info));
		RHI_FUNC(bool RHIBeginRender());
		RHI_FUNC(void RHIEndRender(bool bPresent));

		RHI_FUNC(RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel, uint32 creationFlag, void* data));
		RHI_FUNC(RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, int numSamples, uint32 creationFlag,
			void* data, int dataAlign));
		RHI_FUNC(RHITexture3D*    RHICreateTexture3D(
			Texture::Format format, int sizeX, int sizeY, int sizeZ,
			int numMipLevel, int numSamples , uint32 creationFlag , 
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
		
		RHI_FUNC(RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlag, void* data));
		RHI_FUNC(RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlag, void* data));

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

		RHI_FUNC(RHIShader* RHICreateShader(Shader::Type type));
		RHI_FUNC(RHIShaderProgram* RHICreateShaderProgram());
	};

	template< class T >
	class TStructuredBuffer
	{
	public:
		bool initializeResource(uint32 numElement, uint32 creationFlags = BCF_DefalutValue | BCF_CreateSRV | BCF_UsageDynamic )
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
		static RHITextureCube* LoadTextureCubeFromFile(char const* paths[], TextureLoadOption const& option = TextureLoadOption() );
	};


	template<
		Sampler::Filter Filter = Sampler::ePoint,
		Sampler::AddressMode AddressU = Sampler::eWarp ,
		Sampler::AddressMode AddressV = Sampler::eWarp ,
		Sampler::AddressMode AddressW = Sampler::eWarp >
	class TStaticSamplerState : public StaticRHIStateT < 
		TStaticSamplerState< Filter, AddressU, AddressV, AddressW >
		, RHISamplerState >
	{
	public:
		static RHISamplerStateRef CreateRHI()
		{
			SamplerStateInitializer initializer;
			initializer.filter = Filter;
			initializer.addressU = AddressU;
			initializer.addressV = AddressV;
			initializer.addressW = AddressW;
			return RHICreateSamplerState(initializer);
		}
	};

	template<
		ECullMode CullMode = ECullMode::Back,
		EFillMode FillMode = EFillMode::Solid,
		bool bEnableScissor = false >
	class TStaticRasterizerState : public StaticRHIStateT< 
		TStaticRasterizerState< CullMode, FillMode, bEnableScissor >,
		RHIRasterizerState >
	{
	public:
		static RHIRasterizerStateRef CreateRHI()
		{
			RasterizerStateInitializer initializer;
			initializer.fillMode = FillMode;
			initializer.cullMode = CullMode;
			initializer.bEnableScissor = bEnableScissor;
			return RHICreateRasterizerState(initializer);
		}
	};


	RHIRasterizerState& GetStaticRasterizerState(ECullMode cullMode, EFillMode fillMode);

	template <
		bool bWriteDepth = true,
		ECompareFun DepthFun = ECompareFun::Less,
		bool bEnableStencilTest = false,
		ECompareFun StencilFun = ECompareFun::Always,
		Stencil::Operation StencilFailOp = Stencil::eKeep,
		Stencil::Operation ZFailOp = Stencil::eKeep,
		Stencil::Operation ZPassOp = Stencil::eKeep,
		ECompareFun BackStencilFun = ECompareFun::Always,
		Stencil::Operation BackStencilFailOp = Stencil::eKeep,
		Stencil::Operation BackZFailOp = Stencil::eKeep,
		Stencil::Operation BackZPassOp = Stencil::eKeep,
		uint32 StencilReadMask = -1,
		uint32 StencilWriteMask = -1 >
	class TStaticDepthStencilSeparateState : public StaticRHIStateT<
		TStaticDepthStencilSeparateState< 
			bWriteDepth, DepthFun, bEnableStencilTest, StencilFun , StencilFailOp ,
		    ZFailOp , ZPassOp, BackStencilFun , BackStencilFailOp, BackZFailOp ,BackZPassOp ,
		    StencilReadMask ,StencilWriteMask >, 
		RHIDepthStencilState >
	{
	public:
		static RHIDepthStencilStateRef CreateRHI()
		{
			DepthStencilStateInitializer initializer;
			initializer.bWriteDepth = bWriteDepth;
			initializer.depthFun = DepthFun;
			initializer.bEnableStencilTest = bEnableStencilTest;
			initializer.stencilFun = StencilFun;
			initializer.stencilFailOp = StencilFailOp;
			initializer.zFailOp = ZFailOp;
			initializer.zPassOp = ZPassOp;
			initializer.stencilFunBack = BackStencilFun;
			initializer.stencilFailOpBack = BackStencilFailOp;
			initializer.zFailOpBack = BackZFailOp;
			initializer.zPassOpBack = BackZPassOp;
			initializer.stencilReadMask = StencilReadMask;
			initializer.stencilWriteMask = StencilWriteMask;
			return RHICreateDepthStencilState(initializer);
		}
	};

	template <
		bool bWriteDepth = true,
		ECompareFun DepthFun = ECompareFun::Less,
		bool bEnableStencilTest = false,
		ECompareFun StencilFun = ECompareFun::Always,
		Stencil::Operation StencilFailOp = Stencil::eKeep,
		Stencil::Operation ZFailOp = Stencil::eKeep,
		Stencil::Operation ZPassOp = Stencil::eKeep,
		uint32 StencilReadMask = -1,
		uint32 StencilWriteMask = -1 >
	class TStaticDepthStencilState : public TStaticDepthStencilSeparateState<
		bWriteDepth, DepthFun, bEnableStencilTest, 
		StencilFun, StencilFailOp, ZFailOp, ZPassOp,
		StencilFun, StencilFailOp, ZFailOp, ZPassOp,
		StencilReadMask, StencilWriteMask >
	{

	};

	typedef TStaticDepthStencilState<false, ECompareFun::Always> StaticDepthDisableState;

	template<
		ColorWriteMask WriteColorMask0 = CWM_RGBA,
		Blend::Factor SrcColorFactor0 = Blend::eOne,
		Blend::Factor DestColorFactor0 = Blend::eZero ,
		Blend::Operation ColorOp0 = Blend::eAdd ,
		Blend::Factor SrcAlphaFactor0 = Blend::eOne,
		Blend::Factor DestAlphaFactor0 = Blend::eZero ,
		Blend::Operation AlphaOp0 = Blend::eAdd ,
		bool bEnableAlphaToCoverage = false >
	class TStaticBlendSeparateState : public StaticRHIStateT< 
		TStaticBlendSeparateState< WriteColorMask0 , SrcColorFactor0 , DestColorFactor0 ,ColorOp0 , SrcAlphaFactor0 ,DestAlphaFactor0, AlphaOp0 , bEnableAlphaToCoverage >,
		RHIBlendState >
	{
	public:
		static RHIBlendStateRef CreateRHI()
		{

			BlendStateInitializer initializer;
#define SET_TRAGET_VALUE( INDEX )\
		initializer.targetValues[INDEX].writeMask = WriteColorMask##INDEX;\
		initializer.targetValues[INDEX].op = ColorOp##INDEX;\
		initializer.targetValues[INDEX].srcColor = SrcColorFactor##INDEX;\
		initializer.targetValues[INDEX].destColor = DestColorFactor##INDEX;\
		initializer.targetValues[INDEX].opAlpha = AlphaOp##INDEX;\
		initializer.targetValues[INDEX].srcAlpha = SrcAlphaFactor##INDEX;\
		initializer.targetValues[INDEX].destAlpha = DestAlphaFactor##INDEX;

			SET_TRAGET_VALUE(0);
			initializer.bEnableAlphaToCoverage = bEnableAlphaToCoverage;
#undef SET_TRAGET_VALUE
			return RHICreateBlendState(initializer);

		}
	};


	template<
		ColorWriteMask WriteColorMask0 = CWM_RGBA,
		Blend::Factor SrcFactor0 = Blend::eOne,
		Blend::Factor DestFactor0 = Blend::eZero ,
		Blend::Operation Op0 = Blend::eAdd,
		bool bEnableAlphaToCoverage = false >
	class TStaticBlendState : public TStaticBlendSeparateState<
		WriteColorMask0 , SrcFactor0 , DestFactor0 , Op0 , SrcFactor0, DestFactor0, Op0 , bEnableAlphaToCoverage >
	{
	};

	template<
		ColorWriteMask WriteColorMask0 = CWM_RGBA >
		class TStaticAlphaToCoverageBlendState : public TStaticBlendState<
		WriteColorMask0, Blend::eOne, Blend::eZero, Blend::eAdd, true >
	{
	};
}//namespace Render

#endif // RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
