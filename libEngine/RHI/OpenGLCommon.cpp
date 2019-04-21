#include "OpenGLCommand.h"

#include "GpuProfiler.h"
#include "MarcoCommon.h"
#include "Core/IntegerType.h"

namespace Render
{
	int const GLDefalutUnpackAlignment = 4;

	uint32 RHIResource::TotalCount = 0;

	bool CheckGLStateValid()
	{
		GLenum error = glGetError();
		if( error != GL_NO_ERROR )
		{
			int i = 1;
			return false;
		}

		return true;
	}


	bool  UpdateTexture2D(GLenum textureEnum, int ox, int oy, int w, int h, Texture::Format format, void* data, int level)
	{
		glTexSubImage2D(textureEnum, level, ox, oy, w, h, GLConvert::PixelFormat(format), GLConvert::TextureComponentType(format), data);
		bool result = CheckGLStateValid();
		return result;

	}

	bool  UpdateTexture2D(GLenum textureEnum, int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level)
	{
#if 1
		::glPixelStorei(GL_UNPACK_ROW_LENGTH, pixelStride);
		glTexSubImage2D(textureEnum, level, ox, oy, w, h, GLConvert::PixelFormat(format), GLConvert::TextureComponentType(format), data);
		bool result = CheckGLStateValid();
		::glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#else
		GLenum formatGL = GLConvert::PixelFormat(format);
		GLenum typeGL = GLConvert::TextureComponentType(format);
		uint8* pData = (uint8*)data;
		int dataStride = pixelStride * Texture::GetFormatSize(format);
		for( int dy = 0; dy < h; ++dy )
		{
			glTexSubImage2D(textureEnum, level, ox, oy + dy, w, 1, formatGL, typeGL, pData);
			pData += dataStride;
		}
		bool result = CheckGLStateValid();
#endif
		return result;
	}

