#include "OpenGLCommand.h"

#include "OpenGLCommon.h"
#include "OpenGLShaderCommon.h"

#include "RHI/ShaderCore.h"
#include "RHI/GpuProfiler.h"

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

namespace Render
{
	bool gForceInitState = false;

	void WINAPI GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
	{
		if( type == GL_DEBUG_TYPE_OTHER )
			return;

		LogMsg(message);
	}

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

	struct OpenGLTiming
	{
		OpenGLTiming()
		{
			mHandles[0] = mHandles[1] = 0;
		}

		void start()
		{
			release();
			glGenQueries(2, mHandles);
			glQueryCounter(mHandles[0], GL_TIMESTAMP);
		}
		void end()
		{
			glQueryCounter(mHandles[1], GL_TIMESTAMP);
		}
		bool getTime(uint64& result)
		{
			GLuint isAvailable = GL_TRUE;
			glGetQueryObjectuiv(mHandles[0], GL_QUERY_RESULT_AVAILABLE, &isAvailable);
			if( isAvailable == GL_TRUE )
			{
				glGetQueryObjectuiv(mHandles[1], GL_QUERY_RESULT_AVAILABLE, &isAvailable);

				if( isAvailable == GL_TRUE )
				{
					GLuint64 startTimeStamp;
					glGetQueryObjectui64v(mHandles[0], GL_QUERY_RESULT, &startTimeStamp);
					GLuint64 endTimeStamp;
					glGetQueryObjectui64v(mHandles[1], GL_QUERY_RESULT, &endTimeStamp);

					result = endTimeStamp - startTimeStamp;
					return true;
				}
			}
			return false;
		}
		void release()
		{
			if( mHandles[0] != 0 )
			{
				glDeleteQueries(2, mHandles);
				mHandles[0] = mHandles[1] = 0;
			}
		}

		GLuint mHandles[2];
	};

	class OpenglProfileCore : public RHIProfileCore
	{
	public:
		void releaseRHI() override
		{
		}
		void beginFrame() override
		{

		}
		bool endFrame() override
		{
			return true;
		}
		uint32 fetchTiming() override
		{
			uint32 result = mTimingStorage.size();
			mTimingStorage.resize(mTimingStorage.size() + 1);
			return result;
		}

		void startTiming(uint32 timingHandle) override
		{
			OpenGLTiming& timing = mTimingStorage[timingHandle];
			timing.start();

		}
		void endTiming(uint32 timingHandle) override
		{
			OpenGLTiming& timing = mTimingStorage[timingHandle];
			timing.end();
		}

		bool getTimingDurtion(uint32 timingHandle, uint64& outDurtion) override
		{
			OpenGLTiming& timing = mTimingStorage[timingHandle];
			return timing.getTime(outDurtion);
		}
		double getCycleToMillisecond() override
		{
			return 1.0 / 1000000.0;
		}
		std::vector< OpenGLTiming > mTimingStorage;

	};

