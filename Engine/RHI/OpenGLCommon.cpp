#include "OpenGLCommand.h"

#include "GpuProfiler.h"
#include "MarcoCommon.h"
#include "Core/IntegerType.h"

#include "GLExtensions.h"

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
#define CASE( ENUM ) case ENUM : LogMsg( "Error code = %u (%s)" , error , #ENUM ); break;
		CASE(GL_INVALID_ENUM);
		CASE(GL_INVALID_VALUE);
		CASE(GL_INVALID_OPERATION);
		CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
		CASE(GL_STACK_UNDERFLOW);
		CASE(GL_STACK_OVERFLOW);
#undef CASE
		default:
			LogMsg("Unknown error code = %u", error);
		}
		return false;
	}


	void VerifyFrameBufferStatus()
	{
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			switch (status)
			{
#define CASE(ENUM)  case ENUM : LogWarning(0, "Texture Can't Attach to FrameBuffer : %s", status , #ENUM ); break
			CASE(GL_FRAMEBUFFER_UNDEFINED);
			CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
			CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
			CASE(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
			CASE(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
			CASE(GL_FRAMEBUFFER_UNSUPPORTED);
			CASE(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
			CASE(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
#undef CASE
			}
		}
	}


	class FOpenGLBase
	{
	public:
		static void FramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture, level, layer);
			VerifyFrameBufferStatus();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	};
	class FOpenGL45 : public FOpenGLBase
	{
	public:
		static void FramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer)
		{
			glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, layer);
			VerifyFrameBufferStatus();
		}
	};


	using FOpenGL = FOpenGL45;

	bool  UpdateTexture2D(GLenum textureEnum, int ox, int oy, int w, int h, ETexture::Format format, void* data, int level);

	bool  UpdateTexture2D(GLenum textureEnum, int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level);

	bool OpenGLTexture1D::create( void* data )
	{
		if( !mGLObject.fetchHandle() )
			return false;

		bind();

		glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, mDesc.numMipLevel - 1);
		glTexImage1D(TypeEnumGL, 0, OpenGLTranslate::To(mDesc.format), mDesc.dimension.x, 0,
					 OpenGLTranslate::BaseFormat(mDesc.format), OpenGLTranslate::TextureComponentType(mDesc.format), data);

		if(mDesc.numMipLevel > 1)
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

	bool OpenGLTexture1D::update(int offset, int length, ETexture::Format format, void* data, int level /*= 0*/)
	{
		bind();
		glTexSubImage1D(TypeEnumGL, level, offset, length, OpenGLTranslate::PixelFormat(format), OpenGLTranslate::TextureComponentType(format), data);
		bool result = VerifyOpenGLStatus();
		unbind();
		return result;
	}


	bool OpenGLTexture2D::create(void* data , int alignment)
	{
		if( !mGLObject.fetchHandle() )
			return false;

		bind();

		if(mDesc.numSamples > 1)
		{
			glTexImage2DMultisample(TypeEnumGLMultisample, mDesc.numSamples, OpenGLTranslate::To(mDesc.format), mDesc.dimension.x, 
				mDesc.dimension.y, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, mDesc.numMipLevel - 1);


			if( alignment && alignment != GLDefalutUnpackAlignment )
			{
				glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
				glTexImage2D(TypeEnumGL, 0, OpenGLTranslate::To(mDesc.format), mDesc.dimension.x, mDesc.dimension.y, 0,
							 OpenGLTranslate::BaseFormat(mDesc.format), OpenGLTranslate::TextureComponentType(mDesc.format), data);
				glPixelStorei(GL_UNPACK_ALIGNMENT, GLDefalutUnpackAlignment);
			}
			else
			{
				glTexImage2D(TypeEnumGL, 0, OpenGLTranslate::To(mDesc.format), mDesc.dimension.x, mDesc.dimension.y, 0,
							 OpenGLTranslate::BaseFormat(mDesc.format), OpenGLTranslate::TextureComponentType(mDesc.format), data);
			}


			if(mDesc.numMipLevel > 1)
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


	bool OpenGLTexture2D::createDepth()
	{
		if (!mGLObject.fetchHandle())
			return false;

		bind();
		if (mDesc.numSamples > 1)
		{
			glTexImage2DMultisample(TypeEnumGLMultisample, mDesc.numSamples, OpenGLTranslate::DepthFormat(mDesc.format), mDesc.dimension.x, mDesc.dimension.y, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(TypeEnumGL, 0, OpenGLTranslate::DepthFormat(mDesc.format), mDesc.dimension.x, mDesc.dimension.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		}
		VerifyOpenGLStatus();
		unbind();
		return true;
	}

	bool OpenGLTexture2D::update(int ox, int oy, int w, int h, ETexture::Format format , void* data , int level )
	{
		if(mDesc.numSamples > 1)
		{
			return false;
		}
		bind();
		bool result = UpdateTexture2D(TypeEnumGL, ox, oy, w, h, format, data, level);
		unbind();
		return result;
	}

	bool OpenGLTexture2D::update(int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level /*= 0 */)
	{
		if( mDesc.numSamples > 1 )
		{
			return false;
		}

		bind();
		bool result = UpdateTexture2D(TypeEnumGL, ox, oy, w, h, format, dataImageWidth, data, level);
		unbind();
		return result;
	}

	bool OpenGLTexture3D::create(void* data)
	{
		if( !mGLObject.fetchHandle() )
			return false;

		bind();

		GLenum typeEnumGL = getGLTypeEnum();

		if(mDesc.numSamples > 1)
		{
			glTexImage3DMultisample(TypeEnumGLMultisample, mDesc.numSamples, OpenGLTranslate::To(mDesc.format), mDesc.dimension.x, mDesc.dimension.y, mDesc.dimension.z, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, mDesc.numMipLevel - 1);

			glTexImage3D(TypeEnumGL, 0, OpenGLTranslate::To(mDesc.format), mDesc.dimension.x, mDesc.dimension.y, mDesc.dimension.z, 0,
						 OpenGLTranslate::BaseFormat(mDesc.format), OpenGLTranslate::TextureComponentType(mDesc.format), data);

			if(mDesc.numMipLevel > 1)
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

	bool OpenGLTextureCube::create(void* data[])
	{
		if( !mGLObject.fetchHandle() )
			return false;

		bind();
		glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, mDesc.numMipLevel - 1);
		{
			GLenum formatGL = OpenGLTranslate::To(mDesc.format);
			GLenum baseFormat = OpenGLTranslate::BaseFormat(mDesc.format);
			GLenum componentType = OpenGLTranslate::TextureComponentType(mDesc.format);
			for (int face = 0; face < ETexture::FaceCount; ++face)
			{
				glTexImage2D(OpenGLTranslate::TexureType(ETexture::Face(face)), 0,
					formatGL, mDesc.dimension.x, mDesc.dimension.x, 0,
					baseFormat, componentType, data ? data[face] : nullptr);
			}
		}

		if(mDesc.numMipLevel > 1)
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


	bool OpenGLTextureCube::update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, void* data, int level)
	{
		bind();
		bool result = UpdateTexture2D(OpenGLTranslate::TexureType(face), ox, oy, w, h, format, data, level);
		unbind();
		return result;
	}

	bool OpenGLTextureCube::update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level)
	{
		bind();
		bool result = UpdateTexture2D(OpenGLTranslate::TexureType(face), ox, oy, w, h, format, dataImageWidth, data, level);
		unbind();
		return result;
	}

	bool OpenGLTexture2DArray::create(void* data)
	{
		if( !mGLObject.fetchHandle() )
			return false;

		bind();

		if(mDesc.numSamples > 1)
		{
			glTexImage3DMultisample(TypeEnumGLMultisample, mDesc.numSamples, OpenGLTranslate::To(mDesc.format), mDesc.dimension.x, mDesc.dimension.y, mDesc.dimension.z, true);
		}
		else
		{
			glTexParameteri(TypeEnumGL, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(TypeEnumGL, GL_TEXTURE_MAX_LEVEL, mDesc.numMipLevel - 1);

			glTexImage3D(TypeEnumGL, 0, OpenGLTranslate::To(mDesc.format), mDesc.dimension.x, mDesc.dimension.y, mDesc.dimension.z, 0,
						 OpenGLTranslate::BaseFormat(mDesc.format), OpenGLTranslate::TextureComponentType(mDesc.format), data);

			if(mDesc.numMipLevel > 1 )
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

	OpenGLFrameBuffer::OpenGLFrameBuffer()
	{

	}

	OpenGLFrameBuffer::~OpenGLFrameBuffer()
	{
		mTextures.clear();
	}

	int OpenGLFrameBuffer::addTexture( RHITextureCube& target , ETexture::Face face, int level)
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

	int OpenGLFrameBuffer::addTextureArray(RHITextureCube& target, int level)
	{
		int idx = mTextures.size();
		setTextureArray(idx, target, level);
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
		info.indexLayer = INDEX_NONE;
		info.level = level;
		if (target.getDesc().creationFlags & TCF_RenderBufferOptimize)
		{
			info.typeEnumGL = GL_RENDERBUFFER;
			setRenderBufferInternal(idx, static_cast<OpenGLRenderBuffer&>(target).getHandle(), info);
		}
		else
		{
			info.typeEnumGL = OpenGLCast::To(&target)->getGLTypeEnum();
			setTexture2DInternal(idx, OpenGLCast::GetHandle(target), info);
		}
	}

	void OpenGLFrameBuffer::setTexture( int idx , RHITextureCube& target , ETexture::Face face, int level)
	{
		assert( idx <= mTextures.size() );
		if( idx == mTextures.size() )
		{
			mTextures.push_back(BufferInfo());
		}

		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.indexLayer = face;
		info.level = level;
		info.typeEnumGL = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
		setTextureLayerInternal( idx , OpenGLCast::GetHandle( target ) , info);
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
		setTextureLayerInternal(idx, OpenGLCast::GetHandle(target), info);
	}

	void OpenGLFrameBuffer::setTextureArray(int idx, RHITextureCube& target, int level)
	{
		assert(idx <= mTextures.size());
		if (idx == mTextures.size())
		{
			mTextures.push_back(BufferInfo());
		}

		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.indexLayer = INDEX_NONE;
		info.level = level;
		info.typeEnumGL = GL_TEXTURE_CUBE_MAP;
		setTextureArrayInternal(idx, OpenGLCast::GetHandle(target), info);
	}

	void OpenGLFrameBuffer::setRenderBufferInternal(int idx, GLuint handle, BufferInfo const& info)
	{
		assert(getHandle());
		glBindFramebuffer(GL_FRAMEBUFFER, getHandle());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx, GL_RENDERBUFFER, handle);
		VerifyFrameBufferStatus();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::setTexture2DInternal(int idx, GLuint handle, BufferInfo const& info)
	{
		assert( getHandle() );
		glBindFramebuffer( GL_FRAMEBUFFER , getHandle());
		glFramebufferTexture2D( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 + idx , info.typeEnumGL , handle , info.level );
		VerifyFrameBufferStatus();
		glBindFramebuffer(GL_FRAMEBUFFER, 0 );
	}

	void OpenGLFrameBuffer::setTexture3DInternal(int idx, GLuint handle, BufferInfo const& info)
	{
		assert(getHandle());
		glBindFramebuffer(GL_FRAMEBUFFER, getHandle());
		glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx, info.typeEnumGL, handle, info.level , info.indexLayer);
		VerifyFrameBufferStatus();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::setTextureLayerInternal(int idx, GLuint handle, BufferInfo const& info)
	{
		assert(getHandle());
		FOpenGL::FramebufferTextureLayer(getHandle(), GL_COLOR_ATTACHMENT0 + idx, handle, info.level, info.indexLayer);
	}

	void OpenGLFrameBuffer::setTextureArrayInternal(int idx, GLuint handle, BufferInfo const& info)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, getHandle());
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx, handle, info.level);
		VerifyFrameBufferStatus();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::bind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, getHandle() );
		static const GLenum DrawBuffers[] =
		{ 
			GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3, 
			GL_COLOR_ATTACHMENT4 , GL_COLOR_ATTACHMENT5 , GL_COLOR_ATTACHMENT6 , GL_COLOR_ATTACHMENT7, 
#if 0
			GL_COLOR_ATTACHMENT8 , GL_COLOR_ATTACHMENT9 , GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
			GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15,
#endif
		};

		CHECK(mTextures.size() < ARRAY_SIZE(DrawBuffers));
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

	void OpenGLFrameBuffer::setDepthInternal(RHITextureBase& resource, GLuint handle, ETexture::Format format, GLenum typeEnumGL, bool bArray)
	{
		removeDepth();

		mDepth.bufferRef = &resource;
		mDepth.typeEnumGL = typeEnumGL;

		if( handle )
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getHandle());

			GLenum attachType = GL_DEPTH_ATTACHMENT;
			if( ETexture::ContainStencil(format) )
			{
				if( ETexture::ContainDepth(format) )
					attachType = GL_DEPTH_STENCIL_ATTACHMENT;
				else
					attachType = GL_STENCIL_ATTACHMENT;
			}
			if ( bArray )
			{
				glFramebufferTexture(GL_FRAMEBUFFER, attachType, handle, 0);
			}
			else
			{
				if (typeEnumGL == GL_RENDERBUFFER)
				{
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachType, GL_RENDERBUFFER, handle);
				}
				else
				{
					glFramebufferTexture2D(GL_FRAMEBUFFER, attachType, typeEnumGL, handle, 0);
				}
			}

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status != GL_FRAMEBUFFER_COMPLETE )
			{
				LogWarning(0,"DepthBuffer Can't Attach to FrameBuffer");
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
	}

	void OpenGLFrameBuffer::setDepth(RHITexture2D& target)
	{
		GLenum typeEnumGL = (target.getDesc().creationFlags & TCF_RenderBufferOptimize) ? GL_RENDERBUFFER : OpenGLCast::To(&target)->getGLTypeEnum();
		setDepthInternal(target, OpenGLCast::GetHandle(target), target.getFormat(), typeEnumGL);
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


	bool OpenGLRenderBuffer::create()
	{
		if (!mGLObject.fetchHandle())
			return false;

		glBindRenderbuffer(GL_RENDERBUFFER, getHandle());

		GLenum formtGL = ETexture::IsDepthStencil(mDesc.format) ? OpenGLTranslate::DepthFormat(mDesc.format) : OpenGLTranslate::To(mDesc.format);
		if (mDesc.numSamples > 1)
		{
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, mDesc.numSamples, formtGL, mDesc.dimension.x, mDesc.dimension.y);
		}
		else
		{
			glRenderbufferStorage(GL_RENDERBUFFER, formtGL, mDesc.dimension.x, mDesc.dimension.y);
		}

		VerifyOpenGLStatus();
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		return true;
	}


	struct OpenGLTextureConvInfo
	{
#if _DEBUG
		ETexture::Format formatCheck;
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
		TEXTURE_INFO(ETexture::RGBA8   ,GL_RGBA8   ,GL_RGBA,GL_UNSIGNED_BYTE ,GL_RGBA)
		TEXTURE_INFO(ETexture::RGB8    ,GL_RGB8    ,GL_RGB ,GL_UNSIGNED_BYTE ,GL_RGB)
		TEXTURE_INFO(ETexture::RG8     ,GL_RG8     ,GL_RG  ,GL_UNSIGNED_BYTE ,GL_RG)
		TEXTURE_INFO(ETexture::BGRA8   ,GL_RGBA8   ,GL_RGBA,GL_UNSIGNED_BYTE ,GL_BGRA)
		TEXTURE_INFO(ETexture::RGB10A2 ,GL_RGB10_A2,GL_RGBA,GL_UNSIGNED_BYTE ,GL_BGRA)
		TEXTURE_INFO(ETexture::R16     ,GL_R16     ,GL_RED,GL_UNSIGNED_SHORT ,GL_RED)
		TEXTURE_INFO(ETexture::R8      ,GL_R8      ,GL_RED,GL_UNSIGNED_BYTE  ,GL_RED)
		
		TEXTURE_INFO(ETexture::R32F    ,GL_R32F    ,GL_RED ,GL_FLOAT ,GL_RED)
		TEXTURE_INFO(ETexture::RG32F   ,GL_RG32F   ,GL_RG  ,GL_FLOAT ,GL_RG)
		TEXTURE_INFO(ETexture::RGB32F  ,GL_RGB32F  ,GL_RGB ,GL_FLOAT ,GL_RGB)
		TEXTURE_INFO(ETexture::RGBA32F ,GL_RGBA32F ,GL_RGBA,GL_FLOAT ,GL_RGBA)
		TEXTURE_INFO(ETexture::R16F    ,GL_R16F    ,GL_RED ,GL_FLOAT ,GL_RED)
		TEXTURE_INFO(ETexture::RG16F   ,GL_RG16F   ,GL_RG  ,GL_FLOAT ,GL_RG)
		TEXTURE_INFO(ETexture::RGB16F  ,GL_RGB16F  ,GL_RGB ,GL_FLOAT ,GL_RGB)
		TEXTURE_INFO(ETexture::RGBA16F ,GL_RGBA16F ,GL_RGBA,GL_FLOAT ,GL_RGBA)

		TEXTURE_INFO(ETexture::R32I    ,GL_R32I    ,GL_RED_INTEGER ,GL_INT           ,GL_RED_INTEGER)
		TEXTURE_INFO(ETexture::R16I    ,GL_R16I    ,GL_RED_INTEGER ,GL_SHORT         ,GL_RED_INTEGER)
		TEXTURE_INFO(ETexture::R8I     ,GL_R8I     ,GL_RED_INTEGER ,GL_BYTE          ,GL_RED_INTEGER)
		TEXTURE_INFO(ETexture::R32U    ,GL_R32UI   ,GL_RED_INTEGER ,GL_UNSIGNED_INT  ,GL_RED_INTEGER)
		TEXTURE_INFO(ETexture::R16U    ,GL_R16UI   ,GL_RED_INTEGER ,GL_UNSIGNED_SHORT,GL_RED_INTEGER)
		TEXTURE_INFO(ETexture::R8U     ,GL_R8UI    ,GL_RED_INTEGER ,GL_UNSIGNED_BYTE ,GL_RED_INTEGER)
		
		TEXTURE_INFO(ETexture::RG32I   ,GL_RG32I   ,GL_RG_INTEGER ,GL_INT           ,GL_RG_INTEGER)
		TEXTURE_INFO(ETexture::RG16I   ,GL_RG16I   ,GL_RG_INTEGER ,GL_SHORT         ,GL_RG_INTEGER)
		TEXTURE_INFO(ETexture::RG8I    ,GL_RG8I    ,GL_RG_INTEGER ,GL_BYTE          ,GL_RG_INTEGER)
		TEXTURE_INFO(ETexture::RG32U   ,GL_RG32UI  ,GL_RG_INTEGER ,GL_UNSIGNED_INT  ,GL_RG_INTEGER)
		TEXTURE_INFO(ETexture::RG16U   ,GL_RG16UI  ,GL_RG_INTEGER ,GL_UNSIGNED_SHORT,GL_RG_INTEGER)
		TEXTURE_INFO(ETexture::RG8U    ,GL_RG8UI   ,GL_RG_INTEGER ,GL_UNSIGNED_BYTE ,GL_RG_INTEGER)
		
		TEXTURE_INFO(ETexture::RGB32I  ,GL_RGB32I  ,GL_RGB_INTEGER ,GL_INT            ,GL_RGB_INTEGER)
		TEXTURE_INFO(ETexture::RGB16I  ,GL_RGB16I  ,GL_RGB_INTEGER ,GL_SHORT          ,GL_RGB_INTEGER)
		TEXTURE_INFO(ETexture::RGB8I   ,GL_RGB8I   ,GL_RGB_INTEGER ,GL_BYTE           ,GL_RGB_INTEGER)
		TEXTURE_INFO(ETexture::RGB32U  ,GL_RGB32UI ,GL_RGB_INTEGER ,GL_UNSIGNED_INT   ,GL_RGB_INTEGER)
		TEXTURE_INFO(ETexture::RGB16U  ,GL_RGB16UI ,GL_RGB_INTEGER ,GL_UNSIGNED_SHORT ,GL_RGB_INTEGER)
		TEXTURE_INFO(ETexture::RGB8U   ,GL_RGB8UI  ,GL_RGB_INTEGER ,GL_UNSIGNED_BYTE  ,GL_RGB_INTEGER)
		
		TEXTURE_INFO(ETexture::RGBA32I ,GL_RGBA32I  ,GL_RGBA_INTEGER ,GL_INT            ,GL_RGBA_INTEGER)
		TEXTURE_INFO(ETexture::RGBA16I ,GL_RGBA16I  ,GL_RGBA_INTEGER ,GL_SHORT          ,GL_RGBA_INTEGER)
		TEXTURE_INFO(ETexture::RGBA8I  ,GL_RGBA8I   ,GL_RGBA_INTEGER ,GL_BYTE           ,GL_RGBA_INTEGER)
		TEXTURE_INFO(ETexture::RGBA32U ,GL_RGBA32UI ,GL_RGBA_INTEGER ,GL_UNSIGNED_INT   ,GL_RGBA_INTEGER)
		TEXTURE_INFO(ETexture::RGBA16U ,GL_RGBA16UI ,GL_RGBA_INTEGER ,GL_UNSIGNED_SHORT ,GL_RGBA_INTEGER)
		TEXTURE_INFO(ETexture::RGBA8U  ,GL_RGBA8UI  ,GL_RGBA_INTEGER ,GL_UNSIGNED_BYTE  ,GL_RGBA_INTEGER)

		TEXTURE_INFO(ETexture::SRGB   ,GL_SRGB8        ,GL_RGB  ,GL_UNSIGNED_BYTE ,GL_RGB)
		TEXTURE_INFO(ETexture::SRGBA  ,GL_SRGB8_ALPHA8 ,GL_RGBA ,GL_UNSIGNED_BYTE ,GL_RGBA)
			
		
#undef TEXTURE_INFO
	};
#if _DEBUG
	constexpr bool CheckTexConvMapValid()
	{
		for (int i = 0; i < ARRAY_SIZE(GTexConvMapGL); ++i)
		{
			if (i != (int)GTexConvMapGL[i].formatCheck)
				return false;
		}
		return true;
	}
	static_assert(CheckTexConvMapValid(), "CheckTexConvMapValid Error");
#endif

	GLenum OpenGLTranslate::To(ETexture::Format format)
	{
		return GTexConvMapGL[(int)format].foramt;
	}

	GLenum OpenGLTranslate::TextureComponentType(ETexture::Format format)
	{
		return GTexConvMapGL[format].compType;
	}

	GLenum OpenGLTranslate::BaseFormat(ETexture::Format format)
	{
		return GTexConvMapGL[format].baseFormat;
	}

	GLenum OpenGLTranslate::PixelFormat(ETexture::Format format)
	{
		return GTexConvMapGL[format].pixelFormat;
	}

	GLenum OpenGLTranslate::Image2DType(ETexture::Format format)
	{
		switch (format)
		{
		case ETexture::RGBA8:case ETexture::BGRA8:
		case ETexture::RGB8: case ETexture::RG8: case ETexture::RGB10A2:
		case ETexture::R32F:case ETexture::R16:case ETexture::R8:
		case ETexture::RGB32F:
		case ETexture::RGBA32F:
		case ETexture::RGB16F:
		case ETexture::RGBA16F:
			return GL_IMAGE_2D;
		case ETexture::R8I:case ETexture::R16I:case ETexture::R32I:
		case ETexture::RG8I:case ETexture::RG16I:case ETexture::RG32I:
		case ETexture::RGB8I:case ETexture::RGB16I:case ETexture::RGB32I:
		case ETexture::RGBA8I:case ETexture::RGBA16I:case ETexture::RGBA32I:
			return GL_INT_IMAGE_2D;
		case ETexture::R8U:case ETexture::R16U:case ETexture::R32U:
		case ETexture::RG8U:case ETexture::RG16U:case ETexture::RG32U:
		case ETexture::RGB8U:case ETexture::RGB16U:case ETexture::RGB32U:
		case ETexture::RGBA8U:case ETexture::RGBA16U:case ETexture::RGBA32U:
			return GL_UNSIGNED_INT_IMAGE_2D;
		}
		return 0;
	}

	GLenum OpenGLTranslate::TexureType(ETexture::Face face)
	{
		switch (face)
		{
		case ETexture::FaceX: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case ETexture::FaceInvX: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case ETexture::FaceY: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case ETexture::FaceInvY: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case ETexture::FaceZ: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		case ETexture::FaceInvZ:  return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
		}
		NEVER_REACH("");
		return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	}

	GLenum OpenGLTranslate::To(EAccessOp op)
	{
		switch( op )
		{
		case EAccessOp::ReadOnly: return GL_READ_ONLY;
		case EAccessOp::WriteOnly: return GL_WRITE_ONLY;
		case EAccessOp::ReadAndWrite: return GL_READ_WRITE;
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
		case EShader::Task:     return GL_TASK_SHADER_NV;
		case EShader::Mesh:     return GL_MESH_SHADER_NV;
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
		case EShader::Task:     return GL_TASK_SHADER_BIT_NV;
		case EShader::Mesh:     return GL_MESH_SHADER_BIT_NV;
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

	GLenum OpenGLTranslate::To(EBlend::Factor factor)
	{
		switch( factor )
		{
		case EBlend::SrcAlpha: return GL_SRC_ALPHA;
		case EBlend::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case EBlend::One: return GL_ONE;
		case EBlend::Zero: return GL_ZERO;
		case EBlend::DestAlpha: return GL_DST_ALPHA;
		case EBlend::OneMinusDestAlpha: return GL_ONE_MINUS_DST_ALPHA;
		case EBlend::SrcColor: return GL_SRC_COLOR;
		case EBlend::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case EBlend::DestColor: return GL_DST_COLOR;
		case EBlend::OneMinusDestColor: return GL_ONE_MINUS_DST_COLOR;
		}
		return GL_ONE;
	}

	GLenum OpenGLTranslate::To(EBlend::Operation op)
	{
		switch( op )
		{
		case EBlend::Add: return GL_FUNC_ADD;
		case EBlend::Sub: return GL_FUNC_SUBTRACT;
		case EBlend::ReverseSub: return GL_FUNC_REVERSE_SUBTRACT;
		case EBlend::Max: return GL_MIN;
		case EBlend::Min: return GL_MAX;
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
		case ECompareFunc::GreaterEqual:return GL_GEQUAL;
		case ECompareFunc::Always:      return GL_ALWAYS;
		}
		return GL_LESS;
	}

	GLenum OpenGLTranslate::To(EStencil op)
	{
		switch( op )
		{
		case EStencil::Keep:     return GL_KEEP;
		case EStencil::Zero:     return GL_ZERO;
		case EStencil::Replace:  return GL_REPLACE;
		case EStencil::Incr:     return GL_INCR;
		case EStencil::IncrWarp: return GL_INCR_WRAP;
		case EStencil::Decr:     return GL_DECR;
		case EStencil::DecrWarp: return GL_DECR_WRAP;
		case EStencil::Invert:   return GL_INVERT;
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

	GLenum OpenGLTranslate::To(ESampler::Filter filter)
	{
		switch( filter )
		{
		case ESampler::Point:            return GL_NEAREST;
		case ESampler::Bilinear:         return GL_LINEAR;
		case ESampler::Trilinear:        return GL_LINEAR_MIPMAP_LINEAR;
		case ESampler::AnisotroicPoint:  return GL_LINEAR;
		case ESampler::AnisotroicLinear: return GL_LINEAR_MIPMAP_LINEAR;
		}
		return GL_NEAREST;
	}

	GLenum OpenGLTranslate::To(ESampler::AddressMode mode)
	{
		switch( mode )
		{
		case ESampler::Wrap:   return GL_REPEAT;
		case ESampler::Clamp:  return GL_CLAMP_TO_EDGE;
		case ESampler::Mirror: return GL_MIRRORED_REPEAT;
		case ESampler::Border: return GL_CLAMP_TO_BORDER;
		case ESampler::MirrorOnce: return GL_MIRROR_CLAMP_TO_EDGE;
		}
		return GL_REPEAT;
	}

	GLenum OpenGLTranslate::DepthFormat(ETexture::Format format)
	{
		switch( format )
		{
		case ETexture::ShadowDepth:
		case ETexture::Depth16: return GL_DEPTH_COMPONENT16;
		case ETexture::Depth24: return GL_DEPTH_COMPONENT24;
		case ETexture::Depth32: return GL_DEPTH_COMPONENT32;
		case ETexture::Depth32F:return GL_DEPTH_COMPONENT32F;
		case ETexture::D24S8:   return GL_DEPTH24_STENCIL8;
		case ETexture::D32FS8:  return GL_DEPTH32F_STENCIL8;
		case ETexture::Stencil1: return GL_STENCIL_INDEX1;
		case ETexture::Stencil4: return GL_STENCIL_INDEX4;
		case ETexture::Stencil8: return GL_STENCIL_INDEX8;
		case ETexture::Stencil16: return GL_STENCIL_INDEX16;
		}
		return 0;
	}

	GLenum OpenGLTranslate::BufferUsageEnum(uint32 creationFlags)
	{
		if( creationFlags & BCF_CpuAccessWrite )
			return GL_DYNAMIC_DRAW;
		if (creationFlags & BCF_CpuAccessRead)
			return GL_DYNAMIC_READ;
		else if( creationFlags & BCF_UsageStage )
			return GL_STREAM_COPY;
		return GL_STATIC_DRAW;
	}

	bool OpenGLSamplerState::create(SamplerStateInitializer const& initializer)
	{
		if( !mGLObject.fetchHandle() )
		{
			return false;
		}

		glSamplerParameteri(getHandle(), GL_TEXTURE_MIN_FILTER, OpenGLTranslate::To(initializer.filter));
		glSamplerParameteri(getHandle(), GL_TEXTURE_MAG_FILTER, initializer.filter == ESampler::Point ? GL_NEAREST : GL_LINEAR);
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
			targetValueGL.bEnable = (targetValue.srcColor != EBlend::One) || (targetValue.srcAlpha != EBlend::One) ||
				(targetValue.destColor != EBlend::Zero) || (targetValue.destAlpha != EBlend::Zero);

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
		mStateValue.bEnableDepthTest = initializer.isDepthEnable();
		mStateValue.depthFun = OpenGLTranslate::To(initializer.depthFunc);
		mStateValue.bWriteDepth = initializer.bWriteDepth;

		mStateValue.bEnableStencilTest = initializer.bEnableStencil;

		mStateValue.stencilFunc = OpenGLTranslate::To(initializer.stencilFunc);
		mStateValue.stencilFailOp = OpenGLTranslate::To(initializer.stencilFailOp);
		mStateValue.stencilZFailOp = OpenGLTranslate::To(initializer.zFailOp);
		mStateValue.stencilZPassOp = OpenGLTranslate::To(initializer.zPassOp);

		mStateValue.stencilFuncBack = OpenGLTranslate::To(initializer.stencilFuncBack);
		mStateValue.stencilFailOpBack = OpenGLTranslate::To(initializer.stencilFailOpBack);
		mStateValue.stencilZFailOpBack = OpenGLTranslate::To(initializer.zFailOpBack);
		mStateValue.stencilZPassOpBack = OpenGLTranslate::To(initializer.zPassOpBack);

		mStateValue.bUseSeparateStencilOp =
			(mStateValue.stencilFailOp != mStateValue.stencilFailOpBack) ||
			(mStateValue.stencilZFailOp != mStateValue.stencilZFailOpBack) ||
			(mStateValue.stencilZPassOp != mStateValue.stencilZPassOpBack);

		mStateValue.bUseSeparateStencilFunc = (mStateValue.stencilFunc != mStateValue.stencilFuncBack);
		mStateValue.stencilReadMask = initializer.stencilReadMask;
		mStateValue.stencilWriteMask = initializer.stencilWriteMask;
	}

	OpenGLRasterizerState::OpenGLRasterizerState(RasterizerStateInitializer const& initializer)
	{
		mStateValue.bEnableCull = initializer.cullMode != ECullMode::None;
		mStateValue.bEnableScissor = initializer.bEnableScissor;
		mStateValue.bEnableMultisample = initializer.bEnableMultisample;
		mStateValue.cullFace = OpenGLTranslate::To(initializer.cullMode);
		mStateValue.frontFace = initializer.frontFace == EFrontFace::Default ? GL_CW : GL_CCW;
		mStateValue.fillMode = OpenGLTranslate::To(initializer.fillMode);
	}

	OpenGLInputLayout::OpenGLInputLayout(InputLayoutDesc const& desc)
	{
		mAttributeMask = 0;
		for( auto const& e : desc.mElements )
		{
			if (e.attribute == EVertex::ATTRIBUTE_UNUSED )
				continue;

			Element element;
			element.streamIndex = e.streamIndex;
			element.attribute = e.attribute;
			element.bNormalized = e.bNormalized;
			element.bInstanceData = e.bIntanceData;
			element.instanceStepRate = e.instanceStepRate;
			element.componentNum = EVertex::GetComponentNum(e.format);
			element.componentType = OpenGLTranslate::VertexComponentType(e.format);
			element.bIntType = !e.bNormalized && EVertex::IsIntType(EVertex::GetComponentType(e.format));
			element.stride = desc.getVertexSize(e.streamIndex);
			element.offset = e.offset;

			mAttributeMask |= BIT(e.attribute);

			mElements.push_back(element);
		}

		std::sort(mElements.begin(), mElements.end(), [](auto const& lhs, auto const& rhs)
		{
			return lhs.streamIndex < rhs.streamIndex;
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
			if (e.bIntType)
			{
				glVertexAttribIFormat(e.attribute, e.componentNum, e.componentType, e.offset);
			}
			else
			{
				glVertexAttribFormat(e.attribute, e.componentNum, e.componentType, e.bNormalized, e.offset);
			}
			glVertexAttribBinding(e.attribute, e.streamIndex);
			if (e.bInstanceData)
			{
				glVertexBindingDivisor(e.streamIndex, e.instanceStepRate);
			}
		}
#endif
	}

	template< class T >
	void SetVertexAttrib(uint8 attribute, uint8 componentNum , void* offset)
	{
		T* pData = (T*)offset;
		switch (e.componentNum)
		{
		case 1: glVertexAttrib1f(e.attribute, float(pData[0])); break;
		case 2: glVertexAttrib2f(e.attribute, float(pData[0]), float(pData[1])); break;
		case 3: glVertexAttrib3f(e.attribute, float(pData[0]), float(pData[1]), float(pData[2])); break;
		case 4: glVertexAttrib4f(e.attribute, float(pData[0]), float(pData[1]), float(pData[2]), float(pData[3])); break;
		}
	}

	void OpenGLInputLayout::bindAttribUP(InputStreamState const& state)
	{
		InputStreamInfo const* inputStreams = state.inputStreams;
		int numInputStream = state.inputStreamCount;
		assert(numInputStream > 0);
		
		for (int index = 0; index < mElements.size(); ++index)
		{
			auto const& e = mElements[index];
			if (e.streamIndex >= numInputStream)
				continue;

			auto& inputStream = inputStreams[e.streamIndex];
			assert(inputStream.stride >= 0);
			intptr_t offset = inputStream.offset + e.offset;

			if (inputStream.stride > 0)
			{
				glEnableVertexAttribArray(e.attribute);
				glVertexAttribPointer(e.attribute, e.componentNum, e.componentType, e.bNormalized, inputStream.stride, (void*)offset);
			}
			else
			{
				switch(e.componentType)
				{
				case GL_FLOAT:
					{
						switch (e.componentNum)
						{
						case 1: glVertexAttrib1fv(e.attribute, (float*)offset); break;
						case 2: glVertexAttrib2fv(e.attribute, (float*)offset); break;
						case 3: glVertexAttrib3fv(e.attribute, (float*)offset); break;
						case 4: glVertexAttrib4fv(e.attribute, (float*)offset); break;
						}
					}
					break;
				case GL_INT:
					{
						switch (e.componentNum)
						{
						case 1: glVertexAttribI1iv(e.attribute, (int*)offset); break;
						case 2: glVertexAttribI2iv(e.attribute, (int*)offset); break;
						case 3: glVertexAttribI3iv(e.attribute, (int*)offset); break;
						case 4: glVertexAttribI4iv(e.attribute, (int*)offset); break;
						}
						break;
					}
				default:
					LogWarning(0, "glVertexAttrib not impl");
				}
			}
		}
	}

	void OpenGLInputLayout::unbindAttrib(int numInputStream)
	{
		for( auto const& e : mElements )
		{
			if ( e.streamIndex >= numInputStream )
				break;

			if ( e.bInstanceData )
				glVertexAttribDivisor(e.attribute, 0);

			glDisableVertexAttribArray(e.attribute);		
		}
	}

	void BindElementPointer(OpenGLInputLayout::Element const& e, intptr_t offset, uint32 stride, bool& haveTex)
	{
		offset += e.offset;
		switch( e.attribute )
		{
		case EVertex::ATTRIBUTE_POSITION:
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(e.componentNum, e.componentType, stride, (void*)(offset));
			break;
		case EVertex::ATTRIBUTE_NORMAL:
			assert(e.componentNum == 3);
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(e.componentType, stride, (void*)(offset));
			break;
		case EVertex::ATTRIBUTE_COLOR:
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
		case EVertex::ATTRIBUTE_COLOR2:
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
		case EVertex::ATTRIBUTE_TANGENT:
			glClientActiveTexture(GL_TEXTURE0 + (EVertex::ATTRIBUTE_TANGENT - EVertex::ATTRIBUTE_TEXCOORD));
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(e.componentNum, e.componentType, stride, (void*)(offset));
			haveTex = true;
			break;
		case EVertex::ATTRIBUTE_TEXCOORD:
		case EVertex::ATTRIBUTE_TEXCOORD1:
		case EVertex::ATTRIBUTE_TEXCOORD2:
		case EVertex::ATTRIBUTE_TEXCOORD3:
		case EVertex::ATTRIBUTE_TEXCOORD4:
		case EVertex::ATTRIBUTE_TEXCOORD5:
		case EVertex::ATTRIBUTE_TEXCOORD6:
			{
				glClientActiveTexture(GL_TEXTURE0 + e.attribute - EVertex::ATTRIBUTE_TEXCOORD);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(e.componentNum, e.componentType, stride, (void*)(offset));
				haveTex = true;
			}
			break;
		}
	}

	void BindElementData(OpenGLInputLayout::Element const& e, intptr_t offset)
	{
		offset += e.offset;
		switch( e.attribute )
		{
		case EVertex::ATTRIBUTE_POSITION:
			switch( e.componentNum )
			{
			case 1: glVertex2f(*(float*)(offset), 0); break;
			case 2: glVertex2fv((float*)(offset)); break;
			case 3: glVertex3fv((float*)(offset)); break;
			case 4: glVertex4fv((float*)(offset)); break;
			}
			break;
		case EVertex::ATTRIBUTE_NORMAL:
			assert(e.componentNum == 3);
			glNormal3fv((float*)e.offset); break;
			break;
		case EVertex::ATTRIBUTE_COLOR:
			switch( e.componentNum )
			{
			case 3: glColor3fv((float*)(offset)); break;
			case 4: glColor4fv((float*)(offset)); break;
			}
			break;
		case EVertex::ATTRIBUTE_TANGENT:
			switch( e.componentNum )
			{
			case 1: glMultiTexCoord1fv(GL_TEXTURE0 + (EVertex::ATTRIBUTE_TANGENT - EVertex::ATTRIBUTE_TEXCOORD), (float*)(offset));
			case 2: glMultiTexCoord2fv(GL_TEXTURE0 + (EVertex::ATTRIBUTE_TANGENT - EVertex::ATTRIBUTE_TEXCOORD), (float*)(offset));
			case 3: glMultiTexCoord3fv(GL_TEXTURE0 + (EVertex::ATTRIBUTE_TANGENT - EVertex::ATTRIBUTE_TEXCOORD), (float*)(offset));
			case 4: glMultiTexCoord4fv(GL_TEXTURE0 + (EVertex::ATTRIBUTE_TANGENT - EVertex::ATTRIBUTE_TEXCOORD), (float*)(offset));
			}
			break;
		case EVertex::ATTRIBUTE_TEXCOORD:
		case EVertex::ATTRIBUTE_TEXCOORD1:
		case EVertex::ATTRIBUTE_TEXCOORD2:
		case EVertex::ATTRIBUTE_TEXCOORD3:
		case EVertex::ATTRIBUTE_TEXCOORD4:
		case EVertex::ATTRIBUTE_TEXCOORD5:
		case EVertex::ATTRIBUTE_TEXCOORD6:
			switch( e.componentNum )
			{
			case 1: glMultiTexCoord1fv(GL_TEXTURE0 + e.attribute - EVertex::ATTRIBUTE_TEXCOORD, (float*)(offset));
			case 2: glMultiTexCoord2fv(GL_TEXTURE0 + e.attribute - EVertex::ATTRIBUTE_TEXCOORD, (float*)(offset));
			case 3: glMultiTexCoord3fv(GL_TEXTURE0 + e.attribute - EVertex::ATTRIBUTE_TEXCOORD, (float*)(offset));
			case 4: glMultiTexCoord4fv(GL_TEXTURE0 + e.attribute - EVertex::ATTRIBUTE_TEXCOORD, (float*)(offset));
			}
			break;
		}
	}

	void UnbindElementPointer(OpenGLInputLayout::Element const& e, bool& haveTex)
	{
		switch( e.attribute )
		{
		case EVertex::ATTRIBUTE_POSITION:
			glDisableClientState(GL_VERTEX_ARRAY);
			break;
		case EVertex::ATTRIBUTE_NORMAL:
			glDisableClientState(GL_NORMAL_ARRAY);
			break;
		case EVertex::ATTRIBUTE_COLOR:
			glDisableClientState(GL_COLOR_ARRAY);
			break;
		case EVertex::ATTRIBUTE_COLOR2:
			glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
			break;
		case EVertex::ATTRIBUTE_TANGENT:
			haveTex = true;
			glClientActiveTexture(GL_TEXTURE0 + (EVertex::ATTRIBUTE_TANGENT - EVertex::ATTRIBUTE_TEXCOORD));
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			break;
		case EVertex::ATTRIBUTE_TEXCOORD:
		case EVertex::ATTRIBUTE_TEXCOORD1:
		case EVertex::ATTRIBUTE_TEXCOORD2:
		case EVertex::ATTRIBUTE_TEXCOORD3:
		case EVertex::ATTRIBUTE_TEXCOORD4:
		case EVertex::ATTRIBUTE_TEXCOORD5:
		case EVertex::ATTRIBUTE_TEXCOORD6:
			haveTex = true;
			glClientActiveTexture(GL_TEXTURE0 + e.attribute - EVertex::ATTRIBUTE_TEXCOORD);
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
			auto& inputStream = inputStreams[0];
			glBindBuffer(GL_VERTEX_ARRAY, OpenGLCast::GetHandle(inputStream.buffer));
			bool haveTex = false;
			for( auto const& e : mElements )
			{
				uint32 offset = inputStreams[e.streamIndex].offset;
				uint32 stride = inputStreams[e.streamIndex].stride > 0 ? inputStreams[e.streamIndex].stride : e.stride;
				BindElementPointer(e, offset, stride, haveTex);
			}
			glBindBuffer(GL_VERTEX_ARRAY, 0);
		}
		else
		{
			bool haveTex = false;

			int index = 0;
			for( int indexStream = 0; indexStream < numInputStream; ++indexStream )
			{
				auto& inputStream = inputStreams[indexStream];

				glBindBuffer(GL_VERTEX_ARRAY, OpenGLCast::GetHandle(inputStream.buffer));
				for( ; index < mElements.size(); ++index )
				{
					auto const& e = mElements[index];
					if( e.streamIndex > indexStream )
						break;

					uint32 offset = inputStreams[e.streamIndex].offset;
					uint32 stride = inputStreams[e.streamIndex].stride > 0 ? inputStreams[e.streamIndex].stride : e.stride;
					BindElementPointer(e, offset, stride, haveTex);

				}	
				glBindBuffer(GL_VERTEX_ARRAY, 0);
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
			if ( e.streamIndex >= numInputStream )
				continue;

			intptr_t offset = inputStreams[e.streamIndex].offset;
			uint32 stride = inputStreams[e.streamIndex].stride;
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
			intptr_t offset = 0;
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