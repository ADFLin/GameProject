#ifndef GLCommon_h__
#define GLCommon_h__

#include "BaseType.h"
#include "RHIDefine.h"

#include "glew/GL/glew.h"
#include "GLConfig.h"

#include "CppVersion.h"


#include "LogSystem.h"
#include "TVector2.h"

#include <vector>
#include <functional>

#include "RefCount.h"


namespace RenderGL
{

	template< class RMPolicy >
	class TGLObjectBase
	{
	public:
		TGLObjectBase()
		{
			mHandle = 0;
		}

		~TGLObjectBase()
		{
			destroy();
		}

		bool isValid() { return mHandle != 0; }

		void release()
		{
			destroy();
		}

		void destroy()
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



#if CPP_VARIADIC_TEMPLATE_SUPPORT
		template< class ...Args >
		bool fetchHandle(Args&& ...args)
		{
			if( !mHandle )
				RMPolicy::Create(mHandle, std::forward(args)...);
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
		GLuint mHandle;
	};

	struct RMPTexture
	{
		static void Create(GLuint& handle) { glGenTextures(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteTextures(1, &handle); }
	};
	
	struct RMPShader
	{
		static void Create(GLuint& handle, GLenum type) { handle = glCreateShader(type); }
		static void Destroy(GLuint& handle) { glDeleteShader(handle); }
	};	
	
	struct RMPShaderProgram
	{
		static void Create(GLuint& handle) { handle = glCreateProgram(); }
		static void Destroy(GLuint& handle) { glDeleteProgram(handle); }
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

	class RHIResource : public RefCountedObjectT< RHIResource >
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
		virtual void releaseRHI(){}
		void destroyThis()
		{
			releaseRHI();
			delete this;
		}

		static uint32 TotalCount;
	};

	typedef  TRefCountPtr< RHIResource > RHIResourceRef;


	template< class RMPolicy >
	class TRHIResource : public RHIResource
		               , public TGLObjectBase< RMPolicy >
	{
	public:
		virtual void releaseRHI() { TGLObjectBase< RMPolicy >::destroy();  }
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

	struct SamplerStateInitializer
	{
		Sampler::Filter filter;
		Sampler::AddressMode addressU;
		Sampler::AddressMode addressV;
		Sampler::AddressMode addressW;
	};
	class RHISamplerState : public TRHIResource< RMPSamplerObject >
	{
	public:
		bool create(SamplerStateInitializer const& initializer);
	};

	typedef TRefCountPtr< RHISamplerState > RHISamplerStateRef;

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



	class RHITextureBase : public TRHIResource< RMPTexture >
	{
	public:
		virtual ~RHITextureBase() {}
	protected:
		bool loadFileInternal(char const* path, GLenum texType, GLenum texImageType, IntVector2& outSize , Texture::Format& outFormat);
	};


	template< GLenum TYPE_NAME_GL >
	class TRHITextureBase : public RHITextureBase
	{
	public:
		static GLenum const TypeNameGL = TYPE_NAME_GL;
		void bind()
		{
			glBindTexture(TypeNameGL, mHandle);
		}

		void unbind()
		{
			glBindTexture(TypeNameGL, 0);
		}
		Texture::Format getFormat() { return mFormat; }
	protected:
		Texture::Format mFormat;
	};

	class RenderTarget
	{

	};

	class RHITexture1D : public TRHITextureBase< GL_TEXTURE_1D >
	{
	public:
		bool create(Texture::Format format, int length, int numMipLevel = 0, void* data = nullptr);
		int  getSize() const { return mSize; }
		bool update(int offset, int length, Texture::Format format, void* data, int level = 0);
	private:
		int mSize;
	};


	class RHITexture2D : public TRHITextureBase< GL_TEXTURE_2D >
	{
	public:
		bool loadFromFile( char const* path );
		bool create( Texture::Format format, int width, int height , int numMipLevel = 0 ,void* data = nullptr , int alignment = 0);
		int  getSizeX() const { return mSizeX; }
		int  getSizeY() const { return mSizeY; }
		bool update(int ox, int oy, int w, int h , Texture::Format format , void* data , int level = 0 );
		bool update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level = 0);
	private:
		
		int mSizeX;
		int mSizeY;
	};



	class RHITexture3D : public TRHITextureBase< GL_TEXTURE_3D >
	{
	public:
		bool create(Texture::Format format, int sizeX , int sizeY , int sizeZ );
		int  getSizeX() { return mSizeX; }
		int  getSizeY() { return mSizeY; }
		int  getSizeZ() { return mSizeZ; }
	private:
		int mSizeX;
		int mSizeY;
		int mSizeZ;
	};

	class RHITextureCube : public TRHITextureBase< GL_TEXTURE_CUBE_MAP >
	{
	public:
		bool loadFile( char const* path[] );
		bool create( Texture::Format format, int width, int height, void* data = nullptr );
	};

