#ifndef GLCommon_h__
#define GLCommon_h__

#include "GL/glew.h"
#include "GLConfig.h"

#include "LogSystem.h"
#include "IntegerType.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/Matrix3.h"
#include "Math/Quaternion.h"

#include "TVector2.h"

#include <vector>
#include <functional>

#include "RefCount.h"

#define SHADER_ENTRY( NAME ) #NAME
#define SHADER_PARAM( NAME ) #NAME

namespace GL
{
	typedef ::Math::Vector3 Vector3;
	typedef ::Math::Vector4 Vector4;
	typedef ::Math::Quaternion Quaternion;
	typedef ::Math::Matrix4 Matrix4;
	typedef ::Math::Matrix3 Matrix3;

	template< class T >
	class TGLDataMove
	{

	};

	template< class RMPolicy >
	class TGLObjectBase
	{
	public:
		TGLObjectBase()
		{
			mHandle = 0;
			mbManaged = true;
		}

		~TGLObjectBase()
		{
			destroy();
		}

		bool isVaildate() { return mHandle != 0; }

		void release()
		{
			destroy();
		}

		void destroy()
		{
			if( mbManaged && mHandle )
			{
				RMPolicy::destroy(mHandle);

				GLenum error = glGetError();
				if( error != GL_NO_ERROR )
				{
					//#TODO
					::Msg("Can't Destory GL Object");
				}
			}
			mHandle = 0;
		}

		bool fetchHandle()
		{
			if( !mHandle )
				RMPolicy::create(mHandle);
			return mHandle != 0;
		}

		template< class P1 >
		bool fetchHandle(P1 p1)
		{
			if( !mHandle )
				RMPolicy::create(mHandle, p1);
			return mHandle != 0;
		}

		GLuint releaseResource() { mbManaged = false; return mHandle; }


		bool   mbManaged;
		GLuint mHandle;
	};