	bool OpenGLTexture1D::create(Texture::Format format, int length, int numMipLevel , uint32 creationFlags, void* data )
	{
		if( !mGLObject.fetchHandle() )
			return false;
		mSize = length;

		mFormat = format;
		mNumMipLevel = numMipLevel;

		if( numMipLevel < 1 )
			numMipLevel = 1;

		bind();

		glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);
		glTexImage1D(TypeEnumGL, 0, GLConvert::To(format), length, 0,
					 GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), data);

		if( numMipLevel > 1 )
		{
			glGenerateMipmap(TypeEnumGL);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		glTexParameteri(TypeEnumGL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		CheckGLStateValid();
		unbind();
		return true;
	}

	bool OpenGLTexture1D::update(int offset, int length, Texture::Format format, void* data, int level /*= 0*/)
	{
		bind();
		glTexSubImage1D(TypeEnumGL, level, offset, length, GLConvert::PixelFormat(format), GLConvert::TextureComponentType(format), data);
		bool result = CheckGLStateValid();
		unbind();
		return result;
	}


	bool OpenGLTexture2D::create(Texture::Format format, int width, int height, int numMipLevel,int numSamples, uint32 creationFlags, void* data , int alignment )
	{
		if( !mGLObject.fetchHandle() )
			return false;

		if( numMipLevel < 1 )
			numMipLevel = 1;
		if( numSamples < 1 )
			numSamples = 1;

		mSizeX = width;
		mSizeY = height;
		mFormat = format;
		mNumMipLevel = numMipLevel;
		mNumSamples = numSamples;

		bind();

		if( numSamples > 1 )
		{
			glTexImage2DMultisample(TypeEnumGLMultisample, numSamples, GLConvert::To(format), width, height, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);


			if( alignment && alignment != GLDefalutUnpackAlignment )
			{
				glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
				glTexImage2D(TypeEnumGL, 0, GLConvert::To(format), width, height, 0,
							 GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), data);
				glPixelStorei(GL_UNPACK_ALIGNMENT, GLDefalutUnpackAlignment);
			}
			else
			{
				glTexImage2D(TypeEnumGL, 0, GLConvert::To(format), width, height, 0,
							 GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), data);
			}


			if( numMipLevel > 1 )
			{
				glGenerateMipmap(TypeEnumGL);
				glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
			{
				glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		}

		CheckGLStateValid();
		unbind();
		return true;
	}


	bool OpenGLTexture2D::update(int ox, int oy, int w, int h, Texture::Format format , void* data , int level )
	{
		if( mNumSamples > 1 )
		{
			return false;
		}
		bind();
		bool result = UpdateTexture2D(TypeEnumGL, ox, oy, w, h, format, data, level);
		unbind();
		return result;
	}

	bool OpenGLTexture2D::update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level /*= 0 */)
	{
		if( mNumSamples > 1 )
		{
			return false;
		}

		bind();
		bool result = UpdateTexture2D(TypeEnumGL, ox, oy, w, h, format, pixelStride, data, level);
		unbind();
		return result;
	}

	bool OpenGLTexture3D::create(Texture::Format format, int sizeX, int sizeY, int sizeZ, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		if( !mGLObject.fetchHandle() )
			return false;

		if( numMipLevel < 1 )
			numMipLevel = 1;
		if( numSamples < 1 )
			numSamples = 1;

		mSizeX = sizeX;
		mSizeY = sizeY;
		mSizeZ = sizeZ;
		mFormat = format;
		mNumMipLevel = numMipLevel;
		mNumSamples = numSamples;

		bind();

		GLenum typeEnumGL = getGLTypeEnum();

		if( numSamples > 1 )
		{
			glTexImage3DMultisample(TypeEnumGLMultisample, numSamples, GLConvert::To(format), sizeX, sizeY, sizeZ, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);

			glTexImage3D(TypeEnumGL, 0, GLConvert::To(format), sizeX, sizeY, sizeZ, 0,
						 GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), data);

			if( numMipLevel > 1 )
			{
				glGenerateMipmap(TypeEnumGL);
				glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
			{
				glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		unbind();
		return true;
	}

	bool OpenGLTextureCube::create(Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
	{
		if( !mGLObject.fetchHandle() )
			return false;

		mSize = size;
		mFormat = format;
		mNumMipLevel = numMipLevel;

		if( numMipLevel < 1 )
			numMipLevel = 1;

		bind();
		glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);
		for( int i = 0; i < 6; ++i )
		{

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
						 GLConvert::To(format), size, size, 0,
						 GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), 
						 data ? data[i] : nullptr );
		}

		if( numMipLevel > 1 )
		{
			glGenerateMipmap(TypeEnumGL);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		
		}
		glTexParameteri(TypeEnumGL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		unbind();
		return true;
	}


	bool OpenGLTextureCube::update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, void* data, int level)
	{
		bind();
		bool result = UpdateTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, ox, oy, w, h, format, data, level);
		unbind();
		return result;
	}

	bool OpenGLTextureCube::update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level)
	{
		bind();
		bool result = UpdateTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, ox, oy, w, h, format, pixelStride , data, level);
		unbind();
		return result;
	}

	bool OpenGLTexture2DArray::create(Texture::Format format, int width, int height, int layerSize, int numMipLevel, int numSamples, uint32 createFlags, void* data )
	{
		if( !mGLObject.fetchHandle() )
			return false;

		if( layerSize < 1 )
			layerSize = 1;
		if( numMipLevel < 1 )
			numMipLevel = 1;
		if( numSamples < 1 )
			numSamples = 1;

		mSizeX = width;
		mSizeY = height;
		mLayerNum = layerSize;
		mFormat = format;
		mNumMipLevel = numMipLevel;
		mNumSamples = numSamples;

		bind();

		if( numSamples > 1 )
		{
			glTexImage3DMultisample(TypeEnumGLMultisample, numSamples, GLConvert::To(format), width, height , layerSize, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);

			glTexImage3D(TypeEnumGL, 0, GLConvert::To(format), width, height, layerSize, 0,
						 GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), data);

			if( numMipLevel > 1 )
			{
				glGenerateMipmap(TypeEnumGL);
				glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
			{
				glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		}

		CheckGLStateValid();
		unbind();
		return true;
	}

	bool OpenGLTextureDepth::create(Texture::DepthFormat format, int width, int height, int numMipLevel, int numSamples)
	{
		if( !mGLObject.fetchHandle() )
			return false;
		
		if( numMipLevel < 1 )
			numMipLevel = 1;
		if( numSamples < 1 )
			numSamples = 1;

		mFromat = format;
		mNumMipLevel = numMipLevel;
		mNumSamples = numSamples;
		mSizeX = width;
		mSizeY = height;

		bind();
		if( numSamples > 1 )
		{
			glTexImage2DMultisample(TypeEnumGLMultisample, numSamples, GLConvert::To(format), width, height, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(TypeEnumGL, 0, GLConvert::To(format), width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		}
		CheckGLStateValid();
		unbind();
		return true;	
	}

	OpenGLFrameBuffer::OpenGLFrameBuffer()
	{

	}

	OpenGLFrameBuffer::~OpenGLFrameBuffer()
	{
		mTextures.clear();
	}

	int OpenGLFrameBuffer::addTexture( RHITextureCube& target , Texture::Face face, int level)
	{
		int idx = mTextures.size();
		setTexture(idx, target, face , level);
		return idx;
	}

	int OpenGLFrameBuffer::addTexture( RHITexture2D& target, int level)
	{
		int idx = mTextures.size();
		setTexture(idx , target, level);	
		return idx;
	}


	int OpenGLFrameBuffer::addTexture(RHITexture2DArray& target, int idexLayer, int level /*= 0*/)
	{
		int idx = mTextures.size();
		setTexture(idx, target, idexLayer , level);
		return idx;
	}

	void OpenGLFrameBuffer::setTexture(int idx, RHITexture2D& target, int level)
	{
		assert( idx <= mTextures.size());
		if( idx == mTextures.size() )
		{
			mTextures.push_back(BufferInfo());
		}

		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.idxFace = -1;
		info.level = level;
		info.typeEnumGL = OpenGLCast::To(&target)->getGLTypeEnum();
		setTexture2DInternal( idx , OpenGLCast::GetHandle( target ) , info.typeEnumGL, level );
	}

	void OpenGLFrameBuffer::setTexture( int idx , RHITextureCube& target , Texture::Face face, int level)
	{
		assert( idx <= mTextures.size() );
		if( idx == mTextures.size() )
		{
			mTextures.push_back(BufferInfo());
		}

		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.idxFace = face;
		info.level = level;
		info.typeEnumGL = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
		setTexture2DInternal( idx , OpenGLCast::GetHandle( target ) , GL_TEXTURE_CUBE_MAP_POSITIVE_X + face , level );
	}

	void OpenGLFrameBuffer::setTexture(int idx, RHITexture2DArray& target, int idexLayer, int level /*= 0*/)
	{
		assert(idx <= mTextures.size());
		if( idx == mTextures.size() )
		{
			mTextures.push_back(BufferInfo());
		}

		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.idxLayer = idexLayer;
		info.level = level;
		info.typeEnumGL = OpenGLCast::To(&target)->getGLTypeEnum();
		setTextureLayerInternal(idx, OpenGLCast::GetHandle(target), info.typeEnumGL , level , idexLayer );
	}


	void OpenGLFrameBuffer::setRenderBufferInternal(GLuint handle)
	{
		assert(getHandle());
		glBindFramebuffer(GL_FRAMEBUFFER, getHandle());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_RENDERBUFFER, handle);
		checkStates();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::setTexture2DInternal(int idx, GLuint handle , GLenum texType, int level)
	{
		assert( getHandle() );
		glBindFramebuffer( GL_FRAMEBUFFER , getHandle() );
		glFramebufferTexture2D( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 + idx , texType , handle , level );
		checkStates();
		glBindFramebuffer(GL_FRAMEBUFFER, 0 );
	}

	void OpenGLFrameBuffer::setTexture3DInternal(int idx, GLuint handle, GLenum texType, int level, int idxLayer)
	{
		assert(getHandle());
		glBindFramebuffer(GL_FRAMEBUFFER, getHandle());
		glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx, texType, handle, level , idxLayer);
		checkStates();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	void OpenGLFrameBuffer::setTextureLayerInternal(int idx, GLuint handle, GLenum texType, int level, int idxLayer)
	{
		assert(getHandle());
		glBindFramebuffer(GL_FRAMEBUFFER, getHandle());
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx, handle, level, idxLayer);
		checkStates();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::bindDepthOnly()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, getHandle() );
	}


	void OpenGLFrameBuffer::bind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, getHandle() );
		GLenum DrawBuffers[] =
		{ 
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4,
			GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 , GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9
		};
		glDrawBuffers( mTextures.size() , DrawBuffers );


		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if( status != GL_FRAMEBUFFER_COMPLETE )
		{
			
		}
	}

	void OpenGLFrameBuffer::unbind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

	bool OpenGLFrameBuffer::create()
	{
		if( !mGLObject.fetchHandle() )
			return false;

		return true;
	}

	void OpenGLFrameBuffer::setupTextureLayer(RHITextureCube& target, int level )
	{
		mTextures.clear();
		int idx = mTextures.size();
		BufferInfo info;
		info.bufferRef = &target;
		info.idxFace = -1;
		info.typeEnumGL = GL_TEXTURE_CUBE_MAP;
		mTextures.push_back(info);
		glBindFramebuffer(GL_FRAMEBUFFER, getHandle());
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, OpenGLCast::GetHandle( target ), level);
		checkStates();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::clearBuffer(Vector4 const* colorValue, float const* depthValue, uint8 stencilValue)
	{
		if( colorValue )
		{
			for( int i = 0; i < mTextures.size(); ++i )
			{
				BufferInfo& bufferInfo = mTextures[i];
				glClearBufferfv(GL_COLOR, i, (float const*)colorValue);
			}
		}
		if( depthValue )
		{
			//FIXME
			glClearBufferfv(GL_DEPTH, 0, depthValue);
		}
	}


	void OpenGLFrameBuffer::blitToBackBuffer(int index)
	{
		int width, height;
		assert(index < mTextures.size());
		switch( mTextures[index].typeEnumGL )
		{
		case GL_TEXTURE_CUBE_MAP:
			{
				RHITextureCube* texture = (RHITextureCube*)mTextures[index].bufferRef.get();
				width = texture->getSize();
				height = texture->getSize();
			}
			break;
		case GL_TEXTURE_2D_MULTISAMPLE:
		case GL_TEXTURE_2D:
			{
				RHITexture2D* texture = (RHITexture2D*)mTextures[index].bufferRef.get();
				width = texture->getSizeX();
				height = texture->getSizeY();
			}
			break;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		case GL_TEXTURE_2D_ARRAY:
			{

			}
			break;
		case GL_RENDERBUFFER:
		default:
			return;
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, getHandle());
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + index);		
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::setDepthInternal(RHIResource& resource, GLuint handle, Texture::DepthFormat format, GLenum typeEnumGL)
	{
		removeDepthBuffer();

		mDepth.bufferRef = &resource;
		mDepth.typeEnumGL = typeEnumGL;

		if( handle )
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getHandle());

			GLenum attachType = GL_DEPTH_ATTACHMENT;
			if( Texture::ContainStencil(format) )
			{
				if( Texture::ContainDepth(format) )
					attachType = GL_DEPTH_STENCIL_ATTACHMENT;
				else
					attachType = GL_STENCIL_ATTACHMENT;
			}

			if( typeEnumGL == GL_RENDERBUFFER )
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachType, GL_RENDERBUFFER, handle);
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachType , typeEnumGL, handle, 0);
			}

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status != GL_FRAMEBUFFER_COMPLETE )
			{
				LogWarning(0,"DepthBuffer Can't Attach to FrameBuffer");
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
	}

