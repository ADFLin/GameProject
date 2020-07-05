#pragma once
#ifndef RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
#define RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29

#include "RHICommon.h"
#include "CoreShare.h"
#include "GlobalShader.h"

namespace Render
{

	class SimplePipelineProgram : public GlobalShaderProgram
	{
	public:
		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SimplePipelineShader";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamTexture.bind(parameterMap, SHADER_PARAM(Texture));
			mParamTextureSampler.bind(parameterMap, SHADER_PARAM(TextureSampler));
			mParamColor.bind(parameterMap, SHADER_PARAM(Color));
			mParamXForm.bind(parameterMap, SHADER_PARAM(XForm));
		}

		void setParameters(RHICommandList& commandList, Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler);

		ShaderParameter mParamTexture;
		ShaderParameter mParamTextureSampler;
		ShaderParameter mParamColor;
		ShaderParameter mParamXForm;
	};


	class ShaderCompileOption;
	class MaterialShaderProgram;
	CORE_API extern class SimplePipelineProgram* GSimpleShaderPipline;
	CORE_API extern class SimplePipelineProgram* GSimpleShaderPiplineCT;
	CORE_API extern class SimplePipelineProgram* GSimpleShaderPiplineT;
	CORE_API extern class SimplePipelineProgram* GSimpleShaderPiplineC;
	CORE_API extern class MaterialMaster* GDefalutMaterial;
	CORE_API extern class MaterialShaderProgram GSimpleBasePass;
	

	CORE_API bool InitGlobalRHIResource();
	CORE_API void ReleaseGlobalRHIResource();

	
	class GlobalRHIResourceBase
	{
	public:
		CORE_API GlobalRHIResourceBase();
		virtual ~GlobalRHIResourceBase() {}
		virtual void restoreRHI() = 0;
		virtual void releaseRHI() = 0;

		GlobalRHIResourceBase* mNext;

		CORE_API static void ReleaseAllResource();
		CORE_API static void RestoreAllResource();
	};
	
	template< class RHIResourceType >
	class TGlobalRHIResource : public GlobalRHIResourceBase
	{
	public:
		bool isValid() const { return mResource.isValid(); }
		bool initialize(RHIResourceType* resource)
		{
			mResource = resource;
			return mResource.isValid();
		}
		virtual void restoreRHI() override {}
		virtual void releaseRHI() override { mResource.release(); }

		RHIResourceType* getResource() { return mResource; }

		operator RHIResourceType* (void) { return mResource.get(); }
		operator RHIResourceType* (void) const { return mResource.get(); }
		RHIResourceType*       operator->() { return mResource.get(); }
		RHIResourceType const* operator->() const { return mResource.get(); }
		RHIResourceType&       operator *(void) { return *mResource.get(); }
		RHIResourceType const& operator *(void) const { return *mResource.get(); }

		TRefCountPtr< RHIResourceType > mResource;
	};

	template< class ThisClass , class RHIResource >
	class StaticRHIResourceT
	{
	public:
		static RHIResource& GetRHI()
		{
			static TStaticRHIResourceObject< RHIResource >* sObject;
			if( sObject == nullptr )
			{
				sObject = new TStaticRHIResourceObject<RHIResource>();
				sObject->restoreRHI();
			}
			return sObject->getRHI();
		}
	
	private:
		template< class RHIResource >
		class TStaticRHIResourceObject : public GlobalRHIResourceBase
		{
	
		public:
	
			RHIResource& getRHI() { return *mResource; }
	
			virtual void restoreRHI()
			{
				mResource = ThisClass::CreateRHI();
			}
			virtual void releaseRHI()
			{
				mResource.release();
			}
	
			TRefCountPtr< RHIResource > mResource;
		};
	
	};

	CORE_API extern TGlobalRHIResource<RHITexture2D>    GDefaultMaterialTexture2D;
	CORE_API extern TGlobalRHIResource<RHITexture1D>    GWhiteTexture1D;
	CORE_API extern TGlobalRHIResource<RHITexture1D>    GBlackTexture1D;
	CORE_API extern TGlobalRHIResource<RHITexture2D>    GWhiteTexture2D;
	CORE_API extern TGlobalRHIResource<RHITexture2D>    GBlackTexture2D;
	CORE_API extern TGlobalRHIResource<RHITexture3D>    GWhiteTexture3D;
	CORE_API extern TGlobalRHIResource<RHITexture3D>    GBlackTexture3D;
	CORE_API extern TGlobalRHIResource<RHITextureCube>  GWhiteTextureCube;
	CORE_API extern TGlobalRHIResource<RHITextureCube>  GBlackTextureCube;

	template
	<
		Sampler::Filter Filter = Sampler::ePoint,
		Sampler::AddressMode AddressU = Sampler::eWarp,
		Sampler::AddressMode AddressV = Sampler::eWarp,
		Sampler::AddressMode AddressW = Sampler::eWarp 
	>
	class TStaticSamplerState : public StaticRHIResourceT< 
		TStaticSamplerState< Filter, AddressU, AddressV, AddressW >, 
		RHISamplerState >
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

	template
	<
		ECullMode CullMode = ECullMode::Back,
		EFillMode FillMode = EFillMode::Solid,
		EFrontFace FrontFace = EFrontFace::Default,
		bool bEnableScissor = false 
	>
	class TStaticRasterizerState : public StaticRHIResourceT<
		TStaticRasterizerState< CullMode, FillMode, FrontFace, bEnableScissor >,
		RHIRasterizerState >
	{
	public:
		static RHIRasterizerStateRef CreateRHI()
		{
			RasterizerStateInitializer initializer;
			initializer.fillMode = FillMode;
			initializer.cullMode = CullMode;
			initializer.frontFace = FrontFace;
			initializer.bEnableScissor = bEnableScissor;
			return RHICreateRasterizerState(initializer);
		}
	};


	RHIRasterizerState& GetStaticRasterizerState(ECullMode cullMode, EFillMode fillMode);

	template 
	<
		bool bWriteDepth = true,
		ECompareFunc DepthFun = ECompareFunc::Less,
		bool bEnableStencilTest = false,
		ECompareFunc StencilFun = ECompareFunc::Always,
		Stencil::Operation StencilFailOp = Stencil::eKeep,
		Stencil::Operation ZFailOp = Stencil::eKeep,
		Stencil::Operation ZPassOp = Stencil::eKeep,
		ECompareFunc BackStencilFun = ECompareFunc::Always,
		Stencil::Operation BackStencilFailOp = Stencil::eKeep,
		Stencil::Operation BackZFailOp = Stencil::eKeep,
		Stencil::Operation BackZPassOp = Stencil::eKeep,
		uint32 StencilReadMask = -1,
		uint32 StencilWriteMask = -1 
	>
	class TStaticDepthStencilSeparateState : public StaticRHIResourceT<
		TStaticDepthStencilSeparateState<
			bWriteDepth, DepthFun, bEnableStencilTest, StencilFun, StencilFailOp,
			ZFailOp, ZPassOp, BackStencilFun, BackStencilFailOp, BackZFailOp, BackZPassOp,
			StencilReadMask, StencilWriteMask >,
		RHIDepthStencilState >
	{
	public:
		static RHIDepthStencilStateRef CreateRHI()
		{
			DepthStencilStateInitializer initializer;
			initializer.bWriteDepth = bWriteDepth;
			initializer.depthFunc = DepthFun;
			initializer.bEnableStencilTest = bEnableStencilTest;
			initializer.stencilFunc = StencilFun;
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

	template
	<
		bool bWriteDepth = true,
		ECompareFunc DepthFun = ECompareFunc::Less,
		bool bEnableStencilTest = false,
		ECompareFunc StencilFun = ECompareFunc::Always,
		Stencil::Operation StencilFailOp = Stencil::eKeep,
		Stencil::Operation ZFailOp = Stencil::eKeep,
		Stencil::Operation ZPassOp = Stencil::eKeep,
		uint32 StencilReadMask = -1,
		uint32 StencilWriteMask = -1 
	>
	class TStaticDepthStencilState : public TStaticDepthStencilSeparateState<
		bWriteDepth, DepthFun, bEnableStencilTest,
		StencilFun, StencilFailOp, ZFailOp, ZPassOp,
		StencilFun, StencilFailOp, ZFailOp, ZPassOp,
		StencilReadMask, StencilWriteMask >
	{

	};

	using StaticDepthDisableState = TStaticDepthStencilState<false, ECompareFunc::Always>;

	template
	<
		ColorWriteMask WriteColorMask0 = CWM_RGBA,
		Blend::Factor SrcColorFactor0 = Blend::eOne,
		Blend::Factor DestColorFactor0 = Blend::eZero,
		Blend::Operation ColorOp0 = Blend::eAdd,
		Blend::Factor SrcAlphaFactor0 = Blend::eOne,
		Blend::Factor DestAlphaFactor0 = Blend::eZero,
		Blend::Operation AlphaOp0 = Blend::eAdd,
		bool bEnableAlphaToCoverage = false ,
		bool bEnableIndependent = false ,
		ColorWriteMask WriteColorMask1 = CWM_RGBA,
		Blend::Factor SrcColorFactor1 = Blend::eOne,
		Blend::Factor DestColorFactor1 = Blend::eZero,
		Blend::Operation ColorOp1 = Blend::eAdd,
		Blend::Factor SrcAlphaFactor1 = Blend::eOne,
		Blend::Factor DestAlphaFactor1 = Blend::eZero,
		Blend::Operation AlphaOp1 = Blend::eAdd ,
		ColorWriteMask WriteColorMask2 = CWM_RGBA,
		Blend::Factor SrcColorFactor2 = Blend::eOne,
		Blend::Factor DestColorFactor2 = Blend::eZero,
		Blend::Operation ColorOp2 = Blend::eAdd,
		Blend::Factor SrcAlphaFactor2 = Blend::eOne,
		Blend::Factor DestAlphaFactor2 = Blend::eZero,
		Blend::Operation AlphaOp2 = Blend::eAdd,
		ColorWriteMask WriteColorMask3 = CWM_RGBA,
		Blend::Factor SrcColorFactor3 = Blend::eOne,
		Blend::Factor DestColorFactor3 = Blend::eZero,
		Blend::Operation ColorOp3 = Blend::eAdd,
		Blend::Factor SrcAlphaFactor3 = Blend::eOne,
		Blend::Factor DestAlphaFactor3 = Blend::eZero,
		Blend::Operation AlphaOp3 = Blend::eAdd
	>
	class TStaticBlendSeparateState : public StaticRHIResourceT
		<
			TStaticBlendSeparateState
			< 
				WriteColorMask0, SrcColorFactor0, DestColorFactor0, ColorOp0, SrcAlphaFactor0, DestAlphaFactor0, AlphaOp0,
				bEnableAlphaToCoverage, bEnableIndependent,
				WriteColorMask1, SrcColorFactor1, DestColorFactor1, ColorOp1, SrcAlphaFactor1, DestAlphaFactor1, AlphaOp1, 
				WriteColorMask2, SrcColorFactor2, DestColorFactor2, ColorOp2, SrcAlphaFactor2, DestAlphaFactor2, AlphaOp2, 
				WriteColorMask3, SrcColorFactor3, DestColorFactor3, ColorOp3, SrcAlphaFactor3, DestAlphaFactor3, AlphaOp3 
			>,
			RHIBlendState
		>
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
			initializer.bEnableIndependent = bEnableIndependent;
			if (bEnableIndependent)
			{
				SET_TRAGET_VALUE(1);
				SET_TRAGET_VALUE(2);
				SET_TRAGET_VALUE(3);
			}
#undef SET_TRAGET_VALUE
			return RHICreateBlendState(initializer);
		}
	};


	template
	<
		ColorWriteMask WriteColorMask0 = CWM_RGBA,
		Blend::Factor SrcFactor0 = Blend::eOne,
		Blend::Factor DestFactor0 = Blend::eZero,
		Blend::Operation Op0 = Blend::eAdd,
		bool bEnableAlphaToCoverage = false,
	    bool bEnableIndependent = false,
		ColorWriteMask WriteColorMask1 = CWM_RGBA,
		Blend::Factor SrcFactor1 = Blend::eOne,
		Blend::Factor DestFactor1 = Blend::eZero,
		Blend::Operation Op1 = Blend::eAdd,
		ColorWriteMask WriteColorMask2 = CWM_RGBA,
		Blend::Factor SrcFactor2 = Blend::eOne,
		Blend::Factor DestFactor2 = Blend::eZero,
		Blend::Operation Op2 = Blend::eAdd,
		ColorWriteMask WriteColorMask3 = CWM_RGBA,
		Blend::Factor SrcFactor3 = Blend::eOne,
		Blend::Factor DestFactor3 = Blend::eZero,
		Blend::Operation Op3 = Blend::eAdd
	>
	class TStaticBlendState : public TStaticBlendSeparateState<
			WriteColorMask0, SrcFactor0, DestFactor0, Op0, SrcFactor0, DestFactor0, Op0,
			bEnableAlphaToCoverage ,bEnableIndependent,
			WriteColorMask1, SrcFactor1, DestFactor1, Op1, SrcFactor1, DestFactor1, Op1,
			WriteColorMask2, SrcFactor2, DestFactor2, Op2, SrcFactor2, DestFactor2, Op2,
			WriteColorMask3, SrcFactor3, DestFactor3, Op3, SrcFactor3, DestFactor3, Op3
		>
	{

	};

	template< ColorWriteMask WriteColorMask0 = CWM_RGBA >
	class TStaticAlphaToCoverageBlendState : public TStaticBlendState<
		WriteColorMask0, Blend::eOne, Blend::eZero, Blend::eAdd, true >
	{

	};

}//namespace Render


#endif // RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
