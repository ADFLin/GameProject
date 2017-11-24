#ifndef GLCommon_h__
#define GLCommon_h__

#include "GL/glew.h"
#include "GLConfig.h"

#include "CppVersion.h"
#include "BaseType.h"

#include "LogSystem.h"
#include "TVector2.h"

#include <vector>
#include <functional>

#include "RefCount.h"

#define SHADER_ENTRY( NAME ) #NAME
#define SHADER_PARAM( NAME ) #NAME

namespace RenderGL
{
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
		}

		~TGLObjectBase()
		{
			destroy();
		}

		bool isate() { return mHandle != 0; }

		void release()
		{
			destroy();
		}

		void destroy()
		{
			if( mHandle )
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



#if CPP_VARIADIC_TEMPLATE_SUPPORT
		template< class ...Args >
		bool fetchHandle(Args ...args)
		{
			if( !mHandle )
				RMPolicy::create(mHandle, args...);
			return mHandle != 0;
		}

#else
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

#endif
		GLuint mHandle;
	};

	struct RMPTexture
	{
		static void create(GLuint& handle) { glGenTextures(1, &handle); }
		static void destroy(GLuint& handle) { glDeleteTextures(1, &handle); }
	};
	
	struct RMPShader
	{
		static void create(GLuint& handle, GLenum type) { handle = glCreateShader(type); }
		static void destroy(GLuint& handle) { glDeleteShader(handle); }
	};	
	
	struct RMPShaderProgram
	{
		static void create(GLuint& handle) { handle = glCreateProgram(); }
		static void destroy(GLuint& handle) { glDeleteProgram(handle); }
	};
	
	struct RMPRenderBuffer
	{
		static void create(GLuint& handle) { glGenRenderbuffers(1, &handle); }
		static void destroy(GLuint& handle) { glDeleteRenderbuffers(1, &handle); }
	};

	
	struct RMPBufferObject
	{
		static void create(GLuint& handle) { glGenBuffers(1, &handle); }
		static void destroy(GLuint& handle) { glDeleteBuffers(1, &handle); }
	};

	class RHIResource : public RefCountedObjectT< RHIResource >
	{
	public:
		virtual ~RHIResource(){}
		virtual void destoryResource(){ }

		void destroyThis()
		{
			destoryResource();
			delete this;
		}
	};

	typedef  TRefCountPtr< RHIResource > RHIResourceRef;


	template< class RMPolicy >
	class TRHIResource : public RHIResource
		               , public TGLObjectBase< RMPolicy >
	{
	public:
		virtual void destoryResource() { TGLObjectBase< RMPolicy >::destroy();  }
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
		static GLenum GetFormatType(Format format);
		static GLenum GetImage2DType(Format format);

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
		bool loadFileInternal(char const* path, GLenum texType, Vec2i& outSize);
	};

	class RenderTarget
	{

	};

	class RHITexture2D : public RHITextureBase
	{
	public:
		bool loadFromFile( char const* path );
		bool create( Texture::Format format, int width, int height , void* data = nullptr );
		int  getSizeX() { return mSizeX; }
		int  getSizeY() { return mSizeY; }
		void bind();
		void unbind();
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


	class RHIShader : public TRHIResource< RMPShader >
	{
	public:
		enum Type
		{
			eUnknown  = -1,
			eVertex   = 0,
			ePixel    = 1,
			eGeometry = 2,
			eCompute  = 3,
			eHull     = 4,
			eDomain   = 5,
			NUM_SHADER_TYPE ,
		};


		bool loadFile( Type type , char const* path , char const* def = NULL );
		Type getType();
		bool compileCode( Type type , char const* src[] , int num );
		bool create( Type type );
		void destroy();

		GLuint getParam( GLuint val );
		friend class ShaderProgram;
	};

	
	typedef TRefCountPtr< RHIShader > RHIShaderRef;

	enum AccessOperator
	{
		AO_READ_ONLY ,
		AO_WRITE_ONLY ,
		AO_READ_AND_WRITE ,
	};

	enum class PrimitiveType
	{
		eTriangleList,
		eTriangleStrip,
		eTriangleFan,
		eLineList,
		eQuad,
		ePoints,
	};


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

	class ShaderParameter
	{
	public:
		bool isBound() const
		{
			return mLoc != -1;
		}

		bool bind( ShaderProgram& program , char const* name);
	private:
		friend class ShaderProgram;

		GLint mLoc;
	};

	class ShaderProgram : public TGLObjectBase< RMPShaderProgram >
	{
	public:
		ShaderProgram();

		virtual ~ShaderProgram();
		virtual void bindParameters(){}

		bool create();

		RHIShader* removeShader( RHIShader::Type type );
		void    attachShader( RHIShader& shader );
		void    updateShader(bool bForce = false);

		void checkProgramStatus();

		void    bind();
		void    unbind();

		void setParam( char const* name , float v1 )
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1f(loc, v1); 
		}
		void setParam( char const* name , float v1 , float v2 )
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform2f(loc, v1, v2); 
		}
		void setParam( char const* name , float v1 , float v2 , float v3 )
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform3f(loc, v1, v2, v3);
		}
		void setParam(char const* name, float v1, float v2, float v3 , float v4)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform4f(loc, v1, v2, v3, v4);
		}
		void setParam( char const* name , int v1 )
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1i(loc, v1);
		}

		void setMatrix33( char const* name , float const* value, int num = 1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniformMatrix3fv(loc, num, false, value);
		}
		void setMatrix44( char const* name , float const* value , int num = 1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniformMatrix4fv(loc, num, false, value);
		}
		void setParam(char const* name, float const v[] , int num ) 
		{ 
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1fv(loc, num, v);
		}

		void setVector3(char const* name, float const v[], int num)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform3fv(loc, num, v);
		}

		void setVector4(char const* name, float const v[], int num)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform4fv(loc, num, v);
		}

		void setParam( char const* name, Vector2 const& v) { setParam(name, v.x, v.y); }
		void setParam( char const* name, Vector3 const& v ){ setParam( name , v.x , v.y , v.z ); }
		void setParam( char const* name, Vector4 const& v) { setParam( name, v.x, v.y, v.z , v.w ); }
		void setParam( char const* name, Matrix4 const& m ){ setMatrix44( name , m , 1 ); }
		void setParam( char const* name, Matrix3 const& m ){ setMatrix33( name , m , 1 ); }
		void setParam( char const* name, Matrix4 const v[], int num) { setMatrix44(name, v[0], num);  }
		void setParam( char const* name, Vector3 const v[], int num) { setVector3(name, (float*)&v[0], num); }
		void setParam( char const* name, Vector4 const v[], int num) { setVector4(name, (float*)&v[0], num); }


		void setRWTexture(char const* name, RHITexture2D& tex, AccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			setRWTextureInternal(loc, tex.getFormat(), tex.mHandle, op, idx);
		}
		void setTexture(char const* name, RHITexture2D& tex, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(loc, GL_TEXTURE_2D, tex.mHandle, idx);
		}
		void setTexture(char const* name, RHITextureDepth& tex, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(loc, GL_TEXTURE_2D, tex.mHandle, idx);
		}
		void setTexture(char const* name, RHITextureCube& tex, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(loc, GL_TEXTURE_CUBE_MAP, tex.mHandle, idx);
		}
		void setTexture(char const* name, RHITexture3D& tex, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(loc, GL_TEXTURE_3D, tex.mHandle, idx);
		}