#if USE_DepthRenderBuffer
	void OpenGLFrameBuffer::setDepth( RHIDepthRenderBuffer& buffer)
	{
		setDepthInternal( buffer , buffer.mHandle , buffer.getFormat() , GL_RENDERBUFFER );
	}
#endif

	void OpenGLFrameBuffer::setDepth(RHITextureDepth& target)
	{
		setDepthInternal( target , OpenGLCast::GetHandle( target ) , target.getFormat(), OpenGLCast::To(&target)->getGLTypeEnum() );
	}

	void OpenGLFrameBuffer::removeDepthBuffer()
	{
		if ( mDepth.bufferRef )
		{
			mDepth.bufferRef = nullptr;
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getHandle());
			if( mDepth.typeEnumGL == GL_RENDERBUFFER )
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, mDepth.typeEnumGL, 0, 0);
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
	}

#if USE_DepthRenderBuffer

	bool RHIDepthRenderBuffer::create(int w, int h, Texture::DepthFormat format)
	{
		assert( mHandle == 0 );
		mFormat = format;
		glGenRenderbuffers(1, &mHandle );
		glBindRenderbuffer( GL_RENDERBUFFER , mHandle );
		glRenderbufferStorage( GL_RENDERBUFFER , Texture::Convert( format ), w , h );
		//glRenderbufferStorage( GL_RENDERBUFFER , GL_DEPTH_COMPONENT , w , h );
		glBindRenderbuffer( GL_RENDERBUFFER , 0 );

		return true;
	}