	bool OpenGLSystem::initialize(RHISystemInitParams const& initParam)
	{
		WGLPixelFormat format;
		format.numSamples = initParam.numSamples;
		if( !mGLContext.init(initParam.hDC, format) )
			return false;

		if( glewInit() != GLEW_OK )
			return false;

		char const* vendorStr = (char const*) glGetString(GL_VENDOR);

		if( strstr(vendorStr, "NVIDIA") != nullptr )
		{
			gRHIDeviceVendorName = DeviceVendorName::NVIDIA;
		}
		else if( strstr(vendorStr, "ATI") != nullptr )
		{
			gRHIDeviceVendorName = DeviceVendorName::ATI;
		}
		else if( strstr(vendorStr, "Intel") != nullptr )
		{
			gRHIDeviceVendorName = DeviceVendorName::Intel;
		}

		bool bEnableDebugOutput = true;

		if(bEnableDebugOutput)
		{
			glDebugMessageCallback(GLDebugCallback, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}

		gRHIClipZMin = -1;
		gRHIProjectYSign = 1;


		if( 1 )
		{
			mProfileCore = new OpenglProfileCore;
			GpuProfiler::Get().setCore(mProfileCore);
		}

		mDrawContext.initialize();
		mImmediateCommandList = new RHICommandListImpl(mDrawContext);
		return true;
	}

	void OpenGLSystem::shutdown()
	{
		if( mProfileCore )
		{
			GpuProfiler::Get().setCore(nullptr);
			delete mProfileCore;
			mProfileCore = nullptr;
		}

		delete mImmediateCommandList;

		mGLContext.cleanup();
	}

	class ShaderFormat* OpenGLSystem::createShaderFormat()
	{
		return new ShaderFormatGLSL;
	}

	bool OpenGLSystem::RHIBeginRender()
	{
		if( !mGLContext.makeCurrent() )
			return false;

		if( 1 )
		{
			gForceInitState = true;
			RHISetDepthStencilState(*mImmediateCommandList, TStaticDepthStencilState<>::GetRHI(), 0xff);
			RHISetBlendState(*mImmediateCommandList, TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(*mImmediateCommandList, TStaticRasterizerState<>::GetRHI());
			gForceInitState = true;
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
		return CreateOpenGLResourceT< OpenGLTexture1D >(format, length, numMipLevel, createFlags, data);
	}


	RHITexture2D* OpenGLSystem::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 createFlags, void* data, int dataAlign)
	{
		return CreateOpenGLResourceT< OpenGLTexture2D >(format, w, h, numMipLevel, numSamples, createFlags, data, dataAlign);
	}

	RHITexture3D* OpenGLSystem::RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ, int numMipLevel, int numSamples, uint32 createFlags, void* data)
	{
		return CreateOpenGLResourceT< OpenGLTexture3D >(format, sizeX, sizeY, sizeZ, numMipLevel, numSamples, createFlags , data);
	}

	RHITextureCube* OpenGLSystem::RHICreateTextureCube(Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
	{
		return CreateOpenGLResourceT< OpenGLTextureCube >( format , size , numMipLevel , creationFlags , data );
	}

	RHITexture2DArray* OpenGLSystem::RHICreateTexture2DArray(Texture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		return CreateOpenGLResourceT< OpenGLTexture2DArray >(format, w, h, layerSize, numMipLevel, numSamples, creationFlags, data);
	}

	RHITextureDepth* OpenGLSystem::RHICreateTextureDepth(Texture::DepthFormat format, int w, int h, int numMipLevel, int numSamples)
	{
		return CreateOpenGLResourceT< OpenGLTextureDepth >(format, w , h , numMipLevel, numSamples);
	}

	RHIVertexBuffer* OpenGLSystem::RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		return CreateOpenGLResourceT< OpenGLVertexBuffer >(vertexSize, numVertices, creationFlags, data);
	}

	RHIIndexBuffer* OpenGLSystem::RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		return CreateOpenGLResourceT< OpenGLIndexBuffer >(nIndices, bIntIndex, creationFlags, data);
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

	RHIInputLayout* OpenGLSystem::RHICreateInputLayout(InputLayoutDesc const& desc)
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

	RHIShader* OpenGLSystem::RHICreateShader(Shader::Type type)
	{
		return CreateOpenGLResourceT< OpenGLShader >(type);
	}

	RHIShaderProgram* OpenGLSystem::RHICreateShaderProgram()
	{
		return CreateOpenGLResourceT< OpenGLShaderProgram >();
	}

	void OpenGLContext::RHISetRasterizerState(RHIRasterizerState& rasterizerState)
	{
		mDeviceState.rasterizerStateCommit = &rasterizerState;
		commitRasterizerState();
	}

	void OpenGLContext::RHISetBlendState(RHIBlendState& blendState)
	{
		mDeviceState.blendStateCommit = &blendState;
		commitBlendState();
	}

	void OpenGLContext::RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		mDeviceState.depthStencilStateCommit = &depthStencilState;
		mDeviceState.stencilRefCommit = stencilRef;
		commitDepthStencilState();
	}

