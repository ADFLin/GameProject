#include "OpenGLCommand.h"

#include "GLCommon.h"

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

	void WINAPI GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
	{
		if( type == GL_DEBUG_TYPE_OTHER )
			return;

		LogMsg(message);
	}


	template< class T, class ...Args >
	T* CreateOpenGLObjectT(Args ...args)
	{
		T* result = new T;
		if( result && !result->create(std::forward< Args >(args)...) )
		{
			delete result;
			return nullptr;
		}
		return result;
	}

	bool OpenGLSystem::initialize(RHISystemInitParam const& initParam)
	{
		WGLPixelFormat format;
		format.numSamples = initParam.numSamples;
		if( !mGLContext.init(initParam.hDC, format) )
			return false;

		if( glewInit() != GLEW_OK )
			return false;

		if( 1 )
		{
			//glDebugMessageCallback( NULL ?,  NULL );
			glDebugMessageCallback(GLDebugCallback, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}
		return true;
	}

	void OpenGLSystem::shutdown()
	{
		mGLContext.cleanup();
	}

	bool OpenGLSystem::RHIBeginRender()
	{
		if( !mGLContext.makeCurrent() )
			return false;

		return true;
	}

	void OpenGLSystem::RHIEndRender(bool bPresent)
	{
		::glFlush();

		if( bPresent )
			mGLContext.swapBuffer();
	}

	RHIRenderWindow* OpenGLSystem::RHICreateRenderWindow(PlatformWindowInfo const& info)
	{
		return nullptr;
	}

	RHITexture1D* OpenGLSystem::RHICreateTexture1D(Texture::Format format, int length, int numMipLevel, uint32 createFlags, void* data)
	{
		return CreateOpenGLObjectT< OpenGLTexture1D >(format, length, numMipLevel, data);
	}

	RHITexture2D* OpenGLSystem::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, uint32 createFlags, void* data, int dataAlign)
	{
		return CreateOpenGLObjectT< OpenGLTexture2D >(format, w, h, numMipLevel, data, dataAlign);
	}

	RHITexture3D* OpenGLSystem::RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ, uint32 createFlags )
	{
		return CreateOpenGLObjectT< OpenGLTexture3D >(format, sizeX, sizeY, sizeZ);
	}

	RHITextureDepth* OpenGLSystem::RHICreateTextureDepth(Texture::DepthFormat format, int w, int h)
	{
		return CreateOpenGLObjectT< OpenGLTextureDepth >(format, w , h );
	}

	RHITextureCube* OpenGLSystem::RHICreateTextureCube()
	{
		return new OpenGLTextureCube;
	}

	RHIVertexBuffer* OpenGLSystem::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		return CreateOpenGLObjectT< OpenGLVertexBuffer >(vertexSize, numVertices, creationFlags, data);
	}

	RHIIndexBuffer* OpenGLSystem::RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		return CreateOpenGLObjectT< OpenGLIndexBuffer >(nIndices, bIntIndex, creationFlags, data);
	}

	RHIUniformBuffer* OpenGLSystem::RHICreateUniformBuffer(uint32 elementSize , uint32 numElement , uint32 creationFlags, void* data)
	{
		return CreateOpenGLObjectT< OpenGLUniformBuffer >(elementSize , numElement, creationFlags, data);
	}

	RHIStorageBuffer* OpenGLSystem::RHICreateStorageBuffer(uint32 elementSize, uint32 numElement, uint32 creationFlags, void* data)
	{
		return CreateOpenGLObjectT< OpenGLStorageBuffer >(elementSize , numElement, creationFlags, data);
	}


	RHIFrameBuffer* OpenGLSystem::RHICreateFrameBuffer()
	{
		return CreateOpenGLObjectT< OpenGLFrameBuffer >();
	}

	RHISamplerState* OpenGLSystem::RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		return CreateOpenGLObjectT< OpenGLSamplerState >(initializer);
	}

	RHIRasterizerState* OpenGLSystem::RHICreateRasterizerState(RasterizerStateInitializer const& initializer)
	{
		return new OpenGLRasterizerState(initializer);
	}

	RHIBlendState* OpenGLSystem::RHICreateBlendState(BlendStateInitializer const& initializer)
	{
		return new OpenGLBlendState(initializer);
	}

	RHIDepthStencilState* OpenGLSystem::RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		return new OpenGLDepthStencilState(initializer);
	}

	void OpenGLSystem::RHISetRasterizerState(RHIRasterizerState& rasterizerState)
	{
		if( &rasterizerState == mDeviceState.rasterizerStateUsage )
		{
			return;
		}
		mDeviceState.rasterizerStateUsage = &rasterizerState;

		OpenGLRasterizerState& rasterizerStateGL = static_cast<OpenGLRasterizerState&>(rasterizerState);
		auto& deviceValue = mDeviceState.rasterizerStateValue;
		auto const& setupValue = rasterizerStateGL.mStateValue;

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

	void OpenGLSystem::RHISetBlendState(RHIBlendState& blendState)
	{

		if( &blendState == mDeviceState.blendStateUsage )
			return;

		mDeviceState.blendStateUsage = &blendState;

		OpenGLBlendState& BlendStateGL = static_cast<OpenGLBlendState&>(blendState);


		for( int i = 0; i < NumBlendStateTarget; ++i )
		{
			auto& deviceValue = mDeviceState.blendStateValue.targetValues[i];
			auto const& setupValue = BlendStateGL.mStateValue.targetValues[i];

#define VAR_TEST( NAME )  true || ( deviceValue.##NAME != setupValue.##NAME )
#define VAR_ASSIGN( NAME ) deviceValue.##NAME = setupValue.##NAME 

			bool bFroceReset = false;

			if( VAR_TEST(writeMask) )
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

				if( VAR_TEST(bEnable) )
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
							glBlendFunci(i, setupValue.srcColor, setupValue.destColor);
						}
					}
				}
			}

