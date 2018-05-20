#include "RHICommand.h"

#include <cassert>


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
		RHIRasterizerState* rasterizerStateUsage;
		GLRasterizerStateValue rasterizerStateValue;
		RHIDepthStencilState* depthStencilStateUsage;
		GLDepthStencilStateValue depthStencilStateValue;
		RHIBlendState* blendStateUsage;
		GLBlendStateValue blendStateValue;

		bool bScissorRectEnabled;

		void Initialize()
		{
			rasterizerStateUsage = nullptr;
			rasterizerStateValue = GLRasterizerStateValue(ForceInit);
			depthStencilStateUsage = nullptr;
			depthStencilStateValue = GLDepthStencilStateValue(ForceInit);
			blendStateUsage = nullptr;
			blendStateValue = GLBlendStateValue(ForceInit);
			bScissorRectEnabled = false;
		}
	};

	GLDeviceState gDeviceState;

	RHITexture1D* RHICreateTexture1D(Texture::Format format, int length , int numMipLevel /*= 0*/, void* data /*= nullptr*/)
	{
		RHITexture1D* result = new RHITexture1D;
		if( result && !result->create(format, length, numMipLevel, data) )
		{
			delete result;
			return nullptr;
		}
		return result;
	}

	RHITexture2D* RHICreateTexture2D()
	{
		return new RHITexture2D;
	}

	RHITexture2D* RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, void* data , int dataAlign)
	{
		RHITexture2D* result = new RHITexture2D;
		if( result && !result->create(format, w, h , numMipLevel, data , dataAlign) )
		{
			delete result;
			return nullptr;
		}
		return result;
	}


	RHITexture3D* RHICreateTexture3D(Texture::Format format, int sizeX ,int sizeY , int sizeZ )
	{
		RHITexture3D* result = new RHITexture3D;
		if( result && !result->create(format , sizeX , sizeY , sizeZ ) )
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

	RenderGL::RHIVertexBuffer* RHICreateVertexBuffer()
	{
		RHIVertexBuffer* result = new RHIVertexBuffer;
		if( result && !result->create() )
		{
			delete result;
			return nullptr;
		}
		return result;
	}

	RHIUniformBuffer* RHICreateUniformBuffer(int size)
	{
		RHIUniformBuffer* result = new RHIUniformBuffer;
		if( result && !result->create(size) )
		{
			delete result;
			return nullptr;
		}
		return result;
	}


	void RHISetupFixedPipeline(Matrix4 const& matModelView, Matrix4 const& matProj,  int numTexture /*= 0*/, RHITexture2D** texture /*= nullptr*/)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(matProj);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(matModelView);
		if( numTexture )
		{
			glEnable(GL_TEXTURE);
			for( int i = 0; i < numTexture; ++i )
			{
				glEnable(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, texture[i]->mHandle);
			}
		}
	}

	void RHISetViewport(int x, int y, int w, int h)
	{
		glViewport(x, y, w, h);
	}

	void RHISetScissorRect(bool bEnable, int x, int y, int w, int h)
	{
		if( gDeviceState.bScissorRectEnabled != bEnable )
		{
			EnableGLState(GL_SCISSOR_TEST, bEnable);
			gDeviceState.bScissorRectEnabled = bEnable;
			if( bEnable )
			{
				glScissor(x, y, w, h);
			}
		}
	}

	void RHIDrawPrimitive(PrimitiveType type , int start, int nv)
	{
		glDrawArrays(GLConvert::To(type), start, nv);
	}

	void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType , int indexStart, int nIndex)
	{
		assert(indexType == CVT_UInt || indexType == CVT_UShort);
		glDrawElements(GLConvert::To(type), nIndex, GLConvert::To(indexType), (void*)indexStart);
	}

	RHIRasterizerState::RHIRasterizerState(RasterizerStateInitializer const& initializer)
	{
		mStateValue.bEnalbeCull = initializer.cullMode != ECullMode::None;
		mStateValue.cullFace = GLConvert::To(initializer.cullMode);
		mStateValue.fillMode = GLConvert::To(initializer.fillMode);
	}

	RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer)
	{
		RHIRasterizerState* reslut = new RHIRasterizerState(initializer);
		return reslut;
	}

	void RHISetRasterizerState(RHIRasterizerState& rasterizerState)
	{
		if( &rasterizerState == gDeviceState.rasterizerStateUsage )
		{
			return;
		}

		gDeviceState.rasterizerStateUsage = &rasterizerState;
		auto& deviceValue = gDeviceState.rasterizerStateValue;
		auto const& setupValue = rasterizerState.mStateValue;

#define VAR_TEST( NAME )  true ||  (deviceValue.##NAME != setupValue.##NAME)
#define VAR_ASSIGN( NAME ) deviceValue.##NAME = setupValue.##NAME 

		if( VAR_TEST(fillMode) )
		{
			VAR_ASSIGN(fillMode);
			glPolygonMode(GL_FRONT_AND_BACK, setupValue.fillMode);
		}

		if( VAR_TEST(bEnalbeCull) )
		{
			VAR_ASSIGN(bEnalbeCull);
			EnableGLState(GL_CULL_FACE, setupValue.bEnalbeCull);

			if( VAR_TEST(cullFace) )
			{
				VAR_ASSIGN(cullFace);
				glCullFace(setupValue.cullFace);
			}
		}


#undef VAR_TEST
#undef VAR_ASSIGN
	}

	RenderGL::RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		RHISamplerState* result = new RHISamplerState;
		if( result && !result->create(initializer) )
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
		auto& deviceValue = gDeviceState.depthStencilStateValue;
		if( &depthStencilState == gDeviceState.depthStencilStateUsage )
		{
			if( deviceValue.stencilRef != stencilRef )
			{
				deviceValue.stencilRef = stencilRef;
				glStencilFunc(deviceValue.stencilFun, stencilRef, deviceValue.stencilReadMask);
			}
			return;
		}
		gDeviceState.depthStencilStateUsage = &depthStencilState;
	
		auto const& setupValue = depthStencilState.mStateValue;

#define VAR_TEST( NAME )   true || (deviceValue.##NAME != setupValue.##NAME)
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
		
		for( int i = 0 ; i < NumBlendStateTarget ; ++i )
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

		for( int i = 0; i < NumBlendStateTarget; ++i )
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


	RHIRasterizerState& GetStaticRasterizerState(ECullMode cullMode , EFillMode fillMode)
	{
#define SWITCH_CULL_MODE( FILL_MODE )\
		switch( cullMode )\
		{\
		case ECullMode::Front: return TStaticRasterizerState< ECullMode::Front , FILL_MODE >::GetRHI();\
		case ECullMode::Back: return TStaticRasterizerState< ECullMode::Back , FILL_MODE >::GetRHI();\
		case ECullMode::None: return TStaticRasterizerState< ECullMode::None , FILL_MODE >::GetRHI();\
		}

		switch( fillMode )
		{
		case EFillMode::Point:
			SWITCH_CULL_MODE(EFillMode::Point);
			break;
		case EFillMode::Wireframe:
			SWITCH_CULL_MODE(EFillMode::Wireframe);
			break;
		}

		SWITCH_CULL_MODE(EFillMode::Solid);

#undef SWITCH_CULL_MODE
	}

}