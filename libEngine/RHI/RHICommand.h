#pragma once
#ifndef RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
#define RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D

#include "RHICommon.h"
#include "RHIStaticStateObject.h"

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

	RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info);

	RHITexture1D*    RHICreateTexture1D(Texture::Format format, int length ,
										int numMipLevel = 0, uint32 creationFlags = TCF_DefalutValue,void* data = nullptr);

	RHITexture2D*    RHICreateTexture2D(Texture::Format format, int w, int h, 
										int numMipLevel = 0 , uint32 creationFlags = TCF_DefalutValue, void* data = nullptr, int dataAlign = 0);
	RHITexture3D*    RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ , uint32 creationFlags = TCF_DefalutValue , void* data = nullptr);

	RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int );
	RHITextureCube*  RHICreateTextureCube();

	RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);
	RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);
	RHIUniformBuffer* RHICreateUniformBuffer(uint32 elementSize, uint32 numElement, uint32 creationFlags = BCF_DefalutValue, void* data = nullptr);

	void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset = 0, uint32 size = 0 );
	void  RHIUnlockBuffer(RHIVertexBuffer* buffer);
	void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset = 0, uint32 size = 0);
	void  RHIUnlockBuffer(RHIIndexBuffer* buffer);
	void* RHILockBuffer(RHIUniformBuffer* buffer, ELockAccess access, uint32 offset = 0, uint32 size = 0);
	void  RHIUnlockBuffer(RHIUniformBuffer* buffer);


	RHIFrameBuffer*  RHICreateFrameBuffer();

	RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc);

	RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);

	RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
	RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
	RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);

	//
	void RHISetRasterizerState(RHIRasterizerState& rasterizerState);
	void RHISetBlendState(RHIBlendState& blendState);
	void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef = -1);

	void RHISetViewport(int x, int y, int w, int h);
	void RHISetScissorRect(bool bEnable, int x = 0, int y = 0, int w = 0, int h = 0);

	
	void RHIDrawPrimitive(PrimitiveType type, int vStart, int nv);
	void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex);
	void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, ECompValueType indexType , RHIVertexBuffer* commandBuffer, int offset = 0, int numCommand = 1, int commandStride = 0);
	void RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, int numInstance );

	void RHIDrawPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride);
	void RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride , int* pIndices , int numIndex );

	void RHISetupFixedPipelineState(Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture = 0, RHITexture2D** textures = nullptr);
	void RHISetFrameBuffer(RHIFrameBuffer& frameBuffer, RHITextureDepth* overrideDepthTexture = nullptr);

	void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer);