#undef VAR_TEST
#undef VAR_ASSIGN
		}
	}

	void OpenGLSystem::RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		auto& deviceValue = mDeviceState.depthStencilStateValue;
		if( &depthStencilState == mDeviceState.depthStencilStateUsage )
		{
			if( deviceValue.stencilRef != stencilRef )
			{
				deviceValue.stencilRef = stencilRef;
				glStencilFunc(deviceValue.stencilFun, stencilRef, deviceValue.stencilReadMask);
			}
			return;
		}
		mDeviceState.depthStencilStateUsage = &depthStencilState;
		
		OpenGLDepthStencilState& depthStencilStateGL = static_cast<OpenGLDepthStencilState&>(depthStencilState);

		auto const& setupValue = depthStencilStateGL.mStateValue;

#define VAR_TEST( NAME )   true || (deviceValue.##NAME != setupValue.##NAME)
#define VAR_ASSIGN( NAME ) deviceValue.##NAME = setupValue.##NAME 

		if( VAR_TEST(bEnableDepthTest) )
		{
			VAR_ASSIGN(bEnableDepthTest);
			EnableGLState(GL_DEPTH_TEST, setupValue.bEnableDepthTest);
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

			bool bForceRestOp = false;
			if( VAR_TEST(bUseSeparateStencilOp) )
			{
				VAR_ASSIGN(bUseSeparateStencilOp);
				bForceRestOp = true;
			}

			bool bForceRestFun = false;
			if( VAR_TEST(bUseSeparateStencilFun) )
			{
				VAR_ASSIGN(bUseSeparateStencilFun);
				bForceRestFun = true;
			}

			if( setupValue.bUseSeparateStencilOp )
			{
				if( bForceRestOp || VAR_TEST(stencilFailOp) || VAR_TEST(stencilZFailOp) || VAR_TEST(stencilZPassOp) || VAR_TEST(stencilFailOpBack) || VAR_TEST(stencilZFailOpBack) || VAR_TEST(stencilZPassOpBack) )
				{
					VAR_ASSIGN(stencilFailOp);
					VAR_ASSIGN(stencilZFailOp);
					VAR_ASSIGN(stencilZPassOp);
					VAR_ASSIGN(stencilFailOpBack);
					VAR_ASSIGN(stencilZFailOpBack);
					VAR_ASSIGN(stencilZPassOpBack);
					glStencilOpSeparate(GL_FRONT, setupValue.stencilFailOp, setupValue.stencilZFailOp, setupValue.stencilZPassOp);
					glStencilOpSeparate(GL_BACK, setupValue.stencilFailOpBack, setupValue.stencilZFailOpBack, setupValue.stencilZPassOpBack);
				}
			}
			else
			{
				if( bForceRestOp || VAR_TEST(stencilFailOp) || VAR_TEST(stencilZFailOp) || VAR_TEST(stencilZPassOp) )
				{
					VAR_ASSIGN(stencilFailOp);
					VAR_ASSIGN(stencilZFailOp);
					VAR_ASSIGN(stencilZPassOp);
					glStencilOp(setupValue.stencilFailOp, setupValue.stencilZFailOp, setupValue.stencilZPassOp);
				}
			}
			if( setupValue.bUseSeparateStencilFun )
			{
				if( bForceRestFun || VAR_TEST(stencilFun) || VAR_TEST(stencilFunBack) || deviceValue.stencilRef != stencilRef || VAR_TEST(stencilReadMask) )
				{
					VAR_ASSIGN(stencilFun);
					VAR_ASSIGN(stencilFunBack);
					VAR_ASSIGN(stencilReadMask);
					deviceValue.stencilRef = stencilRef;
					glStencilFuncSeparate(GL_FRONT, setupValue.stencilFun, stencilRef, setupValue.stencilReadMask);
					glStencilFuncSeparate(GL_BACK, setupValue.stencilFunBack, stencilRef, setupValue.stencilReadMask);
				}
			}
			else
			{
				if( bForceRestFun || VAR_TEST(stencilFun) || deviceValue.stencilRef != stencilRef || VAR_TEST(stencilReadMask) )
				{
					VAR_ASSIGN(stencilFun);
					VAR_ASSIGN(stencilReadMask);
					deviceValue.stencilRef = stencilRef;
					glStencilFunc(setupValue.stencilFun, stencilRef, setupValue.stencilReadMask);
				}
			}
		}
		else
		{


		}


