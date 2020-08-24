#include "OpenGLCommand.h"

#include "GpuProfiler.h"
#include "MarcoCommon.h"
#include "Core/IntegerType.h"

namespace Render
{
	int const GLDefalutUnpackAlignment = 4;

	bool VerifyOpenGLStatus()
	{
		GLenum error = glGetError();
		if( error == GL_NO_ERROR )
			return true;
		switch( error )
		{
#define CASE_CODE( ENUM ) case ENUM : LogMsg( "Error code = %u (%s)" , error , #ENUM ); break;
			CASE_CODE(GL_INVALID_ENUM);
			CASE_CODE(GL_INVALID_VALUE);
			CASE_CODE(GL_INVALID_OPERATION);
			CASE_CODE(GL_INVALID_FRAMEBUFFER_OPERATION);
			CASE_CODE(GL_STACK_UNDERFLOW);
			CASE_CODE(GL_STACK_OVERFLOW);
#undef CASE_CODE
		default:
			LogMsg("Unknown error code = %u", error);
		}
		return false;
	}


	bool  UpdateTexture2D(GLenum textureEnum, int ox, int oy, int w, int h, Texture::Format format, void* data, int level)
	{
		glTexSubImage2D(textureEnum, level, ox, oy, w, h, OpenGLTranslate::PixelFormat(format), OpenGLTranslate::TextureComponentType(format), data);
		bool result = VerifyOpenGLStatus();
		return result;

	}

