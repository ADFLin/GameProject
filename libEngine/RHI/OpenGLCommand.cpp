#include "OpenGLCommand.h"

#include "OpenGLCommon.h"

namespace Render
{
	bool gForceInitState = true;

#define GL_STATE_VAR_TEST( NAME )  gForceInitState || ( deviceValue.##NAME != setupValue.##NAME )
#define GL_STATE_VAR_ASSIGN( NAME ) deviceValue.##NAME = setupValue.##NAME 

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
	T* CreateOpenGLResourceT(Args&& ...args)
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

		char const* vendorStr = (char const*) glGetString(GL_VENDOR);

		if( strstr(vendorStr, "NVIDIA") != 0 )
		{
			gRHIDeviceVendorName = DeviceVendorName::NVIDIA;
		}
		else if( strstr(vendorStr, "ATI") != 0 )
		{
			gRHIDeviceVendorName = DeviceVendorName::ATI;
		}
		else if( strstr(vendorStr, "Intel") != 0 )
		{
			gRHIDeviceVendorName = DeviceVendorName::Intel;
		}

		if( 1 )
		{
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

		if( 1 )
		{
			gForceInitState = true;
			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI(), 0xff);
			RHISetBlendState(TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
			//gForceInitState = false;
		}

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
		return CreateOpenGLResourceT< OpenGLTexture1D >(format, length, numMipLevel, data);
	}