#endif


	struct TextureConvInfo
	{
#if _DEBUG
		Texture::Format formatCheck;
#endif
		GLenum foramt;
		int    compNum;
		GLenum compType;
		
		//GLenum GLBaseFormat;
	};

	constexpr TextureConvInfo gTexConvMap[] =
	{
#if _DEBUG
#define TEXTURE_INFO( FORMAT_CHECK , FORMAT , COMP_NUM , COMP_TYPE )\
	{ FORMAT_CHECK , FORMAT , COMP_NUM , COMP_TYPE},
#else
#define TEXTURE_INFO( FORMAT_CHECK , FORMAT , COMP_NUM ,COMP_TYPE )\
	{ FORMAT , COMP_NUM , COMP_TYPE },
#endif
		TEXTURE_INFO(Texture::eRGBA8   ,GL_RGBA8   ,4,GL_UNSIGNED_BYTE)
		TEXTURE_INFO(Texture::eRGB8    ,GL_RGB8    ,3,GL_UNSIGNED_BYTE)

		TEXTURE_INFO(Texture::eR16     ,GL_R16     ,1,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eR8      ,GL_R8      ,1,GL_UNSIGNED_BYTE)
		
		TEXTURE_INFO(Texture::eR32F    ,GL_R32F    ,1,GL_FLOAT)
		TEXTURE_INFO(Texture::eRGB32F  ,GL_RGB32F  ,3,GL_FLOAT)
		TEXTURE_INFO(Texture::eRGBA32F ,GL_RGBA32F ,4,GL_FLOAT)
		TEXTURE_INFO(Texture::eRGB16F  ,GL_RGB16F  ,3,GL_FLOAT)
		TEXTURE_INFO(Texture::eRGBA16F ,GL_RGBA16F ,4,GL_FLOAT)

		TEXTURE_INFO(Texture::eR32I    ,GL_R32I    ,1,GL_INT)
		TEXTURE_INFO(Texture::eR16I    ,GL_R16I    ,1,GL_SHORT)
		TEXTURE_INFO(Texture::eR8I     ,GL_R8I     ,1,GL_BYTE)
		TEXTURE_INFO(Texture::eR32U    ,GL_R32UI   ,1,GL_UNSIGNED_INT)
		TEXTURE_INFO(Texture::eR16U    ,GL_R16UI   ,1,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eR8U     ,GL_R8UI    ,1,GL_UNSIGNED_BYTE)
		
		TEXTURE_INFO(Texture::eRG32I   ,GL_RG32I   ,2,GL_INT)
		TEXTURE_INFO(Texture::eRG16I   ,GL_RG16I   ,2,GL_SHORT)
		TEXTURE_INFO(Texture::eRG8I    ,GL_RG8I    ,2,GL_BYTE)
		TEXTURE_INFO(Texture::eRG32U   ,GL_RG32UI  ,2,GL_UNSIGNED_INT)
		TEXTURE_INFO(Texture::eRG16U   ,GL_RG16UI  ,2,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eRG8U    ,GL_RG8UI   ,2,GL_UNSIGNED_BYTE)
		
		TEXTURE_INFO(Texture::eRGB32I  ,GL_RGB32I  ,3,GL_INT)
		TEXTURE_INFO(Texture::eRGB16I  ,GL_RGB16I  ,3,GL_SHORT)
		TEXTURE_INFO(Texture::eRGB8I   ,GL_RGB8I   ,3,GL_BYTE)
		TEXTURE_INFO(Texture::eRGB32U  ,GL_RGB32UI ,3,GL_UNSIGNED_INT)
		TEXTURE_INFO(Texture::eRGB16U  ,GL_RGB16UI ,3,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eRGB8U   ,GL_RGB8UI  ,3,GL_UNSIGNED_BYTE)
		
		TEXTURE_INFO(Texture::eRGBA32I ,GL_RGBA32I ,4,GL_INT)
		TEXTURE_INFO(Texture::eRGBA16I ,GL_RGBA16I ,4,GL_SHORT)
		TEXTURE_INFO(Texture::eRGBA8I  ,GL_RGBA8I  ,4,GL_BYTE)
		TEXTURE_INFO(Texture::eRGBA32U ,GL_RGBA32UI,4,GL_UNSIGNED_INT)
		TEXTURE_INFO(Texture::eRGBA16U ,GL_RGBA16UI,4,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eRGBA8U  ,GL_RGBA8UI ,4,GL_UNSIGNED_BYTE)

		TEXTURE_INFO(Texture::eSRGB   ,GL_SRGB8 ,3,GL_UNSIGNED_BYTE)
		TEXTURE_INFO(Texture::eSRGBA  ,GL_SRGB8_ALPHA8 ,4,GL_UNSIGNED_BYTE)
			
		
#undef TEXTURE_INFO
	};
#if _DEBUG
	constexpr bool CheckTexConvMapValid_R(int i, int size)
	{
		return (i == size) ? true : ((i == (int)gTexConvMap[i].formatCheck) && CheckTexConvMapValid_R(i + 1, size));
	}
	constexpr bool CheckTexConvMapValid()
	{
		return CheckTexConvMapValid_R(0, sizeof(gTexConvMap) / sizeof(gTexConvMap[0]));
	}
	static_assert(CheckTexConvMapValid(), "CheckTexConvMapValid Error");
#endif



	GLenum GLConvert::To(Texture::Format format)
	{
		return gTexConvMap[(int)format].foramt;
	}

	uint32 Texture::GetFormatSize(Format format)
	{
		uint32 result = 0;
		switch( gTexConvMap[format].compType )
		{
		case GL_FLOAT:
		case GL_INT:
		case GL_UNSIGNED_INT:
			result = 4;
			break;
		case GL_HALF_FLOAT:
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
			result = 2;
			break;
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			result = 1;
			break;
		}

		result *= gTexConvMap[format].compNum;
		return result;
	}



	uint32 Texture::GetComponentNum(Format format)
	{
		return gTexConvMap[format].compNum;
	}

	GLenum GLConvert::To(EAccessOperator op)
	{
		switch( op )
		{
		case AO_READ_ONLY: return GL_READ_ONLY;
		case AO_WRITE_ONLY: return GL_WRITE_ONLY;
		case AO_READ_AND_WRITE: return GL_READ_WRITE;
		}
		assert(0);
		return GL_READ_WRITE;
	}

	GLenum GLConvert::To(PrimitiveType type)
	{
		switch( type )
		{
		case PrimitiveType::TriangleList:  return GL_TRIANGLES;
		case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
		case PrimitiveType::TriangleFan:   return GL_TRIANGLE_FAN;
		case PrimitiveType::LineList:      return GL_LINES;
		case PrimitiveType::LineStrip:     return GL_LINE_STRIP;
		case PrimitiveType::LineLoop:      return GL_LINE_LOOP;
		case PrimitiveType::TriangleAdjacency:   return GL_TRIANGLES_ADJACENCY;
		case PrimitiveType::Quad:          return GL_QUADS;
		case PrimitiveType::Points:        return GL_POINTS;
		case PrimitiveType::Patchs:        return GL_PATCHES;
		}
		return GL_POINTS;
	}

	GLenum GLConvert::To(Shader::Type type)
	{
		switch( type )
		{
		case Shader::eVertex:   return GL_VERTEX_SHADER;
		case Shader::ePixel:    return GL_FRAGMENT_SHADER;
		case Shader::eGeometry: return GL_GEOMETRY_SHADER;
		case Shader::eCompute:  return GL_COMPUTE_SHADER;
		case Shader::eHull:     return GL_TESS_CONTROL_SHADER;
		case Shader::eDomain:   return GL_TESS_EVALUATION_SHADER;
		}
		return 0;
	}

	GLenum GLConvert::To(ELockAccess access)
	{
		switch( access )
		{
		case ELockAccess::ReadOnly:
			return GL_READ_ONLY;
		case ELockAccess::ReadWrite:
			return GL_READ_WRITE;
		case ELockAccess::WriteDiscard:
		case ELockAccess::WriteOnly:
			return GL_WRITE_ONLY;
		}
		return GL_READ_ONLY;
	}

	GLenum GLConvert::To(Blend::Factor factor)
	{
		switch( factor )
		{
		case Blend::eSrcAlpha: return GL_SRC_ALPHA;
		case Blend::eOneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case Blend::eOne: return GL_ONE;
		case Blend::eZero: return GL_ZERO;
		case Blend::eDestAlpha: return GL_DST_ALPHA;
		case Blend::eOneMinusDestAlpha: return GL_ONE_MINUS_DST_ALPHA;
		case Blend::eSrcColor: return GL_SRC_COLOR;
		case Blend::eOneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case Blend::eDestColor: return GL_DST_COLOR;
		case Blend::eOneMinusDestColor: return GL_ONE_MINUS_DST_COLOR;
		}
		return GL_ONE;
	}

	GLenum GLConvert::To(Blend::Operation op)
	{
		switch( op )
		{
		case Blend::eAdd: return GL_FUNC_ADD;
		case Blend::eSub: return GL_FUNC_SUBTRACT;
		case Blend::eReverseSub: return GL_FUNC_REVERSE_SUBTRACT;
		case Blend::eMax: return GL_MIN;
		case Blend::eMin: return GL_MAX;
		}
		return GL_FUNC_ADD;
	}

	GLenum GLConvert::To(ECompareFun fun)
	{
		switch( fun )
		{
		case ECompareFun::Never:
			return GL_NEVER;
		case ECompareFun::Less:
			return GL_LESS;
		case ECompareFun::Equal:
			return GL_EQUAL;
		case ECompareFun::NotEqual:
			return GL_NOTEQUAL;
		case ECompareFun::LessEqual:
			return GL_LEQUAL;
		case ECompareFun::Greater:
			return GL_GREATER;
		case ECompareFun::GeraterEqual:
			return GL_GEQUAL;
		case ECompareFun::Always:
			return GL_ALWAYS;
		}
		return GL_LESS;
	}

	GLenum GLConvert::To(Stencil::Operation op)
	{
		switch( op )
		{
		case Stencil::eKeep:
			return GL_KEEP;
		case Stencil::eZero:
			return GL_ZERO;
		case Stencil::eReplace:
			return GL_REPLACE;
		case Stencil::eIncr:
			return GL_INCR;
		case Stencil::eIncrWarp:
			return GL_INCR_WRAP;
		case Stencil::eDecr:
			return GL_DECR;
		case Stencil::eDecrWarp:
			return GL_DECR_WRAP;
		case Stencil::eInvert:
			return GL_INVERT;
		}
		return GL_KEEP;
	}


	GLenum GLConvert::To(ECullMode mode)
	{
		switch( mode )
		{
		case ECullMode::Front: return GL_FRONT;
		case ECullMode::Back: return GL_BACK;
		}
		return GL_FRONT;
	}

	GLenum GLConvert::To(EFillMode mode)
	{
		switch( mode )
		{
		case EFillMode::Point: return GL_POINT;
		case EFillMode::Wireframe: return GL_LINE;
		case EFillMode::Solid: return GL_FILL;
		}
		return GL_FILL;
	}

	GLenum GLConvert::To(ECompValueType type)
	{
		switch( type )
		{
		case CVT_Float:  return GL_FLOAT;
		case CVT_Half:   return GL_HALF_FLOAT;
		case CVT_UInt:   return GL_UNSIGNED_INT;
		case CVT_Int:    return GL_INT;
		case CVT_UShort: return GL_UNSIGNED_SHORT;
		case CVT_Short:  return GL_SHORT;
		case CVT_UByte:  return GL_UNSIGNED_BYTE;
		case CVT_Byte:   return GL_BYTE;
		}

		assert(0);
		return GL_FLOAT;
	}

	GLenum GLConvert::To(Sampler::Filter filter)
	{
		switch( filter )
		{
		case Sampler::ePoint:
			return GL_NEAREST;
		case Sampler::eBilinear:
			return GL_LINEAR;
		case Sampler::eTrilinear:
			return GL_LINEAR_MIPMAP_LINEAR;
		case Sampler::eAnisotroicPoint:
			return GL_LINEAR;
		case Sampler::eAnisotroicLinear:
			return GL_LINEAR_MIPMAP_LINEAR;
		}
		return GL_NEAREST;
	}

	GLenum GLConvert::To(Sampler::AddressMode mode)
	{
		switch( mode )
		{
		case Sampler::eWarp:
			return GL_REPEAT;
		case Sampler::eClamp:
			return GL_CLAMP_TO_EDGE;
		case Sampler::eMirror:
			return GL_MIRRORED_REPEAT;
		case Sampler::eBorder:
			return GL_CLAMP_TO_BORDER;
		}
		return GL_REPEAT;
	}

	GLenum GLConvert::To(Texture::DepthFormat format)
	{
		switch( format )
		{
		case Texture::eDepth16: return GL_DEPTH_COMPONENT16;
		case Texture::eDepth24: return GL_DEPTH_COMPONENT24;
		case Texture::eDepth32: return GL_DEPTH_COMPONENT32;
		case Texture::eDepth32F:return GL_DEPTH_COMPONENT32F;
		case Texture::eD24S8:   return GL_DEPTH24_STENCIL8;
		case Texture::eD32FS8:  return GL_DEPTH32F_STENCIL8;
		case Texture::eStencil1: return GL_STENCIL_INDEX1;
		case Texture::eStencil4: return GL_STENCIL_INDEX4;
		case Texture::eStencil8: return GL_STENCIL_INDEX8;
		case Texture::eStencil16: return GL_STENCIL_INDEX16;
		}
		return 0;
	}

	GLenum GLConvert::BaseFormat(Texture::Format format)
	{
		switch( format )
		{
		case Texture::eRGB8: case Texture::eRGB32F:case Texture::eRGB16F: case Texture::eSRGB:
			return GL_RGB;
		case Texture::eRGBA8:case Texture::eRGBA32F:case Texture::eRGBA16F: case  Texture::eSRGBA:
			return GL_RGBA;
		case Texture::eR32F: case Texture::eR16: case Texture::eR8:
			return GL_RED;
		case Texture::eR8I:case Texture::eR16I:case Texture::eR32I:
		case Texture::eR8U:case Texture::eR16U:case Texture::eR32U:
			return GL_RED_INTEGER;
		case Texture::eRG8I:case Texture::eRG16I:case Texture::eRG32I:
		case Texture::eRG8U:case Texture::eRG16U:case Texture::eRG32U:
			return GL_RG_INTEGER;
		case Texture::eRGB8I:case Texture::eRGB16I:case Texture::eRGB32I:
		case Texture::eRGB8U:case Texture::eRGB16U:case Texture::eRGB32U:
			return GL_RGB_INTEGER;
		case Texture::eRGBA8I:case Texture::eRGBA16I:case Texture::eRGBA32I:
		case Texture::eRGBA8U:case Texture::eRGBA16U:case Texture::eRGBA32U:
			return GL_RGBA_INTEGER;
		}
		return 0;
	}

	GLenum GLConvert::PixelFormat(Texture::Format format)
	{
		switch( format )
		{
		case Texture::eR32F: case Texture::eR16: case Texture::eR8:
		case Texture::eR8I:case Texture::eR16I:case Texture::eR32I:
		case Texture::eR8U:case Texture::eR16U:case Texture::eR32U:
			return GL_RED;
		case Texture::eRG8I:case Texture::eRG16I:case Texture::eRG32I:
		case Texture::eRG8U:case Texture::eRG16U:case Texture::eRG32U:
			return GL_RG;
		case Texture::eRGB8: case Texture::eSRGB:
		case Texture::eRGB32F:case Texture::eRGB16F:
		case Texture::eRGB8I:case Texture::eRGB16I:case Texture::eRGB32I:
		case Texture::eRGB8U:case Texture::eRGB16U:case Texture::eRGB32U:
			return GL_RGB;
		case Texture::eRGBA8: case Texture::eSRGBA:
		case Texture::eRGBA32F:case Texture::eRGBA16F:
		case Texture::eRGBA8I:case Texture::eRGBA16I:case Texture::eRGBA32I:
		case Texture::eRGBA8U:case Texture::eRGBA16U:case Texture::eRGBA32U:
			return GL_RGBA;
		}
		return 0;
	}

	GLenum GLConvert::TextureComponentType(Texture::Format format)
	{
		return gTexConvMap[format].compType;
	}

	GLenum GLConvert::Image2DType(Texture::Format format)
	{
		switch( format )
		{
		case Texture::eRGBA8:
		case Texture::eRGB8:
		case Texture::eR32F:case Texture::eR16:case Texture::eR8:
		case Texture::eRGB32F:
		case Texture::eRGBA32F:
		case Texture::eRGB16F:
		case Texture::eRGBA16F:
			return GL_IMAGE_2D;
		case Texture::eR8I:case Texture::eR16I:case Texture::eR32I:
		case Texture::eRG8I:case Texture::eRG16I:case Texture::eRG32I:
		case Texture::eRGB8I:case Texture::eRGB16I:case Texture::eRGB32I:
		case Texture::eRGBA8I:case Texture::eRGBA16I:case Texture::eRGBA32I:
			return GL_INT_IMAGE_2D;
		case Texture::eR8U:case Texture::eR16U:case Texture::eR32U:
		case Texture::eRG8U:case Texture::eRG16U:case Texture::eRG32U:
		case Texture::eRGB8U:case Texture::eRGB16U:case Texture::eRGB32U:
		case Texture::eRGBA8U:case Texture::eRGBA16U:case Texture::eRGBA32U:
			return GL_UNSIGNED_INT_IMAGE_2D;
		}
		return 0;
	}

	GLenum GLConvert::BufferUsageEnum(uint32 creationFlags)
	{
		if( creationFlags & BCF_UsageDynamic )
			return GL_DYNAMIC_DRAW;
		else if( creationFlags & BCF_UsageStage )
			return GL_STREAM_DRAW;
		return GL_STATIC_DRAW;
	}

	int Vertex::GetFormatSize(uint8 format)
	{
		int num = Vertex::GetComponentNum(format);
		switch( Vertex::GetCompValueType(Vertex::Format(format)) )
		{
		case CVT_Float:  return sizeof(float) * num;
		case CVT_Half:   return sizeof(float)/2 * num;
		case CVT_UInt:   return sizeof(uint32) * num;
		case CVT_Int:    return sizeof(int) * num;
		case CVT_UShort: return sizeof(uint16) * num;
		case CVT_Short:  return sizeof(int16) * num;
		case CVT_UByte:  return sizeof(uint8) * num;
		case CVT_Byte:   return sizeof(int8) * num;
		}
		return 0;
	}


	bool OpenGLVertexBuffer::create(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		if( !createInternal(vertexSize , numVertices, creationFlags, data) )
			return false;

		return true;
	}

	void OpenGLVertexBuffer::resetData(uint32 vertexSize, uint32 numVertices, uint32 creationFlags , void* data)
	{
		if( !resetDataInternal(vertexSize , numVertices, creationFlags, data) )
			return;

	}

	void OpenGLVertexBuffer::updateData(uint32 vStart , uint32 numVertices, void* data)
	{
		assert( (vStart + numVertices) * mElementSize < getSize() );
		glBindBuffer(GL_ARRAY_BUFFER, getHandle());
		glBufferSubData(GL_ARRAY_BUFFER, vStart * mElementSize , mElementSize * numVertices, data);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}


	bool OpenGLSamplerState::create(SamplerStateInitializer const& initializer)
	{
		if( !mGLObject.fetchHandle() )
		{
			return false;
		}

		glSamplerParameteri(getHandle(), GL_TEXTURE_MIN_FILTER, GLConvert::To(initializer.filter));
		glSamplerParameteri(getHandle(), GL_TEXTURE_MAG_FILTER, initializer.filter == Sampler::ePoint ? GL_NEAREST : GL_LINEAR);
		glSamplerParameteri(getHandle(), GL_TEXTURE_WRAP_S, GLConvert::To(initializer.addressU));
		glSamplerParameteri(getHandle(), GL_TEXTURE_WRAP_T, GLConvert::To(initializer.addressV));
		glSamplerParameteri(getHandle(), GL_TEXTURE_WRAP_R, GLConvert::To(initializer.addressW));
		return true;
	}

	OpenGLBlendState::OpenGLBlendState(BlendStateInitializer const& initializer)
	{
		mStateValue.bEnableAlphaToCoverage = initializer.bEnableAlphaToCoverage;
		for( int i = 0; i < NumBlendStateTarget; ++i )
		{
			auto const& targetValue = initializer.targetValues[i];
			auto& targetValueGL = mStateValue.targetValues[i];
			targetValueGL.writeMask = targetValue.writeMask;
			targetValueGL.bEnable = (targetValue.srcColor != Blend::eOne) || (targetValue.srcAlpha != Blend::eOne) ||
				(targetValue.destColor != Blend::eZero) || (targetValue.destAlpha != Blend::eZero);

			targetValueGL.bSeparateBlend = (targetValue.srcColor != targetValue.srcAlpha) || (targetValue.destColor != targetValue.destAlpha);

			targetValueGL.srcColor = GLConvert::To(targetValue.srcColor);
			targetValueGL.srcAlpha = GLConvert::To(targetValue.srcAlpha);
			targetValueGL.destColor = GLConvert::To(targetValue.destColor);
			targetValueGL.destAlpha = GLConvert::To(targetValue.destAlpha);
			targetValueGL.op = GLConvert::To(targetValue.op);
			targetValueGL.opAlpha = GLConvert::To(targetValue.opAlpha);
		}
	}

	OpenGLDepthStencilState::OpenGLDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		//When depth testing is disabled, writes to the depth buffer are also disabled
		mStateValue.bEnableDepthTest = (initializer.depthFun != ECompareFun::Always) || (initializer.bWriteDepth);
		mStateValue.depthFun = GLConvert::To(initializer.depthFun);
		mStateValue.bWriteDepth = initializer.bWriteDepth;

		mStateValue.bEnableStencilTest = initializer.bEnableStencilTest;

		mStateValue.stencilFun = GLConvert::To(initializer.stencilFun);
		mStateValue.stencilFailOp = GLConvert::To(initializer.stencilFailOp);
		mStateValue.stencilZFailOp = GLConvert::To(initializer.zFailOp);
		mStateValue.stencilZPassOp = GLConvert::To(initializer.zPassOp);

		mStateValue.stencilFunBack = GLConvert::To(initializer.stencilFunBack);
		mStateValue.stencilFailOpBack = GLConvert::To(initializer.stencilFailOpBack);
		mStateValue.stencilZFailOpBack = GLConvert::To(initializer.zFailOpBack);
		mStateValue.stencilZPassOpBack = GLConvert::To(initializer.zPassOpBack);

		mStateValue.bUseSeparateStencilOp =
			(mStateValue.stencilFailOp != mStateValue.stencilFailOpBack) ||
			(mStateValue.stencilZFailOp != mStateValue.stencilZFailOpBack) ||
			(mStateValue.stencilZPassOp != mStateValue.stencilZPassOpBack);

		mStateValue.bUseSeparateStencilFun = (mStateValue.stencilFun != mStateValue.stencilFunBack);
		mStateValue.stencilReadMask = initializer.stencilReadMask;
		mStateValue.stencilWriteMask = initializer.stencilWriteMask;
	}

	OpenGLRasterizerState::OpenGLRasterizerState(RasterizerStateInitializer const& initializer)
	{
		mStateValue.bEnableCull = initializer.cullMode != ECullMode::None;
		mStateValue.bEnableScissor = initializer.bEnableScissor;
		mStateValue.cullFace = GLConvert::To(initializer.cullMode);
		mStateValue.fillMode = GLConvert::To(initializer.fillMode);
	}

	OpenGLInputLayout::OpenGLInputLayout(InputLayoutDesc const& desc)
	{
		for( auto const& e : desc.mElements )
		{
			Element element;
			element.idxStream = e.idxStream;
			element.attribute = e.attribute;
			element.bNormalize = e.bNormalize;
			element.componentNum = Vertex::GetComponentNum(e.format);
			element.componentType = GLConvert::VertexComponentType(e.format);
			element.stride = desc.getVertexSize(e.idxStream);
			element.offset = e.offset;
			element.semantic = e.semantic;
			element.idx = e.idx;

			mElements.push_back(element);
		}

		std::sort(mElements.begin(), mElements.end(), [](auto const& lhs, auto const& rhs)
		{
			return lhs.idxStream < rhs.idxStream;
		});
	}

	void OpenGLInputLayout::bindAttrib(RHIVertexBuffer* vertexBuffer[], int numVertexBuffer, LinearColor const* overwriteColor)
	{
		for( int i = 0; i < numVertexBuffer; ++i )
		{
			GL_BIND_LOCK_OBJECT(*vertexBuffer[i]);
			int index = 0;
			for( ; index < mElements.size(); ++index )
			{
				auto const& e = mElements[index];
				if( e.idxStream > index )
					break;

				glEnableVertexAttribArray(e.attribute);
				glVertexAttribPointer(e.attribute, e.componentNum, e.componentType, e.bNormalize, e.stride, (void*)e.offset);
			}
		}
		if( overwriteColor )
		{
			glDisableVertexAttribArray(Vertex::ATTRIBUTE_COLOR);
			glVertexAttrib4fv(Vertex::ATTRIBUTE_COLOR, *overwriteColor);
		}
	}

	void OpenGLInputLayout::unbindAttrib(int numVertexBuffer, LinearColor const* overwriteColor)
	{
		for( auto const& e : mElements )
		{
			if ( e.idxStream >= numVertexBuffer )
				break;
			glDisableVertexAttribArray(e.attribute);
		}

		if( overwriteColor )
		{
			glDisableVertexAttribArray(Vertex::ATTRIBUTE_COLOR);
		}

	}

	void OpenGLInputLayout::bindPointer(LinearColor const* overwriteColor)
	{
		bool haveTex = false;
		for( auto const& e : mElements )
		{
			switch( e.semantic )
			{
			case Vertex::ePosition:
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(e.componentNum, e.componentType, e.stride, (void*)e.offset);
				break;
			case Vertex::eNormal:
				assert(e.componentNum == 3);
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(e.componentType, e.stride, (void*)e.offset);
				break;
			case Vertex::eColor:
				if( e.idx == 0 )
				{
					if( overwriteColor == nullptr )
					{
						glEnableClientState(GL_COLOR_ARRAY);
						glColorPointer(e.componentNum, e.componentType, e.stride, (void*)e.offset);
					}
				}
				else
				{
					glEnableClientState(GL_SECONDARY_COLOR_ARRAY);
					glSecondaryColorPointer(e.componentNum, e.componentType, e.stride, (void*)e.offset);
				}
				break;
			case Vertex::eTangent:
				glClientActiveTexture(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD));
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(e.componentNum, e.componentType, e.stride, (void*)e.offset);
				haveTex = true;
				break;
			case Vertex::eTexcoord:
				glClientActiveTexture(GL_TEXTURE0 + e.idx);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(e.componentNum, e.componentType, e.stride, (void*)e.offset);
				haveTex = true;
				break;
			}
		}

		if( overwriteColor )
		{
			glColor4fv(*overwriteColor);
		}
	}

	void OpenGLInputLayout::unbindPointer(LinearColor const* overwriteColor)
	{
		bool haveTex = false;
		for( auto const& e : mElements )
		{
			switch( e.semantic )
			{
			case Vertex::ePosition:
				glDisableClientState(GL_VERTEX_ARRAY);
				break;
			case Vertex::eNormal:
				glDisableClientState(GL_NORMAL_ARRAY);
				break;
			case Vertex::eColor:
				if( overwriteColor == nullptr )
					glDisableClientState((e.idx == 0) ? GL_COLOR_ARRAY : GL_SECONDARY_COLOR_ARRAY);
				break;
			case Vertex::eTangent:
				haveTex = true;
				glClientActiveTexture(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD));
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				break;
			case Vertex::eTexcoord:
				haveTex = true;
				glClientActiveTexture(GL_TEXTURE0 + e.idx);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				break;
			}
		}

		if( overwriteColor )
		{
			glColor4f(1, 1, 1, 1);
		}
		if( haveTex )
			glClientActiveTexture(GL_TEXTURE0);
	}



}//namespace GL