	void OpenGLContext::RHISetViewport(int x, int y, int w, int h , float zNear, float zFar)
	{
		glViewport(x, y, w, h);
		glDepthRange(zNear, zFar);
	}

	void OpenGLContext::RHISetScissorRect(int x, int y, int w, int h)
	{
		glScissor(x, y, w, h);
	}

	void OpenGLContext::RHISetupFixedPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* textures[], int numTexture)
	{
		RHISetShaderProgram(nullptr);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(transform);

		glColor4fv(color);
		if( numTexture )
		{
			glEnable(GL_TEXTURE_2D);
			for( int i = 0; i < numTexture; ++i )
			{
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, textures[i] ? OpenGLCast::GetHandle(*textures[i]) : 0);
			}
			glActiveTexture(GL_TEXTURE0);
		}
		else
		{
			glDisable(GL_TEXTURE_2D);
		}
	}

	void OpenGLContext::RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream)
	{
		if ( inputLayout != mLastInputLayout )
		{
			if( mLastInputLayout.isValid() )
			{
				if( mWasBindAttrib )
				{
					OpenGLCast::To(mLastInputLayout)->unbindAttrib(mNumInputStream);
				}
				else
				{
					OpenGLCast::To(mLastInputLayout)->unbindPointer();
				}
			}
			mLastInputLayout = inputLayout;
		}
		mNumInputStream = numInputStream;
		if( numInputStream )
		{
			std::copy(inputStreams, inputStreams + numInputStream, mUsedInputStreams);
		}

	}

	void OpenGLContext::RHISetIndexBuffer(RHIIndexBuffer* indexBuffer)
	{
		mLastIndexBuffer = indexBuffer;	
	}

	void OpenGLContext::RHIDrawPrimitive(PrimitiveType type, int start, int nv)
	{
		if( !commitInputStream() )
			return;

		commitRenderStates();

		glDrawArrays(OpenGLTranslate::To(type), start, nv);
	}

	void OpenGLContext::RHIDrawIndexedPrimitive(PrimitiveType type, int indexStart, int nIndex , uint32 baseVertex )
	{
		if( !mLastIndexBuffer.isValid() )
		{
			return;
		}

		if( !commitInputStream() )
			return;

		commitRenderStates();

		OpenGLCast::To(mLastIndexBuffer)->bind();
		GLenum indexType = mLastIndexBuffer->isIntType() ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
		if( baseVertex )
		{
			glDrawElementsBaseVertex(OpenGLTranslate::To(type), nIndex, indexType, (void*)indexStart, baseVertex);
		}
		else
		{
			glDrawElements(OpenGLTranslate::To(type), nIndex, indexType, (void*)indexStart);
		}
		OpenGLCast::To(mLastIndexBuffer)->unbind();
	}

	void OpenGLContext::RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset , int numCommand, int commandStride )
	{
		if( !commitInputStream() )
			return;

		commitRenderStates();

		assert(commandBuffer);
		GLenum priType = OpenGLTranslate::To(type);

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

	void OpenGLContext::RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset , int numCommand, int commandStride)
	{
		if( !mLastIndexBuffer.isValid() )
		{
			return;
		}

		if( !mLastIndexBuffer.isValid() )
		{
			return;
		}

		if( !commitInputStream() )
			return;
		
		commitRenderStates();

		OpenGLCast::To(mLastIndexBuffer)->bind();
		assert(commandBuffer);
		GLenum priType = OpenGLTranslate::To(type);
		GLenum indexType = mLastIndexBuffer->isIntType() ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, OpenGLCast::GetHandle(*commandBuffer));
		if( numCommand > 1 )
		{
			glMultiDrawElementsIndirect(priType, indexType, (void*)offset, numCommand, commandStride);
		}
		else
		{
			glDrawElementsIndirect(priType, indexType, (void*)offset);
		}
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		OpenGLCast::To(mLastIndexBuffer)->unbind();
	}

	void OpenGLContext::RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
	{
		if( !commitInputStream() )
			return;

		commitRenderStates();

		GLenum priType = OpenGLTranslate::To(type);
		if( baseInstance )
		{
			glDrawArraysInstancedBaseInstance(priType, vStart, nv, numInstance, baseInstance);
		}
		else
		{
			glDrawArraysInstanced(priType, vStart, nv, numInstance);
		}
		
	}

	void OpenGLContext::RHIDrawIndexedPrimitiveInstanced(PrimitiveType type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance)
	{
		if( !mLastIndexBuffer.isValid() )
		{
			return;
		}
		if( !commitInputStream() )
			return;

		commitRenderStates();
		
		GLenum priType = OpenGLTranslate::To(type);
	
		OpenGLCast::To(mLastIndexBuffer)->bind();
		GLenum indexType = mLastIndexBuffer->isIntType() ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
		if( baseVertex )
		{
			if( baseInstance )
			{
				glDrawElementsInstancedBaseVertexBaseInstance(priType, nIndex, indexType, (void*)indexStart, numInstance, baseVertex, baseInstance);
			}
			else
			{
				glDrawElementsInstancedBaseVertex(priType, nIndex, indexType, (void*)indexStart, numInstance, baseVertex);
			}		
		}
		else
		{
			if( baseInstance )
			{
				glDrawElementsInstancedBaseInstance(priType, nIndex, indexType, (void*)indexStart, numInstance, baseInstance);
			}
			else
			{
				glDrawElementsInstanced(priType, nIndex, indexType, (void*)indexStart, numInstance);
			}
			
		}
		OpenGLCast::To(mLastIndexBuffer)->unbind();
	}

	void OpenGLContext::RHIDrawPrimitiveUP(PrimitiveType type, int numVertex, VertexDataInfo dataInfos[], int numData)
	{
		if( !commitInputStreamUP(dataInfos, numData) )
			return;

		commitRenderStates();

		glDrawArrays(OpenGLTranslate::To(type), 0, numVertex);
	}

	void OpenGLContext::RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numVerex, VertexDataInfo dataInfos[], int numVertexData , int const* pIndices, int numIndex)
	{
		if( pIndices == nullptr )
			return;

		if( !commitInputStreamUP(dataInfos, numVertexData) )
			return;

		commitRenderStates();

		glDrawElements(OpenGLTranslate::To(type), numIndex, GL_UNSIGNED_INT, (void*)pIndices);
	}

	void OpenGLContext::RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
	{
		commitSamplerStates();
		glDispatchCompute(numGroupX, numGroupY, numGroupZ);
	}

	void OpenGLContext::RHIFlushCommand()
	{
		glFlush();
	}

	void OpenGLContext::RHISetShaderProgram(RHIShaderProgram* shaderProgram)
	{
		if( mLastShaderProgram.isValid() )
			static_cast<OpenGLShaderProgram&>(*mLastShaderProgram).unbind();

		if( shaderProgram )
		{
			static_cast<OpenGLShaderProgram&>(*shaderProgram).bind();
			resetBindIndex();
			mLastShaderProgram = shaderProgram;
			mbUseFixedPipeline = false;
		}
		else
		{
			mbUseFixedPipeline = true;
		}
	}

