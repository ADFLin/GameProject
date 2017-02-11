#ifndef GLCommon_h__
#define GLCommon_h__

#include "GL/glew.h"
#include "GLConfig.h"

#include "IntegerType.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/Matrix3.h"
#include "Math/Quaternion.h"

#include "TVector2.h"

#include <vector>

#include "LogSystem.h"

namespace GL
{
	typedef ::Math::Vector3 Vector3;
	typedef ::Math::Vector4 Vector4;
	typedef ::Math::Quaternion Quaternion;
	typedef ::Math::Matrix4 Matrix4;
	typedef ::Math::Matrix3 Matrix3;

	typedef TVector2<int> Vec2i;

	class AABBox
	{
	public:

		AABBox() {}
		AABBox( Vector3 const& min , Vector3 const& max )
			:mMin(min),mMax(max){ }

		static AABBox Empty() { return AABBox(Vector3::Zero() , Vector3::Zero()); }

		Vector3 getMin() const { return mMin; }
		Vector3 getMax() const { return mMax; }
		Vector3 getSize() const { return mMax - mMin; }
		float getVolume()
		{
			Vector3 size = getSize();
			return size.x * size.y * size.z;
		}
		void setEmpty()
		{
			mMin = mMax = Vector3::Zero();
		}

		void addPoint(Vector3 const& v)
		{
			mMax.max(v);
			mMin.min(v);
		}
		void translate(Vector3 const& offset)
		{
			mMax += offset;
			mMin += offset;
		}
		void expand(Vector3 const& dv)
		{
			mMax += dv;
			mMin -= dv;
		}

		void makeUnion(AABBox const& other)
		{
			mMin.min(other.mMin);
			mMax.max(other.mMax);
		}
	private:
		Vector3 mMin;
		Vector3 mMax;
	};


	inline bool isNormalize( Vector3 const& v )
	{
		return Math::Abs( 1.0 - v.length2() ) < 1e-6;
	}

	template< class T >
	class GLDataMove
	{

	};

	template< class RMPolicy >
	class GLObject
	{
	public:
		GLObject()
		{
			mHandle = 0;
			mbManaged = true;
		}

		~GLObject()
		{
			destroy();
		}

		void destroy()
		{
			if ( mbManaged && mHandle )
			{
				RMPolicy::destroy( mHandle );

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
			if ( !mHandle )
				RMPolicy::create( mHandle );
			return mHandle != 0;
		}

		template< class P1 >
		bool fetchHandle( P1 p1 )
		{
			if ( !mHandle )
				RMPolicy::create( mHandle , p1 );
			return mHandle != 0;
		}
		
		GLuint release(){ mbManaged = false ; return mHandle; }

		template< class Q >
		void operator = ( GLDataMove< Q >& rhs ){}

		bool   mbManaged;
		GLuint mHandle;
	};



	struct Texture
	{
		enum Type
		{
			e1D ,
			e2D ,
			e3D ,
			eCube ,
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

	class TextureBase : public GLObject< RMPTexture >
	{
	protected:
		bool loadFileInternal(char const* path, GLenum texType, Vec2i& outSize);
	};

	class RenderTarget
	{

	};
	class Texture2D : public TextureBase
	{
	public:
		bool loadFile( char const* path );
		bool create( Texture::Format format , int width , int height );
		int  getSizeX() { return mSizeX; }
		int  getSizeY() { return mSizeY; }
		void bind();
		void unbind();

	private:
		int mSizeX;
		int mSizeY;
	};

	class TextureCube : public TextureBase
	{
	public:
		bool loadFile( char const* path[] );
		bool create( Texture::Format format , int width , int height );
		void bind();
		void unbind();
	};

	class DepthTexture : public TextureBase
	{
	public:
		bool create(Texture::DepthFormat format, int width, int height);

		Texture::DepthFormat getFormat() { return mFromat; }
		Texture::DepthFormat mFromat;
	};

	struct RMPShader
	{  
		static void create( GLuint& handle , GLenum type ){ handle = glCreateShader( type ); }
		static void destroy( GLuint& handle ){ 	glDeleteShader( handle ); }
	};

	class Shader : public GLObject< RMPShader >
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
		bool compileSource( Type type , char const* src[] , int num , bool bUsePreprocess = true );
		bool create( Type type );
		void destroy();

		GLuint getParam( GLuint val );
		friend class ShaderProgram;
	};

	class ShaderProgram
	{
	public:
		ShaderProgram();

		~ShaderProgram();

		bool create();

		Shader* removeShader( Shader::Type type );
		void    attachShader( Shader& shader );
		void    updateShader();

		void checkLinkStatus();

		void    bind();
		void    unbind();