#if 0 //#TODO Can't Bind to texture 2d
		void setTexture2D(char const* name, TextureCube& tex, Texture::Face face, int idx = -1)
		{
			glActiveTexture(GL_TEXTURE0 + idx);
			glBindTexture(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, tex.mHandle);
			setParam(name, idx);
			glActiveTexture(GL_TEXTURE0);
		}
#endif

		void setTexture2D(char const* name, GLuint idTex, int idx = -1)
		{ 
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			setTextureInternal(loc , GL_TEXTURE_2D, idTex, idx); 
		}

#define CHECK_PARAMETER( PARAM , CODE ) assert( param.isBound() ); CODE

		void setParam(ShaderParameter const& param, int v1) { CHECK_PARAMETER( param , setParam(param.mLoc, v1) ); }
		void setParam(ShaderParameter const& param, float v1) { CHECK_PARAMETER(param, setParam(param.mLoc, v1) ); }
		void setParam(ShaderParameter const& param, float v1, float v2) { CHECK_PARAMETER(param, setParam(param.mLoc, v1, v2) ); }
		void setParam(ShaderParameter const& param, float v1, float v2, float v3) { CHECK_PARAMETER(param, setParam(param.mLoc, v1, v2, v3) ); }
		void setParam(ShaderParameter const& param, float v1, float v2, float v3, float v4) { CHECK_PARAMETER(param, setParam(param.mLoc, v1, v2, v3, v4) ); }
		void setParam(ShaderParameter const& param, Vector2 const& v) { CHECK_PARAMETER(param, setParam(param.mLoc, v.x, v.y)); }
		void setParam(ShaderParameter const& param, Vector3 const& v) { CHECK_PARAMETER(param, setParam(param.mLoc, v.x, v.y, v.z) ); }
		void setParam(ShaderParameter const& param, Vector4 const& v) { CHECK_PARAMETER(param, setParam(param.mLoc, v.x, v.y, v.z, v.w) ); }
		void setParam(ShaderParameter const& param, Matrix4 const& m) { CHECK_PARAMETER(param, setMatrix44(param.mLoc, m, 1) ); }
		void setParam(ShaderParameter const& param, Matrix3 const& m) { CHECK_PARAMETER(param, setMatrix33(param.mLoc, m, 1) ); }
		void setParam(ShaderParameter const& param, Matrix4 const v[], int num) { CHECK_PARAMETER(param, setMatrix44(param.mLoc, v[0], num) ); }
		void setParam(ShaderParameter const& param, Vector3 const v[], int num) { CHECK_PARAMETER(param, setVector3(param.mLoc, (float*)&v[0], num) ); }
		void setParam(ShaderParameter const& param, Vector4 const v[], int num) { CHECK_PARAMETER(param, setVector4(param.mLoc, (float*)&v[0], num) ); }

		void setRWTexture(ShaderParameter const& param, RHITexture2D& tex, AccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			CHECK_PARAMETER(param, setRWTextureInternal(param.mLoc, tex.getFormat(), tex.mHandle, op, idx) );
		}
		void setTexture(ShaderParameter const& param, RHITexture2D& tex, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(param.mLoc, GL_TEXTURE_2D, tex.mHandle, idx) );
		}
		void setTexture(ShaderParameter const& param, RHITextureDepth& tex, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(param.mLoc, GL_TEXTURE_2D, tex.mHandle, idx) );
		}
		void setTexture(ShaderParameter const& param, RHITextureCube& tex, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(param.mLoc, GL_TEXTURE_CUBE_MAP, tex.mHandle, idx) );
		}
		void setTexture(ShaderParameter const& param, RHITexture3D& tex, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(param.mLoc, GL_TEXTURE_3D, tex.mHandle, idx) );
		}

