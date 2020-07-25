#include "OpenGLCommand.h"

#include "OpenGLCommon.h"
#include "OpenGLShaderCommon.h"

#include "RHI/ShaderCore.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGlobalResource.h"

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

#define COMMIT_STATE_IMMEDIATELY 1

namespace Render
{
	bool gForceInitState = false;

	void WINAPI GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
	{
		if( type == GL_DEBUG_TYPE_OTHER )
			return;

		LogMsg(message);
	}

	template< class T >
	bool IsStateDirty(T& committedValue, T const& pandingValue)
	{
		if (gForceInitState || committedValue != pandingValue)
		{
			committedValue = pandingValue;
			return true;
		}
		return false;
	}

#define GL_CHECK_STATE_DIRTY( NAME )  IsStateDirty( committedValue.##NAME , pendingValue.##NAME )
#define GL_TEST_STATE_DIRTY( NAME )  ( gForceInitState || ( committedValue.##NAME != pendingValue.##NAME ) )
#define GL_STATE_VAR_ASSIGN( NAME ) committedValue.##NAME = pendingValue.##NAME 

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
		bool getDuration(uint64& outDuration)
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

					outDuration = endTimeStamp - startTimeStamp;
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

		bool getTimingDuration(uint32 timingHandle, uint64& outDuration) override
		{
			OpenGLTiming& timing = mTimingStorage[timingHandle];
			return timing.getDuration(outDuration);
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
			GRHIDeviceVendorName = DeviceVendorName::NVIDIA;
		}
		else if( strstr(vendorStr, "ATI") != nullptr )
		{
			GRHIDeviceVendorName = DeviceVendorName::ATI;
		}
		else if( strstr(vendorStr, "Intel") != nullptr )
		{
			GRHIDeviceVendorName = DeviceVendorName::Intel;
		}

		bool bEnableDebugOutput = true;

		if(bEnableDebugOutput)
		{
			glDebugMessageCallback(GLDebugCallback, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}

		glEnable(GL_PROGRAM_POINT_SIZE);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		GRHIClipZMin = -1;
		GRHIProjectYSign = 1;


		if( 1 )
		{
			mProfileCore = new OpenglProfileCore;
			GpuProfiler::Get().setCore(mProfileCore);
		}

		mDrawContext.initialize();
		mDrawContext.mSystem = this;
		mImmediateCommandList = new RHICommandListImpl(mDrawContext);
		return true;
	}

	void OpenGLSystem::shutdown()
	{
		delete mImmediateCommandList;

		mDrawContext.shutdown();

		mGfxBoundStateMap.clear();

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
			//gForceInitState = true;
		}

		return true;
	}

	void OpenGLSystem::RHIEndRender(bool bPresent)
	{
		::glFlush();

		if( bPresent )
			mGLContext.swapBuffer();
	}

	RHISwapChain* OpenGLSystem::RHICreateSwapChain(SwapChainCreationInfo const& info)
	{
		return nullptr;
	}

	RHITexture1D* OpenGLSystem::RHICreateTexture1D(Texture::Format format, int length,  int numMipLevel, uint32 createFlags, void* data)
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

	RHIShader* OpenGLSystem::RHICreateShader(EShader::Type type)
	{
		return CreateOpenGLResourceT< OpenGLShader >(type);
	}

	RHIShaderProgram* OpenGLSystem::RHICreateShaderProgram()
	{
		return CreateOpenGLResourceT< OpenGLShaderProgram >();
	}

	OpenglShaderPipelineState* OpenGLSystem::getShaderPipeline(GraphicShaderBoundState const& state)
	{
		OpenGLShaderPipelineStateKey key(state);
		if (key.numShaders == 0)
			return nullptr;

		auto iter = mGfxBoundStateMap.find(key);
		if (iter != mGfxBoundStateMap.end())
		{
			return &iter->second;
		}

		OpenglShaderPipelineState shaderPipeline;
		if (!shaderPipeline.create(state))
			return nullptr;

		auto pair = mGfxBoundStateMap.emplace(key, std::move(shaderPipeline));
		CHECK(pair.second);
		return &pair.first->second;
	}

	void OpenGLContext::shutdown()
	{
		if (mLastFrameBuffer)
		{
			mLastFrameBuffer.release();
		}
		if (mLastShaderProgram)
		{
			mLastShaderProgram.release();
		}
		if (mLastIndexBuffer)
		{
			mLastIndexBuffer.release();
		}
		if (mInputLayoutPending)
		{
			mInputLayoutPending.release();
		}
		if (mInputLayoutCommitted)
		{
			mInputLayoutCommitted.release();
		}
	}

	void OpenGLContext::RHISetRasterizerState(RHIRasterizerState& rasterizerState)
	{
		mDeviceState.rasterizerStatePending = &rasterizerState;
#if COMMIT_STATE_IMMEDIATELY
		commitRasterizerState();
#endif
	}

	void OpenGLContext::RHISetBlendState(RHIBlendState& blendState)
	{
		mDeviceState.blendStatePending = &blendState;
#if COMMIT_STATE_IMMEDIATELY
		commitBlendState();
#endif
	}

	void OpenGLContext::RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		mDeviceState.depthStencilStatePending = &depthStencilState;
		mDeviceState.stencilRefPending = stencilRef;
