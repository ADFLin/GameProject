#include "RHICommand.h"


namespace RenderGL
{
	static void EnableGLState(GLenum param, bool bEnable)
	{
		if( bEnable )
		{
			glEnable(param);
		}
		else
		{
			glDisable(param);
		}
	}
	static void EnableGLStateIndex(GLenum param, uint32 index, bool bEnable)
	{
		if( bEnable )
		{
			glEnablei(param, index);
		}
		else
		{
			glDisablei(param, index);
		}
	}

	struct GLDeviceState
	{
		GLDeviceState()
		{
			Initialize();
		}

		RHIDepthStencilState* depthStencilStateUsage;
		GLDepthStencilStateValue depthStencilStateValue;
		RHIBlendState* blendStateUsage;
		GLBlendStateValue blendStateValue;

		void Initialize()
		{
			depthStencilStateUsage = nullptr;
			depthStencilStateValue = GLDepthStencilStateValue(ForceInit);
			blendStateUsage = nullptr;
			blendStateValue = GLBlendStateValue(ForceInit);
		}
	};

	GLDeviceState gDeviceState;

	RHITexture2D* RHICreateTexture2D()
	{
		return new RHITexture2D;
	}

	RHITexture2D* RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, void* data , int dataAlign)
	{
		RHITexture2D* result = new RHITexture2D;
		if( !result->create(format, w, h , numMipLevel, data , dataAlign) )
		{
			delete result;
			return nullptr;
		}
		return result;
	}

	RHITextureDepth* RHICreateTextureDepth()
	{
		return new RHITextureDepth;
	}

	RHITextureCube* RHICreateTextureCube()
	{
		return new RHITextureCube;
	}

	RHIUniformBuffer* RHICreateUniformBuffer(int size)
	{
		RHIUniformBuffer* result = new RHIUniformBuffer;
		if( !result->create(size) )
		{
			delete result;
			return nullptr;
		}
		return result;
	}

	RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		RHIDepthStencilState* reslut = new RHIDepthStencilState(initializer);
		return reslut;
	}

	RHIDepthStencilState::RHIDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		//When depth testing is disabled, writes to the depth buffer are also disabled
		mStateValue.bEnableDepthTest = ( initializer.depthFun != ECompareFun::Always ) || (initializer.bWriteDepth);
		mStateValue.depthFun = GLConvert::To( initializer.depthFun);
		mStateValue.bWriteDepth = initializer.bWriteDepth;
		
		mStateValue.bEnableStencilTest = initializer.bEnableStencilTest;
		mStateValue.stencilFailOp = GLConvert::To(initializer.stencilFailOp);
		mStateValue.stencilZFailOp = GLConvert::To(initializer.zFailOp);
		mStateValue.stencilZPassOp = GLConvert::To(initializer.zPassOp);
		mStateValue.stencilFun = GLConvert::To(initializer.stencilFun);
		mStateValue.stencilReadMask = initializer.stencilReadMask;
		mStateValue.stencilWriteMask = initializer.stencilWriteMask;
	}

	void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState , uint32 stencilRef)
	{
		if( &depthStencilState == gDeviceState.depthStencilStateUsage )
			return;

		gDeviceState.depthStencilStateUsage = &depthStencilState;

		auto& deviceValue = gDeviceState.depthStencilStateValue;
		auto const& setupValue = depthStencilState.mStateValue;

#define VAR_TEST( NAME )  true || (deviceValue.##NAME != setupValue.##NAME)
#define VAR_ASSIGN( NAME ) deviceValue.##NAME = setupValue.##NAME 

		if( VAR_TEST( bEnableDepthTest ) )
		{
			VAR_ASSIGN(bEnableDepthTest);
			EnableGLState(GL_DEPTH_TEST , setupValue.bEnableDepthTest);
		}

		if( setupValue.bEnableDepthTest )
		{
			if( VAR_TEST(bWriteDepth) )
			{
				VAR_ASSIGN(bWriteDepth);
				glDepthMask(setupValue.bWriteDepth);
			}
			if( VAR_TEST(depthFun) )
			{
				VAR_ASSIGN(depthFun);
				glDepthFunc(setupValue.depthFun);
			}
		}
		else
		{
			//#TODO : Check State Value
		}

		if( VAR_TEST(bEnableStencilTest) )
		{
			VAR_ASSIGN(bEnableStencilTest);
			EnableGLState(GL_STENCIL_TEST, setupValue.bEnableStencilTest);
		}

		if( VAR_TEST(stencilWriteMask) )
		{
			VAR_ASSIGN(stencilWriteMask);
			glStencilMask(setupValue.stencilWriteMask);
		}

		if( setupValue.bEnableStencilTest )
		{
			if( VAR_TEST(stencilFailOp) || VAR_TEST(stencilZFailOp) || VAR_TEST(stencilZPassOp) )
			{
				VAR_ASSIGN(stencilFailOp);
				VAR_ASSIGN(stencilZFailOp);
				VAR_ASSIGN(stencilZPassOp);
				glStencilOp(setupValue.stencilFailOp, setupValue.stencilZFailOp, setupValue.stencilZPassOp);
			}
			if( VAR_TEST(stencilFun) || deviceValue.stencilRef != stencilRef || VAR_TEST(stencilReadMask) )
			{
				VAR_ASSIGN(stencilFun);
				VAR_ASSIGN(stencilReadMask);
				deviceValue.stencilRef = stencilRef;
				glStencilFunc(setupValue.stencilFun, stencilRef, setupValue.stencilReadMask);
			}
		}
		else
		{


		}


#undef VAR_TEST
#undef VAR_ASSIGN
	}

	RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer)
	{
		RHIBlendState* result = new RHIBlendState( initializer );
		return result;
	}



	RHIBlendState::RHIBlendState(BlendStateInitializer const& initializer)
	{
		
		for( int i = 0 ; i < NumBlendTarget ; ++i )
		{
			auto const& targetValue = initializer.targetValues[i];
			auto& targetValueGL = mStateValue.targetValues[i];
			targetValueGL.writeMask = targetValue.writeMask;
			targetValueGL.bEnable = (targetValue.srcColor != Blend::eOne) || (targetValue.srcAlpha != Blend::eOne) ||
				(targetValue.destColor != Blend::eZero) || (targetValue.destAlpha != Blend::eZero);

			targetValueGL.bSeparateBlend = (targetValue.srcColor != targetValue.srcAlpha) || (targetValue.destColor != targetValue.destAlpha);

			targetValueGL.srcColor = GLConvert::To(targetValue.srcColor);
			targetValueGL.srcAlpha = GLConvert::To(targetValue.srcAlpha);
			targetValueGL.destColor = GLConvert::To(targetValue.destColor);
			targetValueGL.destAlpha = GLConvert::To(targetValue.destAlpha);
		}
	}


	void RHISetBlendState(RHIBlendState& blendState)
	{
		if( &blendState == gDeviceState.blendStateUsage )
			return;

		gDeviceState.blendStateUsage = &blendState;

		for( int i = 0; i < NumBlendTarget; ++i )
		{
			auto& deviceValue = gDeviceState.blendStateValue.targetValues[i];
			auto const& setupValue = blendState.mStateValue.targetValues[i];

#define VAR_TEST( NAME )  true || ( deviceValue.##NAME != setupValue.##NAME )
#define VAR_ASSIGN( NAME ) deviceValue.##NAME = setupValue.##NAME 

			bool bFroceReset = false;

			if(  VAR_TEST(writeMask) )
			{
				bFroceReset = true;
				VAR_ASSIGN(writeMask);
				glColorMaski(i, setupValue.writeMask & CWM_R,
							 setupValue.writeMask & CWM_G,
							 setupValue.writeMask & CWM_B,
							 setupValue.writeMask & CWM_A);
			}

			if( setupValue.writeMask )
			{
				
				if(  VAR_TEST(bEnable) )
				{
					VAR_ASSIGN(bEnable);
					EnableGLStateIndex(GL_BLEND, i, setupValue.bEnable);
				}

				if( setupValue.bEnable )
				{
					if( bFroceReset || VAR_TEST(bSeparateBlend) || VAR_TEST(srcColor) || VAR_TEST(destColor) || VAR_TEST(srcAlpha) || VAR_TEST(destAlpha) )
					{
						VAR_ASSIGN(bSeparateBlend);
						VAR_ASSIGN(srcColor);
						VAR_ASSIGN(destColor);
						VAR_ASSIGN(srcAlpha);
						VAR_ASSIGN(destAlpha);

						if( setupValue.bSeparateBlend )
						{

							glBlendFuncSeparatei(i, setupValue.srcColor, setupValue.destColor,
												 setupValue.srcAlpha, setupValue.destAlpha);
						}
						else
						{
							glBlendFunci(i,setupValue.srcColor, setupValue.destColor);
						}
					}
				}
			}

#undef VAR_TEST
#undef VAR_ASSIGN
		}
	}

}