#undef CHECK_PARAMETER

		void setRWTexture(int loc, RHITexture2D& tex, AccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			setRWTextureInternal(loc, tex.getFormat(), tex.mHandle, op, idx);
		}
		void setTexture(int loc, RHITexture2D& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_2D, tex.mHandle, idx);
		}
		void setTexture(int loc, RHITextureDepth& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_2D, tex.mHandle, idx);
		}
		void setTexture(int loc, RHITextureCube& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_CUBE_MAP, tex.mHandle, idx);
		}
		void setTexture(int loc, RHITexture3D& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_3D, tex.mHandle, idx);
		}


		void setParam( int loc , float v1 ){  glUniform1f( loc , v1 );	}
		void setParam( int loc , float v1 , float v2 ){  glUniform2f( loc , v1, v2 );  }
		void setParam( int loc , float v1 , float v2 , float v3 ){  glUniform3f( loc , v1, v2 , v3 );  }
		void setParam( int loc, float v1, float v2, float v3 , float v4) { glUniform4f(loc, v1, v2, v3, v4); }
		void setParam( int loc , int v1 ){  glUniform1i( loc , v1 );	}
		void setParam( int loc , Vector3 const& v ){ glUniform3f( loc , v.x , v.y , v.z ); }
		void setParam( int loc , Vector4 const& v ) { glUniform4f( loc, v.x, v.y, v.z , v.w ); }

		void setMatrix33(int loc, float const* value, int num = 1){  glUniformMatrix3fv(loc, num, false, value);  }
		void setMatrix44(int loc, float const* value, int num = 1){  glUniformMatrix4fv(loc, num, false, value);  }
		void setParam(int loc, float const v[], int num){  glUniform1fv(loc, num, v);  }
		void setVector3(int loc, float const v[], int num){  glUniform3fv(loc, num, v);  }
		void setVector4(int loc, float const v[], int num){  glUniform4fv(loc, num, v); }

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

		static int const IdxTextureAutoBindStart = 2;
		void setTextureInternal(int loc, GLenum texType, GLuint idTex, int idx)
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glActiveTexture(GL_TEXTURE0 + idx);
			glBindTexture(texType, idTex);
			setParam(loc, idx);
			glActiveTexture(GL_TEXTURE0);
		}

		void setRWTextureInternal(int loc, Texture::Format format, GLuint idTex, AccessOperator op, int idx)
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glBindImageTexture(idx, idTex, 0, GL_FALSE, 0, GLConvert::To(op), GLConvert::To(format));
			setParam(loc, idx);
		}


		int  getParamLoc( char const* name ){ return glGetUniformLocation( mHandle , name ); }
		static int const NumShader = 3;
		RHIShaderRef mShaders[ NumShader ];
		bool         mNeedLink;

		//#TODO
	public:
		void resetTextureAutoBindIndex() { mIdxTextureAutoBind = IdxTextureAutoBindStart; }
		int  mIdxTextureAutoBind;
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
		enum Format
		{
			eFloat1 ,
			eFloat2 ,
			eFloat3 ,
			eFloat4 ,
			eUInt1 ,
			eUInt2 ,
			eUInt3 ,
			eUInt4 ,
			eInt1 ,
			eInt2 ,
			eInt3 ,
			eInt4 ,
			eUShort1,
			eUShort2,
			eUShort3,
			eUShort4,
			eShort1,
			eShort2,
			eShort3,
			eShort4,
			eUByte1,
			eUByte2,
			eUByte3,
			eUByte4,
			eByte1,
			eByte2,
			eByte3,
			eByte4,

			eUnknowFormat ,
		};
		enum Semantic
		{
			ePosition ,
			eColor,
			eNormal ,
			eTangent ,
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

			ATTRIBUTE_POSITION = ATTRIBUTE0,
			ATTRIBUTE_COLOR = ATTRIBUTE1,
			ATTRIBUTE_NORMAL = ATTRIBUTE2,
			ATTRIBUTE_TANGENT = ATTRIBUTE3,
			ATTRIBUTE_TEXCOORD = ATTRIBUTE4,
			ATTRIBUTE_TEXCOORD1 = ATTRIBUTE5,
			ATTRIBUTE_TEXCOORD2 = ATTRIBUTE6,
			ATTRIBUTE_TEXCOORD3 = ATTRIBUTE7,
			ATTRIBUTE_TEXCOORD4 = ATTRIBUTE8,
			ATTRIBUTE_TEXCOORD5 = ATTRIBUTE9,
			ATTRIBUTE_TEXCOORD6 = ATTRIBUTE10,
			ATTRIBUTE_TEXCOORD7 = ATTRIBUTE11,


			ATTRIBUTE_BONEINDEX = ATTRIBUTE14,
			ATTRIBUTE_BLENDWEIGHT = ATTRIBUTE15,
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
		VertexDecl&   addElement( uint8 attribute ,  Vertex::Format f,  bool bNormailze = false );


		void bind();
		void unbind();

		void setupVAO();
		void setupVAOEnd();
		struct Info
		{
			uint8 attribute;
			uint8 format;
			uint8 offset;
			uint8 semantic;
			uint8 idx;
			bool  bNormalize;
		};

		static uint8  getFormatSize( uint8 format );
		static GLenum getFormatType( uint8 format );
		static int    getElementSize( uint8 format );
		Info const*   findBySematic( Vertex::Semantic s , int idx ) const;
		Info const*   findBySematic( Vertex::Semantic s ) const;
		typedef std::vector< Info > InfoVec;
		InfoVec mInfoVec;
		uint8   mVertexSize;
	};


	class RHIVertexBuffer : public TRHIResource< RMPBufferObject >
	{
	public:
		RHIVertexBuffer()
		{
			mBufferSize = 0;
			mNumVertices = 0;
		}
		bool create( uint32 vertexSize,  uint32 numVertices , void* data , Buffer::Usage usage = Buffer::eStatic )
		{
			if( !fetchHandle() )
				return false;
			glBindBuffer(GL_ARRAY_BUFFER, mHandle);
			glBufferData(GL_ARRAY_BUFFER, vertexSize * numVertices , data , usage == Buffer::eStatic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW );
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			mBufferSize = vertexSize * numVertices;
			mNumVertices = numVertices;
			return true;
		}

		void updateData(uint32 vertexSize, uint32 numVertices, void* data )
		{
			glBindBuffer(GL_ARRAY_BUFFER, mHandle);
			glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize*numVertices, data);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			mBufferSize = vertexSize * numVertices;
			mNumVertices = numVertices;
		}
		void bind() { glBindBuffer(GL_ARRAY_BUFFER, mHandle); }
		void unbind(){ glBindBuffer(GL_ARRAY_BUFFER, 0); }

		uint32 mBufferSize;
		uint32 mNumVertices;

	};

	
	class RHIIndexBuffer : public TRHIResource< RMPBufferObject >
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

	typedef TRefCountPtr< RHIVertexBuffer > RHIVertexBufferRef;
	typedef TRefCountPtr< RHIIndexBuffer > RHIIndexBufferRef;

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