#if COMMIT_STATE_IMMEDIATELY
		commitDepthStencilState();
#endif
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

	void OpenGLContext::RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler)
	{
		RHISetShaderProgram(nullptr);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(transform);

		glColor4fv(color);
		if(texture)
		{
			glEnable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, OpenGLCast::GetHandle(*texture));
			glBindSampler(0, sampler ? OpenGLCast::GetHandle(*sampler) : 0);
		}
		else
		{
			glDisable(GL_TEXTURE_2D);
		}
	}

	void OpenGLContext::RHISetFrameBuffer(RHIFrameBuffer* frameBuffer)
	{
		if (mLastFrameBuffer.isValid())
		{
			OpenGLCast::To(mLastFrameBuffer.get())->unbind();
		}

		mLastFrameBuffer = frameBuffer;
		if (mLastFrameBuffer.isValid())
		{
			OpenGLCast::To(mLastFrameBuffer.get())->bind();
		}
	}

	void OpenGLContext::RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream)
	{
		mInputLayoutPending = inputLayout;
		mInputStreamCountPending = numInputStream;
		if( numInputStream )
		{
			std::copy(inputStreams, inputStreams + numInputStream, mInputStreamsPending);
		}
		mbHasInputStreamPendingSetted = true;
	}

	void OpenGLContext::RHISetIndexBuffer(RHIIndexBuffer* indexBuffer)
	{
		mLastIndexBuffer = indexBuffer;	
	}

	void OpenGLContext::RHIDrawPrimitive(EPrimitive type, int start, int nv)
	{
		if( !commitInputStream() )
			return;

		commitRenderStates();
		int patchPointCount = 0;
		GLenum primitiveGL = OpenGLTranslate::To(type, patchPointCount);
		if (patchPointCount)
		{
			glPatchParameteri(GL_PATCH_VERTICES, patchPointCount);
		}
		glDrawArrays(primitiveGL, start, nv);
	}

	void OpenGLContext::RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex , uint32 baseVertex )
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
		GLenum primitiveGL = commitPrimitiveState(type);

		if( baseVertex )
		{
			glDrawElementsBaseVertex(primitiveGL, nIndex, indexType, (void*)indexStart, baseVertex);
		}
		else
		{
			glDrawElements(primitiveGL, nIndex, indexType, (void*)indexStart);
		}
		//OpenGLCast::To(mLastIndexBuffer)->unbind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLContext::RHIDrawPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset , int numCommand, int commandStride )
	{
		if( !commitInputStream() )
			return;

		commitRenderStates();

		assert(commandBuffer);
		GLenum primitiveGL = commitPrimitiveState(type);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, OpenGLCast::GetHandle(*commandBuffer));

		if( numCommand > 1 )
		{
			glMultiDrawArraysIndirect(primitiveGL, (void*)offset, numCommand, commandStride);
		}
		else
		{
			glDrawArraysIndirect(primitiveGL, (void*)offset);
		}
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}

	void OpenGLContext::RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset , int numCommand, int commandStride)
	{
		if( !mLastIndexBuffer.isValid() )
		{
			return;
		}

		if( !commitInputStream() )
			return;
		
		commitRenderStates();

		OpenGLCast::To(mLastIndexBuffer)->bind();
		assert(commandBuffer);
		GLenum primitiveGL = commitPrimitiveState(type);
		GLenum indexType = mLastIndexBuffer->isIntType() ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, OpenGLCast::GetHandle(*commandBuffer));
		if( numCommand > 1 )
		{
			glMultiDrawElementsIndirect(primitiveGL, indexType, (void*)offset, numCommand, commandStride);
		}
		else
		{
			glDrawElementsIndirect(primitiveGL, indexType, (void*)offset);
		}
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		//OpenGLCast::To(mLastIndexBuffer)->unbind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLContext::RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
	{
		if( !commitInputStream() )
			return;

		commitRenderStates();

		GLenum primitiveGL = commitPrimitiveState(type);
		if( baseInstance )
		{
			glDrawArraysInstancedBaseInstance(primitiveGL, vStart, nv, numInstance, baseInstance);
		}
		else
		{
			glDrawArraysInstanced(primitiveGL, vStart, nv, numInstance);
		}
		
	}

	void OpenGLContext::RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance)
	{
		if( !mLastIndexBuffer.isValid() )
		{
			return;
		}
		if( !commitInputStream() )
			return;

		commitRenderStates();
		
		GLenum primitiveGL = commitPrimitiveState(type);
		OpenGLCast::To(mLastIndexBuffer)->bind();
		GLenum indexType = mLastIndexBuffer->isIntType() ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
		if( baseVertex )
		{
			if( baseInstance )
			{
				glDrawElementsInstancedBaseVertexBaseInstance(primitiveGL, nIndex, indexType, (void*)indexStart, numInstance, baseVertex, baseInstance);
			}
			else
			{
				glDrawElementsInstancedBaseVertex(primitiveGL, nIndex, indexType, (void*)indexStart, numInstance, baseVertex);
			}		
		}
		else
		{
			if( baseInstance )
			{
				glDrawElementsInstancedBaseInstance(primitiveGL, nIndex, indexType, (void*)indexStart, numInstance, baseInstance);
			}
			else
			{
				glDrawElementsInstanced(primitiveGL, nIndex, indexType, (void*)indexStart, numInstance);
			}
			
		}
		//OpenGLCast::To(mLastIndexBuffer)->unbind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLContext::RHIDrawPrimitiveUP(EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numData)
	{
		if( !commitInputStreamUP(dataInfos, numData) )
			return;

		commitRenderStates();
		GLenum primitiveGL = commitPrimitiveState(type);
		glDrawArrays(primitiveGL, 0, numVertex);
	}

	void OpenGLContext::RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVerex, VertexDataInfo dataInfos[], int numVertexData , int const* pIndices, int numIndex)
	{
		if( pIndices == nullptr )
			return;

		if( !commitInputStreamUP(dataInfos, numVertexData) )
			return;

		commitRenderStates();

		GLenum primitiveGL = commitPrimitiveState(type);
		glDrawElements(primitiveGL, numIndex, GL_UNSIGNED_INT, (void*)pIndices);
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
		clearShader(false);

		if( shaderProgram )
		{
			static_cast<OpenGLShaderProgram&>(*shaderProgram).bind();
			resetBindIndex();
			mLastShaderProgram = shaderProgram;
			mbUseShaderPath = true;
		}
		else
		{
			mbUseShaderPath = false;
		}
	}

	void OpenGLContext::RHISetGraphicsShaderBoundState(GraphicShaderBoundState const& state)
	{
		clearShader(true);
		OpenglShaderPipelineState* shaderpipeline = mSystem->getShaderPipeline(state);
		if (shaderpipeline)
		{	
			shaderpipeline->bind();
			resetBindIndex();
			mLastShaderPipeline = shaderpipeline;
			mbUseShaderPath = true;
		}
		else
		{
			mbUseShaderPath = false;
		}
	}


	void OpenGLContext::clearShader(bool bUseShaderPipeline)
	{
		if (!mbUseShaderPath)
			return;

		if (mLastShaderPipeline)
		{
			mLastShaderPipeline->unbind();
			mLastShaderPipeline = nullptr;
		}
		else
		{
			static_cast<OpenGLShaderProgram&>(*mLastShaderProgram).unbind();
			mLastShaderProgram.release();
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

	void OpenGLContext::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim)
	{
		CHECK_PARAMETER(param);
		switch( dim )
		{
		case 1: glUniform1f(param.mLoc, val[0]); break;
		case 2: glUniform2f(param.mLoc, val[0], val[1]); break;
		case 3: glUniform3f(param.mLoc, val[0], val[1], val[2]); break;
		case 4: glUniform4f(param.mLoc, val[0], val[1], val[2], val[3]); break;
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
		setShaderResourceViewInternal( OpenGLCast::GetHandle( shaderProgram ), param, resourceView);
	}

	void OpenGLContext::setShaderResourceView(RHIShader& shader, ShaderParameter const& param, RHIShaderResourceView const& resourceView)
	{
		CHECK_PARAMETER(param);
		setShaderResourceViewInternal(OpenGLCast::GetHandle(shader), param, resourceView);
	}

	void OpenGLContext::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture)
	{
		CHECK_PARAMETER(param);
		setShaderResourceViewInternal(OpenGLCast::GetHandle(shaderProgram), param, *texture.getBaseResourceView());
	}

	void OpenGLContext::setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler)
	{
		CHECK_PARAMETER(param);
		setShaderResourceViewInternal(OpenGLCast::GetHandle(shaderProgram), param, *texture.getBaseResourceView(), sampler);
	}

	void OpenGLContext::setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler)
	{
		CHECK_PARAMETER(param);
		setShaderResourceViewInternal(OpenGLCast::GetHandle(shader), param, *texture.getBaseResourceView(), sampler);
	}

	void OpenGLContext::setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler)
	{
		CHECK_PARAMETER(param);
		setShaderSamplerInternal(OpenGLCast::GetHandle(shaderProgram), param, sampler);
	}

	void OpenGLContext::setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture)
	{
		CHECK_PARAMETER(param);
		setShaderResourceViewInternal(OpenGLCast::GetHandle(shader), param, *texture.getBaseResourceView());
	}

	void OpenGLContext::setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler)
	{
		CHECK_PARAMETER(param);
		setShaderSamplerInternal(OpenGLCast::GetHandle(shader), param, sampler);
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

	void OpenGLContext::setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op)
	{
		CHECK_PARAMETER(param);
		OpenGLShaderResourceView const& resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(*texture.getBaseResourceView());
		int idx = mNextAutoBindSamplerSlotIndex;
		++mNextAutoBindSamplerSlotIndex;
		glBindImageTexture(idx, resourceViewImpl.handle, 0, GL_FALSE, 0, OpenGLTranslate::To(op), OpenGLTranslate::To(texture.getFormat()));
		glProgramUniform1i(OpenGLCast::GetHandle(shader), param.mLoc, idx);
	}

	void OpenGLContext::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		glUniformBlockBinding(OpenGLCast::GetHandle(shaderProgram), param.mLoc, mNextUniformSlot);
		glBindBufferBase(GL_UNIFORM_BUFFER, mNextUniformSlot, OpenGLCast::GetHandle(buffer));
		++mNextUniformSlot;
	}

	void OpenGLContext::setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		glUniformBlockBinding(OpenGLCast::GetHandle(shader), param.mLoc, mNextUniformSlot);
		glBindBufferBase(GL_UNIFORM_BUFFER, mNextUniformSlot, OpenGLCast::GetHandle(buffer));
		++mNextUniformSlot;
	}

	void OpenGLContext::setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		glShaderStorageBlockBinding(OpenGLCast::GetHandle(shaderProgram), param.mLoc, mNextStorageSlot);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mNextStorageSlot, OpenGLCast::GetHandle(buffer));
		++mNextStorageSlot;
	}

	void OpenGLContext::setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		glShaderStorageBlockBinding(OpenGLCast::GetHandle(shader), param.mLoc, mNextStorageSlot);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mNextStorageSlot, OpenGLCast::GetHandle(buffer));
		++mNextStorageSlot;
	}

	void OpenGLContext::setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, param.mLoc, OpenGLCast::GetHandle(buffer));
	}

	void OpenGLContext::setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, param.mLoc, OpenGLCast::GetHandle(buffer));
	}

	void OpenGLContext::setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		switch (dim)
		{
		case 1: glProgramUniform1i(shaderImpl.getHandle(), param.mLoc, val[0]); break;
		case 2: glProgramUniform2i(shaderImpl.getHandle(), param.mLoc, val[0], val[1]); break;
		case 3: glProgramUniform3i(shaderImpl.getHandle(), param.mLoc, val[0], val[1], val[2]); break;
		case 4: glProgramUniform4i(shaderImpl.getHandle(), param.mLoc, val[0], val[1], val[2], val[3]); break;
		}
	}

	void OpenGLContext::setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		switch (dim)
		{
		case 1: glProgramUniform1f(shaderImpl.getHandle(), param.mLoc, val[0]); break;
		case 2: glProgramUniform2f(shaderImpl.getHandle(), param.mLoc, val[0], val[1]); break;
		case 3: glProgramUniform3f(shaderImpl.getHandle(), param.mLoc, val[0], val[1], val[2]); break;
		case 4: glProgramUniform4f(shaderImpl.getHandle(), param.mLoc, val[0], val[1], val[2], val[3]); break;
		}
	}

	void OpenGLContext::setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		glProgramUniformMatrix3fv(shaderImpl.getHandle(), param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		glProgramUniformMatrix4fv(shaderImpl.getHandle(), param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		glProgramUniform3fv(shaderImpl.getHandle(), param.mLoc, dim, (float const *)val);
	}

	void OpenGLContext::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		glProgramUniform4fv(shaderImpl.getHandle(), param.mLoc, dim, (float const *)val);
	}

	void OpenGLContext::setShaderMatrix22(RHIShader& shader, ShaderParameter const& param, float const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		glProgramUniformMatrix2fv(shaderImpl.getHandle(), param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderMatrix43(RHIShader& shader, ShaderParameter const& param, float const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		glProgramUniformMatrix4x3fv(shaderImpl.getHandle(), param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::setShaderMatrix34(RHIShader& shader, ShaderParameter const& param, float const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		glProgramUniformMatrix3x4fv(shaderImpl.getHandle(), param.mLoc, dim, false, (float const *)val);
	}

	void OpenGLContext::commitRenderStates()
	{
#if COMMIT_STATE_IMMEDIATELY == 0
		commitRasterizerState();
		commitDepthStencilState();
		commitBlendState();
#endif
		commitSamplerStates();
	}

	void OpenGLContext::commitSamplerStates()
	{
		if (mSimplerSlotDirtyMask)
		{
			for (int index = 0; index < mNextAutoBindSamplerSlotIndex; ++index)
			{
				if (mSimplerSlotDirtyMask & BIT(index))
				{
					SamplerState& state = mSamplerStates[index];
					if (state.bWrite)
					{


					}
					else
					{
						glActiveTexture(GL_TEXTURE0 + index);
						glBindTexture(state.typeEnum, state.textureHandle);
						glBindSampler(index, state.samplerHandle);
					}
					glProgramUniform1i(state.program, state.loc, index);
				}
			}

			mSimplerSlotDirtyMask = 0;
			glActiveTexture(GL_TEXTURE0);
		}
	}

	void OpenGLContext::commitRasterizerState()
	{
		if (!IsStateDirty(mDeviceState.rasterizerStateCommitted, mDeviceState.rasterizerStatePending))
			return;

		OpenGLRasterizerState& rasterizerStateGL = static_cast<OpenGLRasterizerState&>(*mDeviceState.rasterizerStatePending);
		auto& committedValue = mDeviceState.rasterizerStateValue;
		auto const& pendingValue = rasterizerStateGL.mStateValue;

		if (GL_CHECK_STATE_DIRTY(fillMode))
		{
			glPolygonMode(GL_FRONT_AND_BACK, pendingValue.fillMode);
		}

		if (GL_CHECK_STATE_DIRTY(bEnableScissor))
		{
			EnableGLState(GL_SCISSOR_TEST, pendingValue.bEnableScissor);
		}

		if (GL_CHECK_STATE_DIRTY(bEnableMultisample))
		{
			EnableGLState(GL_MULTISAMPLE, pendingValue.bEnableMultisample);
		}

		if (GL_CHECK_STATE_DIRTY(bEnableCull))
		{
			EnableGLState(GL_CULL_FACE, pendingValue.bEnableCull);

			if (GL_CHECK_STATE_DIRTY(cullFace))
			{
				glCullFace(pendingValue.cullFace);
			}
		}

		if (GL_CHECK_STATE_DIRTY(bEnableCull))
		{
			EnableGLState(GL_CULL_FACE, pendingValue.bEnableCull);
		}
	}

	void OpenGLContext::commitBlendState()
	{
		if (!IsStateDirty(mDeviceState.blendStateCommitted, mDeviceState.blendStatePending))
			return;

		OpenGLBlendState& BlendStateGL = static_cast<OpenGLBlendState&>(*mDeviceState.blendStatePending);
		bool bForceAllReset = false;
		{
			auto& committedValue = mDeviceState.blendStateValue;
			auto const& pendingValue = BlendStateGL.mStateValue;
			if (GL_CHECK_STATE_DIRTY(bEnableAlphaToCoverage))
			{
				EnableGLState(GL_SAMPLE_ALPHA_TO_COVERAGE, pendingValue.bEnableAlphaToCoverage);
			}
			if (GL_CHECK_STATE_DIRTY(bEnableIndependent))
			{
				bForceAllReset = true;
			}
		}

		if (BlendStateGL.mStateValue.bEnableIndependent)
		{
			for (int i = 0; i < MaxBlendStateTargetCount; ++i)
			{
				auto& committedValue = mDeviceState.blendStateValue.targetValues[i];
				auto const& pendingValue = BlendStateGL.mStateValue.targetValues[i];

				bool bForceReset = false;

				if (bForceAllReset || GL_TEST_STATE_DIRTY(writeMask))
				{
					bForceReset = true;
					GL_STATE_VAR_ASSIGN(writeMask);
					glColorMaski(i, pendingValue.writeMask & CWM_R,
						pendingValue.writeMask & CWM_G,
						pendingValue.writeMask & CWM_B,
						pendingValue.writeMask & CWM_A);
				}

				if (bForceAllReset || pendingValue.writeMask)
				{

					if (bForceAllReset || GL_TEST_STATE_DIRTY(bEnable))
					{
						GL_STATE_VAR_ASSIGN(bEnable);
						EnableGLStateIndex(GL_BLEND, i, pendingValue.bEnable);
					}

					if (pendingValue.bEnable)
					{
						if (bForceReset || GL_TEST_STATE_DIRTY(bSeparateBlend) || GL_TEST_STATE_DIRTY(srcColor) || GL_TEST_STATE_DIRTY(destColor) || GL_TEST_STATE_DIRTY(srcAlpha) || GL_TEST_STATE_DIRTY(destAlpha))
						{
							GL_STATE_VAR_ASSIGN(bSeparateBlend);
							GL_STATE_VAR_ASSIGN(srcColor);
							GL_STATE_VAR_ASSIGN(destColor);
							GL_STATE_VAR_ASSIGN(srcAlpha);
							GL_STATE_VAR_ASSIGN(destAlpha);

							if (pendingValue.bSeparateBlend)
							{

								glBlendFuncSeparatei(i, pendingValue.srcColor, pendingValue.destColor,
									pendingValue.srcAlpha, pendingValue.destAlpha);
							}
							else
							{
								glBlendFunci(i, pendingValue.srcColor, pendingValue.destColor);
							}
						}
					}
				}
			}
		}
		else
		{
			auto& committedValue = mDeviceState.blendStateValue.targetValues[0];
			auto const& pendingValue = BlendStateGL.mStateValue.targetValues[0];

			bool bForceReset = false;

			if (bForceAllReset || GL_TEST_STATE_DIRTY(writeMask))
			{
				bForceReset = true;
				GL_STATE_VAR_ASSIGN(writeMask);
				glColorMask(
					pendingValue.writeMask & CWM_R,
					pendingValue.writeMask & CWM_G,
					pendingValue.writeMask & CWM_B,
					pendingValue.writeMask & CWM_A);
			}

			if (bForceAllReset || pendingValue.writeMask)
			{

				if (bForceAllReset || GL_TEST_STATE_DIRTY(bEnable))
				{
					GL_STATE_VAR_ASSIGN(bEnable);
					EnableGLState(GL_BLEND, pendingValue.bEnable);
				}

				if (pendingValue.bEnable)
				{
					if (bForceReset || GL_TEST_STATE_DIRTY(bSeparateBlend) || GL_TEST_STATE_DIRTY(srcColor) || GL_TEST_STATE_DIRTY(destColor) || GL_TEST_STATE_DIRTY(srcAlpha) || GL_TEST_STATE_DIRTY(destAlpha))
					{
						GL_STATE_VAR_ASSIGN(bSeparateBlend);
						GL_STATE_VAR_ASSIGN(srcColor);
						GL_STATE_VAR_ASSIGN(destColor);
						GL_STATE_VAR_ASSIGN(srcAlpha);
						GL_STATE_VAR_ASSIGN(destAlpha);

						if (pendingValue.bSeparateBlend)
						{
							glBlendFuncSeparate(pendingValue.srcColor, pendingValue.destColor,
								pendingValue.srcAlpha, pendingValue.destAlpha);
						}
						else
						{
							glBlendFunc(pendingValue.srcColor, pendingValue.destColor);
						}
					}
				}
			}
		}
	}

	void OpenGLContext::commitDepthStencilState()
	{
		auto& committedValue = mDeviceState.depthStencilStateValue;
		if ( !IsStateDirty(mDeviceState.depthStencilStateCommitted , mDeviceState.depthStencilStatePending) )
		{
			if ( IsStateDirty( committedValue.stencilRef , mDeviceState.stencilRefPending ) )
			{
				glStencilFunc(committedValue.stencilFun, mDeviceState.stencilRefPending, committedValue.stencilReadMask);
			}
			return;
		}

		OpenGLDepthStencilState& depthStencilStateGL = static_cast<OpenGLDepthStencilState&>(*mDeviceState.depthStencilStatePending);

		auto const& pendingValue = depthStencilStateGL.mStateValue;

		if (GL_CHECK_STATE_DIRTY(bEnableDepthTest))
		{
			EnableGLState(GL_DEPTH_TEST, pendingValue.bEnableDepthTest);
		}

		if (pendingValue.bEnableDepthTest)
		{
			if (GL_CHECK_STATE_DIRTY(bWriteDepth))
			{
				glDepthMask(pendingValue.bWriteDepth);
			}
			if (GL_CHECK_STATE_DIRTY(depthFun))
			{
				glDepthFunc(pendingValue.depthFun);
			}
		}
		else
		{
			//#TODO : Check State Value
		}

		if (GL_CHECK_STATE_DIRTY(bEnableStencilTest))
		{
			EnableGLState(GL_STENCIL_TEST, pendingValue.bEnableStencilTest);
		}

		if (GL_CHECK_STATE_DIRTY(stencilWriteMask))
		{
			glStencilMask(pendingValue.stencilWriteMask);
		}

		if (pendingValue.bEnableStencilTest)
		{

			bool bForceRestOp = false;
			if (GL_CHECK_STATE_DIRTY(bUseSeparateStencilOp))
			{
				bForceRestOp = true;
			}

			bool bForceRestFun = false;
			if (GL_CHECK_STATE_DIRTY(bUseSeparateStencilFun))
			{
				bForceRestFun = true;
			}

			if (pendingValue.bUseSeparateStencilOp)
			{
				if (bForceRestOp || GL_TEST_STATE_DIRTY(stencilFailOp) || GL_TEST_STATE_DIRTY(stencilZFailOp) || GL_TEST_STATE_DIRTY(stencilZPassOp) || GL_TEST_STATE_DIRTY(stencilFailOpBack) || GL_TEST_STATE_DIRTY(stencilZFailOpBack) || GL_TEST_STATE_DIRTY(stencilZPassOpBack))
				{
					GL_STATE_VAR_ASSIGN(stencilFailOp);
					GL_STATE_VAR_ASSIGN(stencilZFailOp);
					GL_STATE_VAR_ASSIGN(stencilZPassOp);
					GL_STATE_VAR_ASSIGN(stencilFailOpBack);
					GL_STATE_VAR_ASSIGN(stencilZFailOpBack);
					GL_STATE_VAR_ASSIGN(stencilZPassOpBack);
					glStencilOpSeparate(GL_FRONT, pendingValue.stencilFailOp, pendingValue.stencilZFailOp, pendingValue.stencilZPassOp);
					glStencilOpSeparate(GL_BACK, pendingValue.stencilFailOpBack, pendingValue.stencilZFailOpBack, pendingValue.stencilZPassOpBack);
				}
			}
			else
			{
				if (bForceRestOp || GL_TEST_STATE_DIRTY(stencilFailOp) || GL_TEST_STATE_DIRTY(stencilZFailOp) || GL_TEST_STATE_DIRTY(stencilZPassOp))
				{
					GL_STATE_VAR_ASSIGN(stencilFailOp);
					GL_STATE_VAR_ASSIGN(stencilZFailOp);
					GL_STATE_VAR_ASSIGN(stencilZPassOp);
					glStencilOp(pendingValue.stencilFailOp, pendingValue.stencilZFailOp, pendingValue.stencilZPassOp);
				}
			}
			if (pendingValue.bUseSeparateStencilFun)
			{
				if (bForceRestFun || GL_TEST_STATE_DIRTY(stencilFun) || GL_TEST_STATE_DIRTY(stencilFunBack) || committedValue.stencilRef != mDeviceState.stencilRefPending || GL_TEST_STATE_DIRTY(stencilReadMask))
				{
					GL_STATE_VAR_ASSIGN(stencilFun);
					GL_STATE_VAR_ASSIGN(stencilFunBack);
					GL_STATE_VAR_ASSIGN(stencilReadMask);
					committedValue.stencilRef = mDeviceState.stencilRefPending;
					glStencilFuncSeparate(GL_FRONT, pendingValue.stencilFun, mDeviceState.stencilRefPending, pendingValue.stencilReadMask);
					glStencilFuncSeparate(GL_BACK, pendingValue.stencilFunBack, mDeviceState.stencilRefPending, pendingValue.stencilReadMask);
				}
			}
			else
			{
				if (bForceRestFun || GL_TEST_STATE_DIRTY(stencilFun) || committedValue.stencilRef != mDeviceState.stencilRefPending || GL_TEST_STATE_DIRTY(stencilReadMask))
				{
					GL_STATE_VAR_ASSIGN(stencilFun);
					GL_STATE_VAR_ASSIGN(stencilReadMask);
					committedValue.stencilRef = mDeviceState.stencilRefPending;
					glStencilFunc(pendingValue.stencilFun, mDeviceState.stencilRefPending, pendingValue.stencilReadMask);
				}
			}
		}
		else
		{


		}
	}

#define USE_VAO 1
	bool OpenGLContext::commitInputStream()
	{
		if (checkInputStreamStateDirty())
		{
			if (mInputLayoutCommitted.isValid())
			{
				mWasBindAttrib = mbUseShaderPath;
				auto& inputLayoutImpl = OpenGLCast::To(*mInputLayoutCommitted);
				if (mbUseShaderPath)
				{
#if USE_VAO
					mVAOCommitted = inputLayoutImpl.bindVertexArray(mInputStreamStateCommttied);
#else
					inputLayoutImpl.bindAttrib(mInputStreamStateCommttied);
#endif
				}
				else
				{
#if USE_VAO
					mVAOCommitted = inputLayoutImpl.bindVertexArray(mInputStreamStateCommttied, true);
#else
					inputLayoutImpl.bindPointer(mInputStreamStateCommttied);
#endif
				}
			}
		}
		return true;
	}

	bool OpenGLContext::commitInputStreamUP(VertexDataInfo dataInfos[], int numData)
	{
		if (!mInputLayoutPending.isValid())
			return false;

		mInputStreamCountPending = numData;

		bool bForceDirty = false;
		for (int i = 0; i < numData; ++i)
		{
			mInputStreamsPending[i].buffer = nullptr;
			mInputStreamsPending[i].offset = (intptr_t)dataInfos[i].ptr;
			mInputStreamsPending[i].stride = dataInfos[i].stride;
			if (dataInfos[i].stride == 0)
			{
				//we need reset pointer data. e.g. glVertex4fv ..
				bForceDirty = true;
			}
		}
		mbHasInputStreamPendingSetted = true;

		if ( checkInputStreamStateDirty(bForceDirty) )
		{
			auto& inputLayoutImpl = OpenGLCast::To(*mInputLayoutCommitted);
			mWasBindAttrib = mbUseShaderPath;
			if (mbUseShaderPath)
			{
				inputLayoutImpl.bindAttribUP(mInputStreamStateCommttied);
			}
			else
			{
				inputLayoutImpl.bindPointerUP(mInputStreamStateCommttied);
			}
		}
		return true;
	}

	bool OpenGLContext::checkInputStreamStateDirty(bool bForceDirty)
	{
		bool bDirty = bForceDirty || mInputLayoutPending != mInputLayoutCommitted || mbUseShaderPath != mWasBindAttrib;

		int inputStreamCountCommitted = mInputStreamStateCommttied.inputStreamCount;
		if (mbHasInputStreamPendingSetted)
		{
			bDirty |= mInputStreamStateCommttied.update(mInputStreamsPending, mInputStreamCountPending);
		}

		if (bDirty)
		{
			if (mInputLayoutCommitted.isValid())
			{
				if (mVAOCommitted)
				{
					glBindVertexArray(0);
					mVAOCommitted = 0;
				}
				else
				{
					if (mWasBindAttrib)
					{
						OpenGLCast::To(mInputLayoutCommitted)->unbindAttrib(inputStreamCountCommitted);
					}
					else
					{
						OpenGLCast::To(mInputLayoutCommitted)->unbindPointer();
					}
				}
			}

			mInputLayoutCommitted = mInputLayoutPending;
		}

		return bDirty;
	}

	void OpenGLContext::setShaderResourceViewInternal(GLuint handle, ShaderParameter const& param, RHIShaderResourceView const& resourceView)
	{
		OpenGLShaderResourceView const&  resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(resourceView);
		int index = mNextAutoBindSamplerSlotIndex++;
		SamplerState& state = mSamplerStates[index];
		mSimplerSlotDirtyMask |= BIT(index);

		state.loc = param.mLoc;
		state.textureHandle = resourceViewImpl.handle;
		state.typeEnum = resourceViewImpl.typeEnum;
		state.samplerHandle = 0;
		state.program = handle;
		state.bWrite = false;
	}

	void OpenGLContext::setShaderResourceViewInternal(GLuint handle, ShaderParameter const& param, RHIShaderResourceView const& resourceView, RHISamplerState const& sampler)
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
		state.program = handle;
		state.bWrite = false;
	}

	void OpenGLContext::setShaderSamplerInternal(GLuint handle, ShaderParameter const& param, RHISamplerState& sampler)
	{
		OpenGLSamplerState const&  samplerImpl = static_cast<OpenGLSamplerState const&>(sampler);
		for (int index = 0; index < mNextAutoBindSamplerSlotIndex; ++index)
		{
			if (mSamplerStates[index].loc = param.mLoc)
			{
				mSamplerStates[index].samplerHandle = handle;

				mSimplerSlotDirtyMask |= BIT(index);
				break;
			}
		}
	}

	bool OpenglShaderPipelineState::create(GraphicShaderBoundState const& state)
	{
		if (!mGLObject.fetchHandle())
			return false;

		auto CheckShader = [&](RHIShader* shader, GLbitfield stageBit)
		{
			if (shader)
			{
				glUseProgramStages(getHandle(), stageBit, static_cast<OpenGLShader*>(shader)->getHandle());
			}
		};
		CheckShader(state.vertexShader, GL_VERTEX_SHADER_BIT);
		CheckShader(state.pixelShader, GL_FRAGMENT_SHADER_BIT);
		CheckShader(state.geometryShader, GL_GEOMETRY_SHADER_BIT);
		CheckShader(state.hullShader, GL_TESS_CONTROL_SHADER_BIT);
		CheckShader(state.domainShader, GL_TESS_EVALUATION_SHADER_BIT);

		if (!VerifyOpenGLStatus())
			return false;

		//glValidateProgramPipeline(getHandle());

		return true;
	}

}

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif