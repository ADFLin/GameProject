#ifndef GLCommon_h__
#define GLCommon_h__

#include "BaseType.h"
#include "RHIDefine.h"

#include "GL/glew.h"
#include "GLConfig.h"

#include "CppVersion.h"


#include "LogSystem.h"
#include "TVector2.h"

#include <vector>
#include <functional>

#include "RefCount.h"

#define SHADER_ENTRY( NAME ) #NAME
#define SHADER_PARAM( NAME ) #NAME

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

	class RHIResource : public RefCountedObjectT< RHIResource >
	{
	public:
		virtual ~RHIResource(){}
		virtual void releaseRHI(){ }

		void destroyThis()
		{
			releaseRHI();
			delete this;
		}
	};

	typedef  TRefCountPtr< RHIResource > RHIResourceRef;


	template< class RMPolicy >
	class TRHIResource : public RHIResource
		               , public TGLObjectBase< RMPolicy >
	{
	public:
		virtual void releaseRHI() { TGLObjectBase< RMPolicy >::destroy();  }
	};


	enum VectorComponentType
	{
		VCT_Float,
		VCT_Half,
		VCT_UInt,
		VCT_Int,
		VCT_UShort,
		VCT_Short,
		VCT_UByte,
		VCT_Byte,

		//
		VCT_NInt8,
		VCT_NInt16,

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
		bool loadFileInternal(char const* path, GLenum texType, GLenum texImageType, Vec2i& outSize , Texture::Format& outFormat);
	};

	class RenderTarget
	{

	};

	class RHITexture2D : public RHITextureBase
	{
	public:
		bool loadFromFile( char const* path );
		bool create( Texture::Format format, int width, int height , int numMipLevel = 0 ,void* data = nullptr , int alignment = 0);
		int  getSizeX() const { return mSizeX; }
		int  getSizeY() const { return mSizeY; }
		void bind();
		void unbind();

		bool update(int ox, int oy, int w, int h , Texture::Format format , void* data , int level = 0 );
		bool update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level = 0);
		Texture::Format getFormat() { return mFormat; }
	private:
		Texture::Format mFormat;
		int mSizeX;
		int mSizeY;
	};



	class RHITexture3D : public RHITextureBase
	{
	public:
		bool create(Texture::Format format, int sizeX , int sizeY , int sizeZ );
		int  getSizeX() { return mSizeX; }
		int  getSizeY() { return mSizeY; }
		int  getSizeZ() { return mSizeZ; }
		void bind();
		void unbind();

	private:
		int mSizeX;
		int mSizeY;
		int mSizeZ;
	};

	class RHITextureCube : public RHITextureBase
	{
	public:
		bool loadFile( char const* path[] );
		bool create( Texture::Format format, int width, int height, void* data = nullptr );
		void bind();
		void unbind();
	};

	typedef TRefCountPtr< RHITextureBase > RHITextureRef;
	typedef TRefCountPtr< RHITexture2D > RHITexture2DRef;
	typedef TRefCountPtr< RHITexture3D > RHITexture3DRef;
	typedef TRefCountPtr< RHITextureCube > RHITextureCubeRef;

	class RHITextureDepth : public RHITextureBase
	{
	public:
		bool create(Texture::DepthFormat format, int width, int height);
		void bind();
		void unbind();
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
		static GLenum To(AccessOperator op);
		static GLenum To(Texture::Format format);
		static GLenum To(PrimitiveType type);
		static GLenum To(Shader::Type type);
		static GLenum To(ELockAccess access);
		static GLenum To(Blend::Factor factor);
		static GLenum To(Blend::Operation op)
		{


		}
		static GLenum To(ECompareFun fun);
		static GLenum To(Stencil::Operation op);
		
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
		static VectorComponentType GetVectorComponentType(uint8 format)
		{
			return VectorComponentType(format >> 2);
		}

		enum Format
		{
#define  ENCODE_VECTOR_FORAMT( TYPE , NUM ) (( TYPE << 2 ) | ( NUM - 1 ) )

			eFloat1 = ENCODE_VECTOR_FORAMT(VCT_Float,1),
			eFloat2 = ENCODE_VECTOR_FORAMT(VCT_Float,2),
			eFloat3 = ENCODE_VECTOR_FORAMT(VCT_Float,3),
			eFloat4 = ENCODE_VECTOR_FORAMT(VCT_Float,4),
			eHalf1 = ENCODE_VECTOR_FORAMT(VCT_Half, 1),
			eHalf2 = ENCODE_VECTOR_FORAMT(VCT_Half, 2),
			eHalf3 = ENCODE_VECTOR_FORAMT(VCT_Half, 3),
			eHalf4 = ENCODE_VECTOR_FORAMT(VCT_Half, 4),
			eUInt1  = ENCODE_VECTOR_FORAMT(VCT_UInt,1),
			eUInt2  = ENCODE_VECTOR_FORAMT(VCT_UInt,2),
			eUInt3  = ENCODE_VECTOR_FORAMT(VCT_UInt,3),
			eUInt4  = ENCODE_VECTOR_FORAMT(VCT_UInt,4),
			eInt1 = ENCODE_VECTOR_FORAMT(VCT_Int, 1),
			eInt2 = ENCODE_VECTOR_FORAMT(VCT_Int, 2),
			eInt3 = ENCODE_VECTOR_FORAMT(VCT_Int, 3),
			eInt4 = ENCODE_VECTOR_FORAMT(VCT_Int, 4),
			eUShort1 = ENCODE_VECTOR_FORAMT(VCT_UShort, 1),
			eUShort2 = ENCODE_VECTOR_FORAMT(VCT_UShort, 2),
			eUShort3 = ENCODE_VECTOR_FORAMT(VCT_UShort, 3),
			eUShort4 = ENCODE_VECTOR_FORAMT(VCT_UShort, 4),
			eShort1 = ENCODE_VECTOR_FORAMT(VCT_Short, 1),
			eShort2 = ENCODE_VECTOR_FORAMT(VCT_Short, 2),
			eShort3 = ENCODE_VECTOR_FORAMT(VCT_Short, 3),
			eShort4 = ENCODE_VECTOR_FORAMT(VCT_Short, 4),
			eUByte1 = ENCODE_VECTOR_FORAMT(VCT_UByte, 1),
			eUByte2 = ENCODE_VECTOR_FORAMT(VCT_UByte, 2),
			eUByte3 = ENCODE_VECTOR_FORAMT(VCT_UByte, 3),
			eUByte4 = ENCODE_VECTOR_FORAMT(VCT_UByte, 4),
			eByte1 = ENCODE_VECTOR_FORAMT(VCT_Byte, 1),
			eByte2 = ENCODE_VECTOR_FORAMT(VCT_Byte, 2),
			eByte3 = ENCODE_VECTOR_FORAMT(VCT_Byte, 3),
			eByte4 = ENCODE_VECTOR_FORAMT(VCT_Byte, 4),

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
		uint8 getOffset( int idx ) const { return mInfoVec[idx].offset; }

		VertexDecl&   addElement( Vertex::Semantic s , Vertex::Format f , uint8 idx = 0 );
		VertexDecl&   addElement( uint8 attribute ,  Vertex::Format f,  bool bNormailze = false );


		void bind();
		void unbind();

		void bindAttrib();
		void unbindAttrib();

		VertexElement const*   findBySematic( Vertex::Semantic s , int idx ) const;
		VertexElement const*   findBySematic( Vertex::Semantic s ) const;
		typedef std::vector< VertexElement > InfoVec;
		InfoVec mInfoVec;
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

		void unlock()
		{
			glBindBuffer(GLBufferType, mHandle);
			glUnmapBuffer(GLBufferType);
			glBindBuffer(GLBufferType, 0);
		}
	};

	class RHIVertexBuffer : public TRHIBufferBase< GL_ARRAY_BUFFER >
	{
	public:
		RHIVertexBuffer()
		{
			mBufferSize = 0;
			mNumVertices = 0;
		}
		bool  create( uint32 vertexSize,  uint32 numVertices , void* data = nullptr , Buffer::Usage usage = Buffer::eStatic );
		void  updateData(uint32 vertexSize, uint32 numVertices, void* data );

		uint32 mBufferSize;
		uint32 mNumVertices;

	};

	class RHIIndexBuffer : public TRHIBufferBase< GL_ELEMENT_ARRAY_BUFFER >
	{
	public:
		RHIIndexBuffer()
		{
			mNumIndices = 0;
			mbIntIndex = false;
		}
		bool create(int nIndices, bool bIntIndex , void* data )
		{
			if( !fetchHandle() )
				return false;

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mHandle);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, ((bIntIndex) ? sizeof(GLuint) : sizeof(GLushort)) * nIndices, data , GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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
		bool create(int size);

		int mSize;
	};

	typedef TRefCountPtr< RHIVertexBuffer > RHIVertexBufferRef;
	typedef TRefCountPtr< RHIIndexBuffer > RHIIndexBufferRef;
	typedef TRefCountPtr< RHIUniformBuffer > RHIUniformBufferRef;


	struct PrimitiveDebugInfo
	{
		int32 numVertex;
		int32 numIndex;
		int32 numElement;
	};

	class Mesh
	{
	public:

		Mesh();
		~Mesh();

		bool createBuffer( void* pVertex  , int nV , void* pIdx = nullptr , int nIndices = 0 , bool bIntIndex = false );
		void draw(bool bUseVAO = false);
		void draw(Matrix4 const& transform, bool bUseVAO);
		void drawSection(int idx , bool bUseVAO = false);

		void drawInternal(int idxStart , int num , bool bUseVAO);

		void bindVAO();
		void unbindVAO()
		{
			glBindVertexArray(0);
		}

		PrimitiveType       mType;
		VertexDecl          mDecl;
		RHIVertexBufferRef  mVertexBuffer;
		RHIIndexBufferRef   mIndexBuffer;
		uint32              mVAO;

		struct Section
		{
			int start;
			int num;
		};
		std::vector< Section > mSections;
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
