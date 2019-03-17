#ifndef GLCommon_h__
#define GLCommon_h__

#include "BaseType.h"
#include "RHIDefine.h"
#include "RHICommon.h"

#include "glew/GL/glew.h"
#include "GLConfig.h"

#include "CppVersion.h"

#include "LogSystem.h"
#include "TVector2.h"

#include "RefCount.h"

#include <vector>
#include <functional>
#include <type_traits>

#define IGNORE_NSIGHT_UNSUPPORT_CODE 0

namespace Render
{

	template< class RMPolicy >
	class TOpenGLObject : public RefCountedObjectT< TOpenGLObject< RMPolicy > >
	{
	public:
		TOpenGLObject()
		{
			mHandle = 0;
		}

		~TOpenGLObject()
		{
			destroyHandle();
		}

		bool isValid() { return mHandle != 0; }

#if CPP_VARIADIC_TEMPLATE_SUPPORT
		template< class ...Args >
		bool fetchHandle(Args&& ...args)
		{
			if( !mHandle )
				RMPolicy::Create(mHandle, std::forward<Args>(args)...);
			return mHandle != 0;
		}

#else
		bool fetchHandle()
		{
			if( !mHandle )
				RMPolicy::Create(mHandle);
			return mHandle != 0;
		}

		template< class P1 >
		bool fetchHandle(P1 p1)
		{
			if( !mHandle )
				RMPolicy::Create(mHandle, p1);
			return mHandle != 0;
		}

#endif

		void destroyHandle()
		{
			if( mHandle )
			{
				RMPolicy::Destroy(mHandle);
				GLenum error = glGetError();
				if( error != GL_NO_ERROR )
				{
					//#TODO
					LogError("Can't Destory GL Object");
				}
			}
			mHandle = 0;
		}

		GLuint mHandle;
	};

	struct RMPTexture
	{
		static void Create(GLuint& handle) { glGenTextures(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteTextures(1, &handle); }
	};


	struct RMPRenderBuffer
	{
		static void Create(GLuint& handle) { glGenRenderbuffers(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteRenderbuffers(1, &handle); }
	};

	
	struct RMPBufferObject
	{
		static void Create(GLuint& handle) { glGenBuffers(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteBuffers(1, &handle); }
	};

	struct RMPSamplerObject
	{
		static void Create(GLuint& handle) { glGenSamplers(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteSamplers(1, &handle); }
	};

	template< class RHIResourceType , class RMPolicy >
	class TOpenGLResource : public RHIResourceType
	{
	public:
		GLuint getHandle() const { return mGLObject.mHandle; }

		virtual void incRef() { mGLObject.incRef();  }
		virtual bool decRef() { return mGLObject.decRef();  }
		virtual void releaseResource() { mGLObject.destroyHandle(); }

		TOpenGLObject< RMPolicy > mGLObject;
	};

	template< class RHIResourceType >
	class TOpenGLSimpleResource : public RHIResourceType
	{
	public:
		TOpenGLSimpleResource() { mRefcount = 0; }

		virtual void incRef() { ++mRefcount; }
		virtual bool decRef() { --mRefcount; return mRefcount == 0; }
		virtual void releaseResource() {}

		int mRefcount;
	};

	template< class RHITextureType >
	struct OpenGLTextureTraits {};

	template<>
	struct OpenGLTextureTraits< RHITexture1D >{ static GLenum constexpr EnumValue = GL_TEXTURE_1D; };
	template<>
	struct OpenGLTextureTraits< RHITexture2D >{ static GLenum constexpr EnumValue = GL_TEXTURE_2D; };
	template<>
	struct OpenGLTextureTraits< RHITexture3D >{ static GLenum constexpr EnumValue = GL_TEXTURE_3D; };
	template<>
	struct OpenGLTextureTraits< RHITextureCube >{ static GLenum constexpr EnumValue = GL_TEXTURE_CUBE_MAP; };
	template<>
	struct OpenGLTextureTraits< RHITextureDepth >{ static GLenum constexpr EnumValue = GL_TEXTURE_2D; };


	template< class RHITextureType >
	class TOpengGLTexture : public TOpenGLResource< RHITextureType , RMPTexture >
	{
	protected:
		static GLenum const TypeNameGL = OpenGLTextureTraits< RHITextureType >::EnumValue;

	public:
		void bind()
		{
			glBindTexture(TypeNameGL, getHandle());
		}

		void unbind()
		{
			glBindTexture(TypeNameGL, 0);
		}

	};


	class OpenGLTexture1D : public TOpengGLTexture< RHITexture1D >
	{
	public:
		bool create(Texture::Format format, int length, int numMipLevel , void* data );
		bool update(int offset, int length, Texture::Format format, void* data, int level );
	};


	class OpenGLTexture2D : public TOpengGLTexture< RHITexture2D >
	{
	public:
		bool loadFromFile(char const* path);
		bool create(Texture::Format format, int width, int height, int numMipLevel, void* data, int alignment);

		bool update(int ox, int oy, int w, int h, Texture::Format format, void* data, int level);
		bool update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level);
	};


	class OpenGLTexture3D : public TOpengGLTexture< RHITexture3D >
	{
	public:
		bool create(Texture::Format format, int sizeX, int sizeY, int sizeZ , void* data);
	};


	class OpenGLTextureCube : public TOpengGLTexture< RHITextureCube >
	{
	public:
		bool loadFile(char const* path[]);
		bool create(Texture::Format format, int width, int height, void* data );
	};


	class RHITextureDepth : public RHITextureBase
	{
	public:
		virtual bool create(Texture::DepthFormat format, int width, int height) = 0;
		Texture::DepthFormat getFormat() { return mFromat; }
		Texture::DepthFormat mFromat;
	};

	class OpenGLTextureDepth : public TOpengGLTexture< RHITextureDepth >
	{
	public:
		bool create(Texture::DepthFormat format, int width, int height);
	};
	typedef TRefCountPtr< RHITextureDepth > RHITextureDepthRef;

	class GLConvert
	{
	public:
		static GLenum To(EAccessOperator op);
		static GLenum To(Texture::Format format);
		static GLenum To(PrimitiveType type);
		static GLenum To(Shader::Type type);
		static GLenum To(ELockAccess access);
		static GLenum To(Blend::Factor factor);
		static GLenum To(Blend::Operation op);
		static GLenum To(ECompareFun fun);
		static GLenum To(Stencil::Operation op);
		static GLenum To(ECullMode mode);
		static GLenum To(EFillMode mode);
		static GLenum To(ECompValueType type );
		static GLenum To(Sampler::Filter filter);
		static GLenum To(Sampler::AddressMode mode);
		static GLenum To(Texture::DepthFormat format);

		static GLenum BaseFormat(Texture::Format format);
		static GLenum PixelFormat(Texture::Format format);
		static GLenum TextureComponentType(Texture::Format format);
		static GLenum Image2DType(Texture::Format format);

		static GLenum VertexComponentType(uint8 format)
		{
			return GLConvert::To(Vertex::GetCompValueType(Vertex::Format(format)));
		}
	};

#define USE_DepthRenderBuffer 0

#if USE_DepthRenderBuffer
	class RHIDepthRenderBuffer : public TRHIResource< RMPRenderBuffer >
	{
	public:
		bool create( int w , int h , Texture::DepthFormat format );
		Texture::DepthFormat getFormat() { return mFormat;  }
		Texture::DepthFormat mFormat;

	};


	typedef TRefCountPtr< RHIDepthRenderBuffer > RHIDepthRenderBufferRef;
#endif

	struct RMPFrameBuffer
	{
		static void Create(GLuint& handle) { glGenFramebuffers(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteFramebuffers(1, &handle); }
	};

	class OpenGLFrameBuffer : public TOpenGLResource< RHIFrameBuffer , RMPFrameBuffer >
	{
	public:
		OpenGLFrameBuffer();
		~OpenGLFrameBuffer();

		bool create();

		int  addTextureLayer(RHITextureCube& target);
		int  addTexture(RHITextureCube& target, Texture::Face face);
		int  addTexture( RHITexture2D& target);
		void setTexture( int idx , RHITexture2D& target );
		void setTexture(int idx, RHITextureCube& target, Texture::Face face);

		void bindDepthOnly();
		void bind();
		void unbind();
#if USE_DepthRenderBuffer
		void setDepth( RHIDepthRenderBuffer& buffer );
#endif
		void setDepth( RHITextureDepth& target );
		void removeDepthBuffer();

		void clearBuffer( Vector4 const* colorValue, float const* depthValue = nullptr, uint8 stencilValue = 0);

		
		struct BufferInfo
		{
			RHIResourceRef bufferRef;
			union 
			{
				uint8  idxFace;
				bool   bTexture;
			};
		};
		void setTextureInternal(int idx, GLuint handle, GLenum texType);
		void setDepthInternal(RHIResource& resource, GLuint handle, Texture::DepthFormat format, bool bTexture);


		std::vector< BufferInfo > mTextures;
		BufferInfo  mDepth;
	};



	template< class RHITextureType >
	struct OpenGLBufferTraits {};

	template<>
	struct OpenGLBufferTraits< RHIVertexBuffer >{  static GLenum constexpr EnumValue = GL_ARRAY_BUFFER;  };
	template<>
	struct OpenGLBufferTraits< RHIIndexBuffer > { static GLenum constexpr EnumValue = GL_ELEMENT_ARRAY_BUFFER; };
	template<>
	struct OpenGLBufferTraits< RHIUniformBuffer > { static GLenum constexpr EnumValue = GL_UNIFORM_BUFFER; };

	template < class RHIBufferType >
	class TOpenGLBuffer : public TOpenGLResource< RHIBufferType, RMPBufferObject >
	{
	public:
		static GLenum constexpr GLBufferType = OpenGLBufferTraits< RHIBufferType >::EnumValue;

		void  bind() { glBindBuffer(GLBufferType, getHandle()); }
		void  unbind() { glBindBuffer(GLBufferType, 0); }

		void* lock(ELockAccess access)
		{
			glBindBuffer(GLBufferType, getHandle());
			void* result = glMapBuffer(GLBufferType, GLConvert::To(access));
			glBindBuffer(GLBufferType, 0);
			return result;
		}
		void* lock(ELockAccess access, uint32 offset, uint32 length)
		{
			glBindBuffer(GLBufferType, getHandle());

			GLbitfield accessBits = 0;
			switch( access )
			{
			case ELockAccess::ReadOnly:
				accessBits = GL_MAP_READ_BIT;
				break;
			case ELockAccess::ReadWrite:
				accessBits = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
				break;
			case ELockAccess::WriteDiscard:
			case ELockAccess::WriteOnly:
				accessBits = GL_MAP_WRITE_BIT;
				break;
			}
			void* result = glMapBufferRange(GLBufferType, offset, length, accessBits);
			glBindBuffer(GLBufferType, 0);
			return result;
		}

		void unlock()
		{
			glBindBuffer(GLBufferType, getHandle());
			glUnmapBuffer(GLBufferType);
			glBindBuffer(GLBufferType, 0);
		}

		bool createInternal(uint32 elementSize , uint32 numElements, uint32 creationFlags , void* initData)
		{
			if( !mGLObject.fetchHandle() )
				return false;

			return resetDataInternal(elementSize, numElements, creationFlags, initData);
		}

		bool resetDataInternal(uint32 elementSize, uint32 numElements, uint32 creationFlags, void* initData)
		{
			glBindBuffer(GLBufferType, getHandle());
			glBufferData(GLBufferType, elementSize * numElements , initData, GetUsageEnum(creationFlags));
			glBindBuffer(GLBufferType, 0);

			mCreationFlags = creationFlags;
			mNumElements = numElements;
			mElementSize = elementSize;
			return true;
		}


		static GLenum GetUsageEnum(uint32 creationFlags)
		{
			if( creationFlags & BCF_UsageDynamic )
				return GL_DYNAMIC_DRAW;
			else if( creationFlags & BCF_UsageStage )
				return GL_STREAM_DRAW;
			return GL_STATIC_DRAW;
		}
		
	};

	class OpenGLVertexBuffer : public TOpenGLBuffer< RHIVertexBuffer >
	{
	public:
		bool  create(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		void  resetData(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		void  updateData(uint32 vStart, uint32 numVertices, void* data);
	};

	class OpenGLIndexBuffer : public TOpenGLBuffer< RHIIndexBuffer >
	{
	public:

		bool create(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
		{
			if( !createInternal((bIntIndex ? sizeof(GLuint) : sizeof(GLushort)) , nIndices, creationFlags , data) )
				return false;

			return true;
		}
	};

	class OpenGLUniformBuffer : public TOpenGLBuffer< RHIUniformBuffer >
	{
	public:
		bool create(uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data);
	};

	class OpenGLSamplerState : public TOpenGLResource< RHISamplerState, RMPSamplerObject >
	{
	public:
		bool create(SamplerStateInitializer const& initializer);
	};

	struct GLRasterizerStateValue
	{
		bool   bEnalbeCull;
		GLenum fillMode;
		GLenum cullFace;

		GLRasterizerStateValue() {}
		GLRasterizerStateValue(EForceInit)
		{
			bEnalbeCull = false;
			fillMode = GL_FILL;
			cullFace = GL_BACK;
		}
	};

	class OpenGLRasterizerState : public TOpenGLSimpleResource< RHIRasterizerState >
	{
	public:
		OpenGLRasterizerState(RasterizerStateInitializer const& initializer);
		GLRasterizerStateValue mStateValue;
	};

	struct GLDepthStencilStateValue
	{
		bool   bEnableDepthTest;
		bool   bEnableStencilTest;
		bool   bWriteDepth;
		bool   bUseSeparateStencilFun;
		bool   bUseSeparateStencilOp;

		GLenum depthFun;

		GLenum stencilFun;
		GLenum stencilFailOp;
		GLenum stencilZFailOp;
		GLenum stencilZPassOp;

		GLenum stencilFunBack;
		GLenum stencilFailOpBack;
		GLenum stencilZFailOpBack;
		GLenum stencilZPassOpBack;

		uint32 stencilReadMask;
		uint32 stencilWriteMask;
		uint32 stencilRef;

		GLDepthStencilStateValue() {}

		GLDepthStencilStateValue(EForceInit)
		{
			bEnableDepthTest = false;

			bEnableStencilTest = false;
			bWriteDepth = false;
			bUseSeparateStencilOp = false;
			bUseSeparateStencilFun = false;

			depthFun = GL_LESS;

			stencilFun = stencilFunBack = GL_ALWAYS;
			stencilFailOp = stencilFailOpBack = GL_KEEP;
			stencilZFailOp = stencilZFailOpBack = GL_KEEP;
			stencilZPassOp = stencilZPassOpBack = GL_KEEP;

			stencilReadMask = -1;
			stencilWriteMask = -1;
			stencilRef = -1;

		}
	};

	class OpenGLDepthStencilState : public TOpenGLSimpleResource< RHIDepthStencilState >
	{
	public:
		OpenGLDepthStencilState(DepthStencilStateInitializer const& initializer);
		GLDepthStencilStateValue mStateValue;
	};


	struct GLTargetBlendValue
	{
		ColorWriteMask writeMask;

		GLenum srcColor;
		GLenum srcAlpha;
		GLenum destColor;
		GLenum destAlpha;
		GLenum op;
		GLenum opAlpha;

		bool bEnable;
		bool bSeparateBlend;

		GLTargetBlendValue() {}
		GLTargetBlendValue(EForceInit)
		{
			bEnable = false;
			bSeparateBlend = false;
			srcColor = GL_ONE;
			srcAlpha = GL_ONE;
			destColor = GL_ZERO;
			destAlpha = GL_ZERO;
			writeMask = CWM_RGBA;
			op = GL_FUNC_ADD;
			opAlpha = GL_FUNC_ADD;
		}
	};

	struct GLBlendStateValue
	{
		GLTargetBlendValue targetValues[NumBlendStateTarget];
		GLBlendStateValue() {}
		GLBlendStateValue(EForceInit)
		{
			for( int i = 0; i < NumBlendStateTarget; ++i )
				targetValues[0] = GLTargetBlendValue(ForceInit);
		}
	};

	class OpenGLBlendState : public TOpenGLSimpleResource< RHIBlendState >
	{
	public:
		OpenGLBlendState(BlendStateInitializer const& initializer);
		GLBlendStateValue mStateValue;
	};

	class OpenGLInputLayout : public TOpenGLSimpleResource< RHIInputLayout >
	{
	public:
		OpenGLInputLayout( InputLayoutDesc const& desc );

		void bindAttrib( RHIVertexBuffer* vertexBuffer[] , int numVertexBuffer , LinearColor const* overwriteColor = nullptr);
		void unbindAttrib(int numVertexBuffer, LinearColor const* overwriteColor = nullptr);

		void bindPointer(LinearColor const* overwriteColor = nullptr);

		void unbindPointer(LinearColor const* overwriteColor = nullptr);
		struct Element
		{
			uint8  semantic;
			uint8  offset;
			uint8  idxStream;
			uint8  attribute;
			uint32 stride;
			bool   bNormalize;
			int    componentNum;
			GLenum componentType;
			uint8  idx;

		};

		std::vector< Element > mElements;
	};

	struct PrimitiveDebugInfo
	{
		int32 numVertex;
		int32 numIndex;
		int32 numElement;
	};

	class RHIShader;
	class ShaderProgram;

	struct OpenGLCast
	{
		static OpenGLTexture1D* To(RHITexture1D* tex) { return static_cast<OpenGLTexture1D*>(tex); }
		static OpenGLTexture2D* To(RHITexture2D* tex) { return static_cast<OpenGLTexture2D*>(tex); }
		static OpenGLTexture3D* To(RHITexture3D* tex) { return static_cast<OpenGLTexture3D*>(tex); }
		static OpenGLTextureCube* To(RHITextureCube* tex) { return static_cast<OpenGLTextureCube*>(tex); }
		static OpenGLTextureDepth* To(RHITextureDepth* tex) { return static_cast<OpenGLTextureDepth*>(tex); }

		static OpenGLVertexBuffer* To(RHIVertexBuffer* buffer) { return static_cast<OpenGLVertexBuffer*>(buffer); }
		static OpenGLIndexBuffer* To(RHIIndexBuffer* buffer) { return static_cast<OpenGLIndexBuffer*>(buffer); }
		static OpenGLUniformBuffer* To(RHIUniformBuffer* buffer) { return static_cast<OpenGLUniformBuffer*>(buffer); }

		static OpenGLInputLayout*  To(RHIInputLayout* inputLayout) { return static_cast<OpenGLInputLayout*>(inputLayout); }
		static OpenGLSamplerState* To(RHISamplerState* state ) { return static_cast<OpenGLSamplerState*>(state); }

		static RHIShader* To(RHIShader* shader) { return shader; }
		static ShaderProgram* To(ShaderProgram* shader) { return shader; }
		static OpenGLFrameBuffer* To(OpenGLFrameBuffer* buffer) { return buffer; }

		template < class T >
		static auto To(TRefCountPtr<T>& ptr) { return To(ptr.get()); }

		template < class T >
		static GLuint GetHandle(T& RHIObject)
		{
			return OpenGLCast::To(&RHIObject)->getHandle();
		}

		template < class T >
		static GLuint GetHandle(T const& RHIObject)
		{
			return OpenGLCast::To(const_cast<T*>(&RHIObject))->getHandle();
		}
		template < class T >
		static GLuint GetHandle(TRefCountPtr<T>& refPtr )
		{
			return OpenGLCast::To(refPtr)->getHandle();
		}
	};

	template< class T >
	struct TBindLockScope
	{
		TBindLockScope(T& obj)
			:mObj(obj)
		{
			OpenGLCast::To(&mObj)->bind();
		}

		~TBindLockScope()
		{
			OpenGLCast::To(&mObj)->unbind();
		}

		T& mObj;
	};



	template< class T >
	struct TBindLockScope< T* >
	{
		TBindLockScope(T* obj)
			:mObj(obj)
		{
			assert(mObj);
			OpenGLCast::To(mObj)->bind();
		}

		~TBindLockScope()
		{
			OpenGLCast::To(mObj)->unbind();
		}

		T* mObj;
	};

	template< class T >
	struct TBindLockScope< TRefCountPtr< T > >
	{
		TBindLockScope(TRefCountPtr< T >& obj)
			:mObj(obj)
		{
			assert(mObj);
			OpenGLCast::To(mObj.get())->bind();
		}

		~TBindLockScope()
		{
			OpenGLCast::To(mObj.get())->unbind();
		}

		TRefCountPtr< T >& mObj;
	};

#define GL_BIND_LOCK_OBJECT( obj )\
	Render::TBindLockScope< std::remove_reference< decltype(obj) >::type >  ANONYMOUS_VARIABLE( BindLockObj )( obj );



}

#endif // GLCommon_h__