	typedef TRefCountPtr< RHITextureBase > RHITextureRef;
	typedef TRefCountPtr< RHITexture1D > RHITexture1DRef;
	typedef TRefCountPtr< RHITexture2D > RHITexture2DRef;
	typedef TRefCountPtr< RHITexture3D > RHITexture3DRef;
	typedef TRefCountPtr< RHITextureCube > RHITextureCubeRef;

	class RHITextureDepth : public TRHITextureBase< GL_TEXTURE_2D >
	{
	public:
		bool create(Texture::DepthFormat format, int width, int height);
		Texture::DepthFormat getFormat() { return mFromat; }
		Texture::DepthFormat mFromat;
	};
	typedef TRefCountPtr< RHITextureDepth > RHITextureDepthRef;

	struct Buffer
	{
		enum Usage
		{
			eStatic,
			eDynamic,
		};
	};


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
		static GLenum To(Buffer::Usage usage);
		
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

	class RHIDepthRenderBuffer : public TRHIResource< RMPRenderBuffer >
	{
	public:
		bool create( int w , int h , Texture::DepthFormat format );
		Texture::DepthFormat getFormat() { return mFormat;  }
		Texture::DepthFormat mFormat;

	};

	typedef TRefCountPtr< RHIDepthRenderBuffer > RHIDepthRenderBufferRef;

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
		void setDepth( RHIDepthRenderBuffer& buffer  );
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

	template< uint32 GLBufferType >
	class TRHIBufferBase : public TRHIResource< RMPBufferObject >
	{
	public:
		void  bind() { glBindBuffer(GLBufferType, mHandle); }
		void  unbind() { glBindBuffer(GLBufferType, 0); }

		void* lock(ELockAccess access)
		{
			glBindBuffer(GLBufferType, mHandle);
			void* result = glMapBuffer(GLBufferType, GLConvert::To(access));
			glBindBuffer(GLBufferType, 0);
			return result;
		}
		void* lock(ELockAccess access, uint32 offset, uint32 length)
		{
			glBindBuffer(GLBufferType, mHandle);

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

		bool createInternal( uint32 size , void* initData , GLenum usageType )
		{
			if( !fetchHandle() )
				return false;

			glBindBuffer(GLBufferType, mHandle);
			glBufferData(GLBufferType, size , initData , usageType);
			glBindBuffer(GLBufferType, 0);

			mBufferSize = size;
			return true;
		}

		void unlock()
		{
			glBindBuffer(GLBufferType, mHandle);
			glUnmapBuffer(GLBufferType);
			glBindBuffer(GLBufferType, 0);
		}

		uint32 getSize() const { return mBufferSize; }

		uint32 mBufferSize;
	};

	class RHIVertexBuffer : public TRHIBufferBase< GL_ARRAY_BUFFER >
	{
	public:
		RHIVertexBuffer()
		{
			mBufferSize = 0;
			mNumVertices = 0;
			mVertexSize = 0;
		}
		bool  create();
		bool  create( uint32 vertexSize,  uint32 numVertices , void* data = nullptr , Buffer::Usage usage = Buffer::eStatic );
		
		void  resetData(uint32 vertexSize, uint32 numVertices, void* data = nullptr, Buffer::Usage usage = Buffer::eStatic);
		void  updateData(uint32 vStart, uint32 numVertices, void* data);

		uint32 mNumVertices;
		uint32 mVertexSize;

	};

	class RHIIndexBuffer : public TRHIBufferBase< GL_ELEMENT_ARRAY_BUFFER >
	{
	public:
		RHIIndexBuffer()
		{
			mNumIndices = 0;
			mbIntIndex = false;
		}
		bool create(uint32 nIndices, bool bIntIndex , void* data )
		{
			if ( !createInternal( (bIntIndex ? sizeof(GLuint) : sizeof(GLushort)) * nIndices , data , GL_STATIC_DRAW ) )
				return false;

			mNumIndices = nIndices;
			mbIntIndex = bIntIndex;
			return true;
		}
		void bind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mHandle); }
		void unbind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

		bool   mbIntIndex;
		uint32 mNumIndices;
	};

	class RHIUniformBuffer : public TRHIBufferBase< GL_UNIFORM_BUFFER >
	{
	public:
		bool create(uint32 size);
	};


	class RHIStorageBuffer : public TRHIBufferBase< GL_SHADER_STORAGE_BUFFER >
	{
	public:
		bool create(uint32 size);
	};

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

	template< class T >
	struct TBindLockScope
	{
		TBindLockScope(T& obj)
			:mObj(obj)
		{
			mObj.bind();
		}

		~TBindLockScope()
		{
			mObj.unbind();
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
			mObj->bind();
		}

		~TBindLockScope()
		{
			mObj->unbind();
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
			mObj->bind();
		}

		~TBindLockScope()
		{
			mObj->unbind();
		}

		TRefCountPtr< T >& mObj;
	};

#define GL_BIND_LOCK_OBJECT( obj )\
	RenderGL::TBindLockScope< decltype(obj) >  ANONYMOUS_VARIABLE( BindLockObj )( obj );

}

#endif // GLCommon_h__