#define RHIFUNCTION( FUN ) virtual FUN = 0

	class RHISystem
	{
	public:
		virtual bool initialize(RHISystemInitParam const& initParam) { return true; }
		virtual void shutdown(){}
		
		RHIFUNCTION(RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info));
		RHIFUNCTION(bool RHIBeginRender());
		RHIFUNCTION(void RHIEndRender(bool bPresent));

		RHIFUNCTION(RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel, uint32 creationFlag, void* data));
		RHIFUNCTION(RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, uint32 creationFlag, void* data, int dataAlign));
		RHIFUNCTION(RHITexture3D*    RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ, uint32 creationFlag , void* data));
		RHIFUNCTION(RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h) );
		RHIFUNCTION(RHITextureCube*  RHICreateTextureCube() );
		RHIFUNCTION(RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlag, void* data));
		RHIFUNCTION(RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlag, void* data));
		RHIFUNCTION(RHIUniformBuffer* RHICreateUniformBuffer(uint32 elementSize, uint32 numElement, uint32 creationFlag, void* data));

		RHIFUNCTION(void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size));
		RHIFUNCTION(void  RHIUnlockBuffer(RHIVertexBuffer* buffer));
		RHIFUNCTION(void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size));
		RHIFUNCTION(void  RHIUnlockBuffer(RHIIndexBuffer* buffer));
		RHIFUNCTION(void* RHILockBuffer(RHIUniformBuffer* buffer, ELockAccess access, uint32 offset, uint32 size));
		RHIFUNCTION(void  RHIUnlockBuffer(RHIUniformBuffer* buffer));

		RHIFUNCTION(RHIFrameBuffer*  RHICreateFrameBuffer());

		RHIFUNCTION(RHIInputLayout*  RHICreateInputLayout(InputLayoutDesc const& desc));

		RHIFUNCTION(RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer));

		RHIFUNCTION(RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer));
		RHIFUNCTION(RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer));
		RHIFUNCTION(RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer));

		RHIFUNCTION(void RHISetRasterizerState(RHIRasterizerState& rasterizerState));
		RHIFUNCTION(void RHISetBlendState(RHIBlendState& blendState));
		RHIFUNCTION(void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef = -1));
		
		RHIFUNCTION(void RHISetViewport(int x, int y, int w, int h));
		RHIFUNCTION(void RHISetScissorRect(bool bEnable, int x , int y , int w , int h ));

		RHIFUNCTION(void RHIDrawPrimitive(PrimitiveType type, int vStart, int nv));
		RHIFUNCTION(void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex));
		RHIFUNCTION(void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride));
		RHIFUNCTION(void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, ECompValueType indexType, RHIVertexBuffer* commandBuffer, int offset , int numCommand, int commandStride));
		RHIFUNCTION(void RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, int numInstance));

		RHIFUNCTION(void RHIDrawPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride));
		RHIFUNCTION(void RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride, int* pIndices, int numIndex));

		RHIFUNCTION(void RHISetFrameBuffer(RHIFrameBuffer& frameBuffer, RHITextureDepth* overrideDepthTexture));
		RHIFUNCTION(void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer));
		RHIFUNCTION(void RHISetupFixedPipelineState(Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture , RHITexture2D** textures));

	};



	struct TextureLoadOption
	{
		bool   bHDR = false;
		bool   bSRGB = false;
		bool   bReverseH = false;
		int    numMipLevel = 0;
		uint32 creationFlags = TCF_DefalutValue;

		TextureLoadOption& ReverseH(bool value = true) { bReverseH = value; return *this; }
		TextureLoadOption& HDR(bool value = true) { bHDR = value; return *this; }
		TextureLoadOption& SRGB(bool value = true) { bSRGB = value; return *this; }
		TextureLoadOption& MinpLevel(int value = 0){  numMipLevel = value;  return *this;  }
	};

	class RHIUtility
	{
	public:
		static RHITexture2D* LoadTexture2DFromFile(char const* path, TextureLoadOption const& option = TextureLoadOption() );
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
		EFillMode FillMode = EFillMode::Solid >
	class TStaticRasterizerState : public StaticRHIStateT< 
		TStaticRasterizerState< CullMode, FillMode >, 
		RHIRasterizerState >
	{
	public:
		static RHIRasterizerStateRef CreateRHI()
		{
			RasterizerStateInitializer initializer;
			initializer.fillMode = FillMode;
			initializer.cullMode = CullMode;
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
		Blend::Operation AlphaOp0 = Blend::eAdd >
	class TStaticBlendSeparateState : public StaticRHIStateT< 
		TStaticBlendSeparateState< WriteColorMask0 , SrcColorFactor0 , DestColorFactor0 ,ColorOp0 , SrcAlphaFactor0 ,DestAlphaFactor0, AlphaOp0  >,
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

#undef SET_TRAGET_VALUE
			return RHICreateBlendState(initializer);

		}
	};


	template<
		ColorWriteMask WriteColorMask0 = CWM_RGBA,
		Blend::Factor SrcFactor0 = Blend::eOne,
		Blend::Factor DestFactor0 = Blend::eZero ,
		Blend::Operation Op0 = Blend::eAdd >
	class TStaticBlendState : public TStaticBlendSeparateState<
		WriteColorMask0 , SrcFactor0 , DestFactor0 , Op0 , SrcFactor0, DestFactor0, Op0 >
	{
	};
}//namespace Render

#endif // RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
