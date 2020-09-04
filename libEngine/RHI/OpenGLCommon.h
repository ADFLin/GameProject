#ifndef GLCommon_h__
#define GLCommon_h__

#include "RHIType.h"
#include "RHIDefine.h"
#include "RHICommon.h"

#include "OpenGLHeader.h"

#include "CppVersion.h"

#include "LogSystem.h"

#include "RefCount.h"

#include <vector>
#include <functional>
#include <type_traits>

#define IGNORE_NSIGHT_UNSUPPORT_CODE 0
#define GL_NULL_HANDLE 0

namespace Render
{
	bool VerifyOpenGLStatus();

	template< class RMPolicy >
	class TOpenGLObject
	{
	public:
		TOpenGLObject()
		{
			mHandle = GL_NULL_HANDLE;
		}

		~TOpenGLObject()
		{
			destroyHandle();
		}

		TOpenGLObject(TOpenGLObject&& other)
		{
			mHandle = other.mHandle;
			other.mHandle = GL_NULL_HANDLE;
		}

		TOpenGLObject(TOpenGLObject const& other) { assert(0); }
		TOpenGLObject& operator = (TOpenGLObject const& rhs) = delete;

		bool isValid() { return mHandle != GL_NULL_HANDLE; }

#if CPP_VARIADIC_TEMPLATE_SUPPORT
		template< class ...Args >
		bool fetchHandle(Args&& ...args)
		{
			if( !mHandle )
				RMPolicy::Create(mHandle, std::forward<Args>(args)...);
			return isValid();
		}

#else
		bool fetchHandle()
		{
			if( !mHandle )
				RMPolicy::Create(mHandle);
			return isValid();
		}

		template< class P1 >
		bool fetchHandle(P1 p1)
		{
			if( !mHandle )
				RMPolicy::Create(mHandle, p1);
			return isValid();
		}

#endif

		bool destroyHandle()
		{
			if( mHandle )
			{
				RMPolicy::Destroy(mHandle);
				if (!VerifyOpenGLStatus())
				{
					return false;
				}
				mHandle = GL_NULL_HANDLE;
			}
			return true;
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
	class TOpenGLResource : public  TRefcountResource< RHIResourceType > 
	{
	public:
		GLuint getHandle() const { return mGLObject.mHandle; }

		virtual void releaseResource() 
		{
			if (!mGLObject.destroyHandle())
			{
#if USE_RHI_RESOURCE_TRACE
				LogError("Can't Destory GL Object : %s %s", mTypeName.c_str(), mTrace.toString().c_str());
#else
				LogError("Can't Destory GL Object");
#endif
			}
		}

		TOpenGLObject< RMPolicy > mGLObject;
	};

	template< class RHITextureType >
	struct OpenGLTextureTraits {};

	template<>
	struct OpenGLTextureTraits< RHITexture1D >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_1D; 
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_1D;
	};
	template<>
	struct OpenGLTextureTraits< RHITexture2D >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_2D; 
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_2D_MULTISAMPLE;
	};
	template<>
	struct OpenGLTextureTraits< RHITexture3D >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_3D;
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
	};
	template<>
	struct OpenGLTextureTraits< RHITextureCube >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_CUBE_MAP; 
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_CUBE_MAP;
	};
	template<>
	struct OpenGLTextureTraits< RHITexture2DArray >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_2D_ARRAY;
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
	};

	class OpenGLShaderResourceView : public TPersistentResource< RHIShaderResourceView >
	{
	public:
		GLuint handle;
		GLenum typeEnum;
	};

	template< class RHITextureType >
	class TOpengGLTexture : public TOpenGLResource< RHITextureType , RMPTexture >
	{
	protected:
		TOpengGLTexture()
		{}
		
		static GLenum const TypeEnumGL = OpenGLTextureTraits< RHITextureType >::EnumValue;
		static GLenum const TypeEnumGLMultisample = OpenGLTextureTraits< RHITextureType >::EnumValueMultisample;

		RHIShaderResourceView* getBaseResourceView()
		{ 
			mView.handle = mGLObject.mHandle;
			mView.typeEnum = getGLTypeEnum();
			return &mView; 
		}
	public:
		GLenum getGLTypeEnum() const
		{
			return (mNumSamples > 1) ? TypeEnumGLMultisample : TypeEnumGL;
		}
		void bind() const
		{
			glBindTexture(getGLTypeEnum(), getHandle());
		}

		void unbind() const
		{
			glBindTexture(getGLTypeEnum(), 0);
		}

		OpenGLShaderResourceView mView;
	};


	class OpenGLTexture1D : public TOpengGLTexture< RHITexture1D >
	{
	public:
		bool create(Texture::Format format, int length, int numMipLevel , uint32 createFlags, void* data );
		bool update(int offset, int length, Texture::Format format, void* data, int level );
	};


	class OpenGLTexture2D : public TOpengGLTexture< RHITexture2D >
	{
	public:
		bool create(Texture::Format format, int width, int height, int numMipLevel, int numSamples, uint32 createFlags, void* data, int alignment);
		bool createDepth(Texture::Format format, int width, int height, int numMipLevel, int numSamples);
		bool update(int ox, int oy, int w, int h, Texture::Format format, void* data, int level);
		bool update(int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level);
	};


	class OpenGLTexture3D : public TOpengGLTexture< RHITexture3D >
	{
	public:
		bool create(Texture::Format format, int sizeX, int sizeY, int sizeZ , int numMipLevel, int numSamples, uint32 createFlags, void* data);
	};

	class OpenGLTextureCube : public TOpengGLTexture< RHITextureCube >
	{
	public:
		bool create(Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[]);
		bool update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, void* data, int level );
		bool update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level );
	};

	class OpenGLTexture2DArray : public TOpengGLTexture< RHITexture2DArray >
	{
	public:
		bool create(Texture::Format format, int width, int height, int layerSize, int numMipLevel, int numSamples, uint32 createFlags, void* data);
	};

	class OpenGLTranslate
	{
	public:
		static GLenum To(EPrimitive type, int& outPatchPointCount);
		static GLenum To(EAccessOperator op);
		static GLenum To(Texture::Format format);
		static GLenum To(EShader::Type type);
		static GLbitfield ToStageBit(EShader::Type type);
		static GLenum To(ELockAccess access);
		static GLenum To(Blend::Factor factor);
		static GLenum To(Blend::Operation op);
		static GLenum To(ECompareFunc func);
		static GLenum To(Stencil::Operation op);
		static GLenum To(ECullMode mode);
		static GLenum To(EFillMode mode);
		static GLenum To(EComponentType type );
		static GLenum To(Sampler::Filter filter);
		static GLenum To(Sampler::AddressMode mode);

		static GLenum DepthFormat(Texture::Format format);

		static GLenum BaseFormat(Texture::Format format);
		static GLenum PixelFormat(Texture::Format format);
		static GLenum TextureComponentType(Texture::Format format);
		static GLenum Image2DType(Texture::Format format);

		static GLenum BufferUsageEnum(uint32 creationFlags);

		static GLenum VertexComponentType(uint8 format)
		{
			return OpenGLTranslate::To(Vertex::GetComponentType(Vertex::Format(format)));
		}
	};

#define USE_DepthRenderBuffer 0

#if USE_DepthRenderBuffer
	class RHIDepthRenderBuffer : public TRHIResource< RMPRenderBuffer >
	{
	public:
		bool create( int w , int h , Texture::Format format );
		Texture::Format getFormat() { return mFormat;  }
		Texture::Format mFormat;
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

		void setupTextureLayer(RHITextureCube& target, int level = 0);

		int  addTexture(RHITexture2D& target, int level = 0);
		int  addTexture(RHITextureCube& target, Texture::Face face, int level = 0);
		int  addTexture(RHITexture2DArray& target, int indexLayer, int level = 0);
		void setTexture(int idx, RHITexture2D& target, int level = 0);
		void setTexture(int idx, RHITextureCube& target, Texture::Face face, int level = 0);
		void setTexture(int idx, RHITexture2DArray& target, int indexLayer, int level = 0);

		
		void bindDepthOnly();
		void bind();
		void unbind();
#if USE_DepthRenderBuffer
		void setDepth( RHIDepthRenderBuffer& buffer );
#endif
		void setDepth( RHITexture2D& target );
		void removeDepth();

		void blitToBackBuffer(int index = 0);

		
		struct BufferInfo
		{
			RHIResourceRef bufferRef;
			GLenum typeEnumGL;
			uint8  level;
			union
			{
				uint8  idxFace;
				int    indexLayer;
			};
		};
		void setRenderBufferInternal(GLuint handle);
		void setTexture2DInternal(int idx, GLuint handle, GLenum texType, int level);
		void setTexture3DInternal(int idx, GLuint handle, GLenum texType, int level, int idxLayer);
		void setTextureLayerInternal(int idx, GLuint handle, GLenum texType, int level, int idxLayer);
		void setDepthInternal(RHIResource& resource, GLuint handle, Texture::Format format, GLenum typeEnumGL);

		void checkStates()
		{
			GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( Status != GL_FRAMEBUFFER_COMPLETE )
			{
				LogWarning(0, "Texture Can't Attach to FrameBuffer");
			}
		}

		std::vector< BufferInfo > mTextures;
		BufferInfo  mDepth;
	};



	template< class RHITextureType >
	struct OpenGLBufferTraits {};

	template<>
	struct OpenGLBufferTraits< RHIVertexBuffer >{  static GLenum constexpr EnumValue = GL_ARRAY_BUFFER;  };
	template<>
	struct OpenGLBufferTraits< RHIIndexBuffer > { static GLenum constexpr EnumValue = GL_ELEMENT_ARRAY_BUFFER; };

	template < class RHIBufferType >
	class TOpenGLBuffer : public TOpenGLResource< RHIBufferType, RMPBufferObject >
	{
	public:
		static GLenum constexpr GLBufferType = OpenGLBufferTraits< RHIBufferType >::EnumValue;

		void  bind() const { glBindBuffer(GLBufferType, getHandle()); }
		void  unbind() const { glBindBuffer(GLBufferType, 0); }

		void* lock(ELockAccess access)
		{
			glBindBuffer(GLBufferType, getHandle());
			void* result = glMapBuffer(GLBufferType, OpenGLTranslate::To(access));
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
			glBufferData(GLBufferType, elementSize * numElements , initData, OpenGLTranslate::BufferUsageEnum(creationFlags));
			glBindBuffer(GLBufferType, 0);

			mCreationFlags = creationFlags;
			mNumElements = numElements;
			mElementSize = elementSize;
			return true;
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

	class OpenGLSamplerState : public TOpenGLResource< RHISamplerState, RMPSamplerObject >
	{
	public:
		bool create(SamplerStateInitializer const& initializer);
	};

	struct GLRasterizerStateValue
	{
		GLenum fillMode;
		GLenum cullFace;

		bool   bEnableCull;
		bool   bEnableScissor;
		bool   bEnableMultisample;

		GLRasterizerStateValue() {}
		GLRasterizerStateValue(EForceInit)
		{
			bEnableCull = false;
			bEnableScissor = false;
			bEnableMultisample = false;
			fillMode = GL_FILL;
			cullFace = GL_BACK;
		}
	};

	class OpenGLRasterizerState : public TRefcountResource< RHIRasterizerState >
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

	class OpenGLDepthStencilState : public TRefcountResource< RHIDepthStencilState >
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
		bool bEnableAlphaToCoverage;
		bool bEnableIndependent;
		GLTargetBlendValue targetValues[MaxBlendStateTargetCount];
		GLBlendStateValue() {}
		GLBlendStateValue(EForceInit)
		{
			bEnableAlphaToCoverage = false;
			bEnableIndependent = false;
			for (int i = 0; i < MaxBlendStateTargetCount; ++i)
			{
				targetValues[i] = GLTargetBlendValue(ForceInit);
			}
		}
	};

	class OpenGLBlendState : public TRefcountResource< RHIBlendState >
	{
	public:
		OpenGLBlendState(BlendStateInitializer const& initializer);
		GLBlendStateValue mStateValue;
	};


	struct RMPVertexArrayObject
	{
		static void Create(GLuint& handle) { glGenVertexArrays(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteVertexArrays(1, &handle); }
	};

	class OpenGLInputLayout : public TRefcountResource< RHIInputLayout >
	{
	public:
		OpenGLInputLayout( InputLayoutDesc const& desc );

		void bindAttrib(InputStreamState const& state);
		void bindAttribUP(InputStreamState const& state);
		void unbindAttrib(int numInputStream);
	
		void bindPointer();
		void bindPointer(InputStreamState const& state);
		void bindPointerUP(InputStreamState const& state);
		void unbindPointer();

		GLuint bindVertexArray(InputStreamState const& state, bool bBindPointer = false);

		struct Element
		{
			GLenum componentType;
			uint32 stride;
			uint16 instanceStepRate;
			uint8  offset;
			uint8  idxStream;
			uint8  attribute;
			uint8  componentNum;
			bool   bNormalized;
			bool   bInstanceData;	
		};

		typedef TOpenGLObject< RMPVertexArrayObject > VertexArrayObject;
		std::unordered_map< InputStreamState, VertexArrayObject, MemberFuncHasher > mVAOMap;
		std::vector< Element > mElements;

		virtual void releaseResource() override;

	};

	struct PrimitiveDebugInfo
	{
		int32 numVertex;
		int32 numIndex;
		int32 numElement;
	};

	class OpenGLShader;
	class OpenGLShaderProgram;
	class ShaderProgram;

	template< class TRHIResource >
	struct TOpengGLCastTraits {};

	template<> struct TOpengGLCastTraits< RHITexture1D > { typedef OpenGLTexture1D CastType; };
	template<> struct TOpengGLCastTraits< RHITexture2D > { typedef OpenGLTexture2D CastType; };
	template<> struct TOpengGLCastTraits< RHITexture3D > { typedef OpenGLTexture3D CastType; };
	template<> struct TOpengGLCastTraits< RHITextureCube > { typedef OpenGLTextureCube CastType; };
	template<> struct TOpengGLCastTraits< RHITexture2DArray > { typedef OpenGLTexture2DArray CastType; };
	template<> struct TOpengGLCastTraits< RHIVertexBuffer > { typedef OpenGLVertexBuffer CastType; };
	template<> struct TOpengGLCastTraits< RHIIndexBuffer > { typedef OpenGLIndexBuffer CastType; };
	template<> struct TOpengGLCastTraits< RHIInputLayout > { typedef OpenGLInputLayout CastType; };
	template<> struct TOpengGLCastTraits< RHISamplerState > { typedef OpenGLSamplerState CastType; };
	template<> struct TOpengGLCastTraits< RHIBlendState > { typedef OpenGLBlendState CastType; };
	template<> struct TOpengGLCastTraits< RHIRasterizerState > { typedef OpenGLRasterizerState CastType; };
	template<> struct TOpengGLCastTraits< RHIDepthStencilState > { typedef OpenGLDepthStencilState CastType; };
	template<> struct TOpengGLCastTraits< RHIShaderProgram > { typedef OpenGLShaderProgram CastType; };
	template<> struct TOpengGLCastTraits< RHIShader > { typedef OpenGLShader CastType; };
	template<> struct TOpengGLCastTraits< RHIFrameBuffer > { typedef OpenGLFrameBuffer CastType; };

	struct OpenGLCast
	{
		template< class TRHIResource >
		static auto To(TRHIResource* resource)
		{
			return static_cast<TOpengGLCastTraits< TRHIResource >::CastType*>(resource);
		}
		template< class TRHIResource >
		static auto To(TRHIResource const* resource)
		{
			return static_cast<TOpengGLCastTraits< TRHIResource >::CastType const*>(resource);
		}
		template< class TRHIResource >
		static auto& To(TRHIResource& resource)
		{
			return static_cast<TOpengGLCastTraits< TRHIResource >::CastType&>(resource);
		}

		template< class TRHIResource >
		static auto& To(TRHIResource const& resource)
		{
			return static_cast<TOpengGLCastTraits< TRHIResource >::CastType const&>(resource);
		}

		template < class TRHIResource >
		static auto To(TRefCountPtr<TRHIResource>& ptr) { return To(ptr.get()); }

		template < class TRHIResource >
		static auto To(TRefCountPtr<TRHIResource> const& ptr) { return To(ptr.get()); }

		static auto To(OpenGLFrameBuffer* buffer) { return buffer; }

		template < class TRHIResource >
		static GLuint GetHandle(TRHIResource& RHIObject)
		{
			return OpenGLCast::To(RHIObject).getHandle();
		}

		template < class TRHIResource >
		static GLuint GetHandle(TRHIResource const& RHIObject)
		{
			return OpenGLCast::To(RHIObject).getHandle();
		}
		template < class TRHIResource >
		static GLuint GetHandle(TRefCountPtr<TRHIResource>& refPtr)
		{
			return OpenGLCast::To(refPtr)->getHandle();
		}
	};

	template< class T >
	struct TOpenGLScopedBind
	{
		TOpenGLScopedBind(T& obj)
			:mObj(obj)
		{
			OpenGLCast::To(&mObj)->bind();
		}

		~TOpenGLScopedBind()
		{
			OpenGLCast::To(&mObj)->unbind();
		}

		T& mObj;
	};



	template< class T >
	struct TOpenGLScopedBind< T* >
	{
		TOpenGLScopedBind(T* obj)
			:mObj(obj)
		{
			assert(mObj);
			OpenGLCast::To(mObj)->bind();
		}

		~TOpenGLScopedBind()
		{
			OpenGLCast::To(mObj)->unbind();
		}

		T* mObj;
	};

	template< class T >
	struct TOpenGLScopedBind< TRefCountPtr< T > >
	{
		TOpenGLScopedBind(TRefCountPtr< T >& obj)
			:mObj(obj)
		{
			assert(mObj);
			OpenGLCast::To(mObj.get())->bind();
		}

		~TOpenGLScopedBind()
		{
			OpenGLCast::To(mObj.get())->unbind();
		}

		TRefCountPtr< T >& mObj;
	};

#define GL_SCOPED_BIND_OBJECT( obj )\
	Render::TOpenGLScopedBind< std::remove_reference< decltype(obj) >::type >  ANONYMOUS_VARIABLE( BoundObject )( obj );



}

#endif // GLCommon_h__
