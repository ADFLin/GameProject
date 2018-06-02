#include "GLCommon.h"

#include "GpuProfiler.h"
#include "MarcoCommon.h"
#include "Core/IntegerType.h"
#include "FileSystem.h"

#include "stb/stb_image.h"

#include <fstream>
#include <sstream>

namespace RenderGL
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

	VertexDecl::VertexDecl()
	{
		mVertexSize = 0;
	}

	void VertexDecl::bindPointer(LinearColor const* overwriteColor)
	{
		bool haveTex = false;
		for( VertexElement& info : mElements )
		{
			switch( info.semantic )
			{
			case Vertex::ePosition:
				glEnableClientState( GL_VERTEX_ARRAY );
				glVertexPointer( Vertex::GetComponentNum( info.format ) , Vertex::GetComponentType( info.format ) , mVertexSize , (void*)info.offset );
				break;
			case Vertex::eNormal:
				assert( Vertex::GetComponentNum( info.format ) == 3 );
				glEnableClientState( GL_NORMAL_ARRAY );
				glNormalPointer(Vertex::GetComponentType(info.format ) , mVertexSize , (void*)info.offset );
				break;
			case Vertex::eColor:
				if ( info.idx == 0 )
				{
					if( overwriteColor == nullptr )
					{
						glEnableClientState(GL_COLOR_ARRAY);
						glColorPointer(Vertex::GetComponentNum(info.format), Vertex::GetComponentType(info.format), mVertexSize, (void*)info.offset);
					}
				}
				else
				{
					glEnableClientState( GL_SECONDARY_COLOR_ARRAY );
					glSecondaryColorPointer( Vertex::GetComponentNum( info.format ) , Vertex::GetComponentType( info.format ) , mVertexSize , (void*)info.offset );
				}
				break;
			case Vertex::eTangent:
				glClientActiveTexture(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD));
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(Vertex::GetComponentNum(info.format), Vertex::GetComponentType( info.format), mVertexSize, (void*)info.offset);
				haveTex = true;
				break;
			case Vertex::eTexcoord:
				glClientActiveTexture( GL_TEXTURE0 + info.idx );
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( Vertex::GetComponentNum( info.format ) , Vertex::GetComponentType( info.format ) , mVertexSize , (void*)info.offset );
				haveTex = true;
				break;
			}
		}

		if( overwriteColor )
		{
			glColor4fv(*overwriteColor);
		}
	}

	void VertexDecl::unbindPointer(LinearColor const* overwriteColor)
	{
		bool haveTex = false;
		for( VertexElement& info : mElements )
		{
			switch( info.semantic )
			{
			case Vertex::ePosition:
				glDisableClientState( GL_VERTEX_ARRAY );
				break;
			case Vertex::eNormal:
				glDisableClientState( GL_NORMAL_ARRAY );
				break;
			case Vertex::eColor:
				if ( overwriteColor == nullptr )
					glDisableClientState( ( info.idx == 0) ? GL_COLOR_ARRAY:GL_SECONDARY_COLOR_ARRAY );
				break;
			case Vertex::eTangent:
				haveTex = true;
				glClientActiveTexture(GL_TEXTURE0 + (Vertex::ATTRIBUTE_TANGENT - Vertex::ATTRIBUTE_TEXCOORD));
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				break;
			case Vertex::eTexcoord:
				haveTex = true;
				glClientActiveTexture( GL_TEXTURE1 + info.idx );
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );
				break;
			}
		}

		if( overwriteColor )
		{
			glColor4f(1, 1, 1, 1);
		}
		if ( haveTex )
			glClientActiveTexture( GL_TEXTURE0 );
	}

	void VertexDecl::bindAttrib(LinearColor const* overwriteColor)
	{
		for( VertexElement& info : mElements )
		{
			glEnableVertexAttribArray(info.attribute);
			glVertexAttribPointer(info.attribute, Vertex::GetComponentNum(info.format), Vertex::GetComponentType(info.format), GL_FALSE, mVertexSize, (void*)info.offset);
		}
		if( overwriteColor )
		{
			glDisableVertexAttribArray(Vertex::ATTRIBUTE_COLOR);
			glVertexAttrib4fv(Vertex::ATTRIBUTE_COLOR, *overwriteColor);
		}

	}

	void VertexDecl::unbindAttrib(LinearColor const* overwriteColor)
	{
		for( VertexElement& info : mElements )
		{
			glDisableVertexAttribArray(info.attribute);
		}
	}
	static Vertex::Semantic AttributeToSemantic(uint8 attribute, uint8& idx)
	{
		switch( attribute )
		{
		case Vertex::ATTRIBUTE_POSITION: idx = 0; return Vertex::ePosition;
		case Vertex::ATTRIBUTE_COLOR: idx = 0; return Vertex::eColor;
		case Vertex::ATTRIBUTE_COLOR2: idx = 1; return Vertex::eColor;
		case Vertex::ATTRIBUTE_NORMAL: idx = 0; return Vertex::eNormal;
		case Vertex::ATTRIBUTE_TANGENT: idx = 0; return Vertex::eTangent;
		}

		idx = attribute - Vertex::ATTRIBUTE_TEXCOORD;
		return Vertex::eTexcoord;
	}

	static uint8 SemanticToAttribute(Vertex::Semantic s , uint8 idx)
	{
		switch( s )
		{
		case Vertex::ePosition: return Vertex::ATTRIBUTE_POSITION;
		case Vertex::eColor:    assert(idx < 2); return Vertex::ATTRIBUTE_COLOR + idx;
		case Vertex::eNormal:   return Vertex::ATTRIBUTE_NORMAL;
		case Vertex::eTangent:  return Vertex::ATTRIBUTE_TANGENT;
		case Vertex::eTexcoord: assert(idx < 7); return Vertex::ATTRIBUTE_TEXCOORD + idx;
		}
		return 0;
	}

	VertexDecl& VertexDecl::addElement(Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		VertexElement element;
		element.idxStream = 0;
		element.attribute = SemanticToAttribute(s, idx);
		element.format   = f;
		element.offset   = mVertexSize;
		element.semantic = s;
		element.idx      = idx;
		element.bNormalize = false;
		mElements.push_back( element );
		mVertexSize += Vertex::GetFormatSize( f );
		return *this;
	}

	VertexDecl& VertexDecl::addElement(uint8 attribute, Vertex::Format f, bool bNormailze)
	{
		VertexElement element;
		element.idxStream = 0;
		element.attribute = attribute;
		element.format = f;
		element.offset = mVertexSize;
		element.semantic = AttributeToSemantic(attribute, element.idx);
		element.bNormalize = bNormailze;
		mElements.push_back(element);
		mVertexSize += Vertex::GetFormatSize(f);
		return *this;
	}

	VertexDecl& VertexDecl::addElement(uint8 idxStream , Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		VertexElement element;
		element.idxStream = idxStream;
		element.attribute = SemanticToAttribute(s, idx);
		element.format = f;
		element.offset = mVertexSize;
		element.semantic = s;
		element.idx = idx;
		element.bNormalize = false;
		mElements.push_back(element);
		mVertexSize += Vertex::GetFormatSize(f);
		return *this;
	}

	VertexDecl& VertexDecl::addElement(uint8 idxStream, uint8 attribute, Vertex::Format f, bool bNormailze)
	{
		VertexElement element;
		element.idxStream = idxStream;
		element.attribute = attribute;
		element.format = f;
		element.offset = mVertexSize;
		element.semantic = AttributeToSemantic(attribute, element.idx);
		element.bNormalize = bNormailze;
		mElements.push_back(element);
		mVertexSize += Vertex::GetFormatSize(f);
		return *this;
	}


	VertexElement const* VertexDecl::findBySematic(Vertex::Semantic s , int idx) const
	{
		for( auto const& decl : mElements )
		{
			if ( decl.semantic == s && decl.idx == idx )
				return &decl;
		}
		return nullptr;
	}

	VertexElement const* VertexDecl::findBySematic(Vertex::Semantic s) const
	{
		for( auto const& decl : mElements )
		{
			if( decl.semantic == s )
				return &decl;
		}
		return nullptr;
	}

	int VertexDecl::getSematicOffset(Vertex::Semantic s) const
	{
		VertexElement const* info = findBySematic( s );
		return ( info ) ? info->offset : -1;
	}

	int VertexDecl::getSematicOffset(Vertex::Semantic s , int idx) const
	{
		VertexElement const* info = findBySematic( s ,idx );
		return ( info ) ? info->offset : -1;
	}

	Vertex::Format VertexDecl::getSematicFormat(Vertex::Semantic s) const
	{
		VertexElement const* info = findBySematic( s );
		return ( info ) ? Vertex::Format( info->format ) : Vertex::eUnknowFormat;
	}

	Vertex::Format VertexDecl::getSematicFormat(Vertex::Semantic s , int idx) const
	{
		VertexElement const* info = findBySematic( s , idx );
		return ( info ) ? Vertex::Format( info->format ) : Vertex::eUnknowFormat;
	}

	bool RHITextureBase::loadFileInternal(char const* path, GLenum texType , GLenum texImageType, IntVector2& outSize , Texture::Format& outFormat)
	{
		int w;
		int h;
		int comp;
		unsigned char* image = stbi_load( path , &w, &h, &comp, STBI_default );

		if( !image )
			return false;

		outSize.x = w;
		outSize.y = h;

		glTexParameteri( texType, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( texType, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		//#TODO
		switch( comp )
		{
		case 3:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(texImageType, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			glPixelStorei(GL_UNPACK_ALIGNMENT, GLDefalutUnpackAlignment);
			outFormat = Texture::eRGB8;
			break;
		case 4:
			glTexImage2D(texImageType, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
			outFormat = Texture::eRGBA8;
			break;
		}
		//glGenerateMipmap( texType);
		stbi_image_free(image);
		return true;
	}

	bool RHITexture1D::create(Texture::Format format, int length, int numMipLevel /*= 0*/, void* data /*= nullptr*/)
	{
		if( !fetchHandle() )
			return false;
		mSize = length;

		mFormat = format;

		bind();
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if( numMipLevel )
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);

		glTexImage1D(GL_TEXTURE_1D, 0, GLConvert::To(format), length, 0,
					 Texture::GetBaseFormat(format), Texture::GetComponentType(format), data);

		CheckGLStateValid();
		unbind();
		return true;
	}

	bool RHITexture1D::update(int offset, int length, Texture::Format format, void* data, int level /*= 0*/)
	{
		bind();
		glTexSubImage1D(GL_TEXTURE_1D, level, offset, length, Texture::GetPixelFormat(format), Texture::GetComponentType(format), data);
		bool result = CheckGLStateValid();
		unbind();
		return result;
	}


	bool RHITexture2D::create(Texture::Format format, int width, int height, int numMipLevel, void* data , int alignment )
	{
		if( !fetchHandle() )
			return false;
		mSizeX = width;
		mSizeY = height;
		mFormat = format;

		bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if ( numMipLevel )
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMipLevel - 1);
		if ( alignment && alignment != GLDefalutUnpackAlignment )
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
			glTexImage2D(GL_TEXTURE_2D, 0, GLConvert::To(format), width, height, 0,
						 Texture::GetBaseFormat(format), Texture::GetComponentType(format), data);
			glPixelStorei(GL_UNPACK_ALIGNMENT, GLDefalutUnpackAlignment);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GLConvert::To(format), width, height, 0,
						 Texture::GetBaseFormat(format), Texture::GetComponentType(format), data);
		}
		CheckGLStateValid();
		unbind();
		return true;
	}

	bool RHITexture2D::loadFromFile(char const* path)
	{
		if ( !fetchHandle() )
			return false;

		bind();
		IntVector2 size;
		bool result = loadFileInternal( path , GL_TEXTURE_2D , GL_TEXTURE_2D , size , mFormat );
		if( result )
		{
			mSizeX = size.x;
			mSizeY = size.y;
		}
		unbind();
		return result;
	}


	bool RHITexture2D::update(int ox, int oy, int w, int h, Texture::Format format , void* data , int level )
	{
		bind();
		glTexSubImage2D(GL_TEXTURE_2D, level, ox, oy, w, h, Texture::GetPixelFormat(format), Texture::GetComponentType(format), data);
		bool result = CheckGLStateValid();
		unbind();
		return result;
	}

	bool RHITexture2D::update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level /*= 0 */)
	{
		bind();
#if 1
		::glPixelStorei(GL_UNPACK_ROW_LENGTH, pixelStride);
		glTexSubImage2D(GL_TEXTURE_2D, level, ox, oy, w, h, Texture::GetPixelFormat(format), Texture::GetComponentType(format), data);
		bool result = CheckGLStateValid();
		::glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#else
		GLenum formatGL = Texture::GetPixelFormat(format);
		GLenum typeGL = Texture::GetComponentType(format);
		uint8* pData = (uint8*)data;
		int dataStride = pixelStride * Texture::GetFormatSize(format);
		for( int dy = 0; dy < h; ++dy )
		{
			glTexSubImage2D(GL_TEXTURE_2D, level, ox, oy+dy , w, 1, formatGL , typeGL,pData);
			pData += dataStride;
		}
		bool result = CheckGLStateValid();
#endif
		unbind();
		return result;
	}

	bool RHITexture3D::create(Texture::Format format, int sizeX, int sizeY, int sizeZ)
	{
		if( !fetchHandle() )
			return false;
		
		mSizeX = sizeX;
		mSizeY = sizeY;
		mSizeZ = sizeZ;
		mFormat = format;

		bind();
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage3D(GL_TEXTURE_3D, 0, GLConvert::To(format), sizeX, sizeY , sizeZ, 0, 
					 Texture::GetBaseFormat(format), Texture::GetComponentType(format), NULL);
		unbind();
		return true;
	}

	bool RHITextureCube::create(Texture::Format format, int width, int height , void* data )
	{
		if( !fetchHandle() )
			return false;

		bind();
		for( int i = 0; i < 6; ++i )
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 
						 GLConvert::To(format), width, height, 0,
						 Texture::GetBaseFormat(format), Texture::GetComponentType(format), data );
		}
		unbind();
		return true;
	}

	bool RHITextureCube::loadFile(char const* path[])
	{
		if ( !fetchHandle() )
			return false;

		bind();
		bool result = true; 
		for( int i = 0 ; i < 6 ; ++i )
		{
			IntVector2 size;
			if ( !loadFileInternal( path[i] , GL_TEXTURE_CUBE_MAP , GL_TEXTURE_CUBE_MAP_POSITIVE_X + i , size , mFormat) )
			{
				result = false;
			}
		}
		unbind();
		return result;
	}

	bool RHITextureDepth::create(Texture::DepthFormat format, int width, int height)
	{
		if( !fetchHandle() )
			return false;
		
		mFromat = format;

		bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0,  Texture::Convert(format), width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		GLenum error = glGetError();
		if( error != GL_NO_ERROR )
		{
		}
		unbind();
		return true;	
	}

	FrameBuffer::FrameBuffer()
	{
		mFBO = 0;
	}

	FrameBuffer::~FrameBuffer()
	{
		if( mFBO )
		{
			glDeleteFramebuffers(1, &mFBO);
			mFBO = 0;
		}
		mTextures.clear();
	}

	int FrameBuffer::addTexture( RHITextureCube& target , Texture::Face face  )
	{
		int idx = mTextures.size();

		BufferInfo info;
		info.bufferRef = &target;
		info.idxFace   = face;
		mTextures.push_back( info );
		
		setTextureInternal( idx , target.mHandle , GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.idxFace );
		return idx;
	}

	int FrameBuffer::addTexture( RHITexture2D& target )
	{
		int idx = mTextures.size();

		BufferInfo info;
		info.bufferRef = &target;
		info.idxFace  = 0;
		mTextures.push_back( info );
		setTextureInternal( idx , target.mHandle , GL_TEXTURE_2D );
		
		return idx;
	}

	void FrameBuffer::setTexture( int idx , RHITexture2D& target )
	{
		assert( idx < mTextures.size() );
		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.idxFace  = 0;
		setTextureInternal( idx , target.mHandle , GL_TEXTURE_2D );
	}

	void FrameBuffer::setTexture( int idx , RHITextureCube& target , Texture::Face face )
	{
		assert( idx < mTextures.size() );
		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.idxFace   = face;
		setTextureInternal( idx , target.mHandle , GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.idxFace );
	}


	void FrameBuffer::setTextureInternal(int idx, GLuint handle , GLenum texType)
	{
		assert( mFBO );
		glBindFramebuffer( GL_FRAMEBUFFER , mFBO );
		glFramebufferTexture2D( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 + idx , texType , handle , 0 );
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if( Status != GL_FRAMEBUFFER_COMPLETE )
		{
			LogWarning(0,"Texture Can't Attach to FrameBuffer");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0 );
	}

	void FrameBuffer::bindDepthOnly()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	}


	void FrameBuffer::bind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, mFBO );
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

	void FrameBuffer::unbind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

	bool FrameBuffer::create()
	{
		glGenFramebuffers( 1, &mFBO ); 
		//glBindFramebuffer( GL_DRAW_FRAMEBUFFER , mFBO );
		return mFBO != 0;
	}

	int FrameBuffer::addTextureLayer(RHITextureCube& target)
	{
		int idx = mTextures.size();
		BufferInfo info;
		info.bufferRef = &target;
		info.idxFace = 0;
		mTextures.push_back(info);
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target.mHandle, 0);
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if( Status != GL_FRAMEBUFFER_COMPLETE )
		{
			LogWarning(0,"Texture Can't Attach to FrameBuffer");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return idx;
	}

	void FrameBuffer::clearBuffer(Vector4 const* colorValue, float const* depthValue, uint8 stencilValue)
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

	void FrameBuffer::setDepthInternal( RHIResource& resource , GLuint handle, Texture::DepthFormat format, bool bTexture )
	{
		removeDepthBuffer();

		mDepth.bufferRef = &resource;
		mDepth.bTexture = bTexture;

		if( handle )
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFBO);

			GLenum attachType = GL_DEPTH_ATTACHMENT;
			if( Texture::ContainStencil(format) )
			{
				if( Texture::ContainDepth(format) )
					attachType = GL_DEPTH_STENCIL_ATTACHMENT;
				else
					attachType = GL_STENCIL_ATTACHMENT;
			}

			if( bTexture )
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachType , GL_TEXTURE_2D, handle, 0);
			}
			else
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachType, GL_RENDERBUFFER, handle);
			}


			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status != GL_FRAMEBUFFER_COMPLETE )
			{
				LogWarning(0,"DepthBuffer Can't Attach to FrameBuffer");
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
	}

	void FrameBuffer::setDepth( RHIDepthRenderBuffer& buffer)
	{
		setDepthInternal( buffer , buffer.mHandle , buffer.getFormat() , false );
	}

	void FrameBuffer::setDepth(RHITextureDepth& target)
	{
		setDepthInternal( target , target.mHandle, target.getFormat(), true );
	}

	void FrameBuffer::removeDepthBuffer()
	{
		if ( mDepth.bufferRef )
		{
			mDepth.bufferRef = nullptr;
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFBO);
			if( mDepth.bTexture )
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			}
			else
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

	}

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

	GLenum Texture::Convert(DepthFormat format)
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

	GLenum GLConvert::To(Texture::Format format)
	{
		return gTexConvMap[(int)format].foramt;
	}

	GLenum Texture::GetComponentType(Format format)
	{
		return gTexConvMap[format].compType;
	}

	GLenum Texture::GetPixelFormat(Format format)
	{
		switch( format )
		{
		case eR32F: case eR16: case eR8:
		case eR8I:case eR16I:case eR32I:
		case eR8U:case eR16U:case eR32U:
			return GL_RED;
		case eRG8I:case eRG16I:case eRG32I:
		case eRG8U:case eRG16U:case eRG32U:
			return GL_RG;
		case eRGB8:
		case eRGB32F:case eRGB16F:
		case eRGB8I:case eRGB16I:case eRGB32I:
		case eRGB8U:case eRGB16U:case eRGB32U:
			return GL_RGB;
		case eRGBA8:
		case eRGBA32F:case eRGBA16F:
		case eRGBA8I:case eRGBA16I:case eRGBA32I:
		case eRGBA8U:case eRGBA16U:case eRGBA32U:
			return GL_RGBA;
		}
		return 0;
	}

	GLenum Texture::GetBaseFormat(Format format)
	{
		switch( format )
		{
		case eRGB8: case eRGB32F:case eRGB16F:
			return GL_RGB;
		case eRGBA8:case eRGBA32F:case eRGBA16F:
			return GL_RGBA;
		case eR32F: case eR16: case eR8:
			return GL_RED;
		case eR8I:case eR16I:case eR32I:
		case eR8U:case eR16U:case eR32U:
			return GL_RED_INTEGER;
		case eRG8I:case eRG16I:case eRG32I:
		case eRG8U:case eRG16U:case eRG32U:
			return GL_RG_INTEGER;
		case eRGB8I:case eRGB16I:case eRGB32I:
		case eRGB8U:case eRGB16U:case eRGB32U:
			return GL_RGB_INTEGER;
		case eRGBA8I:case eRGBA16I:case eRGBA32I:
		case eRGBA8U:case eRGBA16U:case eRGBA32U:
			return GL_RGBA_INTEGER;
		}
		return 0;

	}

	GLenum Texture::GetImage2DType(Format format)
	{
		switch( format )
		{
		case eRGBA8:
		case eRGB8:
		case eR32F:case eR16:case eR8:
		case eRGB32F:
		case eRGBA32F:
		case eRGB16F:
		case eRGBA16F:
			return GL_IMAGE_2D;
		case eR8I:case eR16I:case eR32I:
		case eRG8I:case eRG16I:case eRG32I:
		case eRGB8I:case eRGB16I:case eRGB32I:
		case eRGBA8I:case eRGBA16I:case eRGBA32I:
			return GL_INT_IMAGE_2D;
		case eR8U:case eR16U:case eR32U:
		case eRG8U:case eRG16U:case eRG32U:
		case eRGB8U:case eRGB16U:case eRGB32U:
		case eRGBA8U:case eRGBA16U:case eRGBA32U:
			return GL_UNSIGNED_INT_IMAGE_2D;
		}
		return 0;
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
		case PrimitiveType::Quad:          return GL_QUADS;
		case PrimitiveType::Points:        return GL_POINTS;
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
		case ELockAccess::WriteOnly:
			return GL_WRITE_ONLY;
		}
		return GL_READ_ONLY;
	}

	GLenum GLConvert::To(Blend::Factor factor)
	{
		switch( factor )
		{
		case Blend::eSrcAlpha:
			return GL_SRC_ALPHA;
		case Blend::eOneMinusSrcAlpha:
			return GL_ONE_MINUS_SRC_ALPHA;
		case Blend::eOne:
			return GL_ONE;
		case Blend::eZero:
			return GL_ZERO;
		case Blend::eDestAlpha:
			return GL_DST_ALPHA;
		case Blend::eOneMinusDestAlpha:
			return GL_ONE_MINUS_DST_ALPHA;
		case Blend::eSrcColor:
			return GL_SRC_COLOR;
		case Blend::eOneMinusSrcColor:
			return GL_ONE_MINUS_SRC_COLOR;
		case Blend::eOneMinusDestColor:
			return GL_ONE_MINUS_DST_COLOR;
		}
		return GL_ONE;
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
			return GL_NEAREST;
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

	GLenum GLConvert::To(Buffer::Usage usage)
	{
		switch( usage )
		{
		case Buffer::eStatic:
			return GL_STATIC_DRAW;
		case Buffer::eDynamic:
			return GL_DYNAMIC_DRAW;
		}
		return GL_STATIC_DRAW;
	}

	GLenum Vertex::GetComponentType(uint8 format)
	{
		return GLConvert::To(Vertex::GetCompValueType(Vertex::Format(format)));
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

	bool RHIVertexBuffer::create()
	{
		if( !fetchHandle() )
			return false;
		return true;
	}

	bool RHIVertexBuffer::create(uint32 vertexSize, uint32 numVertices, void* data, Buffer::Usage usage /*= Buffer::eStatic */)
	{
		if( !fetchHandle() )
			return false;

		resetData(vertexSize, numVertices, data, usage);
		return true;
	}

	void RHIVertexBuffer::resetData(uint32 vertexSize, uint32 numVertices, void* data , Buffer::Usage usage )
	{
		glBindBuffer(GL_ARRAY_BUFFER, mHandle);
		glBufferData(GL_ARRAY_BUFFER, vertexSize*numVertices, data , GLConvert::To( usage ) );
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		mBufferSize = vertexSize * numVertices;
		mNumVertices = numVertices;
		mVertexSize = vertexSize;
	}

	void RHIVertexBuffer::updateData(uint32 vStart , uint32 numVertices, void* data)
	{
		assert( (vStart + numVertices) * mVertexSize < mBufferSize );
		glBindBuffer(GL_ARRAY_BUFFER, mHandle);
		glBufferSubData(GL_ARRAY_BUFFER, vStart * mVertexSize , mVertexSize * numVertices, data);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}


	bool RHIUniformBuffer::create(int size)
	{
		if( !fetchHandle() )
			return false;

		glBindBuffer(GL_UNIFORM_BUFFER, mHandle);
		glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		mSize = size;
		return true;
	}

	bool RHISamplerState::create(SamplerStateInitializer const& initializer)
	{
		if( !fetchHandle() )
		{
			return false;
		}

		glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GLConvert::To(initializer.filter));
		glSamplerParameteri(mHandle, GL_TEXTURE_MAG_FILTER, initializer.filter == Sampler::ePoint ? GL_NEAREST : GL_LINEAR);
		glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_S, GLConvert::To(initializer.addressU));
		glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_T, GLConvert::To(initializer.addressV));
		glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_R, GLConvert::To(initializer.addressW));
		return true;
	}




}//namespace GL