#undef VAR_TEST
#undef VAR_ASSIGN
	}

	void OpenGLSystem::RHISetViewport(int x, int y, int w, int h)
	{
		glViewport(x, y, w, h);
	}

	void OpenGLSystem::RHISetScissorRect(bool bEnable, int x, int y, int w, int h)
	{
		if( mDeviceState.bScissorRectEnabled != bEnable )
		{
			EnableGLState(GL_SCISSOR_TEST, bEnable);
			mDeviceState.bScissorRectEnabled = bEnable;
			if( bEnable )
			{
				glScissor(x, y, w, h);
			}
		}
	}

	void OpenGLSystem::RHIDrawPrimitive(PrimitiveType type, int start, int nv)
	{
		glDrawArrays(GLConvert::To(type), start, nv);
	}

	void OpenGLSystem::RHISetupFixedPipelineState(Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture , RHITexture2D** textures )
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
				glBindTexture(GL_TEXTURE_2D, textures[i] ? OpenGLCast::GetHandle(*textures[i]) : 0);
			}
		}
	}

	void OpenGLSystem::RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex)
	{
		assert(indexType == CVT_UInt || indexType == CVT_UShort);
		glDrawElements(GLConvert::To(type), nIndex, GLConvert::To(indexType), (void*)indexStart);
	}

}