#define CHECK_PARAMETER( PARAM ) assert( PARAM.isBound() );


	void OpenGLContext::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim)
	{
		CHECK_PARAMETER(param);
		switch( dim )
		{
		case 1: glUniform1i(param.mLoc, val[0]); break;
		case 2: glUniform2i(param.mLoc, val[0], val[1]); break;
		case 3: glUniform3i(param.mLoc, val[0], val[1], val[2]); break;
		case 4: glUniform4i(param.mLoc, val[0], val[1], val[2], val[3]); break;
		}
	}

	void OpenGLContext::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& parameter, float const val[], int dim)
	{
		CHECK_PARAMETER(parameter);
		switch( dim )
		{
		case 1: glUniform1f(parameter.mLoc, val[0]); break;
		case 2: glUniform2f(parameter.mLoc, val[0], val[1]); break;
		case 3: glUniform3f(parameter.mLoc, val[0], val[1], val[2]); break;
		case 4: glUniform4f(parameter.mLoc, val[0], val[1], val[2], val[3]); break;
		}
	}

	void OpenGLContext::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim)
	{
		CHECK_PARAMETER(param);
		glUniformMatrix3fv(param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim)
	{
		CHECK_PARAMETER(param);
		glUniformMatrix4fv(param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim)
	{
		CHECK_PARAMETER(param);
		glUniform3fv(param.mLoc, dim, (float const *)val);
	}

	void OpenGLContext::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim)
	{
		CHECK_PARAMETER(param);
		glUniform4fv(param.mLoc, dim, (float const *)val);
	}

	void OpenGLContext::setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		CHECK_PARAMETER(param);
		glUniformMatrix2fv(param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		CHECK_PARAMETER(param);
		glUniformMatrix4x3fv(param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		CHECK_PARAMETER(param);
		glUniformMatrix3x4fv(param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView)
	{
		CHECK_PARAMETER(param);
		setShaderResourceViewInternal(shaderProgram, param, resourceView);
	}

	void OpenGLContext::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture)
	{
		CHECK_PARAMETER(param);
		setShaderResourceViewInternal(shaderProgram, param, *texture.getBaseResourceView());
	}

	void OpenGLContext::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler)
	{
		CHECK_PARAMETER(param);
		setShaderResourceViewInternal(shaderProgram, param, *texture.getBaseResourceView(), sampler);
	}

	void OpenGLContext::setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler)
	{
		CHECK_PARAMETER(param);

		OpenGLSamplerState const&  samplerImpl = static_cast<OpenGLSamplerState const&>(sampler);
		for (int index = 0; index < mNextAutoBindSamplerSlotIndex; ++index)
		{
			if (mSamplerStates[index].loc = param.mLoc)
			{
				mSamplerStates[index].samplerHandle = samplerImpl.getHandle();

				mSimplerSlotDirtyMask |= BIT(index);
				break;
			}
		}
	}

	void OpenGLContext::setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op)
	{
		CHECK_PARAMETER(param);
		OpenGLShaderResourceView const& resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(*texture.getBaseResourceView());
		int idx = mNextAutoBindSamplerSlotIndex;
		++mNextAutoBindSamplerSlotIndex;
		glBindImageTexture(idx, resourceViewImpl.handle, 0, GL_FALSE, 0, OpenGLTranslate::To(op), OpenGLTranslate::To(texture.getFormat()));
		glUniform1i(param.mLoc, idx);
	}

	void OpenGLContext::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(shaderProgram);
		glUniformBlockBinding(shaderProgramImpl.getHandle(), param.mLoc, mNextUniformSlot);
		glBindBufferBase(GL_UNIFORM_BUFFER, mNextUniformSlot, OpenGLCast::GetHandle(buffer));
		++mNextUniformSlot;
	}

	void OpenGLContext::setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(shaderProgram);
		glShaderStorageBlockBinding(shaderProgramImpl.getHandle(), param.mLoc, mNextStorageSlot);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mNextStorageSlot, OpenGLCast::GetHandle(buffer));
		++mNextStorageSlot;
	}

	void OpenGLContext::setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, param.mLoc, OpenGLCast::GetHandle(buffer));
	}

	void OpenGLContext::commitRasterizerState()
	{
		if (!gForceInitState && mDeviceState.rasterizerStateCommit == mDeviceState.rasterizerStateUsage)
		{
			return;
		}
		mDeviceState.rasterizerStateUsage = mDeviceState.rasterizerStateCommit;

		OpenGLRasterizerState& rasterizerStateGL = static_cast<OpenGLRasterizerState&>(*mDeviceState.rasterizerStateCommit);
		auto& deviceValue = mDeviceState.rasterizerStateValue;
		auto const& setupValue = rasterizerStateGL.mStateValue;

		if (GL_STATE_VAR_TEST(fillMode))
		{
			GL_STATE_VAR_ASSIGN(fillMode);
			glPolygonMode(GL_FRONT_AND_BACK, setupValue.fillMode);
		}

		if (GL_STATE_VAR_TEST(bEnableScissor))
		{
			GL_STATE_VAR_ASSIGN(bEnableScissor);
			EnableGLState(GL_SCISSOR_TEST, setupValue.bEnableScissor);
		}

		if (GL_STATE_VAR_TEST(bEnableCull))
		{
			GL_STATE_VAR_ASSIGN(bEnableCull);
			EnableGLState(GL_CULL_FACE, setupValue.bEnableCull);

			if (GL_STATE_VAR_TEST(cullFace))
			{
				GL_STATE_VAR_ASSIGN(cullFace);
				glCullFace(setupValue.cullFace);
			}
		}
	}

	void OpenGLContext::commitBlendState()
	{
		if (!gForceInitState && mDeviceState.blendStateCommit == mDeviceState.blendStateUsage)
			return;

		mDeviceState.blendStateUsage = mDeviceState.blendStateCommit;
		OpenGLBlendState& BlendStateGL = static_cast<OpenGLBlendState&>(*mDeviceState.blendStateCommit);
		bool bForceAllReset = false;
		{
			auto& deviceValue = mDeviceState.blendStateValue;
			auto const& setupValue = BlendStateGL.mStateValue;
			if (GL_STATE_VAR_TEST(bEnableAlphaToCoverage))
			{
				GL_STATE_VAR_ASSIGN(bEnableAlphaToCoverage);
				EnableGLState(GL_SAMPLE_ALPHA_TO_COVERAGE, setupValue.bEnableAlphaToCoverage);
			}
			if (GL_STATE_VAR_TEST(bEnableIndependent))
			{
				GL_STATE_VAR_ASSIGN(bEnableAlphaToCoverage);
				bForceAllReset = true;
			}
		}

		if (BlendStateGL.mStateValue.bEnableIndependent)
		{
			for (int i = 0; i < NumBlendStateTarget; ++i)
			{
				auto& deviceValue = mDeviceState.blendStateValue.targetValues[i];
				auto const& setupValue = BlendStateGL.mStateValue.targetValues[i];

				bool bForceReset = false;

				if (bForceAllReset || GL_STATE_VAR_TEST(writeMask))
				{
					bForceReset = true;
					GL_STATE_VAR_ASSIGN(writeMask);
					glColorMaski(i, setupValue.writeMask & CWM_R,
						setupValue.writeMask & CWM_G,
						setupValue.writeMask & CWM_B,
						setupValue.writeMask & CWM_A);
				}

				if (bForceAllReset || setupValue.writeMask)
				{

					if (bForceAllReset || GL_STATE_VAR_TEST(bEnable))
					{
						GL_STATE_VAR_ASSIGN(bEnable);
						EnableGLStateIndex(GL_BLEND, i, setupValue.bEnable);
					}

					if (setupValue.bEnable)
					{
						if (bForceReset || GL_STATE_VAR_TEST(bSeparateBlend) || GL_STATE_VAR_TEST(srcColor) || GL_STATE_VAR_TEST(destColor) || GL_STATE_VAR_TEST(srcAlpha) || GL_STATE_VAR_TEST(destAlpha))
						{
							GL_STATE_VAR_ASSIGN(bSeparateBlend);
							GL_STATE_VAR_ASSIGN(srcColor);
							GL_STATE_VAR_ASSIGN(destColor);
							GL_STATE_VAR_ASSIGN(srcAlpha);
							GL_STATE_VAR_ASSIGN(destAlpha);

							if (setupValue.bSeparateBlend)
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
		else
		{
			auto& deviceValue = mDeviceState.blendStateValue.targetValues[0];
			auto const& setupValue = BlendStateGL.mStateValue.targetValues[0];

			bool bForceReset = false;

			if (bForceAllReset || GL_STATE_VAR_TEST(writeMask))
			{
				bForceReset = true;
				GL_STATE_VAR_ASSIGN(writeMask);
				glColorMask(setupValue.writeMask & CWM_R,
					setupValue.writeMask & CWM_G,
					setupValue.writeMask & CWM_B,
					setupValue.writeMask & CWM_A);
			}

			if (bForceAllReset || setupValue.writeMask)
			{

				if (bForceAllReset || GL_STATE_VAR_TEST(bEnable))
				{
					GL_STATE_VAR_ASSIGN(bEnable);
					EnableGLState(GL_BLEND, setupValue.bEnable);
				}

				if (setupValue.bEnable)
				{
					if (bForceReset || GL_STATE_VAR_TEST(bSeparateBlend) || GL_STATE_VAR_TEST(srcColor) || GL_STATE_VAR_TEST(destColor) || GL_STATE_VAR_TEST(srcAlpha) || GL_STATE_VAR_TEST(destAlpha))
					{
						GL_STATE_VAR_ASSIGN(bSeparateBlend);
						GL_STATE_VAR_ASSIGN(srcColor);
						GL_STATE_VAR_ASSIGN(destColor);
						GL_STATE_VAR_ASSIGN(srcAlpha);
						GL_STATE_VAR_ASSIGN(destAlpha);

						if (setupValue.bSeparateBlend)
						{

							glBlendFuncSeparate(setupValue.srcColor, setupValue.destColor,
								setupValue.srcAlpha, setupValue.destAlpha);
						}
						else
						{
							glBlendFunc(setupValue.srcColor, setupValue.destColor);
						}
					}
				}
			}
		}
	}

	void OpenGLContext::commitDepthStencilState()
	{
		auto& deviceValue = mDeviceState.depthStencilStateValue;
		if (!gForceInitState && mDeviceState.depthStencilStateCommit == mDeviceState.depthStencilStateUsage)
		{
			if (deviceValue.stencilRef != mDeviceState.stencilRefCommit )
			{
				deviceValue.stencilRef = mDeviceState.stencilRefCommit;
				glStencilFunc(deviceValue.stencilFun, mDeviceState.stencilRefCommit, deviceValue.stencilReadMask);
			}
			return;
		}
		mDeviceState.depthStencilStateUsage = mDeviceState.depthStencilStateCommit;

		OpenGLDepthStencilState& depthStencilStateGL = static_cast<OpenGLDepthStencilState&>(*mDeviceState.depthStencilStateCommit);

		auto const& setupValue = depthStencilStateGL.mStateValue;

		if (GL_STATE_VAR_TEST(bEnableDepthTest))
		{
			GL_STATE_VAR_ASSIGN(bEnableDepthTest);
			EnableGLState(GL_DEPTH_TEST, setupValue.bEnableDepthTest);
		}

		if (setupValue.bEnableDepthTest)
		{
			if (GL_STATE_VAR_TEST(bWriteDepth))
			{
				GL_STATE_VAR_ASSIGN(bWriteDepth);
				glDepthMask(setupValue.bWriteDepth);
			}
			if (GL_STATE_VAR_TEST(depthFun))
			{
				GL_STATE_VAR_ASSIGN(depthFun);
				glDepthFunc(setupValue.depthFun);
			}
		}
		else
		{
			//#TODO : Check State Value
		}

		if (GL_STATE_VAR_TEST(bEnableStencilTest))
		{
			GL_STATE_VAR_ASSIGN(bEnableStencilTest);
			EnableGLState(GL_STENCIL_TEST, setupValue.bEnableStencilTest);
		}

		if (GL_STATE_VAR_TEST(stencilWriteMask))
		{
			GL_STATE_VAR_ASSIGN(stencilWriteMask);
			glStencilMask(setupValue.stencilWriteMask);
		}

		if (setupValue.bEnableStencilTest)
		{

			bool bForceRestOp = false;
			if (GL_STATE_VAR_TEST(bUseSeparateStencilOp))
			{
				GL_STATE_VAR_ASSIGN(bUseSeparateStencilOp);
				bForceRestOp = true;
			}

			bool bForceRestFun = false;
			if (GL_STATE_VAR_TEST(bUseSeparateStencilFun))
			{
				GL_STATE_VAR_ASSIGN(bUseSeparateStencilFun);
				bForceRestFun = true;
			}

			if (setupValue.bUseSeparateStencilOp)
			{
				if (bForceRestOp || GL_STATE_VAR_TEST(stencilFailOp) || GL_STATE_VAR_TEST(stencilZFailOp) || GL_STATE_VAR_TEST(stencilZPassOp) || GL_STATE_VAR_TEST(stencilFailOpBack) || GL_STATE_VAR_TEST(stencilZFailOpBack) || GL_STATE_VAR_TEST(stencilZPassOpBack))
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
				if (bForceRestOp || GL_STATE_VAR_TEST(stencilFailOp) || GL_STATE_VAR_TEST(stencilZFailOp) || GL_STATE_VAR_TEST(stencilZPassOp))
				{
					GL_STATE_VAR_ASSIGN(stencilFailOp);
					GL_STATE_VAR_ASSIGN(stencilZFailOp);
					GL_STATE_VAR_ASSIGN(stencilZPassOp);
					glStencilOp(setupValue.stencilFailOp, setupValue.stencilZFailOp, setupValue.stencilZPassOp);
				}
			}
			if (setupValue.bUseSeparateStencilFun)
			{
				if (bForceRestFun || GL_STATE_VAR_TEST(stencilFun) || GL_STATE_VAR_TEST(stencilFunBack) || deviceValue.stencilRef != mDeviceState.stencilRefCommit || GL_STATE_VAR_TEST(stencilReadMask))
				{
					GL_STATE_VAR_ASSIGN(stencilFun);
					GL_STATE_VAR_ASSIGN(stencilFunBack);
					GL_STATE_VAR_ASSIGN(stencilReadMask);
					deviceValue.stencilRef = mDeviceState.stencilRefCommit;
					glStencilFuncSeparate(GL_FRONT, setupValue.stencilFun, mDeviceState.stencilRefCommit, setupValue.stencilReadMask);
					glStencilFuncSeparate(GL_BACK, setupValue.stencilFunBack, mDeviceState.stencilRefCommit, setupValue.stencilReadMask);
				}
			}
			else
			{
				if (bForceRestFun || GL_STATE_VAR_TEST(stencilFun) || deviceValue.stencilRef != mDeviceState.stencilRefCommit || GL_STATE_VAR_TEST(stencilReadMask))
				{
					GL_STATE_VAR_ASSIGN(stencilFun);
					GL_STATE_VAR_ASSIGN(stencilReadMask);
					deviceValue.stencilRef = mDeviceState.stencilRefCommit;
					glStencilFunc(setupValue.stencilFun, mDeviceState.stencilRefCommit, setupValue.stencilReadMask);
				}
			}
		}
		else
		{


		}
	}

	void OpenGLContext::setShaderResourceViewInternal(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView)
	{
		OpenGLShaderResourceView const&  resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(resourceView);
		int index = mNextAutoBindSamplerSlotIndex++;
		SamplerState& state = mSamplerStates[index];
		mSimplerSlotDirtyMask |= BIT(index);

		state.loc = param.mLoc;
		state.textureHandle = resourceViewImpl.handle;
		state.typeEnum = resourceViewImpl.typeEnum;
		state.samplerHandle = 0;
		state.bWrite = false;
	}

	void OpenGLContext::setShaderResourceViewInternal(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView, RHISamplerState const& sampler)
	{
		OpenGLShaderResourceView const&  resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(resourceView);
		OpenGLSamplerState const&  samplerImpl = static_cast<OpenGLSamplerState const&>(sampler);
		int index = mNextAutoBindSamplerSlotIndex++;
		mSimplerSlotDirtyMask |= BIT(index);

		SamplerState& state = mSamplerStates[index];
		state.loc = param.mLoc;
		state.textureHandle = resourceViewImpl.handle;
		state.typeEnum = resourceViewImpl.typeEnum;
		state.samplerHandle = samplerImpl.getHandle();
		state.bWrite = false;
	}

}

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif