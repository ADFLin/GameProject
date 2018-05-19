#pragma once
#ifndef RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
#define RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D

#include "GLCommon.h"

namespace RenderGL
{
	RHITexture1D*    RHICreateTexture1D(Texture::Format format, int length ,
										int numMipLevel = 0, void* data = nullptr);
	RHITexture2D*    RHICreateTexture2D();
	RHITexture2D*    RHICreateTexture2D(Texture::Format format, int w, int h, 
										int numMipLevel = 0 , void* data = nullptr, int dataAlign = 0);
	RHITexture3D*    RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ);

	RHITextureDepth* RHICreateTextureDepth();
	RHITextureCube*  RHICreateTextureCube();

	RHIVertexBuffer* RHICreateVertexBuffer();
	RHIUniformBuffer* RHICreateUniformBuffer(int size);


	void RHISetupFixedPipeline(Matrix4 const& matModelView, Matrix4 const& matProj,  int numTexture = 0, RHITexture2D** texture = nullptr);


	void RHISetViewport(int x, int y, int w , int h );
	void RHISetScissorRect(bool bEnable, int x = 0, int y = 0, int w = 0, int h = 0);
	void RHIDrawPrimitive(PrimitiveType type, int vStart, int nv);
	void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex);

	struct RasterizerStateInitializer
	{
		EFillMode fillMode;
		ECullMode cullMode;
	};

	struct GLRasterizerStateValue
	{
		bool   bEnalbeCull;
		GLenum fillMode;
		GLenum cullFace;

		GLRasterizerStateValue(){}
		GLRasterizerStateValue(EForceInit)
		{
			bEnalbeCull = false;
			fillMode = GL_FILL;
			cullFace = GL_BACK;
		}
	};


	class RHIRasterizerState : public RefCountedObjectT< RHIRasterizerState >
	{
	public:
		RHIRasterizerState(RasterizerStateInitializer const& initializer);
		GLRasterizerStateValue mStateValue;
	};

	typedef TRefCountPtr< RHIRasterizerState > RHIRasterizerStateRef;


	struct DepthStencilStateInitializer
	{
		ECompareFun depthFun;
		bool bEnableStencilTest;
		Stencil::Operation stencilFailOp;
		Stencil::Operation zFailOp;
		Stencil::Operation zPassOp;
		ECompareFun stencilFun;
		uint32 stencilReadMask;
		uint32 stencilWriteMask;
		bool   bWriteDepth;
	};

	struct GLDepthStencilStateValue
	{
		bool   bEnableDepthTest;
		bool   bEnableStencilTest;
		bool   bWriteDepth;

		GLenum depthFun;
		GLenum stencilFailOp;
		GLenum stencilZFailOp;
		GLenum stencilZPassOp;
		GLenum stencilFun;
		uint32 stencilReadMask;
		uint32 stencilWriteMask;
		uint32 stencilRef;

		GLDepthStencilStateValue(){}

		GLDepthStencilStateValue(EForceInit)
		{
			bEnableDepthTest = false;

			bEnableStencilTest = false;
			depthFun = GL_LESS;
			stencilFailOp = GL_KEEP;
			stencilZFailOp = GL_KEEP;
			stencilZPassOp = GL_KEEP;

			stencilReadMask = -1;
			stencilWriteMask = -1;
			stencilRef = -1;
			stencilFun = GL_ALWAYS;
		}
	};


	class RHIDepthStencilState : public RefCountedObjectT< RHIDepthStencilState >
	{
	public:
		RHIDepthStencilState(DepthStencilStateInitializer const& initializer);
		GLDepthStencilStateValue mStateValue;
	};

	typedef TRefCountPtr<RHIDepthStencilState> RHIDepthStencilStateRef;


	struct GLTargetBlendValue
	{
		ColorWriteMask writeMask;

		GLenum srcColor;
		GLenum srcAlpha;
		GLenum destColor;
		GLenum destAlpha;

		bool bEnable;
		bool bSeparateBlend;

		GLTargetBlendValue(){}
		GLTargetBlendValue(EForceInit)
		{
			bEnable = false;
			bSeparateBlend = false;
			srcColor = GL_ONE;
			srcAlpha = GL_ONE;
			destColor = GL_ZERO;
			destAlpha = GL_ZERO;
			writeMask = CWM_RGBA;
		}
	};

	constexpr int NumBlendStateTarget = 1;

	struct GLBlendStateValue
	{
		GLTargetBlendValue targetValues[NumBlendStateTarget];
		GLBlendStateValue(){}
		GLBlendStateValue(EForceInit)
		{
			for( int i = 0 ; i < NumBlendStateTarget ; ++i )
				targetValues[0] = GLTargetBlendValue(ForceInit);
		}
	};
	

	struct BlendStateInitializer
	{
		struct TargetValue
		{
			ColorWriteMask   writeMask;
			Blend::Operation op;
			Blend::Factor    srcColor;
			Blend::Factor    srcAlpha;
			Blend::Factor    destColor;
			Blend::Factor    destAlpha;
		};
		TargetValue    targetValues[NumBlendStateTarget];
	};

	class RHIBlendState : public RefCountedObjectT< RHIBlendState >
	{
	public:
		RHIBlendState(BlendStateInitializer const& initializer);
		GLBlendStateValue mStateValue;
	};

	typedef TRefCountPtr< RHIBlendState > RHIBlendStateRef;


	RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer);


	RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);
	void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef = -1);

	RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);
	void           RHISetBlendState(RHIBlendState& blendState);

	RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
	void RHISetRasterizerState(RHIRasterizerState& rasterizerState);

	RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer);
	void RHISetRasterizerState(RHIRasterizerState& rasterizerState);

	template<
		Sampler::Filter Filter = Sampler::ePoint,
		Sampler::AddressMode AddressU = Sampler::eWarp ,
		Sampler::AddressMode AddressV = Sampler::eWarp ,
		Sampler::AddressMode AddressW = Sampler::eWarp >
	class TStaticSamplerState
	{
	public:
		static RHISamplerState& GetRHI()
		{
			struct StaticConstructor
			{
				RHISamplerStateRef RHI;
				StaticConstructor()
				{
					SamplerStateInitializer initializer;
					initializer.filter = Filter;
					initializer.addressU = AddressU;
					initializer.addressV = AddressV;
					initializer.addressW = AddressW;
					RHI = RHICreateSamplerState(initializer);
				}
			};
			static StaticConstructor sConstructor;
			return *sConstructor.RHI;
		}
	};

	template<
		ECullMode CullMode = ECullMode::Back,
		EFillMode FillMode = EFillMode::Solid >
	class TStaticRasterizerState
	{
	public:
		static RHIRasterizerState& GetRHI()
		{
			struct StaticConstructor
			{
				RHIRasterizerStateRef RHI;
				StaticConstructor()
				{
					RasterizerStateInitializer initializer;
					initializer.fillMode = FillMode;
					initializer.cullMode = CullMode;
					RHI = RHICreateRasterizerState(initializer);
				}
			};
			static StaticConstructor sConstructor;
			return *sConstructor.RHI;
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
		uint32 StencilReadMask = -1,
		uint32 StencilWriteMask = -1 >
	class TStaticDepthStencilState
	{
	public:
		static RHIDepthStencilState& GetRHI()
		{
			struct StaticConstructor
			{
				RHIDepthStencilStateRef RHI;

				StaticConstructor()
				{
					DepthStencilStateInitializer initializer;
					initializer.bWriteDepth = bWriteDepth;
					initializer.depthFun = DepthFun;
					initializer.bEnableStencilTest = bEnableStencilTest;
					initializer.stencilFun = StencilFun;
					initializer.stencilFailOp = StencilFailOp;
					initializer.zFailOp = ZFailOp;
					initializer.zPassOp = ZPassOp;
					initializer.stencilReadMask = StencilReadMask;
					initializer.stencilWriteMask = StencilWriteMask;
					RHI = RHICreateDepthStencilState(initializer);
				}
			};
			static StaticConstructor sConstructor;
			return *sConstructor.RHI;
		}
	};

	typedef TStaticDepthStencilState<false, ECompareFun::Always> StaticDepthDisableState;

	template<
		ColorWriteMask WriteColorMask0 = CWM_RGBA,
		Blend::Factor SrcColorFactor0 = Blend::eOne,
		Blend::Factor DestColorFactor0 = Blend::eZero ,
		Blend::Factor SrcAlphaFactor0 = Blend::eOne,
		Blend::Factor DestAlphaFactor0 = Blend::eZero >
	class TStaticBlendState
	{
	public:
		static RHIBlendState& GetRHI()
		{
			struct StaticConstructor
			{
				RHIBlendStateRef RHI;

				StaticConstructor()
				{
					BlendStateInitializer initializer;
#define SET_TRAGET_VALUE( INDEX )\
		initializer.targetValues[INDEX].writeMask = WriteColorMask##INDEX;\
		initializer.targetValues[INDEX].srcColor = SrcColorFactor##INDEX;\
		initializer.targetValues[INDEX].srcAlpha = SrcAlphaFactor##INDEX;\
		initializer.targetValues[INDEX].destColor = DestColorFactor##INDEX;\
		initializer.targetValues[INDEX].destAlpha = DestAlphaFactor##INDEX;

					SET_TRAGET_VALUE(0);

#undef SET_TRAGET_VALUE
					RHI = RHICreateBlendState(initializer);
				}
			};
			static StaticConstructor sConstructor;
			return *sConstructor.RHI;
		}
	};
}//namespace RenderGL

#endif // RHICommand_H_C0CC3E6C_43AE_4582_8203_41997F0A4C7D