	template< class T , class RMPolicy >
	class RHIResource : public RefCountedObjectT< T >
		              , public TGLObjectBase< RMPolicy >
	{
	public:
		RHIResource()
		{
			mHandle = 0;
			mbManaged = true;
		}

		~RHIResource()
		{
			destroy();
		}

		void destroyThis() 
		{ 
			delete static_cast< T*>(this); 
		}
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
			eR32F ,
			eRGB32F ,
			eRGBA32F ,
			eRGB16F,
			eRGBA16F ,

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
		static GLenum Convert( Format format )
		{
			switch( format )
			{
			case eRGBA8:   return GL_RGBA8;
			case eRGB8:    return GL_RGB8;
			case eR32F:    return GL_R32F;
			case eRGB32F:  return GL_RGB32F;
			case eRGBA32F: return GL_RGBA32F;
			case eRGB16F:  return GL_RGB16F;
			case eRGBA16F:  return GL_RGBA16F;
			}
			return 0;
		}

		static GLenum GetBaseFormat(Format format)
		{
			switch( format )
			{
			case eRGB8:
			case eRGB32F:
			case eRGB16F:
				return GL_RGB;
			case eRGBA8:
			case eRGBA32F:
			case eRGBA16F:
				return GL_RGBA;
			case eR32F:   
				return GL_RED;
			}
			return 0;
		}

		static GLenum GetFormatType(Format format)
		{
			switch( format )
			{
			case eRGB8:
			case eRGBA8:
				return GL_UNSIGNED_BYTE;
			case eRGB32F:
			case eRGB16F:
			case eRGBA32F:
			case eRGBA16F:
			case eR32F:
				return GL_FLOAT;
			}
			return 0;
		}

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

		static GLenum Convert( DepthFormat format )
		{
			switch( format )
			{
			case eDepth16: return GL_DEPTH_COMPONENT16;
			case eDepth24: return GL_DEPTH_COMPONENT24;
			case eDepth32: return GL_DEPTH_COMPONENT32;
			case eDepth32F:return GL_DEPTH_COMPONENT32F;
			case eD24S8:   return GL_DEPTH24_STENCIL8;
			case eD32FS8:  return GL_DEPTH32F_STENCIL8;
			case eStencil1: return GL_STENCIL_INDEX1;
			case eStencil4: return GL_STENCIL_INDEX4;
			case eStencil8: return GL_STENCIL_INDEX8;
			case eStencil16: return GL_STENCIL_INDEX16;
			}
			return 0;
		}

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

	struct RMPTexture
	{  
		static void create( GLuint& handle ){ glGenTextures( 1 , &handle ); }
		static void destroy( GLuint& handle ){ glDeleteTextures( 1 , &handle ); }
	};

	class RHITextureBase : public RHIResource< RHITextureBase , RMPTexture >
	{
	public:
		virtual ~RHITextureBase() {}
	protected:
		bool loadFileInternal(char const* path, GLenum texType, Vec2i& outSize);
	};

	class RenderTarget
	{

	};

	class RHITexture2D : public RHITextureBase
	{
	public:
		bool loadFile( char const* path );
		bool create( Texture::Format format , int width , int height );
		bool create( Texture::Format format, int width, int height , void* data );
		int  getSizeX() { return mSizeX; }
		int  getSizeY() { return mSizeY; }
		void bind();
		void unbind();

	private:
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
		bool create( Texture::Format format , int width , int height );
		bool create( Texture::Format format, int width, int height, void* data);
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

	struct RMPShader
	{  
		static void create( GLuint& handle , GLenum type ){ handle = glCreateShader( type ); }
		static void destroy( GLuint& handle ){ 	glDeleteShader( handle ); }
	};

	class RHIShader : public RHIResource< RHIShader , RMPShader >
	{
	public:
		enum Type
		{
			eUnknown  = -1,
			eVertex   = 0,
			ePixel    = 1,
			eGemotery = 2,
			eCompute  = 3,
			NUM_SHADER_TYPE ,
		};


		bool loadFile( Type type , char const* path , char const* def = NULL );
		Type getType();
		bool compileSource( Type type , char const* src[] , int num );
		bool create( Type type );
		void destroy();

		GLuint getParam( GLuint val );
		friend class ShaderProgram;
	};

	
	typedef TRefCountPtr< RHIShader > RHIShaderRef;

	struct RMPShaderProgram
	{
		static void create(GLuint& handle) { handle = glCreateProgram(); }
		static void destroy(GLuint& handle) { glDeleteProgram(handle); }
	};

	class ShaderProgram : public TGLObjectBase< RMPShaderProgram >
	{
	public:
		ShaderProgram();

		~ShaderProgram();


		bool create();

		RHIShader* removeShader( RHIShader::Type type );
		void    attachShader( RHIShader& shader );
		void    updateShader(bool bForce = false);

		void checkProgramStatus();

		void    bind();
		void    unbind();

		void setParam( char const* name , float v1 )
		{
			int loc = getParamLoc( name );
			if( loc != -1 ) 
			{ 
				glUniform1f(loc, v1); 
			}
		}
		void setParam( char const* name , float v1 , float v2 )
		{
			int loc = getParamLoc( name );
			if( loc != -1 )
			{  
				glUniform2f(loc, v1, v2); 
			}
		}
		void setParam( char const* name , float v1 , float v2 , float v3 )
		{
			int loc = getParamLoc( name );
			if( loc != -1 )
			{
				glUniform3f(loc, v1, v2, v3);
			}
		}
		void setParam(char const* name, float v1, float v2, float v3 , float v4)
		{
			int loc = getParamLoc(name);
			if( loc != -1 )
			{
				glUniform4f(loc, v1, v2, v3, v4);
			}
		}
		void setParam( char const* name , int v1 )
		{
			int loc = glGetUniformLocation( mHandle , name );
			if( loc != -1 )
			{
				glUniform1i(loc, v1);
			}
		}

		void setMatrix33( char const* name , float const* value, int num = 1)
		{
			int loc = glGetUniformLocation( mHandle , name );
			if( loc != -1 )
			{
				glUniformMatrix3fv(loc, num, false, value);
			}
		}
		void setMatrix44( char const* name , float const* value , int num = 1)
		{
			int loc = glGetUniformLocation( mHandle , name );
			if( loc != -1 )
			{
				glUniformMatrix4fv(loc, num, false, value);
			}
		}
		void setParam(char const* name, float const v[] , int num ) 
		{ 
			int loc = glGetUniformLocation(mHandle, name);
			if( loc != -1 )
			{
				glUniform1fv(loc, num, v);
			}
		}

		void setVector3(char const* name, float const v[], int num)
		{
			int loc = glGetUniformLocation(mHandle, name);
			if( loc != -1 )
			{
				glUniform3fv(loc, num, v);
			}
		}

		void setVector4(char const* name, float const v[], int num)
		{
			int loc = glGetUniformLocation(mHandle, name);
			if( loc != -1 )
			{
				glUniform4fv(loc, num, v);
			}
		}

		void setParam( char const* name , Vector3 const& v ){ setParam( name , v.x , v.y , v.z ); }
		void setParam( char const* name , Vector4 const& v) { setParam( name, v.x, v.y, v.z , v.w ); }
		void setParam( char const* name , Matrix4 const& m ){ setMatrix44( name , m , 1 ); }
		void setParam( char const* name , Matrix3 const& m ){ setMatrix33( name , m , 1 ); }
		void setParam( char const* name, Matrix4 const v[], int num) { setMatrix44(name, v[0], num);  }
		void setParam( char const* name, Vector3 const v[], int num) { setVector3(name, (float*)&v[0], num); }
		void setParam( char const* name, Vector4 const v[], int num) { setVector4(name, (float*)&v[0], num); }

		void setTexture( char const* name , RHITexture2D& tex , int idx = -1 )
		{ 
			return setTextureInternal( name , GL_TEXTURE_2D , tex.mHandle , idx ); 
		}
		void setTexture(char const* name, RHITextureDepth& tex, int idx = -1)
		{
			return setTextureInternal(name, GL_TEXTURE_2D, tex.mHandle, idx);
		}
		void setTexture( char const* name , RHITextureCube& tex , int idx = -1 )
		{
			return setTextureInternal( name , GL_TEXTURE_CUBE_MAP , tex.mHandle , idx ); 
		}
		void setTexture(char const* name, RHITexture3D& tex, int idx = -1)
		{
			return setTextureInternal(name, GL_TEXTURE_3D, tex.mHandle, idx);
		}
#if 0 //#TODO Can't Bind to texture 2d
		void setTexture2D( char const* name , TextureCube& tex , Texture::Face face , int idx = -1 )
		{
			glActiveTexture( GL_TEXTURE0 + idx );
			glBindTexture( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face , tex.mHandle );
			setParam( name , idx );
			glActiveTexture( GL_TEXTURE0 );
		}
#endif

		void setTexture2D( char const* name , GLuint idTex , int idx = -1 ){ setTextureInternal( name , GL_TEXTURE_2D , idTex , idx ); }
	

		static int const IdxTextureAutoBindStart = 4;
		void setTextureInternal( char const* name , GLenum texType , GLuint idTex , int idx )
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glActiveTexture( GL_TEXTURE0 + idx );
			glBindTexture( texType , idTex );
			setParam( name , idx );
			glActiveTexture( GL_TEXTURE0 );
		}


		void setParam( int loc , float v1 ){  glUniform1f( loc , v1 );	}
		void setParam( int loc , float v1 , float v2 ){  glUniform2f( loc , v1, v2 );  }
		void setParam( int loc , float v1 , float v2 , float v3 ){  glUniform3f( loc , v1, v2 , v3 );  }
		void setParam( int loc , int v1 ){  glUniform1i( loc , v1 );	}
		void setParam( int loc , Vector3 const& v ){ glUniform3f( loc , v.x , v.y , v.z ); }
		void setParam( int loc , Vector4 const& v ) { glUniform4f( loc, v.x, v.y, v.z , v.w ); }

		
		void setTexture2D( int loc , GLuint idTex , int idx )
		{
			glActiveTexture( GL_TEXTURE0 + idx );
			glBindTexture( GL_TEXTURE_2D, idTex );
			setParam( loc , idx );
			glActiveTexture( GL_TEXTURE0 );
		}
		void setTexture1D( int loc , GLuint idTex , int idx )
		{
			glActiveTexture( GL_TEXTURE0 + idx );
			glBindTexture( GL_TEXTURE_1D, idTex );
			setParam( loc , idx );
			glActiveTexture( GL_TEXTURE0 );
		}

		int  getParamLoc( char const* name ){ return glGetUniformLocation( mHandle , name ); }
		static int const NumShader = 3;
		RHIShaderRef mShaders[ NumShader ];
		bool         mNeedLink;
		int          mIdxTextureAutoBind;
	};

	struct RMPRenderBuffer
	{  
		static void create( GLuint& handle ){ glGenRenderbuffers( 1 , &handle ); }
		static void destroy( GLuint& handle ){ 	glDeleteRenderbuffers( 1 , &handle ); }
	};

	class DepthRenderBuffer : public TGLObjectBase< RMPRenderBuffer >
	{
	public:
		bool create( int w , int h , Texture::DepthFormat format );
		Texture::DepthFormat getFormat() { return mFormat;  }
		Texture::DepthFormat mFormat;

	};

	//typedef TRefCountPtr< RHIDepthRenderBuffer > RHIDepthRenderBufferRef;

	class FrameBuffer
	{
	public:
		FrameBuffer();
		~FrameBuffer();

		bool create();

		int  addTexture(RHITextureCube& target, Texture::Face face, bool beManaged = false);
		int  addTexture( RHITexture2D& target , bool beManaged = false );
		void setTexture( int idx , RHITexture2D& target , bool beManaged = false );
		void setTexture( int idx , RHITextureCube& target , Texture::Face face , bool bManaged = false );

		void bindDepthOnly();
		void bind();
		void unbind();
		void setDepth( DepthRenderBuffer& buffer , bool bManaged = false );
		void setDepth( RHITextureDepth& target, bool bManaged = false );
		void removeDepthBuffer();
		GLuint getTextureHandle( int idx ){ return mTextures[idx].handle; }

		void clearBuffer( Vector4 const* colorValue, float const* depthValue = nullptr, uint8 stencilValue = 0);

		void setDepthInternal(GLuint handle, Texture::DepthFormat format, bool bTexture, bool bManaged);

		struct BufferInfo
		{
			BufferInfo()
			{
				handle = 0;
				
			}

			GLuint handle;
			union 
			{
				uint8  idxFace;
				bool   bTexture;
			};
			bool   bManaged;
		};
		void   setTextureInternal( int idx , BufferInfo& info , GLenum texType );

		std::vector< BufferInfo > mTextures;
		BufferInfo  mDepth;
		GLuint      mFBO;
	};

	struct Vertex
	{
		enum Format
		{
			eFloat1 ,
			eFloat2 ,
			eFloat3 ,
			eFloat4 ,

			eUnknowFormat ,
		};
		enum Semantic
		{
			ePosition ,
			eNormal ,
			eColor ,
			eTangent ,
			eTexcoord ,
		};
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


		void bind();
		void unbind();
		void bindVAO()
		{
			if( mVAO )
			{
				glBindVertexArray(mVAO);
			}
			else
			{
				glGenVertexArrays(1, &mVAO);
				glBindVertexArray(mVAO);

			}
			setupVAO();
		}
		void unbindVAO()
		{
			glBindVertexArray(0);
		}
		void setupVAO();
		struct Info
		{
			uint8 semantic;
			uint8 format;
			uint8 offset;
			uint8 idx;
		};

		static uint8  getFormatSize( uint8 format );
		static GLenum getFormatType( uint8 format );
		static int    getElementSize( uint8 format );
		Info const*   findBySematic( Vertex::Semantic s , int idx ) const;
		Info const*   findBySematic( Vertex::Semantic s ) const;
		typedef std::vector< Info > InfoVec;
		InfoVec mInfoVec;
		uint8   mVertexSize;
		GLuint  mVAO;
	};

	struct RMPBufferObject
	{
		static void create(GLuint& handle) { glGenBuffers(1, &handle); }
		static void destroy(GLuint& handle) { glDeleteBuffers(1, &handle); }
	};

	struct Buffer
	{

		enum Usage
		{
			eStatic ,
			eDynamic ,
		};




	};


	class VertexBufferRHI : public RHIResource< VertexBufferRHI , RMPBufferObject >
	{
	public:
		VertexBufferRHI()
		{
			mBufferSize = 0;
			mNumVertices = 0;
		}
		bool create( uint32 vertexSize,  uint32 numVertices , void* data )
		{
			if( !fetchHandle() )
				return false;
			glBindBuffer(GL_ARRAY_BUFFER, mHandle);
			glBufferData(GL_ARRAY_BUFFER, vertexSize * numVertices , data , GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			mBufferSize = vertexSize * numVertices;
			mNumVertices = numVertices;
			return true;
		}
		void bind() { glBindBuffer(GL_ARRAY_BUFFER, mHandle); }
		void unbind(){ glBindBuffer(GL_ARRAY_BUFFER, 0); }

		uint32 mBufferSize;
		uint32 mNumVertices;

	};

	
	class IndexBufferRHI : public RHIResource< IndexBufferRHI , RMPBufferObject >
	{
	public:
		IndexBufferRHI()
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

	typedef TRefCountPtr< VertexBufferRHI > RHIVertexBufferRef;
	typedef TRefCountPtr< IndexBufferRHI > RHIIndexBufferRef;

	struct PrimitiveDebugInfo
	{
		int32 numVertex;
		int32 numIndex;
		int32 numElement;
	};
	class Mesh
	{
	public:
		enum PrimitiveType
		{
			eTriangleList ,
			eTriangleStrip ,
			eTriangleFan ,
			eLineList ,
			eQuad ,
		};
		Mesh();
		~Mesh();

		bool create( void* pVertex  , int nV , void* pIdx , int nIndices , bool bIntIndex );
		void draw(bool bUseVAO = false);
		void drawSection(int idx);

		void drawInternal(int idxStart , int num );

		static GLenum convert( PrimitiveType type )
		{
			switch( type )
			{
			case eTriangleList:  return GL_TRIANGLES;
			case eTriangleStrip: return GL_TRIANGLE_STRIP;
			case eTriangleFan:   return GL_TRIANGLE_FAN;
			case eLineList:      return GL_LINES;
			case eQuad:          return GL_QUADS;
			}
			return GL_POINTS;
		}


		PrimitiveType    mType;
		VertexDecl       mDecl;
		RHIVertexBufferRef  mVertexBuffer;
		RHIIndexBufferRef   mIndexBuffer;
		uint32           mVAO;

		struct Section
		{
			int start;
			int num;
		};
		std::vector< Section > mSections;
	};


	template< class T >
	struct BindLockScope
	{
		BindLockScope(T& obj)
			:mObj(obj)
		{
			mObj.bind();
		}

		~BindLockScope()
		{
			mObj.unbind();
		}

		T& mObj;
	};

	template< class T >
	struct BindLockScope< TRefCountPtr< T > >
	{
		BindLockScope(TRefCountPtr< T >& obj)
			:mObj(obj)
		{
			assert(mObj);
			mObj->bind();
		}

		~BindLockScope()
		{
			mObj->unbind();
		}

		TRefCountPtr< T >& mObj;
	};

#define GL_BIND_LOCK_OBJECT( obj )\
	BindLockScope< decltype(obj) >  ANONYMOUS_VARIABLE( BindLockObj )( obj );

}

#endif // GLCommon_h__