	RHITexture2D* OpenGLSystem::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, uint32 createFlags, void* data, int dataAlign)
	{
		return CreateOpenGLResourceT< OpenGLTexture2D >(format, w, h, numMipLevel, data, dataAlign);
	}

	RHITexture3D* OpenGLSystem::RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ, uint32 createFlags, void* data)
	{
		return CreateOpenGLResourceT< OpenGLTexture3D >(format, sizeX, sizeY, sizeZ, data);
	}

	RHITextureDepth* OpenGLSystem::RHICreateTextureDepth(Texture::DepthFormat format, int w, int h)
	{
		return CreateOpenGLResourceT< OpenGLTextureDepth >(format, w , h );
	}

	RHITextureCube* OpenGLSystem::RHICreateTextureCube()
	{
		return new OpenGLTextureCube;
	}

	RHIVertexBuffer* OpenGLSystem::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		return CreateOpenGLResourceT< OpenGLVertexBuffer >(vertexSize, numVertices, creationFlags, data);
	}

	RHIIndexBuffer* OpenGLSystem::RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		return CreateOpenGLResourceT< OpenGLIndexBuffer >(nIndices, bIntIndex, creationFlags, data);
	}

	RHIUniformBuffer* OpenGLSystem::RHICreateUniformBuffer(uint32 elementSize , uint32 numElement , uint32 creationFlags, void* data)
	{
		return CreateOpenGLResourceT< OpenGLUniformBuffer >(elementSize , numElement, creationFlags, data);
	}

	void* OpenGLSystem::RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		if( size )
			return OpenGLCast::To(buffer)->lock(access, offset, size);
		return OpenGLCast::To(buffer)->lock(access);
	}

	void OpenGLSystem::RHIUnlockBuffer(RHIVertexBuffer* buffer)
	{
		OpenGLCast::To(buffer)->unlock();
	}

	void* OpenGLSystem::RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		if( size )
			return OpenGLCast::To(buffer)->lock(access, offset, size);
		return OpenGLCast::To(buffer)->lock(access);
	}

	void OpenGLSystem::RHIUnlockBuffer(RHIIndexBuffer* buffer)
	{
		OpenGLCast::To(buffer)->unlock();
	}

	void* OpenGLSystem::RHILockBuffer(RHIUniformBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		if( size )
			return OpenGLCast::To(buffer)->lock(access, offset, size);
		return OpenGLCast::To(buffer)->lock(access);
	}

	void OpenGLSystem::RHIUnlockBuffer(RHIUniformBuffer* buffer)
	{
		OpenGLCast::To(buffer)->unlock();
	}

	Render::RHIInputLayout* OpenGLSystem::RHICreateInputLayout(InputLayoutDesc const& desc)
	{
		return new OpenGLInputLayout(desc);
	}

	RHIFrameBuffer* OpenGLSystem::RHICreateFrameBuffer()
	{
		return CreateOpenGLResourceT< OpenGLFrameBuffer >();
	}

	RHISamplerState* OpenGLSystem::RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		return CreateOpenGLResourceT< OpenGLSamplerState >(initializer);
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
		if( !gForceInitState && &rasterizerState == mDeviceState.rasterizerStateUsage )
		{
			return;
		}
		mDeviceState.rasterizerStateUsage = &rasterizerState;

		OpenGLRasterizerState& rasterizerStateGL = static_cast<OpenGLRasterizerState&>(rasterizerState);
		auto& deviceValue = mDeviceState.rasterizerStateValue;
		auto const& setupValue = rasterizerStateGL.mStateValue;

		if( GL_STATE_VAR_TEST(fillMode) )
		{
			GL_STATE_VAR_ASSIGN(fillMode);
			glPolygonMode(GL_FRONT_AND_BACK, setupValue.fillMode);
		}

		if( GL_STATE_VAR_TEST(bEnalbeCull) )
		{
			GL_STATE_VAR_ASSIGN(bEnalbeCull);
			EnableGLState(GL_CULL_FACE, setupValue.bEnalbeCull);

			if( GL_STATE_VAR_TEST(cullFace) )
			{
				GL_STATE_VAR_ASSIGN(cullFace);
				glCullFace(setupValue.cullFace);
			}
		}
	}

	void OpenGLSystem::RHISetBlendState(RHIBlendState& blendState)
	{

		if( !gForceInitState && &blendState == mDeviceState.blendStateUsage )
			return;

		mDeviceState.blendStateUsage = &blendState;

		OpenGLBlendState& BlendStateGL = static_cast<OpenGLBlendState&>(blendState);

		for( int i = 0; i < NumBlendStateTarget; ++i )
		{
			auto& deviceValue = mDeviceState.blendStateValue.targetValues[i];
			auto const& setupValue = BlendStateGL.mStateValue.targetValues[i];

			bool bFroceReset = false;

			if( GL_STATE_VAR_TEST(writeMask) )
			{
				bFroceReset = true;
				GL_STATE_VAR_ASSIGN(writeMask);
				glColorMaski(i, setupValue.writeMask & CWM_R,
							 setupValue.writeMask & CWM_G,
							 setupValue.writeMask & CWM_B,
							 setupValue.writeMask & CWM_A);
			}

			if( setupValue.writeMask )
			{

				if( GL_STATE_VAR_TEST(bEnable) )
				{
					GL_STATE_VAR_ASSIGN(bEnable);
					EnableGLStateIndex(GL_BLEND, i, setupValue.bEnable);
				}

				if( setupValue.bEnable )
				{
					if( bFroceReset || GL_STATE_VAR_TEST(bSeparateBlend) || GL_STATE_VAR_TEST(srcColor) || GL_STATE_VAR_TEST(destColor) || GL_STATE_VAR_TEST(srcAlpha) || GL_STATE_VAR_TEST(destAlpha) )
					{
						GL_STATE_VAR_ASSIGN(bSeparateBlend);
						GL_STATE_VAR_ASSIGN(srcColor);
						GL_STATE_VAR_ASSIGN(destColor);
						GL_STATE_VAR_ASSIGN(srcAlpha);
						GL_STATE_VAR_ASSIGN(destAlpha);

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
		}
	}

	void OpenGLSystem::RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		auto& deviceValue = mDeviceState.depthStencilStateValue;
		if( !gForceInitState && &depthStencilState == mDeviceState.depthStencilStateUsage )
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

		if( GL_STATE_VAR_TEST(bEnableDepthTest) )
		{
			GL_STATE_VAR_ASSIGN(bEnableDepthTest);
			EnableGLState(GL_DEPTH_TEST, setupValue.bEnableDepthTest);
		}

		if( setupValue.bEnableDepthTest )
		{
			if( GL_STATE_VAR_TEST(bWriteDepth) )
			{
				GL_STATE_VAR_ASSIGN(bWriteDepth);
				glDepthMask(setupValue.bWriteDepth);
			}
			if( GL_STATE_VAR_TEST(depthFun) )
			{
				GL_STATE_VAR_ASSIGN(depthFun);
				glDepthFunc(setupValue.depthFun);
			}
		}
		else
		{
			//#TODO : Check State Value
		}

		if( GL_STATE_VAR_TEST(bEnableStencilTest) )
		{
			GL_STATE_VAR_ASSIGN(bEnableStencilTest);
			EnableGLState(GL_STENCIL_TEST, setupValue.bEnableStencilTest);
		}

		if( GL_STATE_VAR_TEST(stencilWriteMask) )
		{
			GL_STATE_VAR_ASSIGN(stencilWriteMask);
			glStencilMask(setupValue.stencilWriteMask);
		}

		if( setupValue.bEnableStencilTest )
		{

			bool bForceRestOp = false;
			if( GL_STATE_VAR_TEST(bUseSeparateStencilOp) )
			{
				GL_STATE_VAR_ASSIGN(bUseSeparateStencilOp);
				bForceRestOp = true;
			}

			bool bForceRestFun = false;
			if( GL_STATE_VAR_TEST(bUseSeparateStencilFun) )
			{
				GL_STATE_VAR_ASSIGN(bUseSeparateStencilFun);
				bForceRestFun = true;
			}

			if( setupValue.bUseSeparateStencilOp )
			{
				if( bForceRestOp || GL_STATE_VAR_TEST(stencilFailOp) || GL_STATE_VAR_TEST(stencilZFailOp) || GL_STATE_VAR_TEST(stencilZPassOp) || GL_STATE_VAR_TEST(stencilFailOpBack) || GL_STATE_VAR_TEST(stencilZFailOpBack) || GL_STATE_VAR_TEST(stencilZPassOpBack) )
				{
					GL_STATE_VAR_ASSIGN(stencilFailOp);
					GL_STATE_VAR_ASSIGN(stencilZFailOp);
					GL_STATE_VAR_ASSIGN(stencilZPassOp);
					GL_STATE_VAR_ASSIGN(stencilFailOpBack);
					GL_STATE_VAR_ASSIGN(stencilZFailOpBack);
					GL_STATE_VAR_ASSIGN(stencilZPassOpBack);
					glStencilOpSeparate(GL_FRONT, setupValue.stencilFailOp, setupValue.stencilZFailOp, setupValue.stencilZPassOp);
					glStencilOpSeparate(GL_BACK, setupValue.stencilFailOpBack, setupValue.stencilZFailOpBack, setupValue.stencilZPassOpBack);
				}
			}
			else
			{
				if( bForceRestOp || GL_STATE_VAR_TEST(stencilFailOp) || GL_STATE_VAR_TEST(stencilZFailOp) || GL_STATE_VAR_TEST(stencilZPassOp) )
				{
					GL_STATE_VAR_ASSIGN(stencilFailOp);
					GL_STATE_VAR_ASSIGN(stencilZFailOp);
					GL_STATE_VAR_ASSIGN(stencilZPassOp);
					glStencilOp(setupValue.stencilFailOp, setupValue.stencilZFailOp, setupValue.stencilZPassOp);
				}
			}
			if( setupValue.bUseSeparateStencilFun )
			{
				if( bForceRestFun || GL_STATE_VAR_TEST(stencilFun) || GL_STATE_VAR_TEST(stencilFunBack) || deviceValue.stencilRef != stencilRef || GL_STATE_VAR_TEST(stencilReadMask) )
				{
					GL_STATE_VAR_ASSIGN(stencilFun);
					GL_STATE_VAR_ASSIGN(stencilFunBack);
					GL_STATE_VAR_ASSIGN(stencilReadMask);
					deviceValue.stencilRef = stencilRef;
					glStencilFuncSeparate(GL_FRONT, setupValue.stencilFun, stencilRef, setupValue.stencilReadMask);
					glStencilFuncSeparate(GL_BACK, setupValue.stencilFunBack, stencilRef, setupValue.stencilReadMask);
				}
			}
			else
			{
				if( bForceRestFun || GL_STATE_VAR_TEST(stencilFun) || deviceValue.stencilRef != stencilRef || GL_STATE_VAR_TEST(stencilReadMask) )
				{
					GL_STATE_VAR_ASSIGN(stencilFun);
					GL_STATE_VAR_ASSIGN(stencilReadMask);
					deviceValue.stencilRef = stencilRef;
					glStencilFunc(setupValue.stencilFun, stencilRef, setupValue.stencilReadMask);
				}
			}
		}
		else
		{


		}
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


	void OpenGLSystem::RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset , int numCommand, int commandStride )
	{
		assert(commandBuffer);
		GLenum priType = GLConvert::To(type);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, OpenGLCast::GetHandle(*commandBuffer));
		if( numCommand > 1 )
		{
			glMultiDrawArraysIndirect(priType, (void*)offset, numCommand, commandStride);
		}
		else
		{
			glDrawArraysIndirect(priType , (void*)offset);
		}
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}

	void OpenGLSystem::RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, ECompValueType indexType, RHIVertexBuffer* commandBuffer, int offset , int numCommand, int commandStride)
	{
		assert(commandBuffer);
		GLenum priType = GLConvert::To(type);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, OpenGLCast::GetHandle(*commandBuffer));
		if( numCommand > 1 )
		{
			glMultiDrawElementsIndirect(priType, GLConvert::To(indexType) , (void*)offset, numCommand, commandStride);
		}
		else
		{
			glDrawElementsIndirect(priType, GLConvert::To(indexType) , (void*)offset);
		}
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}

}
