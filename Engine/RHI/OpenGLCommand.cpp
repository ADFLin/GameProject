#include "OpenGLCommand.h"

#include "OpenGLCommon.h"
#include "OpenGLShaderCommon.h"

#include "RHI/ShaderCore.h"
#include "RHI/GpuProfiler.h"

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
#include "GLExtensions.h"
#include "GL/wglew.h"

#include "ConsoleSystem.h"
#include "BitUtility.h"

#define COMMIT_STATE_IMMEDIATELY 1

namespace Render
{
	EXPORT_RHI_SYSTEM_MODULE(RHISystemName::OpenGL, OpenGLSystem);

	bool GForceInitState = false;
	bool GbOpenglOutputDebugMessage = true;

	TConsoleVariable<bool> CVarOpenGLFixedPiplineUseShader{ true , "r.OpenGLFixedPiplineUseShader", CVF_TOGGLEABLE };


	template < class TFunc >
	bool GetGLExtenionFunc(TFunc& func, char const* funcName)
	{
		func = (TFunc) wglGetProcAddress(funcName);
		return func != nullptr;
	}

	void APIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,  GLvoid const* userParam)
	{
		if (!GbOpenglOutputDebugMessage)
			return;
#if 1
		if( type == GL_DEBUG_TYPE_OTHER )
			return;
#endif

		LogWarning(0, "Opengl Debug : %s", message);
	}

	template< class T >
	bool CheckStateDirty(T& committedValue, T const& pandingValue)
	{
		if (committedValue == pandingValue)
			return GForceInitState;

		committedValue = pandingValue;
		return true;
	}

	template< class T >
	bool IsStateDirty(T const& committedValue, T const& pandingValue)
	{
		return GForceInitState || committedValue != pandingValue;
	}

