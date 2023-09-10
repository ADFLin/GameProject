#pragma once
#ifndef RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
#define RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29

#include "RHICommon.h"
#include "RHICommand.h"
#include "CoreShare.h"
#include "GlobalShader.h"

#include "LogSystem.h"

namespace Render
{

	struct SimplePipelineParamValues
	{
		Matrix4 transform;
		LinearColor color;
		RHITexture2D* texture;
		RHISamplerState* sampler;

		SimplePipelineParamValues()
		{
			texture = nullptr;
			sampler = nullptr;
			transform == Matrix4::Identity();
			color = LinearColor(1, 1, 1, 1);
		}
	};

	class SimplePipelineProgram : public GlobalShaderProgram
	{
	public:

		DECLARE_EXPORTED_SHADER_PROGRAM(SimplePipelineProgram, Global, CORE_API);
		

		SHADER_PERMUTATION_TYPE_BOOL(HaveVertexColor, SHADER_PARAM(HAVE_VERTEX_COLOR));
		SHADER_PERMUTATION_TYPE_BOOL(HaveTexcoord, SHADER_PARAM(HAVE_TEXTCOORD));
		using PermutationDomain = TShaderPermutationDomain<HaveVertexColor, HaveTexcoord>;


		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SimplePipelineShader";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}


		CORE_API static SimplePipelineProgram* Get(uint32 attributeMask, bool bUseTexture);
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, Texture);
			mParamColor.bind(parameterMap, SHADER_PARAM(Color));
			mParamXForm.bind(parameterMap, SHADER_PARAM(XForm));
		}
		void setTextureParam(RHICommandList& commandList, RHITexture2D* texture, RHISamplerState* sampler);
		void setParameters(RHICommandList& commandList, Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler);
		void setParameters(RHICommandList& commandList, SimplePipelineParamValues const& inValues)
		{
			setParameters(commandList, inValues.transform, inValues.color, inValues.texture, inValues.sampler);
		}

		DEFINE_TEXTURE_PARAM(Texture);
		ShaderParameter mParamColor;
		ShaderParameter mParamXForm;
	};

	class ShaderCompileOption;
	class MaterialShaderProgram;

	CORE_API extern class MaterialMaster* GDefalutMaterial;
	CORE_API extern class MaterialShaderProgram GSimpleBasePass;
	

	CORE_API bool InitGlobalRenderResource();
	CORE_API void ReleaseGlobalRenderResource();

	class IRenderResource
	{
	public:
		virtual void restoreRHI() = 0;
		virtual void releaseRHI() = 0;
	};
	
	class IGlobalRenderResource : public IRenderResource
	{
	public:
		CORE_API IGlobalRenderResource();
		virtual ~IGlobalRenderResource() = default;
		virtual void restoreRHI() = 0;
		virtual void releaseRHI() = 0;

		IGlobalRenderResource* mNext;

		CORE_API static void ReleaseAllResources();
		CORE_API static void RestoreAllResources();
	};
	
	template< class RHIResourceType >
	class TGlobalRenderResource : public IGlobalRenderResource
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

	template< class ThisClass , class RHIResourceType >
	class StaticRHIResourceT
	{
	public:
		static RHIResourceType& GetRHI()
		{
			static StaticRenderResource* sObject = nullptr;
			if( sObject == nullptr )
			{
				sObject = new StaticRenderResource();

				uint32 key = ThisClass::GetHashKey();
				RHIResourceType* resource = RHIResource::QueryResource<RHIResourceType>(key);

				if (resource)
				{
#if RHI_CHECK_RESOURCE_HASH
					if (!(resource->mSetupValue == ThisClass::GetSetupValue()))
					{
						LogError("QueryResource fail");
					}
					else
#endif
					{
						sObject->mResource = resource;
						return *resource;
					}
				}

				{
					TRACE_RESOURCE_TAG_SCOPE("StaticRHI");
					sObject->mKey = key;

					sObject->restoreRHI();
#if RHI_CHECK_RESOURCE_HASH
					sObject->mResource->mSetupValue = ThisClass::GetSetupValue();
#endif
				}
			}
			return sObject->getRHI();
		}
	
	private:
		class StaticRenderResource : public IGlobalRenderResource
		{
		public:
			RHIResourceType& getRHI() { return *mResource; }
			void restoreRHI() final 
			{  
				mResource = ThisClass::CreateRHI();
				if (mKey)
				{
					FRHIResourceTable::Register(*mResource, mKey);
				}
			}
			void releaseRHI() final {  mResource.release();  }
	
			TRefCountPtr< RHIResourceType > mResource;
			uint32 mKey = 0;
		};
	};

	CORE_API extern TGlobalRenderResource<RHITexture2D>    GDefaultMaterialTexture2D;
	CORE_API extern TGlobalRenderResource<RHITexture1D>    GWhiteTexture1D;
	CORE_API extern TGlobalRenderResource<RHITexture1D>    GBlackTexture1D;
	CORE_API extern TGlobalRenderResource<RHITexture2D>    GWhiteTexture2D;
	CORE_API extern TGlobalRenderResource<RHITexture2D>    GBlackTexture2D;
	CORE_API extern TGlobalRenderResource<RHITexture3D>    GWhiteTexture3D;
	CORE_API extern TGlobalRenderResource<RHITexture3D>    GBlackTexture3D;
	CORE_API extern TGlobalRenderResource<RHITextureCube>  GWhiteTextureCube;
	CORE_API extern TGlobalRenderResource<RHITextureCube>  GBlackTextureCube;

	template
	<
		ESampler::Filter Filter = ESampler::Point,
		ESampler::AddressMode AddressU = ESampler::Warp,
		ESampler::AddressMode AddressV = ESampler::Warp,
		ESampler::AddressMode AddressW = ESampler::Warp 
	>
	class TStaticSamplerState : public StaticRHIResourceT< 
		TStaticSamplerState< Filter, AddressU, AddressV, AddressW >, 
		RHISamplerState >
	{
	public:
		static RHISamplerState* CreateRHI()
		{
			SamplerStateInitializer initializer = GetSetupValue();
			return RHICreateSamplerState(initializer);
		}
		static SamplerStateInitializer GetSetupValue()
		{
			SamplerStateInitializer initializer;
			initializer.filter = Filter;
			initializer.addressU = AddressU;
			initializer.addressV = AddressV;
			initializer.addressW = AddressW;
			return initializer;
		}

		static uint32 GetHashKey()
		{
			return GetSetupValue().getTypeHash();
		}

	};

	template
	<
		ECullMode CullMode = ECullMode::Back,
		EFillMode FillMode = EFillMode::Solid,
		EFrontFace FrontFace = EFrontFace::Default,
		bool bEnableScissor = false ,
		bool bEnableMultisample = false
	>
	class TStaticRasterizerState : public StaticRHIResourceT<
		TStaticRasterizerState< CullMode, FillMode, FrontFace, bEnableScissor , bEnableMultisample >,
		RHIRasterizerState >
	{
	public:
		static RHIRasterizerStateRef CreateRHI()
		{
			RasterizerStateInitializer initializer = GetSetupValue();
			return RHICreateRasterizerState(initializer);
		}
		static RasterizerStateInitializer GetSetupValue()
		{
			RasterizerStateInitializer initializer;
			initializer.fillMode = FillMode;
			initializer.cullMode = CullMode;
			initializer.frontFace = FrontFace;
			initializer.bEnableScissor = bEnableScissor;
			initializer.bEnableMultisample = bEnableMultisample;
			return initializer;
		}
		static uint32 GetHashKey()
		{
			return 0;
		}
	};


	RHIRasterizerState& GetStaticRasterizerState(ECullMode cullMode, EFillMode fillMode);

	template 
	<
		bool bWriteDepth = true,
		ECompareFunc DepthFun = ECompareFunc::DepthNear,
		bool bEnableStencilTest = false,
		ECompareFunc StencilFun = ECompareFunc::Always,
		EStencil StencilFailOp = EStencil::Keep,
		EStencil ZFailOp = EStencil::Keep,
		EStencil ZPassOp = EStencil::Keep,
		ECompareFunc BackStencilFun = ECompareFunc::Always,
		EStencil BackStencilFailOp = EStencil::Keep,
		EStencil BackZFailOp = EStencil::Keep,
		EStencil BackZPassOp = EStencil::Keep,
		uint32 StencilReadMask = -1,
		uint32 StencilWriteMask = -1 
	>
	class TStaticDepthStencilSeparateState : public StaticRHIResourceT<
		TStaticDepthStencilSeparateState
		<
			bWriteDepth, DepthFun, bEnableStencilTest, StencilFun, StencilFailOp,
			ZFailOp, ZPassOp, BackStencilFun, BackStencilFailOp, BackZFailOp, BackZPassOp,
			StencilReadMask, StencilWriteMask 
		>,
		RHIDepthStencilState >
	{
	public:
		static RHIDepthStencilStateRef CreateRHI()
		{
			DepthStencilStateInitializer initializer = GetSetupValue();
			return RHICreateDepthStencilState(initializer);
		}
		static DepthStencilStateInitializer GetSetupValue()
		{
			DepthStencilStateInitializer initializer;
			initializer.bWriteDepth = bWriteDepth;
			initializer.depthFunc = DepthFun;
			initializer.bEnableStencilTest = bEnableStencilTest;
			initializer.stencilFunc = StencilFun;
			initializer.stencilFailOp = StencilFailOp;
			initializer.zFailOp = ZFailOp;
			initializer.zPassOp = ZPassOp;
			initializer.stencilFuncBack = BackStencilFun;
			initializer.stencilFailOpBack = BackStencilFailOp;
			initializer.zFailOpBack = BackZFailOp;
			initializer.zPassOpBack = BackZPassOp;
			initializer.stencilReadMask = StencilReadMask;
			initializer.stencilWriteMask = StencilWriteMask;
			return initializer;
		}
		static uint32 GetHashKey()
		{
			return 0;
		}
	};

	template
	<
		bool bWriteDepth = true,
		ECompareFunc DepthFun = ECompareFunc::DepthNearEqual,
		bool bEnableStencilTest = false,
		ECompareFunc StencilFun = ECompareFunc::Always,
		EStencil StencilFailOp = EStencil::Keep,
		EStencil ZFailOp = EStencil::Keep,
		EStencil ZPassOp = EStencil::Keep,
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
		EBlend::Factor SrcColorFactor0 = EBlend::One,
		EBlend::Factor DestColorFactor0 = EBlend::Zero,
		EBlend::Operation ColorOp0 = EBlend::Add,
		EBlend::Factor SrcAlphaFactor0 = EBlend::One,
		EBlend::Factor DestAlphaFactor0 = EBlend::Zero,
		EBlend::Operation AlphaOp0 = EBlend::Add,
		bool bEnableAlphaToCoverage = false ,
		bool bEnableIndependent = false ,
		ColorWriteMask WriteColorMask1 = CWM_RGBA,
		EBlend::Factor SrcColorFactor1 = EBlend::One,
		EBlend::Factor DestColorFactor1 = EBlend::Zero,
		EBlend::Operation ColorOp1 = EBlend::Add,
		EBlend::Factor SrcAlphaFactor1 = EBlend::One,
		EBlend::Factor DestAlphaFactor1 = EBlend::Zero,
		EBlend::Operation AlphaOp1 = EBlend::Add ,
		ColorWriteMask WriteColorMask2 = CWM_RGBA,
		EBlend::Factor SrcColorFactor2 = EBlend::One,
		EBlend::Factor DestColorFactor2 = EBlend::Zero,
		EBlend::Operation ColorOp2 = EBlend::Add,
		EBlend::Factor SrcAlphaFactor2 = EBlend::One,
		EBlend::Factor DestAlphaFactor2 = EBlend::Zero,
		EBlend::Operation AlphaOp2 = EBlend::Add,
		ColorWriteMask WriteColorMask3 = CWM_RGBA,
		EBlend::Factor SrcColorFactor3 = EBlend::One,
		EBlend::Factor DestColorFactor3 = EBlend::Zero,
		EBlend::Operation ColorOp3 = EBlend::Add,
		EBlend::Factor SrcAlphaFactor3 = EBlend::One,
		EBlend::Factor DestAlphaFactor3 = EBlend::Zero,
		EBlend::Operation AlphaOp3 = EBlend::Add
	>
	class TStaticBlendState : public StaticRHIResourceT
		<
			TStaticBlendState
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
		static RHIBlendState* CreateRHI()
		{
			BlendStateInitializer initializer = GetSetupValue();
			return RHICreateBlendState(initializer);
		}
		static BlendStateInitializer GetSetupValue()
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
			return initializer;
		}

		static uint32 GetHashKey()
		{
			return 0;
		}
	};


	template< ColorWriteMask WriteColorMask0 = CWM_RGBA >
	class TStaticAlphaToCoverageBlendState : public TStaticBlendState<
		WriteColorMask0, EBlend::One, EBlend::Zero, EBlend::Add, EBlend::One, EBlend::Zero, EBlend::Add, true >
	{

	};

}//namespace Render


#endif // RHIStaticStateObject_H_3909FF48_83F9_4C22_AEBB_258FB6C78C29