	bool  UpdateTexture2D(GLenum textureEnum, int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level)
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
		int dataStride = dataImageWidth * Texture::GetFormatSize(format);
		for( int dy = 0; dy < h; ++dy )
		{
			glTexSubImage2D(textureEnum, level, ox, oy + dy, w, 1, formatGL, typeGL, pData);
			pData += dataStride;
		}
		bool result = VerifyOpenGLStatus();
#endif
		return result;
	}

	bool OpenGLTexture1D::create(Texture::Format format, int length, int numMipLevel, uint32 creationFlags, void* data )
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
		glTexImage1D(TypeEnumGL, 0, OpenGLTranslate::To(format), length, 0,
					 OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), data);

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

		VerifyOpenGLStatus();
		unbind();
		return true;
	}

	bool OpenGLTexture1D::update(int offset, int length, Texture::Format format, void* data, int level /*= 0*/)
	{
		bind();
		glTexSubImage1D(TypeEnumGL, level, offset, length, OpenGLTranslate::PixelFormat(format), OpenGLTranslate::TextureComponentType(format), data);
		bool result = VerifyOpenGLStatus();
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
			glTexImage2DMultisample(TypeEnumGLMultisample, numSamples, OpenGLTranslate::To(format), width, height, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);


			if( alignment && alignment != GLDefalutUnpackAlignment )
			{
				glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
				glTexImage2D(TypeEnumGL, 0, OpenGLTranslate::To(format), width, height, 0,
							 OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), data);
				glPixelStorei(GL_UNPACK_ALIGNMENT, GLDefalutUnpackAlignment);
			}
			else
			{
				glTexImage2D(TypeEnumGL, 0, OpenGLTranslate::To(format), width, height, 0,
							 OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), data);
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

		VerifyOpenGLStatus();
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

	bool OpenGLTexture2D::update(int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level /*= 0 */)
	{
		if( mNumSamples > 1 )
		{
			return false;
		}

		bind();
		bool result = UpdateTexture2D(TypeEnumGL, ox, oy, w, h, format, dataImageWidth, data, level);
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
			glTexImage3DMultisample(TypeEnumGLMultisample, numSamples, OpenGLTranslate::To(format), sizeX, sizeY, sizeZ, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);

			glTexImage3D(TypeEnumGL, 0, OpenGLTranslate::To(format), sizeX, sizeY, sizeZ, 0,
						 OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), data);

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
		{
			GLenum formatGL = OpenGLTranslate::To(format);
			GLenum baseFormat = OpenGLTranslate::BaseFormat(format);
			GLenum componentType = OpenGLTranslate::TextureComponentType(format);
			for (int i = 0; i < 6; ++i)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
					formatGL, size, size, 0,
					baseFormat, componentType, data ? data[i] : nullptr);
			}
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

	bool OpenGLTextureCube::update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level)
	{
		bind();
		bool result = UpdateTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, ox, oy, w, h, format, dataImageWidth, data, level);
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
			glTexImage3DMultisample(TypeEnumGLMultisample, numSamples, OpenGLTranslate::To(format), width, height , layerSize, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);

			glTexImage3D(TypeEnumGL, 0, OpenGLTranslate::To(format), width, height, layerSize, 0,
						 OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), data);

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

		VerifyOpenGLStatus();
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

		mFormat = format;
		mNumMipLevel = numMipLevel;
		mNumSamples = numSamples;
		mSizeX = width;
		mSizeY = height;

		bind();
		if( numSamples > 1 )
		{
			glTexImage2DMultisample(TypeEnumGLMultisample, numSamples, OpenGLTranslate::To(format), width, height, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(TypeEnumGL, 0, OpenGLTranslate::To(format), width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		}
		VerifyOpenGLStatus();
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


	int OpenGLFrameBuffer::addTexture(RHITexture2DArray& target, int indexLayer, int level /*= 0*/)
	{
		int idx = mTextures.size();
		setTexture(idx, target, indexLayer, level);
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

	void OpenGLFrameBuffer::setTexture(int idx, RHITexture2DArray& target, int indexLayer, int level /*= 0*/)
	{
		assert(idx <= mTextures.size());
		if( idx == mTextures.size() )
		{
			mTextures.push_back(BufferInfo());
		}

		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.indexLayer = indexLayer;
		info.level = level;
		info.typeEnumGL = OpenGLCast::To(&target)->getGLTypeEnum();
		setTextureLayerInternal(idx, OpenGLCast::GetHandle(target), info.typeEnumGL , level , indexLayer);
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
		removeDepth();

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
		setDepthInternal(target, OpenGLCast::GetHandle(target), target.getFormat(), OpenGLCast::To(&target)->getGLTypeEnum());
	}

	void OpenGLFrameBuffer::removeDepth()
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

	struct OpenGLTextureConvInfo
	{
#if _DEBUG
		Texture::Format formatCheck;
#endif
		GLenum foramt;
		GLenum pixelFormat;
		GLenum compType;
		GLenum baseFormat;
	};

	constexpr OpenGLTextureConvInfo GTexConvMapGL[] =
	{
#if _DEBUG
#define TEXTURE_INFO( FORMAT_CHECK , FORMAT , PIXEL_FORMAT , COMP_TYPE , BASE_FORMAT )\
	{ FORMAT_CHECK , FORMAT , PIXEL_FORMAT , COMP_TYPE, BASE_FORMAT },
#else
#define TEXTURE_INFO( FORMAT_CHECK , FORMAT ,PIXEL_FORMAT ,COMP_TYPE,  BASE_FORMAT )\
	{ FORMAT , PIXEL_FORMAT , COMP_TYPE ,BASE_FORMAT },
#endif
		TEXTURE_INFO(Texture::eRGBA8   ,GL_RGBA8   ,GL_RGBA,GL_UNSIGNED_BYTE ,GL_RGBA)
		TEXTURE_INFO(Texture::eRGB8    ,GL_RGB8    ,GL_RGB ,GL_UNSIGNED_BYTE ,GL_RGB)
		TEXTURE_INFO(Texture::eBGRA8   ,GL_RGBA8   ,GL_RGBA,GL_UNSIGNED_BYTE ,GL_BGRA)
		TEXTURE_INFO(Texture::eRGB10A2 ,GL_RGB10_A2,GL_RGBA,GL_UNSIGNED_BYTE ,GL_BGRA)
		TEXTURE_INFO(Texture::eR16     ,GL_R16     ,GL_RED,GL_UNSIGNED_SHORT ,GL_RED)
		TEXTURE_INFO(Texture::eR8      ,GL_R8      ,GL_RED,GL_UNSIGNED_BYTE  ,GL_RED)
		
		TEXTURE_INFO(Texture::eR32F    ,GL_R32F    ,GL_RED ,GL_FLOAT ,GL_RED)
		TEXTURE_INFO(Texture::eRG32F   ,GL_RG32F   ,GL_RG  ,GL_FLOAT ,GL_RG)
		TEXTURE_INFO(Texture::eRGB32F  ,GL_RGB32F  ,GL_RGB ,GL_FLOAT ,GL_RGB)
		TEXTURE_INFO(Texture::eRGBA32F ,GL_RGBA32F ,GL_RGBA,GL_FLOAT ,GL_RGBA)
		TEXTURE_INFO(Texture::eR16F    ,GL_R16F    ,GL_RED ,GL_FLOAT ,GL_RED)
		TEXTURE_INFO(Texture::eRG16F   ,GL_RG16F   ,GL_RG  ,GL_FLOAT ,GL_RG)
		TEXTURE_INFO(Texture::eRGB16F  ,GL_RGB16F  ,GL_RGB ,GL_FLOAT ,GL_RGB)
		TEXTURE_INFO(Texture::eRGBA16F ,GL_RGBA16F ,GL_RGBA,GL_FLOAT ,GL_RGBA)

		TEXTURE_INFO(Texture::eR32I    ,GL_R32I    ,GL_RED ,GL_INT           ,GL_RED_INTEGER)
		TEXTURE_INFO(Texture::eR16I    ,GL_R16I    ,GL_RED ,GL_SHORT         ,GL_RED_INTEGER)
		TEXTURE_INFO(Texture::eR8I     ,GL_R8I     ,GL_RED ,GL_BYTE          ,GL_RED_INTEGER)
		TEXTURE_INFO(Texture::eR32U    ,GL_R32UI   ,GL_RED ,GL_UNSIGNED_INT  ,GL_RED_INTEGER)
		TEXTURE_INFO(Texture::eR16U    ,GL_R16UI   ,GL_RED ,GL_UNSIGNED_SHORT,GL_RED_INTEGER)
		TEXTURE_INFO(Texture::eR8U     ,GL_R8UI    ,GL_RED ,GL_UNSIGNED_BYTE ,GL_RED_INTEGER)
		
		TEXTURE_INFO(Texture::eRG32I   ,GL_RG32I   ,GL_RG ,GL_INT           ,GL_RG_INTEGER)
		TEXTURE_INFO(Texture::eRG16I   ,GL_RG16I   ,GL_RG ,GL_SHORT         ,GL_RG_INTEGER)
		TEXTURE_INFO(Texture::eRG8I    ,GL_RG8I    ,GL_RG ,GL_BYTE          ,GL_RG_INTEGER)
		TEXTURE_INFO(Texture::eRG32U   ,GL_RG32UI  ,GL_RG ,GL_UNSIGNED_INT  ,GL_RG_INTEGER)
		TEXTURE_INFO(Texture::eRG16U   ,GL_RG16UI  ,GL_RG ,GL_UNSIGNED_SHORT,GL_RG_INTEGER)
		TEXTURE_INFO(Texture::eRG8U    ,GL_RG8UI   ,GL_RG ,GL_UNSIGNED_BYTE ,GL_RG_INTEGER)
		
		TEXTURE_INFO(Texture::eRGB32I  ,GL_RGB32I  ,GL_RGB ,GL_INT            ,GL_RGB_INTEGER)
		TEXTURE_INFO(Texture::eRGB16I  ,GL_RGB16I  ,GL_RGB ,GL_SHORT          ,GL_RGB_INTEGER)
		TEXTURE_INFO(Texture::eRGB8I   ,GL_RGB8I   ,GL_RGB ,GL_BYTE           ,GL_RGB_INTEGER)
		TEXTURE_INFO(Texture::eRGB32U  ,GL_RGB32UI ,GL_RGB ,GL_UNSIGNED_INT   ,GL_RGB_INTEGER)
		TEXTURE_INFO(Texture::eRGB16U  ,GL_RGB16UI ,GL_RGB ,GL_UNSIGNED_SHORT ,GL_RGB_INTEGER)
		TEXTURE_INFO(Texture::eRGB8U   ,GL_RGB8UI  ,GL_RGB ,GL_UNSIGNED_BYTE  ,GL_RGB_INTEGER)
		
		TEXTURE_INFO(Texture::eRGBA32I ,GL_RGBA32I  ,GL_RGBA ,GL_INT            ,GL_RGBA_INTEGER)
		TEXTURE_INFO(Texture::eRGBA16I ,GL_RGBA16I  ,GL_RGBA ,GL_SHORT          ,GL_RGBA_INTEGER)
		TEXTURE_INFO(Texture::eRGBA8I  ,GL_RGBA8I   ,GL_RGBA ,GL_BYTE           ,GL_RGBA_INTEGER)
		TEXTURE_INFO(Texture::eRGBA32U ,GL_RGBA32UI ,GL_RGBA ,GL_UNSIGNED_INT   ,GL_RGBA_INTEGER)
		TEXTURE_INFO(Texture::eRGBA16U ,GL_RGBA16UI ,GL_RGBA ,GL_UNSIGNED_SHORT ,GL_RGBA_INTEGER)
		TEXTURE_INFO(Texture::eRGBA8U  ,GL_RGBA8UI  ,GL_RGBA ,GL_UNSIGNED_BYTE  ,GL_RGBA_INTEGER)

		TEXTURE_INFO(Texture::eSRGB   ,GL_SRGB8        ,GL_RGB  ,GL_UNSIGNED_BYTE ,GL_RGB  ,GL_RGB)
		TEXTURE_INFO(Texture::eSRGBA  ,GL_SRGB8_ALPHA8 ,GL_RGBA ,GL_UNSIGNED_BYTE ,GL_RGBA ,GL_RGBA)
			
		
#undef TEXTURE_INFO
	};
#if _DEBUG
	constexpr bool CheckTexConvMapValid_R(int i, int size)
	{
		return (i == size) ? true : ((i == (int)GTexConvMapGL[i].formatCheck) && CheckTexConvMapValid_R(i + 1, size));
	}
	constexpr bool CheckTexConvMapValid()
	{
		return CheckTexConvMapValid_R(0, sizeof(GTexConvMapGL) / sizeof(GTexConvMapGL[0]));
	}
	static_assert(CheckTexConvMapValid(), "CheckTexConvMapValid Error");
#endif

	GLenum OpenGLTranslate::To(Texture::Format format)
	{
		return GTexConvMapGL[(int)format].foramt;
	}

	GLenum OpenGLTranslate::TextureComponentType(Texture::Format format)
	{
		return GTexConvMapGL[format].compType;
	}

	GLenum OpenGLTranslate::BaseFormat(Texture::Format format)
	{
		return GTexConvMapGL[format].baseFormat;
	}

	GLenum OpenGLTranslate::PixelFormat(Texture::Format format)
	{
		return GTexConvMapGL[format].pixelFormat;
	}

	GLenum OpenGLTranslate::Image2DType(Texture::Format format)
	{
		switch (format)
		{
		case Texture::eRGBA8:case Texture::eBGRA8:
		case Texture::eRGB8: case Texture::eRGB10A2:
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

	GLenum OpenGLTranslate::To(EAccessOperator op)
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

	GLenum OpenGLTranslate::To(EPrimitive type, int& outPatchPointCount)
	{
		switch( type )
		{
		case EPrimitive::TriangleList:  return GL_TRIANGLES;
		case EPrimitive::TriangleStrip: return GL_TRIANGLE_STRIP;
		case EPrimitive::TriangleFan:   return GL_TRIANGLE_FAN;
		case EPrimitive::LineList:      return GL_LINES;
		case EPrimitive::LineStrip:     return GL_LINE_STRIP;
		case EPrimitive::LineLoop:      return GL_LINE_LOOP;
		case EPrimitive::TriangleAdjacency:   return GL_TRIANGLES_ADJACENCY;
		case EPrimitive::Quad:          return GL_QUADS;
		case EPrimitive::Polygon:       return GL_POLYGON;
		case EPrimitive::Points:        return GL_POINTS;
		}

		if (type >= EPrimitive::PatchPoint1)
		{
			outPatchPointCount = 1 + int(type) - int(EPrimitive::PatchPoint1);
			return GL_PATCHES;
		}
		return GL_POINTS;
	}

	GLenum OpenGLTranslate::To(EShader::Type type)
	{
		switch( type )
		{
		case EShader::Vertex:   return GL_VERTEX_SHADER;
		case EShader::Pixel:    return GL_FRAGMENT_SHADER;
		case EShader::Geometry: return GL_GEOMETRY_SHADER;
		case EShader::Compute:  return GL_COMPUTE_SHADER;
		case EShader::Hull:     return GL_TESS_CONTROL_SHADER;
		case EShader::Domain:   return GL_TESS_EVALUATION_SHADER;
		}
		return 0;
	}
	GLbitfield OpenGLTranslate::ToStageBit(EShader::Type type)
	{
		switch (type)
		{
		case EShader::Vertex:   return GL_VERTEX_SHADER_BIT;
		case EShader::Pixel:    return GL_FRAGMENT_SHADER_BIT;
		case EShader::Geometry: return GL_GEOMETRY_SHADER_BIT;
		case EShader::Compute:  return GL_COMPUTE_SHADER_BIT;
		case EShader::Hull:     return GL_TESS_CONTROL_SHADER_BIT;
		case EShader::Domain:   return GL_TESS_EVALUATION_SHADER_BIT;
		}
		return 0;
	}

	GLenum OpenGLTranslate::To(ELockAccess access)
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

	GLenum OpenGLTranslate::To(Blend::Factor factor)
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

	GLenum OpenGLTranslate::To(Blend::Operation op)
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

	GLenum OpenGLTranslate::To(ECompareFunc func)
	{
		switch(func)
		{
		case ECompareFunc::Never:       return GL_NEVER;
		case ECompareFunc::Less:        return GL_LESS;
		case ECompareFunc::Equal:       return GL_EQUAL;
		case ECompareFunc::NotEqual:    return GL_NOTEQUAL;
		case ECompareFunc::LessEqual:   return GL_LEQUAL;
		case ECompareFunc::Greater:     return GL_GREATER;
		case ECompareFunc::GeraterEqual:return GL_GEQUAL;
		case ECompareFunc::Always:      return GL_ALWAYS;
		}
		return GL_LESS;
	}

	GLenum OpenGLTranslate::To(Stencil::Operation op)
	{
		switch( op )
		{
		case Stencil::eKeep:     return GL_KEEP;
		case Stencil::eZero:     return GL_ZERO;
		case Stencil::eReplace:  return GL_REPLACE;
		case Stencil::eIncr:     return GL_INCR;
		case Stencil::eIncrWarp: return GL_INCR_WRAP;
		case Stencil::eDecr:     return GL_DECR;
		case Stencil::eDecrWarp: return GL_DECR_WRAP;
		case Stencil::eInvert:   return GL_INVERT;
		}
		return GL_KEEP;
	}

	GLenum OpenGLTranslate::To(ECullMode mode)
	{
		switch( mode )
		{
		case ECullMode::Front: return GL_FRONT;
		case ECullMode::Back: return GL_BACK;
		}
		return GL_FRONT;
	}

	GLenum OpenGLTranslate::To(EFillMode mode)
	{
		switch( mode )
		{
		case EFillMode::Point: return GL_POINT;
		case EFillMode::Wireframe: return GL_LINE;
		case EFillMode::Solid: return GL_FILL;
		}
		return GL_FILL;
	}

	GLenum OpenGLTranslate::To(EComponentType type)
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

	GLenum OpenGLTranslate::To(Sampler::Filter filter)
	{
		switch( filter )
		{
		case Sampler::ePoint:            return GL_NEAREST;
		case Sampler::eBilinear:         return GL_LINEAR;
		case Sampler::eTrilinear:        return GL_LINEAR_MIPMAP_LINEAR;
		case Sampler::eAnisotroicPoint:  return GL_LINEAR;
		case Sampler::eAnisotroicLinear: return GL_LINEAR_MIPMAP_LINEAR;
		}
		return GL_NEAREST;
	}

	GLenum OpenGLTranslate::To(Sampler::AddressMode mode)
	{
		switch( mode )
		{
		case Sampler::eWarp:   return GL_REPEAT;
		case Sampler::eClamp:  return GL_CLAMP_TO_EDGE;
		case Sampler::eMirror: return GL_MIRRORED_REPEAT;
		case Sampler::eBorder: return GL_CLAMP_TO_BORDER;
		}
		return GL_REPEAT;
	}

	GLenum OpenGLTranslate::To(Texture::DepthFormat format)
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

	GLenum OpenGLTranslate::BufferUsageEnum(uint32 creationFlags)
	{
		if( creationFlags & BCF_CpuAccessWrite )
			return GL_DYNAMIC_DRAW;
		else if( creationFlags & BCF_UsageStage )
			return GL_STREAM_DRAW;
		return GL_STATIC_DRAW;
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

		glSamplerParameteri(getHandle(), GL_TEXTURE_MIN_FILTER, OpenGLTranslate::To(initializer.filter));
		glSamplerParameteri(getHandle(), GL_TEXTURE_MAG_FILTER, initializer.filter == Sampler::ePoint ? GL_NEAREST : GL_LINEAR);
		glSamplerParameteri(getHandle(), GL_TEXTURE_WRAP_S, OpenGLTranslate::To(initializer.addressU));
		glSamplerParameteri(getHandle(), GL_TEXTURE_WRAP_T, OpenGLTranslate::To(initializer.addressV));
		glSamplerParameteri(getHandle(), GL_TEXTURE_WRAP_R, OpenGLTranslate::To(initializer.addressW));
		return true;
	}

	OpenGLBlendState::OpenGLBlendState(BlendStateInitializer const& initializer)
	{
		mStateValue.bEnableAlphaToCoverage = initializer.bEnableAlphaToCoverage;
		mStateValue.bEnableIndependent = initializer.bEnableIndependent;
		for( int i = 0; i < (initializer.bEnableIndependent ? MaxBlendStateTargetCount : 1); ++i )
		{
			auto const& targetValue = initializer.targetValues[i];
			auto& targetValueGL = mStateValue.targetValues[i];
			targetValueGL.writeMask = targetValue.writeMask;
			targetValueGL.bEnable = (targetValue.srcColor != Blend::eOne) || (targetValue.srcAlpha != Blend::eOne) ||
				(targetValue.destColor != Blend::eZero) || (targetValue.destAlpha != Blend::eZero);

			targetValueGL.bSeparateBlend = (targetValue.srcColor != targetValue.srcAlpha) || (targetValue.destColor != targetValue.destAlpha) || (targetValue.op != targetValue.opAlpha);

			targetValueGL.srcColor = OpenGLTranslate::To(targetValue.srcColor);
			targetValueGL.srcAlpha = OpenGLTranslate::To(targetValue.srcAlpha);
			targetValueGL.destColor = OpenGLTranslate::To(targetValue.destColor);
			targetValueGL.destAlpha = OpenGLTranslate::To(targetValue.destAlpha);
			targetValueGL.op = OpenGLTranslate::To(targetValue.op);
			targetValueGL.opAlpha = OpenGLTranslate::To(targetValue.opAlpha);
		}
	}

	OpenGLDepthStencilState::OpenGLDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		//When depth testing is disabled, writes to the depth buffer are also disabled
		mStateValue.bEnableDepthTest = (initializer.depthFunc != ECompareFunc::Always) || (initializer.bWriteDepth);
		mStateValue.depthFun = OpenGLTranslate::To(initializer.depthFunc);
		mStateValue.bWriteDepth = initializer.bWriteDepth;

		mStateValue.bEnableStencilTest = initializer.bEnableStencilTest;

		mStateValue.stencilFun = OpenGLTranslate::To(initializer.stencilFunc);
		mStateValue.stencilFailOp = OpenGLTranslate::To(initializer.stencilFailOp);
		mStateValue.stencilZFailOp = OpenGLTranslate::To(initializer.zFailOp);
		mStateValue.stencilZPassOp = OpenGLTranslate::To(initializer.zPassOp);

		mStateValue.stencilFunBack = OpenGLTranslate::To(initializer.stencilFunBack);
		mStateValue.stencilFailOpBack = OpenGLTranslate::To(initializer.stencilFailOpBack);
		mStateValue.stencilZFailOpBack = OpenGLTranslate::To(initializer.zFailOpBack);
		mStateValue.stencilZPassOpBack = OpenGLTranslate::To(initializer.zPassOpBack);

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
		mStateValue.bEnableMultisample = initializer.bEnableMultisample;
		mStateValue.cullFace = OpenGLTranslate::To(initializer.cullMode);
		mStateValue.fillMode = OpenGLTranslate::To(initializer.fillMode);
	}

	OpenGLInputLayout::OpenGLInputLayout(InputLayoutDesc const& desc)
	{
		for( auto const& e : desc.mElements )
		{
			if (e.attribute == Vertex::ATTRIBUTE_UNUSED )
				continue;

			Element element;
			element.idxStream = e.idxStream;
			element.attribute = e.attribute;
			element.bNormalized = e.bNormalized;
			element.bInstanceData = e.bIntanceData;
			element.instanceStepRate = e.instanceStepRate;
			element.componentNum = Vertex::GetComponentNum(e.format);
			element.componentType = OpenGLTranslate::VertexComponentType(e.format);
			element.stride = desc.getVertexSize(e.idxStream);
			element.offset = e.offset;

			mElements.push_back(element);
		}

		std::sort(mElements.begin(), mElements.end(), [](auto const& lhs, auto const& rhs)
		{
			return lhs.idxStream < rhs.idxStream;
		});
	}

	void OpenGLInputLayout::bindAttrib(InputStreamState const& state)
	{
		InputStreamInfo const* inputStreams = state.inputStreams;
		int numInputStream = state.inputStreamCount;
		assert(numInputStream > 0);
#if 0
		int index = 0;
		for( int indexStream = 0; indexStream < numInputStream; ++indexStream )
		{
			auto& inputStream = inputStreams[indexStream];
			int stride = (inputStream.stride >= 0) ? inputStream.stride : inputStream.buffer->getElementSize();

			OpenGLCast::To(inputStream.buffer)->bind();

			for( ; index < mElements.size(); ++index )
			{
				auto const& e = mElements[index];
				if( e.idxStream > indexStream )
					break;

				glEnableVertexAttribArray(e.attribute);
				if (e.bInstanceData)
				{
					glVertexBindingDivisor(e.attribute, e.instanceStepRate);
				}

				if (stride == 0)
				{
					glBindVertexBuffer(e.attribute, OpenGLCast::GetHandle(*inputStream.buffer), inputStream.offset, inputStream.stride);
				}
				else
				{
					glVertexAttribPointer(e.attribute, e.componentNum, e.componentType, e.bNormalized, stride, (void*)(inputStream.offset + e.offset));
				}	
			}

			OpenGLCast::To(inputStream.buffer)->unbind();
		}

#else
		for (int idxStream = 0 ; idxStream < numInputStream; ++idxStream)
		{
			auto& inputStream = inputStreams[idxStream];
			int stride = (inputStream.stride >= 0) ? inputStream.stride : inputStream.buffer->getElementSize();
			glBindVertexBuffer(idxStream, OpenGLCast::GetHandle(*inputStream.buffer), inputStream.offset, stride);
		}
		for (int index = 0; index < mElements.size(); ++index)
		{
			auto const& e = mElements[index];
			glEnableVertexAttribArray(e.attribute);
			glVertexAttribFormat(e.attribute, e.componentNum, e.componentType, e.bNormalized, e.offset);
			glVertexAttribBinding(e.attribute, e.idxStream);
			if (e.bInstanceData)
			{
				glVertexBindingDivisor(e.idxStream, e.instanceStepRate);
			}
		}
#endif
	}

	void OpenGLInputLayout::bindAttribUP(InputStreamState const& state)
	{
		InputStreamInfo const* inputStreams = state.inputStreams;
		int numInputStream = state.inputStreamCount;
		assert(numInputStream > 0);
		
		for (int index = 0; index < mElements.size(); ++index)
		{
			auto const& e = mElements[index];
			if (e.idxStream >= numInputStream)
				continue;

			auto& inputStream = inputStreams[e.idxStream];
			assert(inputStream.stride >= 0);
			uintptr_t offset = inputStream.offset + e.offset;

			glEnableVertexAttribArray(e.attribute);
			if (inputStream.stride > 0)
			{
				glVertexAttribPointer(e.attribute, e.componentNum, e.componentType, e.bNormalized, inputStream.stride, (void*)offset);
			}
			else
			{
				switch (e.componentNum)
				{
				case 1: glVertexAttrib1fv(e.attribute, (float*)offset); break;
				case 2: glVertexAttrib2fv(e.attribute, (float*)offset); break;
				case 3: glVertexAttrib3fv(e.attribute, (float*)offset); break;
				case 4: glVertexAttrib4fv(e.attribute, (float*)offset); break;
				}
			}
		}
	}

	void OpenGLInputLayout::unbindAttrib(int numInputStream)
	{
		for( auto const& e : mElements )
		{
			if ( e.idxStream >= numInputStream )
				break;

			if ( e.bInstanceData )
				glVertexAttribDivisor(e.attribute, 0);
			glDisableVertexAttribArray(e.attribute);
			
		}
	}

	void BindElementPointer(OpenGLInputLayout::Element const& e, uint32 offset, uint32 stride, bool& haveTex)
	{
		offset += e.offset;
		switch( e.attribute )
		{
		case Vertex::ATTRIBUTE_POSITION:
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(e.componentNum, e.componentType, stride, (void*)(offset));
			break;
		case Vertex::ATTRIBUTE_NORMAL:
			assert(e.componentNum == 3);
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(e.componentType, stride, (void*)(offset));
			break;
		case Vertex::ATTRIBUTE_COLOR:
			if (stride > 0)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(e.componentNum, e.componentType, stride, (void*)(offset));
			}
			else
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColor4fv((GLfloat const*)offset);
			}
			break;
		case Vertex::ATTRIBUTE_COLOR2:
			if (stride > 0 )
			{
				glEnableClientState(GL_SECONDARY_COLOR_ARRAY);
				glSecondaryColorPointer(e.componentNum, e.componentType, stride, (void*)(offset));
			}
			else
			{
				//#TODO: LOG
			}
			break;
		case Vertex::ATTRIBUTE_TANGENT:
			glClientActiveTexture(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD));
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(e.componentNum, e.componentType, stride, (void*)(offset));
			haveTex = true;
			break;
		case Vertex::ATTRIBUTE_TEXCOORD:
		case Vertex::ATTRIBUTE_TEXCOORD1:
		case Vertex::ATTRIBUTE_TEXCOORD2:
		case Vertex::ATTRIBUTE_TEXCOORD3:
		case Vertex::ATTRIBUTE_TEXCOORD4:
		case Vertex::ATTRIBUTE_TEXCOORD5:
		case Vertex::ATTRIBUTE_TEXCOORD6:
			{
				glClientActiveTexture(GL_TEXTURE0 + e.attribute - Vertex::ATTRIBUTE_TEXCOORD);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(e.componentNum, e.componentType, stride, (void*)(offset));
				haveTex = true;
			}
			break;
		}
	}

	void BindElementData(OpenGLInputLayout::Element const& e, uint32 offset)
	{
		offset += e.offset;
		switch( e.attribute )
		{
		case Vertex::ATTRIBUTE_POSITION:
			switch( e.componentNum )
			{
			case 1: glVertex2f(*(float*)(offset), 0); break;
			case 2: glVertex2fv((float*)(offset)); break;
			case 3: glVertex3fv((float*)(offset)); break;
			case 4: glVertex4fv((float*)(offset)); break;
			}
			break;
		case Vertex::ATTRIBUTE_NORMAL:
			assert(e.componentNum == 3);
			glNormal3fv((float*)e.offset); break;
			break;
		case Vertex::ATTRIBUTE_COLOR:
			switch( e.componentNum )
			{
			case 3: glColor3fv((float*)(offset)); break;
			case 4: glColor4fv((float*)(offset)); break;
			}
			break;
		case Vertex::ATTRIBUTE_TANGENT:
			switch( e.componentNum )
			{
			case 1: glMultiTexCoord1fv(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD), (float*)(offset));
			case 2: glMultiTexCoord2fv(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD), (float*)(offset));
			case 3: glMultiTexCoord3fv(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD), (float*)(offset));
			case 4: glMultiTexCoord4fv(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD), (float*)(offset));
			}
			break;
		case Vertex::ATTRIBUTE_TEXCOORD:
		case Vertex::ATTRIBUTE_TEXCOORD1:
		case Vertex::ATTRIBUTE_TEXCOORD2:
		case Vertex::ATTRIBUTE_TEXCOORD3:
		case Vertex::ATTRIBUTE_TEXCOORD4:
		case Vertex::ATTRIBUTE_TEXCOORD5:
		case Vertex::ATTRIBUTE_TEXCOORD6:
			switch( e.componentNum )
			{
			case 1: glMultiTexCoord1fv(GL_TEXTURE0 + e.attribute - Vertex::ATTRIBUTE_TEXCOORD, (float*)(offset));
			case 2: glMultiTexCoord2fv(GL_TEXTURE0 + e.attribute - Vertex::ATTRIBUTE_TEXCOORD, (float*)(offset));
			case 3: glMultiTexCoord3fv(GL_TEXTURE0 + e.attribute - Vertex::ATTRIBUTE_TEXCOORD, (float*)(offset));
			case 4: glMultiTexCoord4fv(GL_TEXTURE0 + e.attribute - Vertex::ATTRIBUTE_TEXCOORD, (float*)(offset));
			}
			break;
		}
	}

	void UnbindElementPointer(OpenGLInputLayout::Element const& e, bool& haveTex)
	{
		switch( e.attribute )
		{
		case Vertex::ATTRIBUTE_POSITION:
			glDisableClientState(GL_VERTEX_ARRAY);
			break;
		case Vertex::ATTRIBUTE_NORMAL:
			glDisableClientState(GL_NORMAL_ARRAY);
			break;
		case Vertex::ATTRIBUTE_COLOR:
			glDisableClientState(GL_COLOR_ARRAY);
			break;
		case Vertex::ATTRIBUTE_COLOR2:
			glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
			break;
		case Vertex::ATTRIBUTE_TANGENT:
			haveTex = true;
			glClientActiveTexture(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD));
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			break;
		case Vertex::ATTRIBUTE_TEXCOORD:
		case Vertex::ATTRIBUTE_TEXCOORD1:
		case Vertex::ATTRIBUTE_TEXCOORD2:
		case Vertex::ATTRIBUTE_TEXCOORD3:
		case Vertex::ATTRIBUTE_TEXCOORD4:
		case Vertex::ATTRIBUTE_TEXCOORD5:
		case Vertex::ATTRIBUTE_TEXCOORD6:
			haveTex = true;
			glClientActiveTexture(GL_TEXTURE0 + e.attribute - Vertex::ATTRIBUTE_TEXCOORD);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			break;
		}
	}

	void OpenGLInputLayout::bindPointer(InputStreamState const& state)
	{
		InputStreamInfo const* inputStreams = state.inputStreams;
		int numInputStream = state.inputStreamCount;

		if( numInputStream == 1 )
		{
			OpenGLCast::To(inputStreams[0].buffer)->bind();
			bool haveTex = false;
			for( auto const& e : mElements )
			{
				uint32 offset = inputStreams[e.idxStream].offset;
				uint32 stride = inputStreams[e.idxStream].stride > 0 ? inputStreams[e.idxStream].stride : e.stride;
				BindElementPointer(e, offset, stride, haveTex);
			}
			OpenGLCast::To(inputStreams[0].buffer)->unbind();
		}
		else
		{
			bool haveTex = false;

			int index = 0;
			for( int indexStream = 0; indexStream < numInputStream; ++indexStream )
			{
				auto& inputStream = inputStreams[indexStream];

				OpenGLCast::To(inputStream.buffer)->bind();

				for( ; index < mElements.size(); ++index )
				{
					auto const& e = mElements[index];
					if( e.idxStream > indexStream )
						break;

					uint32 offset = inputStreams[e.idxStream].offset;
					uint32 stride = inputStreams[e.idxStream].stride > 0 ? inputStreams[e.idxStream].stride : e.stride;
					BindElementPointer(e, offset, stride, haveTex);

				}	
				OpenGLCast::To(inputStream.buffer)->unbind();
			}
		}
	}

	void OpenGLInputLayout::bindPointerUP(InputStreamState const& state)
	{
		InputStreamInfo const* inputStreams = state.inputStreams;
		int numInputStream = state.inputStreamCount;

		bool haveTex = false;
		for (auto const& e : mElements)
		{
			if ( e.idxStream >= numInputStream )
				continue;

			uint32 offset = inputStreams[e.idxStream].offset;
			uint32 stride = inputStreams[e.idxStream].stride;
			if (stride > 0)
			{
				BindElementPointer(e, offset, stride, haveTex);
			}
			else
			{
				BindElementData(e, offset);
			}
		}
	}

	void OpenGLInputLayout::bindPointer()
	{
		bool haveTex = false;
		for( auto const& e : mElements )
		{
			uint32 offset = 0;
			uint32 stride = e.stride;
			BindElementPointer(e, offset, stride, haveTex);
		}
	}

	void OpenGLInputLayout::unbindPointer()
	{
		bool haveTex = false;
		for( auto const& e : mElements )
		{
			UnbindElementPointer(e, haveTex);
		}

		if( haveTex )
		{
			glClientActiveTexture(GL_TEXTURE0);
		}
	}

	GLuint OpenGLInputLayout::bindVertexArray(InputStreamState const& state, bool bBindPointer)
	{
		auto iter = mVAOMap.find(state);
		if (iter != mVAOMap.end())
		{
			glBindVertexArray(iter->second.mHandle);
			return iter->second.mHandle;
		}
		GLuint result = 0;
		VertexArrayObject vao;
		if (vao.fetchHandle())
		{
			result = vao.mHandle;
			mVAOMap.emplace(state, std::move(vao));
			glBindVertexArray(result);
		}

		if (bBindPointer)
			bindPointer(state);
		else
			bindAttrib(state);
		return result;
	}

	void OpenGLInputLayout::releaseResource()
	{
		mVAOMap.clear();
	}

}//namespace Render