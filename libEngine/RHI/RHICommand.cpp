#include "RHICommand.h"

#include "GLCommon.h"
#include <cassert>

#include "stb/stb_image.h"

#include "OpenGLCommand.h"
#include "D3D11Command.h"

namespace RenderGL
{
#if CORE_SHARE_CODE
	RHISystem* gRHISystem = nullptr;
#endif

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

	bool RHISystemInitialize(RHISytemName name)
	{
		if( gRHISystem == nullptr )
		{
			switch( name )
			{
			case RHISytemName::D3D11: gRHISystem = new D3D11System; break;
			case RHISytemName::Opengl:gRHISystem = new OpenGLSystem; break;
			}

			if( gRHISystem && !gRHISystem->initialize() )
			{
				delete gRHISystem;
				gRHISystem = nullptr;
			}
		}
		
		return gRHISystem != nullptr;
	}


	void RHISystemShutdown()
	{

	}

	RHITexture1D* RHICreateTexture1D(Texture::Format format, int length, int numMipLevel /*= 0*/, void* data /*= nullptr*/)
	{
		return gRHISystem->RHICreateTexture1D(format, length, numMipLevel, data);
	}

	RHITexture2D* RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, void* data , int dataAlign)
	{
		return gRHISystem->RHICreateTexture2D(format, w, h, numMipLevel, data, dataAlign);
	}

	RHITexture3D* RHICreateTexture3D(Texture::Format format, int sizeX ,int sizeY , int sizeZ )
	{
		return gRHISystem->RHICreateTexture3D(format, sizeX, sizeY, sizeZ);
	}

	RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h)
	{
		return gRHISystem->RHICreateTextureDepth(format, w, h);
	}

	RHITextureCube* RHICreateTextureCube()
	{
		return gRHISystem->RHICreateTextureCube();
	}

	RHIVertexBuffer* RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		return gRHISystem->RHICreateVertexBuffer(vertexSize, numVertices, creationFlags, data);
	}

	RHIIndexBuffer* RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		return gRHISystem->RHICreateIndexBuffer(nIndices, bIntIndex, creationFlags, data);
	}

	RHIUniformBuffer* RHICreateUniformBuffer(uint32 size, uint32 creationFlags, void* data)
	{
		return gRHISystem->RHICreateUniformBuffer(size, creationFlags, data);
	}

	RHIStorageBuffer* RHICreateStorageBuffer(uint32 size, uint32 creationFlags, void* data)
	{
		return gRHISystem->RHICreateStorageBuffer(size, creationFlags, data);
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
				glBindTexture(GL_TEXTURE_2D, texture[i] ? OpenGLCast::GetHandle( *texture[i] ) : 0 );
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

	RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		OpenGLSamplerState* result = new OpenGLSamplerState;
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

	RHITexture2D* RHIUtility::LoadTexture2DFromFile(char const* path)
	{
		int w;
		int h;
		int comp;
		unsigned char* image = stbi_load(path, &w, &h, &comp, STBI_default);

		if( !image )
			return false;


		RHITexture2D* result = nullptr;
		//#TODO
		switch( comp )
		{
		case 3:
			result = RHICreateTexture2D(Texture::eRGB8, w, h, 0, image, 1);
			break;
		case 4:
			result = RHICreateTexture2D(Texture::eRGBA8, w, h, 0, image);
			break;
		}

		return result;
	}


}