#define GL_CHECK_STATE_DIRTY( NAME )  CheckStateDirty( committedValue.##NAME , pendingValue.##NAME )
#define GL_IS_STATE_DIRTY( NAME )     IsStateDirty( committedValue.##NAME , pendingValue.##NAME )
#define GL_COMMIT_STATE_VALUE( NAME )   committedValue.##NAME = pendingValue.##NAME 

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

	template< class T, typename P0 , class ...Args >
	T* CreateOpenGLResource1T( P0&& p0 , Args&& ...args)
	{
		T* result = new T(p0);
		if (result && !result->create(std::forward< Args >(args)...))
		{
			delete result;
			return nullptr;
		}
		return result;
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

	class OpenGLProfileCore : public RHIProfileCore
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
		TArray< OpenGLTiming > mTimingStorage;

	};

	bool OpenGLSystem::initialize(RHISystemInitParams const& initParam)
	{

		WGLPixelFormat format;
		format.numSamples = initParam.numSamples;
		if( !mGLContext.init(initParam.hDC, format) )
			return false;

		if( glewInit() != GLEW_OK )
			return false;

		char const* versionStr = (char const*) glGetString(GL_VERSION);
		LogMsg("Opengl Version = %s", versionStr);

		char const* vendorStr = (char const*) glGetString(GL_VENDOR);

		if( FCString::StrStr(vendorStr, "NVIDIA") != nullptr )
		{
			GRHIDeviceVendorName = DeviceVendorName::NVIDIA;
		}
		else if(FCString::StrStr(vendorStr, "ATI") != nullptr )
		{
			GRHIDeviceVendorName = DeviceVendorName::ATI;
		}
		else if(FCString::StrStr(vendorStr, "Intel") != nullptr )
		{
			GRHIDeviceVendorName = DeviceVendorName::Intel;
		}
		else
		{
			GRHIDeviceVendorName = DeviceVendorName::Unknown;
		}

		if (!initParam.bVSyncEnable)
		{
			wglSwapIntervalEXT(0);
		}


		if(initParam.bDebugMode)
		{
			glDebugMessageCallback(GLDebugCallback, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}

		glEnable(GL_TEXTURE_3D);
		glEnable(GL_PROGRAM_POINT_SIZE);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		GRHIClipZMin = -1;
		GRHIProjectionYSign = 1;
		GRHIViewportOrgToNDCPosY = -1;

		if (GRHIDeviceVendorName == DeviceVendorName::NVIDIA)
		{
			auto const* extensions = (char const*)glGetString(GL_EXTENSIONS);

#define GET_GLFUNC( NAME ) GetGLExtenionFunc(NAME, #NAME) 
			if ( FCString::StrStr(extensions, "GL_NV_mesh_shader") )
			{
				if (GET_GLFUNC(glDrawMeshTasksNV)&&
					GET_GLFUNC(glDrawMeshTasksIndirectNV) &&
					GET_GLFUNC(glMultiDrawMeshTasksIndirectNV) &&
					GET_GLFUNC(glMultiDrawMeshTasksIndirectCountNV))
				{
					GRHISupportMeshShader = true;
				}
			}
			if ( FCString::StrStr(extensions, "GL_ARB_shader_viewport_layer_array"))
			{
				GRHISupportVPAndRTArrayIndexFromAnyShaderFeedingRasterizer = true;
			}
#undef GET_GLFUNC
		}

		if( 1 )
		{
			mProfileCore = new OpenGLProfileCore;
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
			TGuardValue<bool> scopedValue(GForceInitState, true);
			mDrawContext.RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI(), 0xff);
			mDrawContext.RHISetBlendState(TStaticBlendState<>::GetRHI());
			mDrawContext.RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
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

	RHITexture1D* OpenGLSystem::RHICreateTexture1D(TextureDesc const& desc, void* data)
	{
		return CreateOpenGLResource1T< OpenGLTexture1D >(desc, data);
	}

	RHITexture2D* OpenGLSystem::RHICreateTexture2D(TextureDesc const& desc, void* data, int dataAlign)
	{
		if (desc.creationFlags & TCF_RenderBufferOptimize)
		{
			return CreateOpenGLResourceT<OpenGLRenderBuffer>();
		}
		else if (ETexture::IsDepthStencil(desc.format))
		{
			OpenGLTexture2D* result = new OpenGLTexture2D(desc);
			if (result && !result->createDepth())
			{
				delete result;
				return nullptr;
			}
			return result;
		}
		return CreateOpenGLResource1T< OpenGLTexture2D >(desc, data, dataAlign);
	}

	RHITexture3D* OpenGLSystem::RHICreateTexture3D(TextureDesc const& desc, void* data)
	{
		return CreateOpenGLResource1T< OpenGLTexture3D >(desc, data);
	}

	RHITextureCube* OpenGLSystem::RHICreateTextureCube(TextureDesc const& desc, void* data[])
	{
		return CreateOpenGLResource1T< OpenGLTextureCube >(desc, data );
	}

	RHITexture2DArray* OpenGLSystem::RHICreateTexture2DArray(TextureDesc const& desc, void* data)
	{
		return CreateOpenGLResource1T< OpenGLTexture2DArray >(desc, data);
	}

	RHIBuffer* OpenGLSystem::RHICreateBuffer(BufferDesc const& desc, void* data)
	{
		return CreateOpenGLResourceT< OpenGLBuffer >(desc, data);
	}

	void* OpenGLSystem::RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		if( size )
			return OpenGLCast::To(buffer)->lock(access, offset, size);
		return OpenGLCast::To(buffer)->lock(access);
	}

	void OpenGLSystem::RHIUnlockBuffer(RHIBuffer* buffer)
	{
		OpenGLCast::To(buffer)->unlock();
	}

	void OpenGLSystem::RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
	{
		auto GetFormatClientSize = [](ETexture::Format format) -> int
		{
			int formatSize = ETexture::GetFormatSize(format);
			if (ETexture::GetComponentType(format) == CVT_Half)
			{
				formatSize *= 2;
			}
			return formatSize;
		};
		int formatSize = GetFormatClientSize(format);
		int dataSize = Math::Max(texture.getSizeX() >> level, 1) * Math::Max(texture.getSizeY() >> level, 1) * formatSize;
		outData.resize(dataSize);

		OpenGLCast::To(&texture)->bind();
		glGetTexImage(GL_TEXTURE_2D, level, OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), &outData[0]);
		OpenGLCast::To(&texture)->unbind();
	}

	void OpenGLSystem::RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
	{
		auto GetFormatClientSize = [](ETexture::Format format)
		{
			int formatSize = ETexture::GetFormatSize(format);
			if (ETexture::GetComponentType(format) == CVT_Half)
			{
				formatSize *= 2;
			}
			return formatSize;
		};

		int formatSize = GetFormatClientSize(format);
		int textureSize = Math::Max(texture.getSize() >> level, 1);
		int faceDataSize = textureSize * textureSize * formatSize;
		outData.resize(ETexture::FaceCount * faceDataSize);

		OpenGLCast::To(&texture)->bind();
		for (int face = 0; face < ETexture::FaceCount; ++face)
		{
			glGetTexImage(OpenGLTranslate::TexureType(ETexture::Face(face)), level, OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), &outData[faceDataSize*face]);
		}
		OpenGLCast::To(&texture)->unbind();
	}

	bool  UpdateTexture2D(GLenum textureEnum, int ox, int oy, int w, int h, ETexture::Format format, void* data, int level)
	{
		glTexSubImage2D(textureEnum, level, ox, oy, w, h, OpenGLTranslate::PixelFormat(format), OpenGLTranslate::TextureComponentType(format), data);
		bool result = VerifyOpenGLStatus();
		return result;
	}

	bool  UpdateTexture2D(GLenum textureEnum, int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level)
	{
#if 1
		::glPixelStorei(GL_UNPACK_ROW_LENGTH, dataImageWidth);
		glTexSubImage2D(textureEnum, level, ox, oy, w, h, OpenGLTranslate::PixelFormat(format), OpenGLTranslate::TextureComponentType(format), data);
		bool result = VerifyOpenGLStatus();
		::glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#else
		GLenum formatGL = OpenGLTranslate::PixelFormat(format);
		GLenum typeGL = OpenGLTranslate::TextureComponentType(format);
		uint8* pData = (uint8*)data;
		int dataStride = dataImageWidth * ETexture::GetFormatSize(format);
		for (int dy = 0; dy < h; ++dy)
		{
			glTexSubImage2D(textureEnum, level, ox, oy + dy, w, 1, formatGL, typeGL, pData);
			pData += dataStride;
		}
		bool result = VerifyOpenGLStatus();
#endif
		return result;
	}

	bool OpenGLSystem::RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth)
	{
		if (texture.getDesc().numSamples > 1)
			return false;

		OpenGLCast::To(&texture)->bind();
		bool result;
		if (dataWidth)
		{
			result = UpdateTexture2D(OpenGLTextureTraits< RHITexture2D >::EnumValue, ox, oy, w, h, texture.getDesc().format, dataWidth, data, level);
		}
		else
		{
			result = UpdateTexture2D(OpenGLTextureTraits< RHITexture2D >::EnumValue, ox, oy, w, h, texture.getDesc().format, data, level);
		}
		OpenGLCast::To(&texture)->unbind();
	}

	void OpenGLSystem::RHIUpdateBuffer(RHIBuffer& buffer, int start, int numElements, void* data)
	{
		OpenGLCast::To(buffer).updateData(start, numElements, data);
	}

	//void* OpenGLSystem::RHILockTexture(RHITextureBase* texture, ELockAccess access, uint32 offset, uint32 size)
	//{
	//	switch (texture->getType())
	//	{
	//	case ETexture::Type1D:
	//	case ETexture::Type2D:
	//	}
	//	return nullptr;
	//}

	//void OpenGLSystem::RHIUnlockTexture(RHITextureBase* texture)
	//{

	//}

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

	template< class TShaderBoundState >
	OpenGLShaderBoundState* OpenGLSystem::getShaderBoundStateT(TShaderBoundState const& state)
	{
		ShaderBoundStateKey key;
		key.initialize(state);
		if (!key.isValid())
			return nullptr;

		auto iter = mGfxBoundStateMap.find(key);
		if (iter != mGfxBoundStateMap.end())
		{
			return &iter->second;
		}

		OpenGLShaderBoundState boundState;
		if (!boundState.create(state))
			return nullptr;

		auto pair = mGfxBoundStateMap.emplace(key, std::move(boundState));
		CHECK(pair.second);
		return &pair.first->second;
	}

	OpenGLShaderBoundState* OpenGLSystem::getShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		return getShaderBoundStateT(stateDesc);
	}

	OpenGLShaderBoundState* OpenGLSystem::getShaderBoundState(MeshShaderStateDesc const& stateDesc)
	{
		return getShaderBoundStateT(stateDesc);
	}

	void OpenGLContext::initialize()
	{
		if (GRHISupportMeshShader)
		{
			glGetIntegerv(GL_MAX_DRAW_MESH_TASKS_COUNT_NV, &mMaxDrawMeshTasksCount);
			LogMsg("MaxDrawMeshTasksCount = %d", mMaxDrawMeshTasksCount);

			GLint MaxMeshOutVertexCount = 0;
			glGetIntegerv(GL_MAX_MESH_OUTPUT_VERTICES_NV, &MaxMeshOutVertexCount);
			LogMsg("MaxMeshOutVertexCount = %d", MaxMeshOutVertexCount);

			GLint MaxMeshOutPrimitiveCount = 0;
			glGetIntegerv(GL_MAX_MESH_OUTPUT_PRIMITIVES_NV, &MaxMeshOutPrimitiveCount);
			LogMsg("MaxMeshOutPrimitiveCount = %d", MaxMeshOutPrimitiveCount);
		}

		for (int i = 0; i < ARRAY_SIZE(mResolveFrameBuffers); ++i)
		{
			mResolveFrameBuffers[i].fetchHandle();
		}
	}

	void OpenGLContext::shutdown()
	{
		mLastFrameBuffer.release();
		mLastComputeShader.release();
		mLastShaderProgram.release();
		mLastIndexBuffer.release();
		mInputLayoutPending.release();
		mInputLayoutCommitted.release();
		mInputStreamStateCommttied.release();
		for(auto& inputStream : mInputStreamsPending)
		{
			inputStream.buffer.release();
		}

		for (int i = 0; i < ARRAY_SIZE(mResolveFrameBuffers); ++i)
		{
			mResolveFrameBuffers[i].destroyHandle();
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

	void OpenGLContext::RHISetViewport(ViewportInfo const& viewport)
	{
		glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
		glDepthRange(viewport.zNear, viewport.zFar);
	}

	void OpenGLContext::RHISetViewports(ViewportInfo const viewports[], int numViewports)
	{
		for (int i = 0; i < numViewports; ++i)
		{
			ViewportInfo const& vp = viewports[i];
			glViewportIndexedf(i, vp.x, vp.y, vp.w, vp.h);
			glDepthRangeIndexed(i, vp.zNear, vp.zFar);
		}
	}

	void OpenGLContext::RHISetScissorRect(int x, int y, int w, int h)
	{
		glScissor(x, y, w, h);
	}

	void OpenGLContext::RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler)
	{
		if (CVarOpenGLFixedPiplineUseShader)
		{
			mFixedShaderParams.transform = transform;
			mFixedShaderParams.color = color;
			mFixedShaderParams.texture = texture;
			mFixedShaderParams.sampler = sampler;

			mbUseShaderPath = false;
		}
		else
		{
			RHISetShaderProgram(nullptr);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(transform);

			glColor4fv(color);
			if (texture)
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
	}

	void OpenGLContext::RHISetFrameBuffer(RHIFrameBuffer* frameBuffer)
	{
		if (mLastFrameBuffer.isValid())
		{
			OpenGLCast::To(mLastFrameBuffer)->unbind();
		}

		mLastFrameBuffer = frameBuffer;

		if (mLastFrameBuffer.isValid())
		{
			EnableGLState(GL_MULTISAMPLE, OpenGLCast::To(mLastFrameBuffer)->haveMutlisample());
			OpenGLCast::To(mLastFrameBuffer)->bind();
		}
		else
		{

		}
	}

	void OpenGLContext::RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil)
	{
		GLbitfield clearBitsGL = 0;
		if (HaveBits(clearBits, EClearBits::Color))
		{
			if (numColor == 1)
			{
				glClearColor(colors[0].r, colors[0].b, colors[0].g, colors[0].a);
				clearBitsGL |= GL_COLOR_BUFFER_BIT;
			}
			else
			{
				for (int i = 0; i < numColor; ++i)
				{
					glClearBufferfv(GL_COLOR, i, colors[i]);
				}
			}

		}
		if (HaveBits(clearBits, EClearBits::Depth))
		{
			glClearDepth(depth);
			clearBitsGL |= GL_DEPTH_BUFFER_BIT;
		}
		if (HaveBits(clearBits, EClearBits::Stencil))
		{
			glClearStencil(stenceil);
			clearBitsGL |= GL_STENCIL_BUFFER_BIT;
		}
		if (clearBitsGL)
		{
			glClear(clearBitsGL);
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

	void OpenGLContext::RHISetIndexBuffer(RHIBuffer* indexBuffer)
	{
		mLastIndexBuffer = indexBuffer;	
	}

	void OpenGLContext::RHIDrawPrimitive(EPrimitive type, int start, int nv)
	{
		if( !commitInputStream() )
			return;

		commitGraphicStates();
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

		commitGraphicStates();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLCast::GetHandle(mLastIndexBuffer));

		GLenum indexType = OpenGLTranslate::IndexType(mLastIndexBuffer);
		int indexTypeSize = mLastIndexBuffer->getElementSize();
		int indexOffset = indexTypeSize * indexStart;

		GLenum primitiveGL = commitPrimitiveState(type);

		if( baseVertex )
		{
			glDrawElementsBaseVertex(primitiveGL, nIndex, indexType, (void*)indexOffset, baseVertex);
		}
		else
		{
			glDrawElements(primitiveGL, nIndex, indexType, (void*)indexOffset);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLContext::RHIDrawPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset , int numCommand, int commandStride )
	{
		if( !commitInputStream() )
			return;

		commitGraphicStates();

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

	void OpenGLContext::RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIBuffer* commandBuffer, int offset , int numCommand, int commandStride)
	{
		if( !mLastIndexBuffer.isValid() )
		{
			return;
		}

		if( !commitInputStream() )
			return;
		
		commitGraphicStates();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLCast::GetHandle(mLastIndexBuffer));
		assert(commandBuffer);
		GLenum primitiveGL = commitPrimitiveState(type);
		GLenum indexType = OpenGLTranslate::IndexType(mLastIndexBuffer);

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
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLContext::RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
	{
		if( !commitInputStream() )
			return;

		commitGraphicStates();

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

		commitGraphicStates();
		
		GLenum primitiveGL = commitPrimitiveState(type);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLCast::GetHandle(mLastIndexBuffer));

		GLenum indexType = OpenGLTranslate::IndexType(mLastIndexBuffer);
		int indexTypeSize = mLastIndexBuffer->getElementSize();
		int indexOffset = indexTypeSize * indexStart;

		if( baseVertex )
		{
			if( baseInstance )
			{
				glDrawElementsInstancedBaseVertexBaseInstance(primitiveGL, nIndex, indexType, (void*)indexOffset, numInstance, baseVertex, baseInstance);
			}
			else
			{
				glDrawElementsInstancedBaseVertex(primitiveGL, nIndex, indexType, (void*)indexOffset, numInstance, baseVertex);
			}		
		}
		else
		{
			if( baseInstance )
			{
				glDrawElementsInstancedBaseInstance(primitiveGL, nIndex, indexType, (void*)indexOffset, numInstance, baseInstance);
			}
			else
			{
				glDrawElementsInstanced(primitiveGL, nIndex, indexType, (void*)indexOffset, numInstance);
			}
			
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLContext::RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numData, uint32 numInstance)
	{
		if( !commitInputStreamUP(dataInfos, numData) )
			return;

		commitGraphicStates();
		GLenum primitiveGL = commitPrimitiveState(type);
		if ( numInstance != 1)
		{
			glDrawArraysInstanced(primitiveGL, 0 , numVertices, numInstance);
		}
		else
		{
			glDrawArrays(primitiveGL, 0, numVertices);
		}	
	}

	void OpenGLContext::RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVerex, VertexDataInfo dataInfos[], int numVertexData , uint32 const* pIndices, int numIndex, uint32 numInstance)
	{
		if( pIndices == nullptr )
			return;

		if( !commitInputStreamUP(dataInfos, numVertexData) )
			return;

		commitGraphicStates();
		GLenum primitiveGL = commitPrimitiveState(type);
		if (numInstance != 1)
		{
			glDrawElementsInstanced(primitiveGL, numIndex, GL_UNSIGNED_INT, (void*)pIndices, numInstance);
		}
		else
		{
			glDrawElements(primitiveGL, numIndex, GL_UNSIGNED_INT, (void*)pIndices);
		}
	}

	void OpenGLContext::RHIDrawMeshTasks(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
	{
		commitGraphicStates();

		uint32 count = numGroupX * numGroupY * numGroupZ;
		GLuint start = 0;
		while (count > mMaxDrawMeshTasksCount)
		{
			glDrawMeshTasksNV(start, mMaxDrawMeshTasksCount);
			start += mMaxDrawMeshTasksCount;
			count -= mMaxDrawMeshTasksCount;
		}
		if (count)
		{
			glDrawMeshTasksNV(start, count);
		}	
	}

	void OpenGLContext::RHIDrawMeshTasksIndirect(RHIBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		commitGraphicStates();

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, OpenGLCast::GetHandle(*commandBuffer));
		if (numCommand > 1)
		{
			glMultiDrawMeshTasksIndirectNV((GLint*)offset, numCommand, commandStride);
		}
		else
		{
			glDrawMeshTasksIndirectNV((GLint*)offset);
		}
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}

	void OpenGLContext::RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
	{
		commitSamplerStates();
		glDispatchCompute(numGroupX, numGroupY, numGroupZ);
	}

	void OpenGLContext::RHIResolveTexture(RHITextureBase& destTexture, int destSubIndex, RHITextureBase& srcTexture, int srcSubIndex)
	{
		if (destTexture.getDesc().type != ETexture::Type2D || srcTexture.getDesc().type != ETexture::Type2D)
			return;

		IntVector3 srcDim = srcTexture.getDesc().dimension;
		IntVector3 dstDim = destTexture.getDesc().dimension;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mResolveFrameBuffers[0].mHandle);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mResolveFrameBuffers[1].mHandle);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D_MULTISAMPLE, static_cast<OpenGLTexture2D&>(srcTexture).getHandle(), srcSubIndex);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, static_cast<OpenGLTexture2D&>(destTexture).getHandle(), destSubIndex);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + 1);
		glBlitFramebuffer(0, 0, srcDim.x, srcDim.y, 0, 0, dstDim.x, dstDim.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
			OpenGLShaderProgram& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(*shaderProgram);

			shaderProgramImpl.bind();
			resetBindIndex();
			mLastShaderProgram = shaderProgram;
			mbUseShaderPath = true;
		}
		else
		{
			mbUseShaderPath = false;
		}
	}

	void OpenGLContext::RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc)
	{
		clearShader(true);
		OpenGLShaderBoundState* shaderpipeline = mSystem->getShaderBoundState(stateDesc);
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

	void OpenGLContext::RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc)
	{
		clearShader(true);
		OpenGLShaderBoundState* shaderpipeline = mSystem->getShaderBoundState(stateDesc);
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

	void OpenGLContext::RHISetComputeShader(RHIShader* shader)
	{
		clearShader(false);

		if (shader)
		{
			static_cast<OpenGLShader&>(*shader).bind();
			resetBindIndex();
			mLastComputeShader = shader;
			mbUseShaderPath = true;
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
		else if (mLastComputeShader)
		{
			static_cast<OpenGLShader&>(*mLastComputeShader).unbind();
			mLastComputeShader.release();
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

	void OpenGLContext::setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector2 const val[], int dim)
	{
		CHECK_PARAMETER(param);
		glUniform2fv(param.mLoc, dim, (float const *)val);
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

	template< class TShaderObject >
	void OpenGLContext::setShaderRWTextureT(TShaderObject& shaderObject, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op)
	{
		CHECK_PARAMETER(param);
		OpenGLShaderResourceView const& resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(*texture.getBaseResourceView());
		GLuint shaderHandle = OpenGLCast::GetHandle(shaderObject);
		int indexSlot = fetchSamplerStateSlot(shaderHandle, param.mLoc);
		glBindImageTexture(indexSlot, resourceViewImpl.handle, level, TRUE, 0, OpenGLTranslate::To(op), OpenGLTranslate::To(texture.getFormat()));
		glUniform1i(param.mLoc, indexSlot);
	}

	template< class TShaderObject >
	void OpenGLContext::setShaderRWSubTextureT(TShaderObject& shaderObject, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
	{
		CHECK_PARAMETER(param);
		OpenGLShaderResourceView const& resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(*texture.getBaseResourceView());
		GLuint shaderHandle = OpenGLCast::GetHandle(shaderObject);
		int indexSlot = fetchSamplerStateSlot(shaderHandle, param.mLoc);
		glBindImageTexture(indexSlot, resourceViewImpl.handle, level, FALSE, subIndex, OpenGLTranslate::To(op), OpenGLTranslate::To(texture.getFormat()));
		glUniform1i(param.mLoc, indexSlot);
	}

	void OpenGLContext::setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op)
	{
		setShaderRWTextureT(shaderProgram, param, texture, level, op);
	}

	void OpenGLContext::setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int level, EAccessOp op)
	{
		setShaderRWTextureT(shader, param, texture, level, op);
	}

	void OpenGLContext::setShaderRWSubTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
	{
		setShaderRWSubTextureT(shaderProgram, param, texture, subIndex, level, op);
	}

	void OpenGLContext::setShaderRWSubTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, int subIndex, int level, EAccessOp op)
	{
		setShaderRWSubTextureT(shader, param, texture, subIndex, level, op);
	}

	void OpenGLContext::clearShaderUAV(RHIShaderProgram& shaderProgram, ShaderParameter const& param)
	{
		GLuint shaderHandle = OpenGLCast::GetHandle(shaderProgram);
		int indexSlot = fetchSamplerStateSlot(shaderHandle, param.mLoc);
		glBindImageTexture(indexSlot, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glUniform1i(param.mLoc, indexSlot);
	}

	template< class TShaderObject >
	void OpenGLContext::setShaderUniformBufferT(TShaderObject& shaderObject, ShaderParameter const& param, RHIBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		GLuint shaderHandle = OpenGLCast::GetHandle(shaderObject);
		GLuint bufferHandle = OpenGLCast::GetHandle(buffer);
		int slotIndex = mBufferBindings[BBT_Uniform].fetchSlot(shaderHandle, param.mLoc, bufferHandle);
		if (slotIndex != INDEX_NONE)
		{
			glUniformBlockBinding(shaderHandle, param.mLoc, slotIndex);
			glBindBufferBase(GL_UNIFORM_BUFFER, slotIndex, bufferHandle);
		}
	}

	void OpenGLContext::setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer)
	{
		setShaderUniformBufferT(shaderProgram, param, buffer);
	}

	void OpenGLContext::setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
	{
		setShaderUniformBufferT(shader, param, buffer);
	}	
	
	template< class TShaderObject >
	void OpenGLContext::setShaderStorageBufferT(TShaderObject& shaderObject, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op)
	{
		CHECK_PARAMETER(param);
		GLuint shaderHandle = OpenGLCast::GetHandle(shaderObject);
		GLuint bufferHandle = OpenGLCast::GetHandle(buffer);
		int slotIndex = mBufferBindings[BBT_Storage].fetchSlot(shaderHandle, param.mLoc, bufferHandle);
		if (slotIndex != INDEX_NONE)
		{
			glShaderStorageBlockBinding(shaderHandle, param.mLoc, slotIndex);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slotIndex, bufferHandle);
		}
	}

	void OpenGLContext::setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op)
	{
		setShaderStorageBufferT(shaderProgram, param, buffer, op);
	}

	void OpenGLContext::setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer, EAccessOp op)
	{
		setShaderStorageBufferT(shader, param, buffer, op);
	}

	void OpenGLContext::setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, param.mLoc, OpenGLCast::GetHandle(buffer));
	}

	void OpenGLContext::setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIBuffer& buffer)
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

	void OpenGLContext::setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector2 const val[], int dim)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);
		CHECK_PARAMETER(param);
		glProgramUniform2fv(shaderImpl.getHandle(), param.mLoc, dim, (float const *)val);
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


	void OpenGLContext::markRenderStateDirty()
	{
		//TODO
	}

	void OpenGLContext::commitGraphicStates()
	{
#if COMMIT_STATE_IMMEDIATELY == 0
		commitRasterizerState();
		commitDepthStencilState();
		commitBlendState();
		commitSamplerStates();
#endif
		commitFixedShaderState();
	}

	void OpenGLContext::commitSamplerStates()
	{
		if(mSimplerSlotDirtyMask)
		{
			do
			{
#define OPENGL_USE_NEW_BIND_TEXTURE 1
				uint32 dirtyBit = FBitUtility::ExtractTrailingBit(mSimplerSlotDirtyMask);
				mSimplerSlotDirtyMask &= ~dirtyBit;
				int index = FBitUtility::ToIndex32(dirtyBit);

				SamplerState& state = mSamplerStates[index];
				if (state.bWrite)
				{


				}
				else
				{
#if OPENGL_USE_NEW_BIND_TEXTURE
					glBindTextureUnit(index, state.textureHandle);
#else
					glActiveTexture(GL_TEXTURE0 + index);
					glBindTexture(state.typeEnum, state.textureHandle);
#endif
					glBindSampler(index, state.samplerHandle);
				}
				glProgramUniform1i(state.shaderHandle, state.loc, index);

			} 
			while (mSimplerSlotDirtyMask);

#if OPENGL_USE_NEW_BIND_TEXTURE

#else
			glActiveTexture(GL_TEXTURE0);
#endif
		}
	}

	void OpenGLContext::commitRasterizerState()
	{
		if (!CheckStateDirty(mDeviceState.rasterizerStateCommitted, mDeviceState.rasterizerStatePending))
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

			if (GL_CHECK_STATE_DIRTY(frontFace))
			{
				glFrontFace(pendingValue.frontFace);
			}

			if (GL_CHECK_STATE_DIRTY(cullFace))
			{
				glCullFace(pendingValue.cullFace);
			}
		}
	}

	void OpenGLContext::commitBlendState()
	{
		if (!CheckStateDirty(mDeviceState.blendStateCommitted, mDeviceState.blendStatePending))
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

				if (bForceAllReset || GL_IS_STATE_DIRTY(writeMask))
				{
					bForceReset = true;
					GL_COMMIT_STATE_VALUE(writeMask);
					glColorMaski(i, pendingValue.writeMask & CWM_R,
						pendingValue.writeMask & CWM_G,
						pendingValue.writeMask & CWM_B,
						pendingValue.writeMask & CWM_A);
				}

				if (bForceAllReset || pendingValue.writeMask)
				{

					if (bForceAllReset || GL_IS_STATE_DIRTY(bEnable))
					{
						GL_COMMIT_STATE_VALUE(bEnable);
						EnableGLStateIndex(GL_BLEND, i, pendingValue.bEnable);
					}

					if (pendingValue.bEnable)
					{
						if (bForceReset || GL_IS_STATE_DIRTY(bSeparateBlend) || GL_IS_STATE_DIRTY(srcColor) || GL_IS_STATE_DIRTY(destColor) || GL_IS_STATE_DIRTY(srcAlpha) || GL_IS_STATE_DIRTY(destAlpha))
						{
							GL_COMMIT_STATE_VALUE(bSeparateBlend);
							GL_COMMIT_STATE_VALUE(srcColor);
							GL_COMMIT_STATE_VALUE(destColor);
							GL_COMMIT_STATE_VALUE(srcAlpha);
							GL_COMMIT_STATE_VALUE(destAlpha);

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

			if (bForceAllReset || GL_IS_STATE_DIRTY(writeMask))
			{
				bForceReset = true;
				GL_COMMIT_STATE_VALUE(writeMask);
				glColorMask(
					pendingValue.writeMask & CWM_R,
					pendingValue.writeMask & CWM_G,
					pendingValue.writeMask & CWM_B,
					pendingValue.writeMask & CWM_A);
			}

			if (bForceAllReset || pendingValue.writeMask)
			{

				if (bForceAllReset || GL_IS_STATE_DIRTY(bEnable))
				{
					GL_COMMIT_STATE_VALUE(bEnable);
					EnableGLState(GL_BLEND, pendingValue.bEnable);
				}

				if (pendingValue.bEnable)
				{
					if (bForceReset || GL_IS_STATE_DIRTY(bSeparateBlend) || GL_IS_STATE_DIRTY(srcColor) || GL_IS_STATE_DIRTY(destColor) || GL_IS_STATE_DIRTY(srcAlpha) || GL_IS_STATE_DIRTY(destAlpha))
					{
						GL_COMMIT_STATE_VALUE(bSeparateBlend);
						GL_COMMIT_STATE_VALUE(srcColor);
						GL_COMMIT_STATE_VALUE(destColor);
						GL_COMMIT_STATE_VALUE(srcAlpha);
						GL_COMMIT_STATE_VALUE(destAlpha);

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
		if ( !CheckStateDirty(mDeviceState.depthStencilStateCommitted , mDeviceState.depthStencilStatePending) )
		{
			if ( CheckStateDirty( committedValue.stencilRef , mDeviceState.stencilRefPending ) )
			{
				glStencilFunc(committedValue.stencilFunc, mDeviceState.stencilRefPending, committedValue.stencilReadMask);
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

			bool bForceRestFunc = false;
			if (GL_CHECK_STATE_DIRTY(bUseSeparateStencilFun))
			{
				bForceRestFunc = true;
			}

			if (pendingValue.bUseSeparateStencilOp)
			{
				if (bForceRestOp || GL_IS_STATE_DIRTY(stencilFailOp) || GL_IS_STATE_DIRTY(stencilZFailOp) || GL_IS_STATE_DIRTY(stencilZPassOp) || GL_IS_STATE_DIRTY(stencilFailOpBack) || GL_IS_STATE_DIRTY(stencilZFailOpBack) || GL_IS_STATE_DIRTY(stencilZPassOpBack))
				{
					GL_COMMIT_STATE_VALUE(stencilFailOp);
					GL_COMMIT_STATE_VALUE(stencilZFailOp);
					GL_COMMIT_STATE_VALUE(stencilZPassOp);
					GL_COMMIT_STATE_VALUE(stencilFailOpBack);
					GL_COMMIT_STATE_VALUE(stencilZFailOpBack);
					GL_COMMIT_STATE_VALUE(stencilZPassOpBack);
					glStencilOpSeparate(GL_FRONT, pendingValue.stencilFailOp, pendingValue.stencilZFailOp, pendingValue.stencilZPassOp);
					glStencilOpSeparate(GL_BACK, pendingValue.stencilFailOpBack, pendingValue.stencilZFailOpBack, pendingValue.stencilZPassOpBack);
				}
			}
			else
			{
				if (bForceRestOp || GL_IS_STATE_DIRTY(stencilFailOp) || GL_IS_STATE_DIRTY(stencilZFailOp) || GL_IS_STATE_DIRTY(stencilZPassOp))
				{
					GL_COMMIT_STATE_VALUE(stencilFailOp);
					GL_COMMIT_STATE_VALUE(stencilZFailOp);
					GL_COMMIT_STATE_VALUE(stencilZPassOp);
					glStencilOp(pendingValue.stencilFailOp, pendingValue.stencilZFailOp, pendingValue.stencilZPassOp);
				}
			}
			if (pendingValue.bUseSeparateStencilFun)
			{
				if (bForceRestFunc || GL_IS_STATE_DIRTY(stencilFunc) || GL_IS_STATE_DIRTY(stencilFunBack) || committedValue.stencilRef != mDeviceState.stencilRefPending || GL_IS_STATE_DIRTY(stencilReadMask))
				{
					GL_COMMIT_STATE_VALUE(stencilFunc);
					GL_COMMIT_STATE_VALUE(stencilFunBack);
					GL_COMMIT_STATE_VALUE(stencilReadMask);
					committedValue.stencilRef = mDeviceState.stencilRefPending;
					glStencilFuncSeparate(GL_FRONT, pendingValue.stencilFunc, mDeviceState.stencilRefPending, pendingValue.stencilReadMask);
					glStencilFuncSeparate(GL_BACK, pendingValue.stencilFunBack, mDeviceState.stencilRefPending, pendingValue.stencilReadMask);
				}
			}
			else
			{
				if (bForceRestFunc || GL_IS_STATE_DIRTY(stencilFunc) || committedValue.stencilRef != mDeviceState.stencilRefPending || GL_IS_STATE_DIRTY(stencilReadMask))
				{
					GL_COMMIT_STATE_VALUE(stencilFunc);
					GL_COMMIT_STATE_VALUE(stencilReadMask);
					committedValue.stencilRef = mDeviceState.stencilRefPending;
					glStencilFunc(pendingValue.stencilFunc, mDeviceState.stencilRefPending, pendingValue.stencilReadMask);
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
				mWasBindAttrib = mbUseShaderPath || CVarOpenGLFixedPiplineUseShader;
				auto& inputLayoutImpl = OpenGLCast::To(*mInputLayoutCommitted);
				if (mWasBindAttrib)
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
		if (mInputLayoutPending.isValid())
		{
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

			if (checkInputStreamStateDirty(bForceDirty))
			{
				auto& inputLayoutImpl = OpenGLCast::To(*mInputLayoutCommitted);
				mWasBindAttrib = mbUseShaderPath || CVarOpenGLFixedPiplineUseShader;
				if (mWasBindAttrib)
				{
					inputLayoutImpl.bindAttribUP(mInputStreamStateCommttied);
				}
				else
				{
					inputLayoutImpl.bindPointerUP(mInputStreamStateCommttied);
				}
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

	void OpenGLContext::commitFixedShaderState()
	{
		if (mbUseShaderPath == false)
		{
			if (CVarOpenGLFixedPiplineUseShader)
			{
				if (mInputLayoutCommitted)
				{
					OpenGLInputLayout* inputLayoutImpl = OpenGLCast::To(mInputLayoutCommitted);
					SimplePipelineProgram* program = SimplePipelineProgram::Get(inputLayoutImpl->mAttributeMask, mFixedShaderParams.texture);

					RHISetShaderProgram(program->getRHI());
					program->setParameters(RHICommandListImpl(*this), mFixedShaderParams);
				}

			}
		}
	}

	int OpenGLContext::fetchSamplerStateSlot(GLuint shaderHandle, int loc)
	{
		for (int indexSlot = 0; indexSlot != mNextSamplerSlotIndex; ++indexSlot)
		{
			SamplerState const& state = mSamplerStates[indexSlot];
			if (state.shaderHandle == shaderHandle &&
				state.loc == loc)
			{
				return indexSlot;
			}
		}

		int newSlotIndex = mNextSamplerSlotIndex;
		++mNextSamplerSlotIndex;
		SamplerState& newSamplerState = mSamplerStates[newSlotIndex];

		newSamplerState.loc = loc;
		newSamplerState.shaderHandle = shaderHandle;
		newSamplerState.samplerHandle = GL_NULL_HANDLE;
		newSamplerState.textureHandle = GL_NULL_HANDLE;
		return newSlotIndex;
	}

	void OpenGLContext::setShaderResourceViewInternal(GLuint shaderHandle, ShaderParameter const& param, RHIShaderResourceView const& resourceView)
	{
		OpenGLShaderResourceView const&  resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(resourceView);

		int indexSlot = fetchSamplerStateSlot(shaderHandle, param.mLoc);
		SamplerState& state = mSamplerStates[indexSlot];

		if (state.typeEnum != resourceViewImpl.typeEnum ||
			state.textureHandle != resourceViewImpl.handle ||
			state.bWrite != false)
		{
			mSimplerSlotDirtyMask |= BIT(indexSlot);
			state.textureHandle = resourceViewImpl.handle;
			state.typeEnum = resourceViewImpl.typeEnum;
			state.bWrite = false;
#if COMMIT_STATE_IMMEDIATELY 
			commitSamplerStates();
#endif
		}
	}

	void OpenGLContext::setShaderResourceViewInternal(GLuint shaderHandle, ShaderParameter const& param, RHIShaderResourceView const& resourceView, RHISamplerState const& sampler)
	{
		OpenGLShaderResourceView const&  resourceViewImpl = static_cast<OpenGLShaderResourceView const&>(resourceView);
		OpenGLSamplerState const&  samplerImpl = static_cast<OpenGLSamplerState const&>(sampler);
		int indexSlot = fetchSamplerStateSlot(shaderHandle, param.mLoc);
		SamplerState& state = mSamplerStates[indexSlot];

		if ( state.typeEnum != resourceViewImpl.typeEnum ||
			 state.textureHandle != resourceViewImpl.handle ||
			 state.samplerHandle != samplerImpl.getHandle() ||
			 state.bWrite != false )
		{
			mSimplerSlotDirtyMask |= BIT(indexSlot);
			state.textureHandle = resourceViewImpl.handle;
			state.typeEnum = resourceViewImpl.typeEnum;
			state.samplerHandle = samplerImpl.getHandle();
			state.bWrite = false;
#if COMMIT_STATE_IMMEDIATELY 
			commitSamplerStates();
#endif
		}
	}

	void OpenGLContext::setShaderSamplerInternal(GLuint shaderHandle, ShaderParameter const& param, RHISamplerState& sampler)
	{
		OpenGLSamplerState const&  samplerImpl = static_cast<OpenGLSamplerState const&>(sampler);

		int indexSlot = fetchSamplerStateSlot(shaderHandle, param.mLoc);
		SamplerState& state = mSamplerStates[indexSlot];
		if (state.samplerHandle != samplerImpl.getHandle())
		{
			mSimplerSlotDirtyMask |= BIT(indexSlot);
			state.samplerHandle = samplerImpl.getHandle();
#if COMMIT_STATE_IMMEDIATELY 
			commitSamplerStates();
#endif
		}
	}

	bool OpenGLShaderBoundState::create(GraphicsShaderStateDesc const& stateDesc)
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
		CheckShader(stateDesc.vertex, GL_VERTEX_SHADER_BIT);
		CheckShader(stateDesc.pixel, GL_FRAGMENT_SHADER_BIT);
		CheckShader(stateDesc.geometry, GL_GEOMETRY_SHADER_BIT);
		CheckShader(stateDesc.hull, GL_TESS_CONTROL_SHADER_BIT);
		CheckShader(stateDesc.domain, GL_TESS_EVALUATION_SHADER_BIT);

		if (!VerifyOpenGLStatus())
			return false;

		//glValidateProgramPipeline(getHandle());

		return true;
	}
	bool OpenGLShaderBoundState::create(MeshShaderStateDesc const& stateDesc)
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

		CheckShader(stateDesc.task, GL_TASK_SHADER_BIT_NV);
		CheckShader(stateDesc.mesh, GL_MESH_SHADER_BIT_NV);
		CheckShader(stateDesc.pixel, GL_FRAGMENT_SHADER_BIT);
		if (!VerifyOpenGLStatus())
			return false;

		//glValidateProgramPipeline(getHandle());

		return true;
	}
}

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif