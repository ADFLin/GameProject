#ifndef GLCommon_h__
#define GLCommon_h__

#include "BaseType.h"
#include "RHIDefine.h"

#include "glew/GL/glew.h"
#include "GLConfig.h"

#include "CppVersion.h"


#include "LogSystem.h"
#include "TVector2.h"

#include "RefCount.h"

#include <vector>
#include <functional>
#include <type_traits>


namespace RenderGL
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



	class RHIResource
	{
	public:
		RHIResource()
		{
			++TotalCount;
		}
		virtual ~RHIResource()
		{
			--TotalCount;
		}
		virtual void incRef() = 0;
		virtual bool decRef() = 0;
		virtual void releaseResource() = 0;

		void destroyThis()
		{
			releaseResource();
			delete this;
		}
		static uint32 TotalCount;
	};

	typedef  TRefCountPtr< RHIResource > RHIResourceRef;


	template< class RHIResourceType , class RMPolicy >
	class TOpenGLResource : public RHIResourceType
	{
	public:
		GLuint getHandle() { return mGLObject.mHandle; }

		virtual void incRef() { mGLObject.incRef();  }
		virtual bool decRef() { return mGLObject.decRef();  }
		virtual void releaseResource() { mGLObject.destroyHandle(); }

		TOpenGLObject< RMPolicy > mGLObject;
	};



	enum ECompValueType
	{
		CVT_Float,
		CVT_Half,
		CVT_UInt,
		CVT_Int,
		CVT_UShort,
		CVT_Short,
		CVT_UByte,
		CVT_Byte,

		//
		CVT_NInt8,
		CVT_NInt16,

	};



	struct Texture
	{
		enum Type
		{
			e1D ,
			e2D ,
			e3D ,
			eCube ,
			eDepth ,
		};
		enum Format
		{
			eRGBA8 ,
			eRGB8  ,

			eR16,
			eR8,

			eR32F ,
			eRGB32F ,
			eRGBA32F ,
			eRGB16F,
			eRGBA16F ,

			eR32I ,
			eR16I ,
			eR8I  ,
			eR32U ,
			eR16U ,
			eR8U  ,

			eRG32I,
			eRG16I,
			eRG8I,
			eRG32U,
			eRG16U,
			eRG8U,

			eRGB32I,
			eRGB16I,
			eRGB8I,
			eRGB32U,
			eRGB16U,
			eRGB8U,

			eRGBA32I,
			eRGBA16I,
			eRGBA8I,
			eRGBA32U,
			eRGBA16U,
			eRGBA8U,

			eFloatRGBA = eRGBA16F ,
		};

		enum Face
		{
			eFaceX    = 0,
			eFaceInvX = 1,
			eFaceY    = 2,
			eFaceInvY = 3,
			eFaceZ    = 4,
			eFaceInvZ = 5,
		};

		static GLenum GetBaseFormat(Format format);
		static GLenum GetPixelFormat(Format format);
		static GLenum GetComponentType(Format format);
		static GLenum GetImage2DType(Format format);
		static uint32 GetFormatSize(Format format);
		static uint32 GetComponentNum(Format format);

		enum DepthFormat
		{
			eDepth16 ,
			eDepth24 ,
			eDepth32 ,
			eDepth32F ,

			eD24S8 ,
			eD32FS8 ,

			eStencil1 ,
			eStencil4 ,
			eStencil8 ,
			eStencil16 ,
		};

		static GLenum Convert( DepthFormat format );

		static bool ContainDepth(DepthFormat format)
		{
			return format != eStencil1 &&
				   format != eStencil4 &&
				   format != eStencil8 &&
				   format != eStencil16;
		}
		static bool ContainStencil(DepthFormat format)
		{
			return format == eD24S8 || format == eD32FS8;
		}
	};


	class RHITexture1D;
	class RHITexture2D;
	class RHITexture3D;
	class RHITextureDepth;
	class RHITextureCube;

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


	class RHITextureBase : public RHIResource
	{
	public:
		virtual ~RHITextureBase() {}
		virtual RHITexture1D* getTexture1D() { return nullptr; }
		virtual RHITexture2D* getTexture2D() { return nullptr; }
		virtual RHITexture3D* getTexture3D() { return nullptr; }
		virtual RHITextureCube* getTextureCube() { return nullptr; }

		Texture::Format getFormat() const { return mFormat; }
	protected:
		Texture::Format mFormat;
	};

	class RHITexture1D : public RHITextureBase
	{
	public:
		virtual bool create(Texture::Format format, int length, int numMipLevel = 0, void* data = nullptr) = 0;
		virtual bool update(int offset, int length, Texture::Format format, void* data, int level = 0) = 0;

		int  getSize() const { return mSize; }
	protected:
		int mSize;
	};

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


	class RHITexture2D : public RHITextureBase
	{
	public:
		virtual bool update(int ox, int oy, int w, int h , Texture::Format format , void* data , int level = 0 ) = 0;
		virtual bool update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level = 0) = 0;

		int  getSizeX() const { return mSizeX; }
		int  getSizeY() const { return mSizeY; }
	protected:
		
		int mSizeX;
		int mSizeY;
	};

	class OpenGLTexture2D : public TOpengGLTexture< RHITexture2D >
	{
	public:
		bool loadFromFile(char const* path);
		bool create(Texture::Format format, int width, int height, int numMipLevel, void* data, int alignment);

		bool update(int ox, int oy, int w, int h, Texture::Format format, void* data, int level);
		bool update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level);
	};

	class RHITexture3D : public RHITextureBase
	{
	public:
		virtual bool create(Texture::Format format, int sizeX , int sizeY , int sizeZ ) = 0;
		int  getSizeX() { return mSizeX; }
		int  getSizeY() { return mSizeY; }
		int  getSizeZ() { return mSizeZ; }
	
	protected:
		int mSizeX;
		int mSizeY;
		int mSizeZ;
	};

	class OpenGLTexture3D : public TOpengGLTexture< RHITexture3D >
	{
	public:
		bool create(Texture::Format format, int sizeX, int sizeY, int sizeZ);
	};

	class RHITextureCube : public RHITextureBase
	{
	public:
		virtual bool loadFile( char const* path[] ) = 0;
		virtual bool create( Texture::Format format, int width, int height, void* data = nullptr ) = 0;
	};


	class OpenGLTextureCube : public TOpengGLTexture< RHITextureCube >
	{
	public:
		bool loadFile(char const* path[]);
		bool create(Texture::Format format, int width, int height, void* data );
	};

	typedef TRefCountPtr< RHITextureBase > RHITextureRef;
	typedef TRefCountPtr< RHITexture1D > RHITexture1DRef;
	typedef TRefCountPtr< RHITexture2D > RHITexture2DRef;
	typedef TRefCountPtr< RHITexture3D > RHITexture3DRef;
	typedef TRefCountPtr< RHITextureCube > RHITextureCubeRef;

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
		static GLenum To(Blend::Operation op)
		{
			//TODO

		}
		static GLenum To(ECompareFun fun);
		static GLenum To(Stencil::Operation op);
		static GLenum To(ECullMode mode);
		static GLenum To(EFillMode mode);
		static GLenum To(ECompValueType type );
		static GLenum To(Sampler::Filter filter);
		static GLenum To(Sampler::AddressMode mode);
		
	};

	class AtomicCounterBuffer
	{
	public:
		AtomicCounterBuffer()
		{
			mHandle = 0;
		}
		~AtomicCounterBuffer()
		{
			glDeleteBuffers(1, &mHandle);
		}
		bool create()
		{
			glGenBuffers(1, &mHandle);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mHandle);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
			return true;
		}

		void bind(int idx = 0 )
		{

			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, idx, mHandle);
		}
		void unbind()
		{

			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, 0);
		}

		void setValue(GLuint value)
		{
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mHandle);
			GLuint* ptr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
													GL_MAP_WRITE_BIT |
													GL_MAP_INVALIDATE_BUFFER_BIT |
													GL_MAP_UNSYNCHRONIZED_BIT);
			ptr[0] = value;
			glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		}

		GLuint mHandle;
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


	class FrameBuffer
	{
	public:
		FrameBuffer();
		~FrameBuffer();

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
			bool   bManaged;
		};
		void setTextureInternal(int idx, GLuint handle, GLenum texType);
		void setDepthInternal(RHIResource& resource, GLuint handle, Texture::DepthFormat format, bool bTexture);


		std::vector< BufferInfo > mTextures;
		BufferInfo  mDepth;
		GLuint      mFBO;
	};

	struct Vertex
	{
		static int GetComponentNum(uint8 format)
		{
			return (format & 0x3) + 1;
		}
		static ECompValueType GetCompValueType(uint8 format)
		{
			return ECompValueType(format >> 2);
		}

		enum Format
		{
#define  ENCODE_VECTOR_FORAMT( TYPE , NUM ) (( TYPE << 2 ) | ( NUM - 1 ) )

			eFloat1 = ENCODE_VECTOR_FORAMT(CVT_Float,1),
			eFloat2 = ENCODE_VECTOR_FORAMT(CVT_Float,2),
			eFloat3 = ENCODE_VECTOR_FORAMT(CVT_Float,3),
			eFloat4 = ENCODE_VECTOR_FORAMT(CVT_Float,4),
			eHalf1 = ENCODE_VECTOR_FORAMT(CVT_Half, 1),
			eHalf2 = ENCODE_VECTOR_FORAMT(CVT_Half, 2),
			eHalf3 = ENCODE_VECTOR_FORAMT(CVT_Half, 3),
			eHalf4 = ENCODE_VECTOR_FORAMT(CVT_Half, 4),
			eUInt1  = ENCODE_VECTOR_FORAMT(CVT_UInt,1),
			eUInt2  = ENCODE_VECTOR_FORAMT(CVT_UInt,2),
			eUInt3  = ENCODE_VECTOR_FORAMT(CVT_UInt,3),
			eUInt4  = ENCODE_VECTOR_FORAMT(CVT_UInt,4),
			eInt1 = ENCODE_VECTOR_FORAMT(CVT_Int, 1),
			eInt2 = ENCODE_VECTOR_FORAMT(CVT_Int, 2),
			eInt3 = ENCODE_VECTOR_FORAMT(CVT_Int, 3),
			eInt4 = ENCODE_VECTOR_FORAMT(CVT_Int, 4),
			eUShort1 = ENCODE_VECTOR_FORAMT(CVT_UShort, 1),
			eUShort2 = ENCODE_VECTOR_FORAMT(CVT_UShort, 2),
			eUShort3 = ENCODE_VECTOR_FORAMT(CVT_UShort, 3),
			eUShort4 = ENCODE_VECTOR_FORAMT(CVT_UShort, 4),
			eShort1 = ENCODE_VECTOR_FORAMT(CVT_Short, 1),
			eShort2 = ENCODE_VECTOR_FORAMT(CVT_Short, 2),
			eShort3 = ENCODE_VECTOR_FORAMT(CVT_Short, 3),
			eShort4 = ENCODE_VECTOR_FORAMT(CVT_Short, 4),
			eUByte1 = ENCODE_VECTOR_FORAMT(CVT_UByte, 1),
			eUByte2 = ENCODE_VECTOR_FORAMT(CVT_UByte, 2),
			eUByte3 = ENCODE_VECTOR_FORAMT(CVT_UByte, 3),
			eUByte4 = ENCODE_VECTOR_FORAMT(CVT_UByte, 4),
			eByte1 = ENCODE_VECTOR_FORAMT(CVT_Byte, 1),
			eByte2 = ENCODE_VECTOR_FORAMT(CVT_Byte, 2),
			eByte3 = ENCODE_VECTOR_FORAMT(CVT_Byte, 3),
			eByte4 = ENCODE_VECTOR_FORAMT(CVT_Byte, 4),

#undef ENCODE_VECTOR_FORAMT
			eUnknowFormat ,
		};


		static GLenum GetComponentType(uint8 format);
		static int    GetFormatSize(uint8 format);
		enum Semantic
		{
			ePosition ,
			eTangent,
			eNormal,
			eColor ,
			eTexcoord ,
		};

		enum Attribute
		{
			ATTRIBUTE0,
			ATTRIBUTE1,
			ATTRIBUTE2,
			ATTRIBUTE3,
			ATTRIBUTE4,
			ATTRIBUTE5,
			ATTRIBUTE6,
			ATTRIBUTE7,
			ATTRIBUTE8,
			ATTRIBUTE9,
			ATTRIBUTE10,
			ATTRIBUTE11,
			ATTRIBUTE12,
			ATTRIBUTE13,
			ATTRIBUTE14,
			ATTRIBUTE15,


			// for NVidia
			//gl_Vertex	0
			//gl_Normal	2
			//gl_Color	3
			//gl_SecondaryColor	4
			//gl_FogCoord	5
			//gl_MultiTexCoord0	8
			//gl_MultiTexCoord1	9
			//gl_MultiTexCoord2	10
			//gl_MultiTexCoord3	11
			//gl_MultiTexCoord4	12
			//gl_MultiTexCoord5	13
			//gl_MultiTexCoord6	14
			//gl_MultiTexCoord7	15
			ATTRIBUTE_POSITION  = ATTRIBUTE0,
			ATTRIBUTE_TANGENT   = ATTRIBUTE15,
			ATTRIBUTE_NORMAL    = ATTRIBUTE2,
			ATTRIBUTE_COLOR     = ATTRIBUTE3,
			ATTRIBUTE_COLOR2    = ATTRIBUTE4,
			ATTRIBUTE_BONEINDEX = ATTRIBUTE6,
			ATTRIBUTE_BLENDWEIGHT = ATTRIBUTE7,
			ATTRIBUTE_TEXCOORD  = ATTRIBUTE8,
			ATTRIBUTE_TEXCOORD1 = ATTRIBUTE9,
			ATTRIBUTE_TEXCOORD2 = ATTRIBUTE10,
			ATTRIBUTE_TEXCOORD3 = ATTRIBUTE11,
			ATTRIBUTE_TEXCOORD4 = ATTRIBUTE12,
			ATTRIBUTE_TEXCOORD5 = ATTRIBUTE13,
			ATTRIBUTE_TEXCOORD6 = ATTRIBUTE14,
			//ATTRIBUTE_TEXCOORD7 = ATTRIBUTE15,
		};
	};

	enum TextureCreationFlag : uint32
	{
		TCF_CreateSRV = BIT(0),
		TCF_CreateUAV = BIT(1),
		TCF_RenderTarget = BIT(2),
	};

	enum BufferCreationFlag : uint32
	{
		BCF_UsageDynamic = BIT(0),
		BCF_UsageStage = BIT(1),
	};


	struct VertexElement
	{
		uint8 idxStream;
		uint8 attribute;
		uint8 format;
		uint8 offset;
		uint8 semantic;
		uint8 idx;
		bool  bNormalize;
	};

	class VertexDecl
	{
	public:
		VertexDecl();

		void  setSize( uint8 size ){ mVertexSize = size; }
		uint8 getVertexSize() const { return mVertexSize; }
		int   getSematicOffset( Vertex::Semantic s ) const;
		int   getSematicOffset( Vertex::Semantic s , int idx ) const;
		Vertex::Format  getSematicFormat( Vertex::Semantic s ) const;
		Vertex::Format  getSematicFormat( Vertex::Semantic s , int idx ) const;
		uint8 getOffset( int idx ) const { return mElements[idx].offset; }

		VertexDecl&   addElement( Vertex::Semantic s , Vertex::Format f , uint8 idx = 0 );
		VertexDecl&   addElement( uint8 attribute ,  Vertex::Format f,  bool bNormailze = false );
		VertexDecl&   addElement( uint8 idxStream , Vertex::Semantic s, Vertex::Format f, uint8 idx = 0);
		VertexDecl&   addElement( uint8 idxStream , uint8 attribute, Vertex::Format f, bool bNormailze = false);

		void bindPointer(LinearColor const* overwriteColor = nullptr);
		void unbindPointer(LinearColor const* overwriteColor = nullptr);

		void bindAttrib(LinearColor const* overwriteColor = nullptr);
		void unbindAttrib(LinearColor const* overwriteColor = nullptr);

		VertexElement const*   findBySematic( Vertex::Semantic s , int idx ) const;
		VertexElement const*   findBySematic( Vertex::Semantic s ) const;

		std::vector< VertexElement > mElements;
		uint8   mVertexSize;
	};

	class RHIBufferBase : public RHIResource
	{
	public:
		RHIBufferBase()
		{
			mBufferSize = 0;
			mCreationFlags = 0;
		}
		uint32 getSize() const { return mBufferSize; }

		virtual void* lock(ELockAccess access) = 0;
		virtual void* lock(ELockAccess access, uint32 offset, uint32 length) = 0;
		virtual void  unlock() = 0;

	protected:
		uint32 mCreationFlags;
		uint32 mBufferSize;
	};


	class RHIVertexBuffer;
	class RHIIndexBuffer;
	class RHIUniformBuffer;
	class RHIStorageBuffer;

	template< class RHITextureType >
	struct OpenGLBufferTraits {};

	template<>
	struct OpenGLBufferTraits< RHIVertexBuffer >{  static GLenum constexpr EnumValue = GL_ARRAY_BUFFER;  };
	template<>
	struct OpenGLBufferTraits< RHIIndexBuffer > { static GLenum constexpr EnumValue = GL_ELEMENT_ARRAY_BUFFER; };
	template<>
	struct OpenGLBufferTraits< RHIUniformBuffer > { static GLenum constexpr EnumValue = GL_UNIFORM_BUFFER; };
	template<>
	struct OpenGLBufferTraits< RHIStorageBuffer > { static GLenum constexpr EnumValue = GL_SHADER_STORAGE_BUFFER; };


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

		bool createInternal(uint32 size, uint32 creationFlags , void* initData)
		{
			if( !mGLObject.fetchHandle() )
				return false;

			return resetDataInternal(size, creationFlags, initData);
		}

		bool resetDataInternal(uint32 size, uint32 creationFlags, void* initData)
		{
			glBindBuffer(GLBufferType, getHandle());
			glBufferData(GLBufferType, size, initData, GetUsageEnum(creationFlags));
			glBindBuffer(GLBufferType, 0);

			mCreationFlags = creationFlags;
			mBufferSize = size;
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

	class RHIVertexBuffer : public RHIBufferBase
	{
	public:
		RHIVertexBuffer()
		{
			mNumVertices = 0;
			mVertexSize = 0;
		}

		virtual void  resetData(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data) = 0;
		virtual void  updateData(uint32 vStart, uint32 numVertices, void* data) = 0;

		uint32 mNumVertices;
		uint32 mVertexSize;

	};

	class OpenGLVertexBuffer : public TOpenGLBuffer< RHIVertexBuffer >
	{
	public:
		bool  create(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		void  resetData(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data);
		void  updateData(uint32 vStart, uint32 numVertices, void* data);
	};

	class RHIIndexBuffer : public RHIBufferBase
	{
	public:
		RHIIndexBuffer()
		{
			mNumIndices = 0;
			mbIntIndex = false;
		}

		bool   mbIntIndex;
		uint32 mNumIndices;
	};


	class OpenGLIndexBuffer : public TOpenGLBuffer< RHIIndexBuffer >
	{
	public:

		bool create(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
		{
			if( !createInternal((bIntIndex ? sizeof(GLuint) : sizeof(GLushort)) * nIndices, creationFlags , data) )
				return false;
			mNumIndices = nIndices;
			mbIntIndex = bIntIndex;
			return true;
		}
	};

	class RHIUniformBuffer : public RHIBufferBase
	{
	public:
	};

	class OpenGLUniformBuffer : public TOpenGLBuffer< RHIUniformBuffer >
	{
	public:
		bool create(uint32 size, uint32 creationFlags, void* data);
	};

	class RHIStorageBuffer : public RHIBufferBase
	{
	public:
	};

	class OpenGLStorageBuffer : public TOpenGLBuffer< RHIStorageBuffer >
	{
	public:
		bool create(uint32 size, uint32 creationFlags, void* data);
	};

	struct SamplerStateInitializer
	{
		Sampler::Filter filter;
		Sampler::AddressMode addressU;
		Sampler::AddressMode addressV;
		Sampler::AddressMode addressW;
	};





	class RHISamplerState : public RHIResource
	{

	};


	class OpenGLSamplerState : public TOpenGLResource< RHISamplerState, RMPSamplerObject >
	{
	public:
		bool create(SamplerStateInitializer const& initializer);
	};

	typedef TRefCountPtr< RHISamplerState > RHISamplerStateRef;

	typedef TRefCountPtr< RHIVertexBuffer > RHIVertexBufferRef;
	typedef TRefCountPtr< RHIIndexBuffer > RHIIndexBufferRef;
	typedef TRefCountPtr< RHIUniformBuffer > RHIUniformBufferRef;
	typedef TRefCountPtr< RHIStorageBuffer > RHIStorageBufferRef;


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
		static OpenGLStorageBuffer* To(RHIStorageBuffer* buffer) { return static_cast<OpenGLStorageBuffer*>(buffer); }

		static OpenGLSamplerState* To(RHISamplerState* state ) { return static_cast<OpenGLSamplerState*>(state); }

		static RHIShader* To(RHIShader* shader) { return shader; }
		static ShaderProgram* To(ShaderProgram* shader) { return shader; }
		static FrameBuffer* To(FrameBuffer* buffer) { return buffer; }

		template < class T >
		static auto To(TRefCountPtr<T>& ptr) { return To(ptr.get()); }

		template < class T >
		static GLuint GetHandle(T& RHIObject)
		{
			return OpenGLCast::To(&RHIObject)->getHandle();
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
	RenderGL::TBindLockScope< std::remove_reference< decltype(obj) >::type >  ANONYMOUS_VARIABLE( BindLockObj )( obj );



}

#endif // GLCommon_h__
