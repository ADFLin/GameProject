#include "GLCommon.h"

namespace RenderGL
{
	RHITexture2D*    RHICreateTexture2D();
	RHITexture2D*    RHICreateTexture2D(Texture::Format format, int w, int h, 
										int numMipLevel = 0 , void* data = nullptr, int dataAlign = 0);
	RHITextureDepth* RHICreateTextureDepth();
	RHITextureCube*  RHICreateTextureCube();


	RHIUniformBuffer* RHICreateUniformBuffer(int size);


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

	RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer);
	void RHISetDepthStencilState( RHIDepthStencilState& depthStencilState , uint32 stencilRef = -1 );
	template <
		bool bWriteDepth = true,
		ECompareFun DepthFun = ECompareFun::Less,
		bool bEnableStencilTest = false ,
		ECompareFun StencilFun = ECompareFun::Always,
		Stencil::Operation StencilFailOp = Stencil::eKeep ,
		Stencil::Operation ZFailOp = Stencil::eKeep ,
		Stencil::Operation ZPassOp = Stencil::eKeep ,
		uint32 StencilReadMask = -1 ,
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

	constexpr int NumBlendTarget = 1;

	struct GLBlendStateValue
	{

		GLTargetBlendValue targetValues[NumBlendTarget];

		GLBlendStateValue(){}
		GLBlendStateValue(EForceInit)
		{
			for( int i = 0 ; i < NumBlendTarget ; ++i )
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

		
		TargetValue    targetValues[NumBlendTarget];
	};

	class RHIBlendState : public RefCountedObjectT< RHIBlendState >
	{
	public:
		RHIBlendState(BlendStateInitializer const& initializer);
		GLBlendStateValue mStateValue;
	};

	typedef TRefCountPtr< RHIBlendState > RHIBlendStateRef;

	RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer);

	void RHISetBlendState(RHIBlendState& blendState);

	template< 
		ColorWriteMask WriteColorMask0 = CWM_RGBA ,
		Blend::Factor SrcFactor0 = Blend::eOne,
		Blend::Factor DestFactor0 = Blend::eZero >
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
		initializer.targetValues[INDEX].srcColor = initializer.targetValues[INDEX].srcAlpha = SrcFactor##INDEX;\
		initializer.targetValues[INDEX].destColor = initializer.targetValues[INDEX].destAlpha = DestFactor##INDEX;

					SET_TRAGET_VALUE(0);

#undef SET_TRAGET_VALUE
					RHI = RHICreateBlendState( initializer );
				}
			};
			static StaticConstructor sConstructor;
			return *sConstructor.RHI;
		}
	};



}//namespace RenderGL