		void setParam( char const* name , float v1 )
		{
			int loc = getParamLoc( name );
			glUniform1f( loc , v1 );	
		}
		void setParam( char const* name , float v1 , float v2 )
		{
			int loc = getParamLoc( name );
			glUniform2f( loc , v1, v2 );	
		}
		void setParam( char const* name , float v1 , float v2 , float v3 )
		{
			int loc = getParamLoc( name );
			glUniform3f( loc , v1, v2 , v3 );	
		}

		void setParam( char const* name , int v1 )
		{
			int loc = glGetUniformLocation( mHandle , name );
			glUniform1i( loc , v1 );	
		}

		void setMatrix33( char const* name , float const* value )
		{
			int loc = glGetUniformLocation( mHandle , name );
			glUniformMatrix3fv( loc , 1 , false , value );	
		}
		void setMatrix44( char const* name , float const* value )
		{
			int loc = glGetUniformLocation( mHandle , name );
			glUniformMatrix4fv( loc , 1 , false , value );	
		}

		void setParam( char const* name , Vector3 const& v ){ setParam( name , v.x , v.y , v.z ); }
		void setParam( char const* name , Matrix4 const& m ){ setMatrix44( name , m ); }
		void setParam( char const* name , Matrix3 const& m ){ setMatrix33( name , m ); }
		void setTexture( char const* name , Texture2D& tex , int idx = 0)
		{ return setTextureInternal( name , GL_TEXTURE_2D , tex.mHandle , idx ); }
		void setTexture( char const* name , TextureCube& tex , int idx = 0)
		{ return setTextureInternal( name , GL_TEXTURE_CUBE_MAP , tex.mHandle , idx ); }
#if 0 //#TODO Can't Bind to texture 2d
		void setTexture2D( char const* name , TextureCube& tex , Texture::Face face , int idx )
		{
			glActiveTexture( GL_TEXTURE0 + idx );
			glBindTexture( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face , tex.mHandle );
			setParam( name , idx );
			glActiveTexture( GL_TEXTURE0 );
		}
#endif

		void setTexture2D( char const* name , GLuint idTex , int idx ){ setTextureInternal( name , GL_TEXTURE_2D , idTex , idx ); }
	
		void setTextureInternal( char const* name , GLenum texType , GLuint idTex , int idx )
		{
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
		GLuint  mHandle;
		Shader* mShader[ NumShader ];
		bool    mNeedLink;
	};

	struct RMPRenderBuffer
	{  
		static void create( GLuint& handle ){ glGenRenderbuffers( 1 , &handle ); }
		static void destroy( GLuint& handle ){ 	glDeleteRenderbuffers( 1 , &handle ); }
	};

	class DepthRenderBuffer : public GLObject< RMPRenderBuffer >
	{
	public:
		bool create( int w , int h , Texture::DepthFormat format );
		Texture::DepthFormat getFormat() { return mFormat;  }
		Texture::DepthFormat mFormat;
	};


	class FrameBuffer
	{
	public:
		FrameBuffer();
		~FrameBuffer();

		bool create();

		
		int  addTexture(TextureCube& target, Texture::Face face, bool beManaged = false);
		int  addTexture( Texture2D& target , bool beManaged = false );
		void setTexture( int idx , Texture2D& target , bool beManaged = false );
		void setTexture( int idx , TextureCube& target , Texture::Face face , bool bManaged = false );
		void bind();
		void unbind();
		void setDepth( DepthRenderBuffer& buffer , bool bManaged = false );
		void setDepth( DepthTexture& target, bool bManaged = false );
		void clearDepth();
		GLuint getTextureHandle( int idx ){ return mTextures[idx].handle; }

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
				uint8  idx;
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
			eTexcoord ,
		};
	};

	class VertexDecl
	{
	public:
		VertexDecl();

		void  setSize( uint8 size ){ mVertexSize = size; }
		uint8 getSize() const { return mVertexSize; }
		int   getSematicOffset( Vertex::Semantic s ) const;
		int   getSematicOffset( Vertex::Semantic s , int idx ) const;
		Vertex::Format  getSematicFormat( Vertex::Semantic s ) const;
		Vertex::Format  getSematicFormat( Vertex::Semantic s , int idx ) const;
		uint8 getOffset( int idx ) const { return mInfoVec[idx].offset; }

		VertexDecl&   addElement( Vertex::Semantic s , Vertex::Format f , uint8 idx = 0 );


		void bind();
		void unbind();
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
		GLuint  mHandle;
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

		bool createBuffer( void* pVertex  , int nV , void* pIdx , int nIndices , bool bIntIndex );
		void draw();

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

		PrimitiveType mType;    
		GLuint     mVboVertex;
		GLuint     mVboIndex;
		int        mNumVertices;
		int        mNumIndices;
		bool       mbIntIndex;
		VertexDecl mDecl;
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

#define GL_BIND_LOCK_OBJECT( obj )\
	BindLockScope< decltype(obj) >  ANONYMOUS_VARIABLE( BindLockObj )( obj );
	
}

#endif // GLCommon_h__
